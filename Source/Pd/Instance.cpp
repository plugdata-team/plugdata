/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/CachedStringWidth.h"
#include "Dialogs/Dialogs.h"

#include <algorithm>
#include "Instance.h"
#include "Patch.h"
#include "MessageListener.h"
#include "Objects/ImplementationBase.h"
#include "Utility/SettingsFile.h"

extern "C" {

#include <g_undo.h>
#include <m_imp.h>
#include "z_print_util.h"
}

#include "Pd/Interface.h"
#include "Setup.h"

EXTERN int sys_load_lib(t_canvas* canvas, char const* classname);

namespace pd {

class ConsoleMessageHandler final : public Timer {
    Instance* instance;

public:
    explicit ConsoleMessageHandler(Instance* parent)
        : instance(parent)
    {
        startTimerHz(30);
    }

    void addMessage(void* object, String const& message, bool type)
    {
        if (consoleMessages.size()) {
            auto& [lastObject, lastMessage, lastType, lastLength, numMessages] = consoleMessages.back();
            if (object == lastObject && message == lastMessage && type == lastType) {
                numMessages++;
            } else {
                consoleMessages.emplace_back(object, message, type, CachedStringWidth<14>::calculateStringWidth(message) + 40, 1);
            }
        } else {
            consoleMessages.emplace_back(object, message, type, CachedStringWidth<14>::calculateStringWidth(message) + 40, 1);
        }

        if (consoleMessages.size() > 800)
            consoleMessages.pop_front();
    }

    void logMessage(void* object, SmallString const& message)
    {
        pendingMessages.enqueue({ object, message, false });
    }

    void logWarning(void* object, SmallString const& warning)
    {
        pendingMessages.enqueue({ object, warning, true });
    }

    void logError(void* object, SmallString const& error)
    {
        pendingMessages.enqueue({ object, error, true });
    }

    void processPrint(void* object, char const* message)
    {
        std::function<void(SmallString const&)> const forwardMessage =
            [this, object](SmallString const& message) {
                if (message.startsWith("error")) {
                    logError(object, message.substring(7));
                } else if (message.startsWith("verbose(0):") || message.startsWith("verbose(1):")) {
                    logError(object, message.substring(12));
                } else {
                    if (message.startsWith("verbose(")) {
                        logMessage(object, message.substring(12));
                    } else {
                        logMessage(object, message);
                    }
                }
            };

        static int length = 0;
        printConcatBuffer[length] = '\0';

        int len = static_cast<int>(strlen(message));
        while (length + len >= 2048) {
            int const d = 2048 - 1 - length;
            strncat(printConcatBuffer.data(), message, d);

            // Send concatenated line to plugdata!
            forwardMessage(SmallString(printConcatBuffer.data()));

            message += d;
            len -= d;
            length = 0;
            printConcatBuffer[0] = '\0';
        }

        strncat(printConcatBuffer.data(), message, len);
        length += len;

        if (length > 0 && printConcatBuffer[length - 1] == '\n') {
            printConcatBuffer[length - 1] = '\0';

            // Send concatenated line to plugdata!
            forwardMessage(SmallString(printConcatBuffer.data()));

            length = 0;
        }
    }
    
    std::deque<std::tuple<void*, String, int, int, int>> consoleMessages;
    std::deque<std::tuple<void*, String, int, int, int>> consoleHistory;
    
private:
    void timerCallback() override
    {
        auto item = std::tuple<void*, SmallString, bool>();
        int numReceived = 0;
        bool newWarning = false;

        while (pendingMessages.try_dequeue(item)) {
            auto& [object, message, type] = item;
            addMessage(object, message.toString(), type);

            numReceived++;
            newWarning = newWarning || type;
        }

        // Check if any item got assigned
        if (numReceived) {
            instance->updateConsole(numReceived, newWarning);
        }
    }
    
    StackArray<char, 2048> printConcatBuffer;

    moodycamel::ConcurrentQueue<std::tuple<void*, SmallString, bool>> pendingMessages = moodycamel::ConcurrentQueue<std::tuple<void*, SmallString, bool>>(512);
};

struct Instance::dmessage {

    dmessage(pd::Instance* instance, void* ref, SmallString const& dest, SmallString const& sel, SmallArray<pd::Atom> const& atoms)
        : object(ref, instance)
        , destination(dest)
        , selector(sel)
        , list(std::move(atoms))
    {
    }

    WeakReference object;
    SmallString destination;
    SmallString selector;
    SmallArray<pd::Atom> list;
};

struct Instance::internal {

    static void instance_multi_bang(pd::Instance* ptr, char const* recv)
    {
        ptr->enqueueGuiMessage({ SmallString("bang"), String::fromUTF8(recv) });
    }

    static void instance_multi_float(pd::Instance* ptr, char const* recv, float f)
    {
        ptr->enqueueGuiMessage({ SmallString("float"), SmallString(recv), SmallArray<Atom>(1, { f }) });
    }

    static void instance_multi_symbol(pd::Instance* ptr, char const* recv, char const* sym)
    {
        ptr->enqueueGuiMessage({ SmallString("symbol"), SmallString(recv), SmallArray<Atom>(1, ptr->generateSymbol(sym)) });
    }

