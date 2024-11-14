/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"
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

#include "Pd/Interface.h"
#include "Setup.h"
#include "z_print_util.h"

EXTERN int sys_load_lib(t_canvas* canvas, char const* classname);

struct pd::Instance::internal {

    static void instance_multi_bang(pd::Instance* ptr, char const* recv)
    {
        ptr->enqueueGuiMessage({ String("bang"), String::fromUTF8(recv) });
    }

    static void instance_multi_float(pd::Instance* ptr, char const* recv, float f)
    {
        ptr->enqueueGuiMessage({ String("float"), String::fromUTF8(recv), SmallArray<Atom>(1, { f }) });
    }

    static void instance_multi_symbol(pd::Instance* ptr, char const* recv, char const* sym)
    {
        ptr->enqueueGuiMessage({ String("symbol"), String::fromUTF8(recv), SmallArray<Atom>(1, ptr->generateSymbol(sym)) });
    }

    static void instance_multi_list(pd::Instance* ptr, char const* recv, int argc, t_atom* argv)
    {
        Message mess { String("list"), String::fromUTF8(recv), SmallArray<Atom>(argc) };
        for (int i = 0; i < argc; ++i) {
            if (argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv + i));
            else if (argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(atom_getsymbol(argv + i));
        }

        ptr->enqueueGuiMessage(mess);
    }

    static void instance_multi_message(pd::Instance* ptr, char const* recv, char const* msg, int argc, t_atom* argv)
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

    static void instance_multi_noteon(pd::Instance* ptr, int channel, int pitch, int velocity)
    {
        ptr->enqueueFunctionAsync([ptr, channel, pitch, velocity]() mutable {
            ptr->receiveNoteOn(channel + 1, pitch, velocity);
        });
    }

    static void instance_multi_controlchange(pd::Instance* ptr, int channel, int controller, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, controller, value]() mutable {
            ptr->receiveControlChange(channel + 1, controller, value);
        });
    }

    static void instance_multi_programchange(pd::Instance* ptr, int channel, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, value]() mutable {
            ptr->receiveProgramChange(channel + 1, value);
        });
    }

    static void instance_multi_pitchbend(pd::Instance* ptr, int channel, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, value]() mutable {
            ptr->receivePitchBend(channel + 1, value);
        });
    }

    static void instance_multi_aftertouch(pd::Instance* ptr, int channel, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, value]() mutable {
            ptr->receiveAftertouch(channel + 1, value);
        });
    }

    static void instance_multi_polyaftertouch(pd::Instance* ptr, int channel, int pitch, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, pitch, value]() mutable {
            ptr->receivePolyAftertouch(channel + 1, pitch, value);
        });
    }

    static void instance_multi_midibyte(pd::Instance* ptr, int port, int byte)
    {
        ptr->enqueueFunctionAsync([ptr, port, byte]() mutable {
            ptr->receiveMidiByte(port + 1, byte);
        });
    }

    static void instance_multi_print(pd::Instance* ptr, void* object, char const* s)
    {
        ptr->consoleHandler.processPrint(object, s);
    }
};
}