    static void instance_multi_list(pd::Instance* ptr, char const* recv, int const argc, t_atom* argv)
    {
        Message mess { SmallString("list"), SmallString(recv), SmallArray<Atom>(argc) };
        for (int i = 0; i < argc; ++i) {
            if (argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv + i));
            else if (argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(atom_getsymbol(argv + i));
        }

        ptr->enqueueGuiMessage(mess);
    }

    static void instance_multi_message(pd::Instance* ptr, char const* recv, char const* msg, int const argc, t_atom* argv)
    {
        Message mess { msg, String::fromUTF8(recv), SmallArray<Atom>(argc) };
        for (int i = 0; i < argc; ++i) {
            if (argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv + i));
            else if (argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(atom_getsymbol(argv + i));
        }
        ptr->enqueueGuiMessage(mess);
    }

    static void instance_multi_noteon(pd::Instance* ptr, int const channel, int const pitch, int const velocity)
    {
        ptr->receiveNoteOn(channel + 1, pitch, velocity);
    }

    static void instance_multi_controlchange(pd::Instance* ptr, int const channel, int const controller, int const value)
    {
        ptr->receiveControlChange(channel + 1, controller, value);
    }

    static void instance_multi_programchange(pd::Instance* ptr, int const channel, int const value)
    {
        ptr->receiveProgramChange(channel + 1, value);
    }

    static void instance_multi_pitchbend(pd::Instance* ptr, int const channel, int const value)
    {
        ptr->receivePitchBend(channel + 1, value);
    }

    static void instance_multi_aftertouch(pd::Instance* ptr, int const channel, int const value)
    {
        ptr->receiveAftertouch(channel + 1, value);
    }

    static void instance_multi_polyaftertouch(pd::Instance* ptr, int const channel, int const pitch, int const value)
    {
        ptr->receivePolyAftertouch(channel + 1, pitch, value);
    }

    static void instance_multi_midibyte(pd::Instance* ptr, int const port, int const byte)
    {
        ptr->receiveMidiByte(port + 1, byte);
    }

    static void instance_multi_print(pd::Instance* ptr, void* object, char const* s)
    {
        ptr->consoleMessageHandler->processPrint(object, s);
    }
};

Instance::Instance()
    : messageDispatcher(std::make_unique<MessageDispatcher>())
    , consoleMessageHandler(std::make_unique<ConsoleMessageHandler>(this))
{
    pd::Setup::initialisePd();
    objectImplementations = std::make_unique<::ObjectImplementationManager>(this);
}

Instance::~Instance()
{
    objectImplementations.reset(nullptr); // Make sure it gets deallocated before pd instance gets deleted

    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    pd_free(static_cast<t_pd*>(messageReceiver));
    pd_free(static_cast<t_pd*>(midiReceiver));
    pd_free(static_cast<t_pd*>(printReceiver));
    pd_free(static_cast<t_pd*>(parameterReceiver));
    pd_free(static_cast<t_pd*>(pluginLatencyReceiver));
    pd_free(static_cast<t_pd*>(parameterChangeReceiver));
    pd_free(static_cast<t_pd*>(parameterCreateReceiver));
    pd_free(static_cast<t_pd*>(parameterDestroyReceiver));
    pd_free(static_cast<t_pd*>(parameterRangeReceiver));
    pd_free(static_cast<t_pd*>(parameterModeReceiver));
    pd_free(static_cast<t_pd*>(dataBufferReceiver));

    libpd_free_instance(static_cast<t_pdinstance*>(instance));
}

// ag: Stuff to be done after unpacking the library data on first launch.
void Instance::initialisePd(String& pdlua_version)
{
    instance = libpd_new_instance();

    libpd_set_instance(static_cast<t_pdinstance*>(instance));

    setup_lock(
        &audioLock,
        [](void* lock) {
            static_cast<CriticalSection*>(lock)->enter();
        },
        [](void* lock) {
            static_cast<CriticalSection*>(lock)->exit();
        });

    setup_weakreferences(
        [](void* instance, void* ref) {
            static_cast<pd::Instance*>(instance)->clearWeakReferences(ref);
        },
        [](void* instance, void* ref, void* weakref) {
            auto** reference_state = static_cast<pd_weak_reference**>(weakref);
            *reference_state = new pd_weak_reference(true);
            static_cast<pd::Instance*>(instance)->registerWeakReference(ref, *reference_state);
        },
        [](void* instance, void* ref, void* weakref) {
            auto** reference_state = static_cast<pd_weak_reference**>(weakref);
            static_cast<pd::Instance*>(instance)->unregisterWeakReference(ref, *reference_state);
            delete *reference_state;
        },
        [](void* ref) -> int {
            return static_cast<pd_weak_reference*>(ref)->load();
        });

    midiReceiver = pd::Setup::createMIDIHook(this, reinterpret_cast<t_plugdata_noteonhook>(internal::instance_multi_noteon), reinterpret_cast<t_plugdata_controlchangehook>(internal::instance_multi_controlchange), reinterpret_cast<t_plugdata_programchangehook>(internal::instance_multi_programchange),
        reinterpret_cast<t_plugdata_pitchbendhook>(internal::instance_multi_pitchbend), reinterpret_cast<t_plugdata_aftertouchhook>(internal::instance_multi_aftertouch), reinterpret_cast<t_plugdata_polyaftertouchhook>(internal::instance_multi_polyaftertouch),
        reinterpret_cast<t_plugdata_midibytehook>(internal::instance_multi_midibyte));

    messageReceiver = pd::Setup::createReceiver(this, "pd", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    parameterReceiver = pd::Setup::createReceiver(this, "param", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    pluginLatencyReceiver = pd::Setup::createReceiver(this, "latency_compensation", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    dataBufferReceiver = pd::Setup::createReceiver(this, "to_daw_databuffer", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    parameterChangeReceiver = pd::Setup::createReceiver(this, "param_change", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    parameterCreateReceiver = pd::Setup::createReceiver(this, "param_create", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    parameterDestroyReceiver = pd::Setup::createReceiver(this, "param_destroy", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    parameterRangeReceiver = pd::Setup::createReceiver(this, "param_range", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    parameterModeReceiver = pd::Setup::createReceiver(this, "param_mode", reinterpret_cast<t_plugdata_banghook>(internal::instance_multi_bang), reinterpret_cast<t_plugdata_floathook>(internal::instance_multi_float), reinterpret_cast<t_plugdata_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_plugdata_listhook>(internal::instance_multi_list), reinterpret_cast<t_plugdata_messagehook>(internal::instance_multi_message));

    // Register callback for special Pd messages
    auto gui_trigger = [](void* instance, char const* name, int const argc, t_atom* argv) {
        switch (hash(name)) {
        case hash("canvas_vis"): {
            auto* inst = static_cast<Instance*>(instance);
            if (inst->initialiseIntoPluginmode)
                return;

            auto* pd = static_cast<PluginProcessor*>(inst);
            t_canvas* glist = reinterpret_cast<struct _glist*>(argv->a_w.w_gpointer);

            if (auto const vis = atom_getfloat(argv + 1)) {
                
                File patchFile;
                if (canvas_isabstraction(glist)) {
                    patchFile = File(String::fromUTF8(canvas_getdir(glist)->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
                }
                
                MessageManager::callAsync([pd, inst = juce::WeakReference(inst), patchToOpen = pd::WeakReference(glist, pd), patchFile] {
                    if(!inst) return;
                    PluginEditor* activeEditor = nullptr;
                    for (auto* editor : pd->getEditors()) {
                        if (editor->isActiveWindow())
                        {
                            activeEditor = editor;
                            break;
                        }
                    }
                    if (!activeEditor || !patchToOpen.isValid())
                        return;
                    
                    for(auto& patch : pd->patches)
                    {
                        if (patch->getRawPointer() == patchToOpen.getRaw<t_glist>())
                        {
                            activeEditor->getTabComponent().openPatch(patch);
                            return;
                        }
                    }
                    
                    pd::Patch::Ptr subpatch = new pd::Patch(patchToOpen, pd, false);
                    if(patchFile.exists())
                    {
                        subpatch->setCurrentFile(URL(patchFile));
                    }
                    activeEditor->getTabComponent().openPatch(subpatch);

                });
            } else {
                MessageManager::callAsync([pd, inst = juce::WeakReference(inst), glist] {
                    if(!inst) return;
                    for (auto* editor : pd->getEditors()) {
                        for (auto* canvas : editor->getCanvases()) {
                            auto canvasPtr = canvas->patch.getPointer();
                            if (canvasPtr && canvasPtr.get() == glist) {
                                canvas->editor->getTabComponent().closeTab(canvas);
                                break;
                            }
                        }
                    }
                });
            }
            break;
        }
        case hash("canvas_undo_redo"): {
            auto* inst = static_cast<Instance*>(instance);
            auto* pd = static_cast<PluginProcessor*>(inst);
            auto* glist = reinterpret_cast<t_canvas*>(argv->a_w.w_gpointer);
            auto* undoName = atom_getsymbol(argv + 1);
            auto* redoName = atom_getsymbol(argv + 2);
            MessageManager::callAsync([pd, glist, undoName, redoName] {
                for (auto const& patch : pd->patches) {
                    if (patch->ptr.getRaw<t_canvas>() == glist) {
                        patch->updateUndoRedoState(SmallString(undoName->s_name), SmallString(redoName->s_name));
                    }
                }
            });
            break;
        }
        case hash("canvas_title"): {
            auto* inst = static_cast<Instance*>(instance);
            auto* pd = static_cast<PluginProcessor*>(inst);
            auto* glist = reinterpret_cast<t_canvas*>(argv->a_w.w_gpointer);
            auto* title = atom_getsymbol(argv + 1);
            int isDirty = atom_getfloat(argv + 2);

            MessageManager::callAsync([pd, glist, title, isDirty] {
                for (auto const& patch : pd->patches) {
                    if (patch->ptr.getRaw<t_canvas>() == glist) {
                        patch->updateTitle(SmallString(title->s_name), isDirty);
                    }
                }
            });
            break;
        }
        case hash("openpanel"): {
#if ENABLE_TESTING
            break; // Don't open files during testing
#endif
            const auto openMode = argc >= 4 ? static_cast<int>(atom_getfloat(argv + 3)) : -1;
            static_cast<Instance*>(instance)->createPanel(atom_getfloat(argv), atom_getsymbol(argv + 1)->s_name, atom_getsymbol(argv + 2)->s_name, "callback", openMode);

            break;
        }
        case hash("elsepanel"): {
#if ENABLE_TESTING
            break; // Don't open files during testing
#endif
            static_cast<Instance*>(instance)->createPanel(atom_getfloat(argv), atom_getsymbol(argv + 1)->s_name, atom_getsymbol(argv + 2)->s_name, "symbol");
            break;
        }
        case hash("openfile"):
        case hash("openfile_open"): {
#if ENABLE_TESTING
            break; // Don't open files during testing
#endif
            const auto url = String::fromUTF8(atom_getsymbol(argv)->s_name);
            if (URL::isProbablyAWebsiteURL(url)) {
                URL(url).launchInDefaultBrowser();
            } else {
                if (File(url).exists()) {
                    File(url).startAsProcess();
                } else if (argc > 1) {
                    auto const fullPath = File(String::fromUTF8(atom_getsymbol(argv)->s_name)).getChildFile(url);
                    if (fullPath.exists()) {
                        fullPath.startAsProcess();
                    }
                }
            }

            break;
        }
        case hash("cyclone_editor"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            auto* inst = static_cast<Instance*>(instance);
            SmallString title;

            if (argc > 5) {
                SmallString owner = SmallString(atom_getsymbol(argv + 3)->s_name);
                title = SmallString(atom_getsymbol(argv + 4)->s_name);
            } else {
                title = SmallString(atom_getsymbol(argv + 3)->s_name);
            }
            
            auto save = [title, inst](String text, uint64_t const ptr) {
                inst->lockAudioThread();
                pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("clear"), 0, nullptr);

                // remove repeating spaces
                text = text.replace("\r ", "\r");
                text = text.replace(";\r", ";");
                text = text.replace("\r;", ";");
                text = text.replace(" ;", ";");
                text = text.replace("; ", ";");
                text = text.replace(",", " , ");
                text = text.replaceCharacters("\r", " ");

                while (text.contains("  ")) {
                    text = text.replace("  ", " ");
                }
                text = text.trimStart();
                auto lines = StringArray::fromTokens(text, ";", "\"");

                int count = 0;
                for (auto const& line : lines) {
                    count++;
                    auto words = StringArray::fromTokens(line, " ", "\"");

                    auto atoms = SmallArray<t_atom>();
                    atoms.reserve(words.size() + 1);

                    for (auto const& word : words) {
                        atoms.emplace_back();
                        // check if string is a valid number
                        auto charptr = word.getCharPointer();
                        auto ptr = charptr;
                        CharacterFunctions::readDoubleValue(ptr); // Removes double value from char*
                        if (*charptr == ',') {
                            SETCOMMA(&atoms.back());
                        } else if (ptr - charptr == word.getNumBytesAsUTF8() && ptr - charptr != 0) {
                            SETFLOAT(&atoms.back(), word.getFloatValue());
                        } else {
                            SETSYMBOL(&atoms.back(), inst->generateSymbol(word));
                        }
                    }

                    if (count != lines.size()) {
                        atoms.emplace_back();
                        SETSEMI(&atoms.back());
                    }

                    pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("addline"), atoms.size(), atoms.data());
                }

                pd_typedmess(reinterpret_cast<t_pd*>(ptr), inst->generateSymbol("end"), 0, nullptr);
                inst->unlockAudioThread();
            };
            
            static_cast<Instance*>(instance)->showTextEditorDialog(ptr, title, save, [](uint64_t){});
            break;
        }
        case hash("cyclone_editor_append"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            auto const text = String::fromUTF8(atom_getsymbol(argv + 1)->s_name);

            static_cast<Instance*>(instance)->addTextToTextEditor(ptr, text);
            break;
        }
        case hash("cyclone_editor_close"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            static_cast<Instance*>(instance)->hideTextEditorDialog(ptr);
            break;
        }
        case hash("coll_check_open"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            bool const open = static_cast<bool>(atom_getfloat(argv + 1));
            bool const wasOpen = static_cast<Instance*>(instance)->isTextEditorDialogShown(ptr);

            StackArray<t_atom, 2> atoms;
            SETFLOAT(&atoms[0], wasOpen);
            SETFLOAT(&atoms[1], open);

            pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("_is_opened"), 2, atoms.data());
            break;
        }
        case hash("pdtk_textwindow_open"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            auto* inst = static_cast<Instance*>(instance);
            auto* title = static_cast<t_symbol*>(atom_getsymbol(argv + 1));
            
            auto save = [inst](String text, uint64_t const ptr) {
                inst->lockAudioThread();
                pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("clear"), 0, nullptr);

                // remove repeating spaces
                text = text.replace("\r ", "\r");
                text = text.replace(";\r", ";");
                text = text.replace("\r;", ";");
                text = text.replace(" ;", ";");
                text = text.replace("; ", ";");
                text = text.replace(",", " , ");
                text = text.replaceCharacters("\r", " ");

                while (text.contains("  ")) {
                    text = text.replace("  ", " ");
                }
                text = text.trimStart();
                auto lines = StringArray::fromTokens(text, ";", "\"");

                int count = 0;
                for (auto const& line : lines) {
                    count++;
                    auto words = StringArray::fromTokens(line, " ", "\"");

                    auto atoms = SmallArray<t_atom>();
                    atoms.reserve(words.size() + 1);

                    for (auto const& word : words) {
                        atoms.emplace_back();
                        // check if string is a valid number
                        auto charptr = word.getCharPointer();
                        auto ptr = charptr;
                        CharacterFunctions::readDoubleValue(ptr); // Removes double value from char*
                        if (*charptr == ',') {
                            SETCOMMA(&atoms.back());
                        } else if (ptr - charptr == word.getNumBytesAsUTF8() && ptr - charptr != 0) {
                            SETFLOAT(&atoms.back(), word.getFloatValue());
                        } else {
                            SETSYMBOL(&atoms.back(), inst->generateSymbol(word));
                        }
                    }

                    if (count != lines.size()) {
                        atoms.emplace_back();
                        SETSEMI(&atoms.back());
                    }

                    pd_typedmess(reinterpret_cast<t_pd*>(ptr), gensym("addline"), atoms.size(), atoms.data());
                }

                pd_typedmess(reinterpret_cast<t_pd*>(ptr), inst->generateSymbol("notify"), 0, nullptr);
                inst->unlockAudioThread();
            };
            
            auto close = [inst](uint64_t const ptr)
            {
                inst->lockAudioThread();
                pd_typedmess(reinterpret_cast<t_pd*>(ptr), inst->generateSymbol("close"), 0, nullptr);
                inst->unlockAudioThread();
            };
            
            static_cast<Instance*>(instance)->showTextEditorDialog(ptr, String::fromUTF8(title->s_name), save, close);
            break;
        }
        case hash("pdtk_textwindow_doclose"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            static_cast<Instance*>(instance)->hideTextEditorDialog(ptr);
            break;
        }
        case hash("pdtk_textwindow_clear"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            static_cast<Instance*>(instance)->clearTextEditor(ptr);
            break;
        }
        case hash("pdtk_textwindow_appendatoms"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            auto const argv_start = argv + 1;

            // Create a binbuf to store the atoms
            t_binbuf *b = binbuf_new();
            binbuf_add(b, argc - 1, argv_start);  // Add atoms to binbuf

            // Convert binbuf to a string
            char *text = nullptr;
            int length = 0;
            binbuf_gettext(b, &text, &length);
            if (text) {
                auto editorText = String::fromUTF8(text, length);
                editorText = editorText.replace("; ", ";\n");
                editorText = editorText.replace("\n\n", "\n");
                static_cast<Instance*>(instance)->addTextToTextEditor(ptr, editorText);
            }
            
            freebytes(text, length);
            binbuf_free(b);
            break;
        }
        case hash("pdtk_textwindow_raise"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            static_cast<Instance*>(instance)->raiseTextEditorDialog(ptr);
            break;
        }
        case hash("pdtk_textwindow_destroy"): {
            auto const ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            static_cast<Instance*>(instance)->hideTextEditorDialog(ptr);
            break;
        }
        }
    };

    register_gui_triggers(static_cast<t_pdinstance*>(instance), this, gui_trigger, &MessageDispatcher::enqueueMessage);

    static bool initialised = false;
    if (!initialised) {
        // Make sure we set the maininstance when initialising objects
        // Whenever a new instance is created, the functions will be copied from this one
        libpd_set_instance(libpd_main_instance());

        set_class_prefix(gensym("else"));
        class_set_extern_dir(gensym("9.else"));
        pd::Setup::initialiseELSE();
        set_class_prefix(gensym("cyclone"));
        class_set_extern_dir(gensym("10.cyclone"));
        pd::Setup::initialiseCyclone();

        set_class_prefix(gensym("Gem"));

        class_set_extern_dir(gensym("14.gem"));
        pd::Setup::initialiseGem(ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Gem").getFullPathName().toStdString());

        class_set_extern_dir(gensym(""));
        set_class_prefix(nullptr);
        initialised = true;

        clear_class_loadsym();

        // We want to initialise pdlua separately for each instance
        auto const extra = ProjectInfo::appDataDir.getChildFile("Extra");
        StackArray<char, 1000> vers;
        vers[0] = 0;
        pd::Setup::initialisePdLua(extra.getFullPathName().getCharPointer(), vers.data(), 1000, &registerLuaClass);
        if (vers[0])
            pdlua_version = vers.data();
    }

    setThis();
    pd::Setup::initialisePdLuaInstance();

    // ag: need to do this here to suppress noise from chatty externals
    printReceiver = pd::Setup::createPrintHook(this, reinterpret_cast<t_plugdata_printhook>(internal::instance_multi_print));
    libpd_set_verbose(0);

    set_plugdata_debugging_enabled(SettingsFile::getInstance()->getProperty<bool>("debug_connections"));
}

int Instance::getBlockSize()
{
    return libpd_blocksize();
}

void Instance::prepareDSP(int const nins, int const nouts, double const samplerate, int const blockSize)
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_init_audio(nins, nouts, static_cast<int>(samplerate));
}

void Instance::startDSP()
{
    lockAudioThread();
    t_atom av;
    libpd_set_float(&av, 1.f);
    libpd_message("pd", "dsp", 1, &av);
    unlockAudioThread();
}

void Instance::releaseDSP()
{
    lockAudioThread();
    t_atom av;
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_set_float(&av, 0.f);
    libpd_message("pd", "dsp", 1, &av);
    unlockAudioThread();
}

void Instance::performDSP(float const* inputs, float* outputs)
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_process_raw(inputs, outputs);
}

void Instance::sendNoteOn(int const channel, int const pitch, int const velocity) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_noteon(channel - 1, pitch, velocity);
}

void Instance::sendControlChange(int const channel, int const controller, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_controlchange(channel - 1, controller, value);
}

void Instance::sendProgramChange(int const channel, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_programchange(channel - 1, value);
}

void Instance::sendPitchBend(int const channel, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_pitchbend(channel - 1, value);
}

void Instance::sendAfterTouch(int const channel, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_aftertouch(channel - 1, value);
}

void Instance::sendPolyAfterTouch(int const channel, int const pitch, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_polyaftertouch(channel - 1, pitch, value);
}

void Instance::sendSysEx(int const port, int const byte) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_sysex(port, byte);
}

void Instance::sendSysRealTime(int const port, int const byte) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_sysrealtime(port, byte);
}

void Instance::sendMidiByte(int const port, int const byte) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_midibyte(port, byte);
}

void Instance::sendBang(char const* receiver) const
{
    if (!ProjectInfo::isStandalone && !instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_bang(receiver);
}

void Instance::sendFloat(char const* receiver, float const value) const
{
    if (!ProjectInfo::isStandalone && !instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(instance));

    libpd_float(receiver, value);
}

void Instance::sendSymbol(char const* receiver, char const* symbol) const
{
    if (!ProjectInfo::isStandalone && !instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_symbol(receiver, symbol);
}

void Instance::sendList(char const* receiver, SmallArray<Atom> const& list) const
{
    auto argv = SmallArray<t_atom>(list.size());
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].isFloat())
            libpd_set_float(argv.data() + i, list[i].getFloat());
        else
            libpd_set_symbol(argv.data() + i, list[i].getSymbol()->s_name);
    }
    libpd_list(receiver, static_cast<int>(list.size()), argv.data());
}

void Instance::sendTypedMessage(void* object, char const* msg, SmallArray<Atom> const& list) const
{
    if (!object)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(instance));