namespace pd {

Instance::Instance()
    : messageDispatcher(std::make_unique<MessageDispatcher>())
    , consoleHandler(this)
{
    pd::Setup::initialisePd();
    objectImplementations = std::make_unique<::ObjectImplementationManager>(this);
}

Instance::~Instance()
{
    objectImplementations.reset(nullptr); // Make sure it gets deallocated before pd instance gets deleted

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

    // JYG added this
    pd_free(static_cast<t_pd*>(dataBufferReceiver));

    libpd_set_instance(static_cast<t_pdinstance*>(instance));
    libpd_free_instance(static_cast<t_pdinstance*>(instance));
}

// ag: Stuff to be done after unpacking the library data on first launch.
void Instance::initialisePd(String& pdlua_version)
{
    instance = libpd_new_instance();

    libpd_set_instance(static_cast<t_pdinstance*>(instance));

    setup_lock(
        static_cast<void const*>(&audioLock),
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
            auto** reference_state = reinterpret_cast<pd_weak_reference**>(weakref);
            *reference_state = new pd_weak_reference(true);
            static_cast<pd::Instance*>(instance)->registerWeakReference(ref, *reference_state);
        },
        [](void* instance, void* ref, void* weakref) {
            auto** reference_state = reinterpret_cast<pd_weak_reference**>(weakref);
            static_cast<pd::Instance*>(instance)->unregisterWeakReference(ref, *reference_state);
            delete *reference_state;
        },
        [](void* ref) -> int {
            return ((pd_weak_reference*)ref)->load();
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

    // JYG added This
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
    auto gui_trigger = [](void* instance, char const* name, int argc, t_atom* argv) {
        switch (hash(name)) {
        case hash("canvas_vis"): {
            auto* inst = static_cast<Instance*>(instance);
            if(inst->initialiseIntoPluginmode) return;
            
            auto* pd = static_cast<PluginProcessor*>(inst);
            t_canvas* glist = (t_canvas*)argv->a_w.w_gpointer;
            auto vis = atom_getfloat(argv + 1);

            if (vis) {
                pd::Patch::Ptr subpatch = new pd::Patch(pd::WeakReference(glist, pd), pd, false);
                if (canvas_isabstraction(glist)) {
                    auto path = File(String::fromUTF8(canvas_getdir(glist)->s_name)).getChildFile(String::fromUTF8(glist->gl_name->s_name)).withFileExtension("pd");
                    subpatch->setCurrentFile(URL(path));
                }

                MessageManager::callAsync([pd, subpatch]() {
                    for (auto* editor : pd->getEditors()) {
                        if (!editor->isActiveWindow())
                            continue;
                        editor->getTabComponent().openPatch(subpatch);
                    }
                });
            } else {
                MessageManager::callAsync([pd, glist]() {
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
        case hash("openpanel"): {
            auto openMode = argc >= 4 ? static_cast<int>(atom_getfloat(argv + 3)) : -1;
            static_cast<Instance*>(instance)->createPanel(atom_getfloat(argv), atom_getsymbol(argv + 1)->s_name, atom_getsymbol(argv + 2)->s_name, "callback", openMode);

            break;
        }
        case hash("elsepanel"): {
            static_cast<Instance*>(instance)->createPanel(atom_getfloat(argv), atom_getsymbol(argv + 1)->s_name, atom_getsymbol(argv + 2)->s_name, "symbol");
            break;
        }
        case hash("openfile"):
        case hash("openfile_open"): {
#if ENABLE_TESTING
            break; // Don't open files during testing
#endif
            auto url = String::fromUTF8(atom_getsymbol(argv)->s_name);
            if (URL::isProbablyAWebsiteURL(url)) {
                URL(url).launchInDefaultBrowser();
            } else {
                if (File(url).exists()) {
                    File(url).startAsProcess();
                } else if (argc > 1) {
                    auto fullPath = File(String::fromUTF8(atom_getsymbol(argv)->s_name)).getChildFile(url);
                    if (fullPath.exists()) {
                        fullPath.startAsProcess();
                    }
                }
            }

            break;
        }
        case hash("cyclone_editor"): {
            auto ptr = (uint64_t)argv->a_w.w_gpointer;
            auto width = atom_getfloat(argv + 1);
            auto height = atom_getfloat(argv + 2);
            String owner, title;

            if (argc > 5) {
                owner = String::fromUTF8(atom_getsymbol(argv + 3)->s_name);
                title = String::fromUTF8(atom_getsymbol(argv + 4)->s_name);
            } else {
                title = String::fromUTF8(atom_getsymbol(argv + 3)->s_name);
            }

            static_cast<Instance*>(instance)->showTextEditorDialog(ptr, Rectangle<int>(width, height), title);

            break;
        }
        case hash("cyclone_editor_append"): {
            auto ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            auto text = String::fromUTF8(atom_getsymbol(argv + 1)->s_name);

            static_cast<Instance*>(instance)->addTextToTextEditor(ptr, text);
            break;
        }
        case hash("coll_check_open"): {
            auto ptr = reinterpret_cast<uint64_t>(argv->a_w.w_gpointer);
            bool open = static_cast<bool>(atom_getfloat(argv + 1));
            bool wasOpen = static_cast<Instance*>(instance)->isTextEditorDialogShown(ptr);

            StackArray<t_atom, 2> atoms;
            SETFLOAT(&atoms[0], wasOpen);
            SETFLOAT(&atoms[1], open);

            pd_typedmess((t_pd*)ptr, gensym("_is_opened"), 2, atoms.data());
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
        auto extra = ProjectInfo::appDataDir.getChildFile("Extra");
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

void Instance::processSend(dmessage mess)
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
            sendTypedMessage(obj.get(), mess.selector.toRawUTF8(), mess.list);
        }
    } else {
        sendMessage(mess.destination.toRawUTF8(), mess.selector.toRawUTF8(), mess.list);
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

    auto it = std::find(refs.begin(), refs.end(), ref);

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

void Instance::enqueueFunctionAsync(std::function<void(void)> const& fn)
{
    functionQueue.enqueue(fn);
}

void Instance::enqueueGuiMessage(Message const& message)
{
    guiMessageQueue.enqueue(message);
    triggerAsyncUpdate();
    
    // We need to handle pluginmode message on loadbang immediately, to prevent loading Canvas twice
    if(message.selector == "pluginmode" && message.destination == "pd")
    {
        if(message.list.size() && message.list[0].isFloat() && message.list[0].getFloat() == 0.0f) return;
        initialiseIntoPluginmode = true;
    }
}

void Instance::sendDirectMessage(void* object, String const& msg, SmallArray<Atom>&& list)
{
    lockAudioThread();
    processSend(dmessage(this, object, String(), msg, std::move(list)));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, SmallArray<Atom>&& list)
{
    lockAudioThread();
    processSend(dmessage(this, object, String(), "list", std::move(list)));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, String const& msg)
{

    lockAudioThread();
    processSend(dmessage(this, object, String(), "symbol", SmallArray<Atom>(1, generateSymbol(msg))));
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
        auto const dest = hash(mess.destination);

        switch (dest) {
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
                float value = mess.list[1].getFloat();
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
                float min = mess.list[1].getFloat();
                float max = mess.list[2].getFloat();
                setParameterRange(name, min, max);
            }
            break;
        case hash("param_mode"):
            if (mess.list.size() >= 2) {
                if (!mess.list[0].isSymbol() || !mess.list[1].isFloat())
                    return;
                auto name = mess.list[0].toString();
                float mode = mess.list[1].getFloat();
                setParameterMode(name, mode);
            }
            break;
        case hash("param_change"):
            if (mess.list.size() >= 2) {
                if (!mess.list[0].isSymbol() || !mess.list[1].isFloat())
                    return;
                auto name = mess.list[0].toString();
                int state = mess.list[1].getFloat() != 0;
                performParameterChange(1, name, state);
            }
            break;
            // JYG added this
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

    sys_lock();
    std::function<void(void)> callback;
    while (functionQueue.try_dequeue(callback)) {
        callback();
    }
    sys_unlock();
}

Patch::Ptr Instance::openPatch(File const& toOpen)
{
    String dirname = toOpen.getParentDirectory().getFullPathName().replace("\\", "/");
    auto const* dir = dirname.toRawUTF8();

    String filename = toOpen.getFileName();
    auto const* file = filename.toRawUTF8();

    setThis();

    auto* cnv = static_cast<t_canvas*>(pd::Interface::createCanvas(file, dir));

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

void Instance::logMessage(String const& message)
{
    consoleHandler.logMessage(nullptr, message);
}

void Instance::logError(String const& error)
{
    consoleHandler.logError(nullptr, error);
}

void Instance::logWarning(String const& warning)
{
    consoleHandler.logWarning(nullptr, warning);
}

std::deque<std::tuple<void*, String, int, int, int>>& Instance::getConsoleMessages()
{
    return consoleHandler.consoleMessages;
}

std::deque<std::tuple<void*, String, int, int, int>>& Instance::getConsoleHistory()
{
    return consoleHandler.consoleHistory;
}

void Instance::createPanel(int type, char const* snd, char const* location, char const* callbackName, int openMode)
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
            [this, obj, defaultFile, openMode, callback = String(callbackName)]() mutable {
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
                        String pathname = files[i].getFullPathName();
                        libpd_set_symbol(atoms.data() + i, pathname.toRawUTF8());
                    }

                    pd_typedmess(obj, generateSymbol(callback), atoms.size(), atoms.data());

                    unlockAudioThread();
                    openChooser.reset(nullptr);
                });
            });
    } else {
        MessageManager::callAsync(
            [this, obj, defaultFile, callback = String(callbackName)]() mutable {

#if JUCE_IOS
                Component* dialogParent = dynamic_cast<AudioProcessor*>(this)->getActiveEditor();
#else
                Component* dialogParent = nullptr;
#endif
                if (defaultFile.exists()) {
                    SettingsFile::getInstance()->setLastBrowserPathForId("savepanel", defaultFile);
                }

                Dialogs::showSaveDialog([this, obj, callback](URL result) {
                    auto pathName = result.getLocalFile().getFullPathName();
                    auto const* path = pathName.toRawUTF8();

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

bool Instance::isLuaClass(hash32 objectNameHash)
{
    return luaClasses.contains(objectNameHash);
}

} // namespace pd