    auto argv = SmallArray<t_atom>(list.size());

    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].isFloat())
            libpd_set_float(argv.data() + i, list[i].getFloat());
        else
            libpd_set_symbol(argv.data() + i, list[i].getSymbol()->s_name);
    }

    pd_typedmess(static_cast<t_pd*>(object), generateSymbol(msg), static_cast<int>(list.size()), argv.data());
}

void Instance::sendMessage(char const* receiver, char const* msg, SmallArray<Atom> const& list) const
{
    sendTypedMessage(generateSymbol(receiver)->s_thing, msg, list);
}

void Instance::processSend(dmessage const& mess)
{
    if (auto obj = mess.object.get<t_pd>()) {
        if (mess.selector == "list") {
            auto argv = SmallArray<t_atom>(mess.list.size());
            for (size_t i = 0; i < mess.list.size(); ++i) {
                if (mess.list[i].isFloat())
                    SETFLOAT(argv.data() + i, mess.list[i].getFloat());
                else if (mess.list[i].isSymbol()) {
                    SETSYMBOL(argv.data() + i, mess.list[i].getSymbol());
                } else
                    SETFLOAT(argv.data() + i, 0.0);
            }
            pd_list(obj.get(), generateSymbol("list"), static_cast<int>(mess.list.size()), argv.data());
        } else if (mess.selector == "float" && !mess.list.empty() && mess.list[0].isFloat()) {
            pd_float(obj.get(), mess.list[0].getFloat());
        } else if (mess.selector == "symbol" && !mess.list.empty() && mess.list[0].isSymbol()) {
            pd_symbol(obj.get(), mess.list[0].getSymbol());
        } else {
            sendTypedMessage(obj.get(), mess.selector.data(), mess.list);
        }
    } else {
        sendMessage(mess.destination.data(), mess.selector.data(), mess.list);
    }
}

void Instance::registerMessageListener(void* object, MessageListener* messageListener)
{
    messageDispatcher->addMessageListener(object, messageListener);
}

void Instance::unregisterMessageListener(MessageListener* messageListener)
{
    messageDispatcher->removeMessageListener(messageListener->object, messageListener);
}

void Instance::registerWeakReference(void* ptr, pd_weak_reference* ref)
{
    weakReferenceLock.enter();
    pdWeakReferences[ptr].add(ref);
    weakReferenceLock.exit();
}

void Instance::unregisterWeakReference(void* ptr, pd_weak_reference const* ref)
{
    weakReferenceLock.enter();

    auto& refs = pdWeakReferences[ptr];

    auto const it = std::ranges::find(refs, ref);

    if (it != refs.end()) {
        refs.erase(it);
    }

    weakReferenceLock.exit();
}

void Instance::clearWeakReferences(void* ptr)
{
    weakReferenceLock.enter();
    for (auto* ref : pdWeakReferences[ptr]) {
        *ref = false;
    }
    pdWeakReferences.erase(ptr);
    weakReferenceLock.exit();
}

void Instance::enqueueFunctionAsync(std::function<void()> const& fn)
{
    functionQueue.enqueue(fn);
}

void Instance::enqueueGuiMessage(Message const& message)
{
    guiMessageQueue.enqueue(message);
    triggerAsyncUpdate();

    // We need to handle pluginmode message on loadbang immediately, to prevent loading Canvas twice
    if (message.selector == "pluginmode" && message.destination == "pd") {
        if (message.list.size() && message.list[0].isFloat() && message.list[0].getFloat() == 0.0f)
            return;
        initialiseIntoPluginmode = true;
    }
}

void Instance::sendDirectMessage(void* object, SmallString const& msg, SmallArray<Atom>&& list)
{
    lockAudioThread();
    processSend(dmessage(this, object, SmallString(), msg, std::move(list)));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, SmallArray<Atom>&& list)
{
    lockAudioThread();
    processSend(dmessage(this, object, SmallString(), "list", std::move(list)));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, SmallString const& msg)
{
    lockAudioThread();
    processSend(dmessage(this, object, SmallString(), "symbol", SmallArray<Atom>(1, generateSymbol(msg))));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, float const msg)
{
    lockAudioThread();
    processSend(dmessage(this, object, String(), "float", SmallArray<Atom>(1, msg)));
    unlockAudioThread();
}

void Instance::handleAsyncUpdate()
{
    Message mess;
    while (guiMessageQueue.try_dequeue(mess)) {

        switch (hash(mess.destination)) {
        case hash("pd"):
            receiveSysMessage(mess.selector, mess.list);
            break;
        case hash("latency_compensation"):
            if (mess.list.size() == 1) {
                if (!mess.list[0].isFloat())
                    return;
                performLatencyCompensationChange(mess.list[0].getFloat());
            }
            break;
        case hash("param"):
            if (mess.list.size() >= 2) {
                if (!mess.list[0].isSymbol() || !mess.list[1].isFloat())
                    return;
                auto name = mess.list[0].toString();
                float const value = mess.list[1].getFloat();
                performParameterChange(0, name, value);
            }
            break;
        case hash("param_create"):
            if (mess.list.size() >= 1) {
                if (!mess.list[0].isSymbol())
                    return;
                auto name = mess.list[0].toString();
                enableAudioParameter(name);
            }
            break;
        case hash("param_destroy"):
            if (mess.list.size() >= 1) {
                if (!mess.list[0].isSymbol())
                    return;
                auto name = mess.list[0].toString();
                disableAudioParameter(name);
            }
            break;
        case hash("param_range"):
            if (mess.list.size() >= 3) {
                if (!mess.list[0].isSymbol() || !mess.list[1].isFloat() || !mess.list[2].isFloat())
                    return;
                auto name = mess.list[0].toString();
                float const min = mess.list[1].getFloat();
                float const max = mess.list[2].getFloat();
                setParameterRange(name, min, max);
            }
            break;
        case hash("param_mode"):
            if (mess.list.size() >= 2) {
                if (!mess.list[0].isSymbol() || !mess.list[1].isFloat())
                    return;
                auto name = mess.list[0].toString();
                float const mode = mess.list[1].getFloat();
                setParameterMode(name, mode);
            }
            break;
        case hash("param_change"):
            if (mess.list.size() >= 2) {
                if (!mess.list[0].isSymbol() || !mess.list[1].isFloat())
                    return;
                auto name = mess.list[0].toString();
                int const state = mess.list[1].getFloat() != 0;
                performParameterChange(1, name, state);
            }
            break;
        case hash("to_daw_databuffer"):
            fillDataBuffer(mess.list);
            break;
        default:
            break;
        }
    }
}

void Instance::sendMessagesFromQueue()
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));

    std::function<void()> callback;
    while (functionQueue.try_dequeue(callback)) {
        callback();
    }
}

Patch::Ptr Instance::openPatch(File const& toOpen)
{
    String const dirname = toOpen.getParentDirectory().getFullPathName().replace("\\", "/");
    auto const* dir = dirname.toRawUTF8();

    String const filename = toOpen.getFileName();
    auto const* file = filename.toRawUTF8();

    setThis();

    auto* cnv = pd::Interface::createCanvas(file, dir);

    return new Patch(pd::WeakReference(cnv, this), this, true, toOpen);
}

void Instance::setThis() const
{
    libpd_set_instance(static_cast<t_pdinstance*>(instance));
}

t_symbol* Instance::generateSymbol(char const* symbol) const
{
    setThis();
    return gensym(symbol);
}

t_symbol* Instance::generateSymbol(String const& symbol) const
{
    return generateSymbol(symbol.toRawUTF8());
}

t_symbol* Instance::generateSymbol(SmallString const& symbol) const
{
    return generateSymbol(symbol.data());
}

void Instance::logMessage(String const& message)
{
    consoleMessageHandler->logMessage(nullptr, message);
}

void Instance::logError(String const& error)
{
    consoleMessageHandler->logError(nullptr, error);
}

void Instance::logWarning(String const& warning)
{
    consoleMessageHandler->logWarning(nullptr, warning);
}

std::deque<std::tuple<void*, String, int, int, int>>& Instance::getConsoleMessages()
{
    return consoleMessageHandler->consoleMessages;
}

std::deque<std::tuple<void*, String, int, int, int>>& Instance::getConsoleHistory()
{
    return consoleMessageHandler->consoleHistory;
}

void Instance::createPanel(int const type, char const* snd, char const* location, char const* callbackName, int openMode)
{
#if ENABLE_TESTING
    return; // Don't open file dialogs when running tests, that's annoying
#endif

    auto* obj = generateSymbol(snd)->s_thing;

    auto defaultFile = File(location);
    if (!defaultFile.exists()) {
        defaultFile = SettingsFile::getInstance()->getLastBrowserPathForId("openpanel");
        if (!defaultFile.exists())
            defaultFile = ProjectInfo::appDataDir;
    }

    if (type) {
        MessageManager::callAsync(
            [this, obj, defaultFile, openMode, callback = SmallString(callbackName)]() mutable {
                FileBrowserComponent::FileChooserFlags folderChooserFlags;

                if (openMode <= 0) {
                    folderChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles);
                } else if (openMode == 1) {
                    folderChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories);
                } else {
                    folderChooserFlags = static_cast<FileBrowserComponent::FileChooserFlags>(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories | FileBrowserComponent::canSelectFiles | FileBrowserComponent::canSelectMultipleItems);
                }

                static std::unique_ptr<FileChooser> openChooser;
                openChooser = std::make_unique<FileChooser>("Open...", defaultFile, "", SettingsFile::getInstance()->wantsNativeDialog());
                openChooser->launchAsync(folderChooserFlags, [this, obj, callback](FileChooser const& fileChooser) {
                    auto const files = fileChooser.getResults();
                    if (files.isEmpty())
                        return;

                    auto parentDirectory = files.getFirst().getParentDirectory();
                    SettingsFile::getInstance()->setLastBrowserPathForId("openpanel", parentDirectory);

                    lockAudioThread();

                    SmallArray<t_atom> atoms(files.size());

                    for (int i = 0; i < atoms.size(); i++) {
                        auto pathname = files[i].getFullPathName();
#if JUCE_WINDOWS
                        pathname = pathname.replaceCharacter('\\', '/');
#endif
                        libpd_set_symbol(atoms.data() + i, pathname.toRawUTF8());
                    }

                    pd_typedmess(obj, generateSymbol(callback), atoms.size(), atoms.data());

                    unlockAudioThread();
                    openChooser.reset(nullptr);
                });
            });
    } else {
        MessageManager::callAsync(
            [this, obj, defaultFile, callback = SmallString(callbackName)]() mutable {

#if JUCE_IOS
                Component* dialogParent = dynamic_cast<AudioProcessor*>(this)->getActiveEditor();
#else
                Component* dialogParent = nullptr;
#endif
                if (defaultFile.exists()) {
                    SettingsFile::getInstance()->setLastBrowserPathForId("savepanel", defaultFile);
                }

                Dialogs::showSaveDialog([this, obj, callback](URL const& result) {
                    auto pathname = result.getLocalFile().getFullPathName();
#if JUCE_WINDOWS
                    pathname = pathname.replaceCharacter('\\', '/');
#endif

                    auto const* path = pathname.toRawUTF8();

                    t_atom argv;
                    libpd_set_symbol(&argv, path);

                    lockAudioThread();
                    pd_typedmess(obj, generateSymbol(callback), 1, &argv);
                    unlockAudioThread();
                },
                    "", "savepanel", dialogParent);
            });
    }
}

bool Instance::loadLibrary(String const& libraryToLoad)
{
    return sys_load_lib(nullptr, libraryToLoad.toRawUTF8());
}

void Instance::lockAudioThread()
{
    audioLock.enter();
}

void Instance::unlockAudioThread()
{
    audioLock.exit();
}

void Instance::updateObjectImplementations()
{
    objectImplementations->updateObjectImplementations();
}

void Instance::clearObjectImplementationsForPatch(pd::Patch* p)
{
    if (auto patch = p->getPointer()) {
        objectImplementations->clearObjectImplementationsForPatch(patch.get());
    }
}

void Instance::registerLuaClass(char const* className)
{
    luaClasses.insert(hash(className));
}

bool Instance::isLuaClass(hash32 const objectNameHash)
{
    return luaClasses.contains(objectNameHash);
}

} // namespace pd
