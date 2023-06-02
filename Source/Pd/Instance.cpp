/*
 // Copyright (c) 2015-2022 Pierre Guillot and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>

#include "Utility/Config.h"
#include "Utility/Fonts.h"

#include <algorithm>
#include "Instance.h"
#include "Patch.h"
#include "MessageListener.h"
#include "Objects/ImplementationBase.h"
#include "Utility/SettingsFile.h"

#include "OfeliaMessageManager.h"

extern "C" {

#include <g_undo.h>
#include <m_imp.h>

#include "x_libpd_extra_utils.h"
#include "x_libpd_mod_utils.h"
#include "x_libpd_multi.h"
#include "z_print_util.h"

int sys_load_lib(t_canvas* canvas, char const* classname);
void set_class_prefix(t_symbol* dir);

struct pd::Instance::internal {

    static void instance_multi_bang(pd::Instance* ptr, char const* recv)
    {
        ptr->enqueueFunctionAsync([ptr, recv]() { ptr->processMessage({ String("bang"), String::fromUTF8(recv) }); });
    }

    static void instance_multi_float(pd::Instance* ptr, char const* recv, float f)
    {
        ptr->enqueueFunctionAsync([ptr, recv, f]() mutable { ptr->processMessage({ String("float"), String::fromUTF8(recv), std::vector<Atom>(1, { f }) }); });
    }

    static void instance_multi_symbol(pd::Instance* ptr, char const* recv, char const* sym)
    {
        ptr->enqueueFunctionAsync([ptr, recv, sym]() mutable { ptr->processMessage({ String("symbol"), String::fromUTF8(recv), std::vector<Atom>(1, String::fromUTF8(sym)) }); });
    }

    static void instance_multi_list(pd::Instance* ptr, char const* recv, int argc, t_atom* argv)
    {
        Message mess { String("list"), String::fromUTF8(recv), std::vector<Atom>(argc) };
        for (int i = 0; i < argc; ++i) {
            if (argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv + i));
            else if (argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(String::fromUTF8(atom_getsymbol(argv + i)->s_name));
        }

        ptr->enqueueFunctionAsync([ptr, mess]() mutable { ptr->processMessage(mess); });
    }

    static void instance_multi_message(pd::Instance* ptr, char const* recv, char const* msg, int argc, t_atom* argv)
    {
        Message mess { msg, String::fromUTF8(recv), std::vector<Atom>(argc) };
        for (int i = 0; i < argc; ++i) {
            if (argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv + i));
            else if (argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(String::fromUTF8(atom_getsymbol(argv + i)->s_name));
        }
        ptr->enqueueFunctionAsync([ptr, mess]() mutable { ptr->processMessage(std::move(mess)); });
    }

    static void instance_multi_noteon(pd::Instance* ptr, int channel, int pitch, int velocity)
    {
        ptr->enqueueFunctionAsync([ptr, channel, pitch, velocity]() mutable { ptr->processMidiEvent({ midievent::NOTEON, channel, pitch, velocity }); });
    }

    static void instance_multi_controlchange(pd::Instance* ptr, int channel, int controller, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, controller, value]() mutable { ptr->processMidiEvent({ midievent::CONTROLCHANGE, channel, controller, value }); });
    }

    static void instance_multi_programchange(pd::Instance* ptr, int channel, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, value]() mutable { ptr->processMidiEvent({ midievent::PROGRAMCHANGE, channel, value, 0 }); });
    }

    static void instance_multi_pitchbend(pd::Instance* ptr, int channel, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, value]() mutable { ptr->processMidiEvent({ midievent::PITCHBEND, channel, value, 0 }); });
    }

    static void instance_multi_aftertouch(pd::Instance* ptr, int channel, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, value]() mutable { ptr->processMidiEvent({ midievent::AFTERTOUCH, channel, value, 0 }); });
    }

    static void instance_multi_polyaftertouch(pd::Instance* ptr, int channel, int pitch, int value)
    {
        ptr->enqueueFunctionAsync([ptr, channel, pitch, value]() mutable { ptr->processMidiEvent({ midievent::POLYAFTERTOUCH, channel, pitch, value }); });
    }

    static void instance_multi_midibyte(pd::Instance* ptr, int port, int byte)
    {
        ptr->enqueueFunctionAsync([ptr, port, byte]() mutable { ptr->processMidiEvent({ midievent::MIDIBYTE, port, byte, 0 }); });
    }

    static void instance_multi_print(pd::Instance* ptr, char const* s)
    {
        ptr->consoleHandler.processPrint(s);
    }
};
}

namespace pd {

Instance::Instance(String const& symbol)
    : consoleHandler(this)
{
    libpd_multi_init();
    objectImplementations = std::make_unique<::ObjectImplementationManager>(this);
}

Instance::~Instance()
{
    pd_free(static_cast<t_pd*>(m_message_receiver));
    pd_free(static_cast<t_pd*>(m_midi_receiver));
    pd_free(static_cast<t_pd*>(m_print_receiver));
    pd_free(static_cast<t_pd*>(m_parameter_receiver));
    pd_free(static_cast<t_pd*>(m_parameter_change_receiver));

    // JYG added this
    pd_free(static_cast<t_pd*>(m_databuffer_receiver));

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_free_instance(static_cast<t_pdinstance*>(m_instance));
}

// ag: Stuff to be done after unpacking the library data on first launch.
void Instance::initialisePd(String& pdlua_version)
{
    m_instance = libpd_new_instance();

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));

    set_instance_lock(
        static_cast<void const*>(&audioLock),
        [](void* lock) {
            static_cast<CriticalSection*>(lock)->enter();
        },
        [](void* lock) {
            static_cast<CriticalSection*>(lock)->exit();
        },
        [](void* instance, t_pd* ref) {
            static_cast<pd::Instance*>(instance)->clearWeakReferences(ref);
         });

    m_midi_receiver = libpd_multi_midi_new(this, reinterpret_cast<t_libpd_multi_noteonhook>(internal::instance_multi_noteon), reinterpret_cast<t_libpd_multi_controlchangehook>(internal::instance_multi_controlchange), reinterpret_cast<t_libpd_multi_programchangehook>(internal::instance_multi_programchange),
        reinterpret_cast<t_libpd_multi_pitchbendhook>(internal::instance_multi_pitchbend), reinterpret_cast<t_libpd_multi_aftertouchhook>(internal::instance_multi_aftertouch), reinterpret_cast<t_libpd_multi_polyaftertouchhook>(internal::instance_multi_polyaftertouch),
        reinterpret_cast<t_libpd_multi_midibytehook>(internal::instance_multi_midibyte));

    m_message_receiver = libpd_multi_receiver_new(this, "pd", reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));

    m_parameter_receiver = libpd_multi_receiver_new(this, "param", reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));

    // JYG added This
    m_databuffer_receiver = libpd_multi_receiver_new(this, "databuffer", reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));

    m_parameter_change_receiver = libpd_multi_receiver_new(this, "param_change", reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));

    m_atoms = malloc(sizeof(t_atom) * 512);

    // Register callback when pd's gui changes
    // Needs to be done on pd's thread
    auto gui_trigger = [](void* instance, char const* name, t_atom* arg1, t_atom* arg2, t_atom* arg3) {
        if (String::fromUTF8(name) == "openpanel") {

            static_cast<Instance*>(instance)->createPanel(atom_getfloat(arg1), atom_getsymbol(arg3)->s_name, atom_getsymbol(arg2)->s_name);
        }
        if (String::fromUTF8(name) == "openfile") {
            File(String::fromUTF8(atom_getsymbol(arg1)->s_name)).startAsProcess();
        }
        if (String::fromUTF8(name) == "repaint") {
            static_cast<Instance*>(instance)->updateDrawables();
        }
    };

    auto message_trigger = [](void* instance, void* target, t_symbol* symbol, int argc, t_atom* argv) {
        ScopedLock lock(static_cast<Instance*>(instance)->messageListenerLock);

        auto& listeners = static_cast<Instance*>(instance)->messageListeners;
        if (!symbol || !listeners.count(target))
            return;

        auto sym = String::fromUTF8(symbol->s_name);

        // Create a new vector to hold the null listeners
        std::vector<std::vector<juce::WeakReference<pd::MessageListener>>::iterator> nullListeners;

        for (auto it = listeners[target].begin(); it != listeners[target].end(); ++it) {
            auto listener = it->get();
            if (listener) {
                listener->receiveMessage(sym, argc, argv);
            } else
                nullListeners.push_back(it);
        }

        // Remove all the null listeners from the original vector using the iterators in the nullListeners vector
        for (int i = nullListeners.size() - 1; i >= 0; i--) {
            listeners[target].erase(nullListeners[i]);
        }
    };

    register_gui_triggers(static_cast<t_pdinstance*>(m_instance), this, gui_trigger, message_trigger);

    // Make sure we set the maininstance when initialising objects
    // Whenever a new instance is created, the functions will be copied from this one
    libpd_set_instance(libpd_get_instance(0));

    static bool initialised = false;
    if (!initialised) {

        File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");
        auto library = homeDir.getChildFile("Library");
        auto extra = library.getChildFile("Extra");

        set_class_prefix(gensym("else"));
        libpd_init_else();
        set_class_prefix(gensym("cyclone"));
        libpd_init_cyclone();
        set_class_prefix(nullptr);

        // Class prefix doesn't seem to work for pdlua
        char vers[1000];
        *vers = 0;
        libpd_init_pdlua(extra.getFullPathName().getCharPointer(), vers, 1000);
        if (*vers)
            pdlua_version = vers;

        initialised = true;
    }

    setThis();

    // ag: need to do this here to suppress noise from chatty externals
    m_print_receiver = libpd_multi_print_new(this, reinterpret_cast<t_libpd_multi_printhook>(internal::instance_multi_print));
    libpd_set_verbose(0);
}

int Instance::getBlockSize() const
{
    return libpd_blocksize();
}

void Instance::prepareDSP(int const nins, int const nouts, double const samplerate, int const blockSize)
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_init_audio(nins, nouts, static_cast<int>(samplerate));
}

void Instance::startDSP()
{
    t_atom av;
    libpd_set_float(&av, 1.f);
    libpd_message("pd", "dsp", 1, &av);
}

void Instance::releaseDSP()
{
    t_atom av;
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_set_float(&av, 0.f);
    libpd_message("pd", "dsp", 1, &av);
}

void Instance::performDSP(float const* inputs, float* outputs)
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_process_raw(inputs, outputs);
}

void Instance::sendNoteOn(int const channel, int const pitch, int const velocity) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_noteon(channel - 1, pitch, velocity);
}

void Instance::sendControlChange(int const channel, int const controller, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_controlchange(channel - 1, controller, value);
}

void Instance::sendProgramChange(int const channel, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_programchange(channel - 1, value);
}

void Instance::sendPitchBend(int const channel, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_pitchbend(channel - 1, value);
}

void Instance::sendAfterTouch(int const channel, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_aftertouch(channel - 1, value);
}

void Instance::sendPolyAfterTouch(int const channel, int const pitch, int const value) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_polyaftertouch(channel - 1, pitch, value);
}

void Instance::sendSysEx(int const port, int const byte) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_sysex(port, byte);
}

void Instance::sendSysRealTime(int const port, int const byte) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_sysrealtime(port, byte);
}

void Instance::sendMidiByte(int const port, int const byte) const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_midibyte(port, byte);
}

void Instance::sendBang(char const* receiver) const
{
    if (!ProjectInfo::isStandalone && !m_instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_bang(receiver);
}

void Instance::sendFloat(char const* receiver, float const value) const
{
    if (!ProjectInfo::isStandalone && !m_instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));

    libpd_float(receiver, value);
}

void Instance::sendSymbol(char const* receiver, char const* symbol) const
{
    if (!ProjectInfo::isStandalone && !m_instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_symbol(receiver, symbol);
}

void Instance::sendList(char const* receiver, std::vector<Atom> const& list) const
{
    auto* argv = static_cast<t_atom*>(m_atoms);
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].isFloat())
            libpd_set_float(argv + i, list[i].getFloat());
        else
            libpd_set_symbol(argv + i, list[i].getSymbol().toRawUTF8());
    }
    libpd_list(receiver, static_cast<int>(list.size()), argv);
}

void Instance::sendTypedMessage(void* object, char const* msg, std::vector<Atom> const& list) const
{
    if (!object)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));

    auto* argv = static_cast<t_atom*>(m_atoms);

    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].isFloat())
            libpd_set_float(argv + i, list[i].getFloat());
        else
            libpd_set_symbol(argv + i, list[i].getSymbol().toRawUTF8());
    }

    pd_typedmess(static_cast<t_pd*>(object), generateSymbol(msg), static_cast<int>(list.size()), argv);
}

void Instance::sendMessage(char const* receiver, char const* msg, std::vector<Atom> const& list) const
{
    sendTypedMessage(generateSymbol(receiver)->s_thing, msg, list);
}

void Instance::processMessage(Message mess)
{
    if (mess.destination == "pd") {
        receiveSysMessage(mess.selector, mess.list);
    }
    if (mess.destination == "param" && mess.list.size() >= 2) {
        if (!mess.list[0].isSymbol() || !mess.list[1].isFloat())
            return;
        auto name = mess.list[0].getSymbol();
        float value = mess.list[1].getFloat();
        performParameterChange(0, name, value);
    } else if (mess.destination == "param_change" && mess.list.size() >= 2) {
        if (!mess.list[0].isSymbol() || !mess.list[1].isFloat())
            return;
        auto name = mess.list[0].getSymbol();
        int state = mess.list[1].getFloat() != 0;
        performParameterChange(1, name, state);
        // JYG added This
    } else if (mess.destination == "databuffer") {
        fillDataBuffer(mess.list);

    } else if (mess.selector == "bang") {
        receiveBang(mess.destination);
    } else if (mess.selector == "float") {
        receiveFloat(mess.destination, mess.list[0].getFloat());
    } else if (mess.selector == "symbol") {
        receiveSymbol(mess.destination, mess.list[0].getSymbol());
    } else if (mess.selector == "list") {
        receiveList(mess.destination, mess.list);
    } else {
        receiveMessage(mess.destination, mess.selector, mess.list);
    }
}

void Instance::processMidiEvent(midievent event)
{
    switch (event.type) {
    case midievent::NOTEON:
        receiveNoteOn(event.midi1 + 1, event.midi2, event.midi3);
        break;
    case midievent::CONTROLCHANGE:
        receiveControlChange(event.midi1 + 1, event.midi2, event.midi3);
        break;
    case midievent::PROGRAMCHANGE:
        receiveProgramChange(event.midi1 + 1, event.midi2);
        break;
    case midievent::PITCHBEND:
        receivePitchBend(event.midi1 + 1, event.midi2);
        break;
    case midievent::AFTERTOUCH:
        receiveAftertouch(event.midi1 + 1, event.midi2);
        break;
    case midievent::POLYAFTERTOUCH:
        receivePolyAftertouch(event.midi1 + 1, event.midi2, event.midi3);
        break;
    case midievent::MIDIBYTE:
        receiveMidiByte(event.midi1, event.midi2);
        break;
    default:
        break;
    }
}

void Instance::processSend(dmessage mess)
{
    if (auto obj = mess.object.get<t_pd>()) {
        if (mess.selector == "list") {
            auto* argv = static_cast<t_atom*>(m_atoms);
            for (size_t i = 0; i < mess.list.size(); ++i) {
                if (mess.list[i].isFloat())
                    SETFLOAT(argv + i, mess.list[i].getFloat());
                else if (mess.list[i].isSymbol()) {
                    SETSYMBOL(argv + i, generateSymbol(mess.list[i].getSymbol()));
                } else
                    SETFLOAT(argv + i, 0.0);
            }
            pd_list(obj.get(), generateSymbol("list"), static_cast<int>(mess.list.size()), argv);
        } else if (mess.selector == "float" && !mess.list.empty() && mess.list[0].isFloat()) {
            pd_float(obj.get(), mess.list[0].getFloat());
        } else if (mess.selector == "symbol" && !mess.list.empty() && mess.list[0].isSymbol()) {
            pd_symbol(obj.get(), generateSymbol(mess.list[0].getSymbol()));
        } else {
            sendTypedMessage(obj.get(), mess.selector.toRawUTF8(), mess.list);
        }
    } else {
        sendMessage(mess.destination.toRawUTF8(), mess.selector.toRawUTF8(), mess.list);
    }
}

void Instance::registerMessageListener(void* object, MessageListener* messageListener)
{
    ScopedLock lock(messageListenerLock);
    messageListeners[object].emplace_back(messageListener);
}

void Instance::unregisterMessageListener(void* object, MessageListener* messageListener)
{
    ScopedLock lock(messageListenerLock);

    if (messageListeners.count(object))
        return;

    auto& listeners = messageListeners[object];
    auto it = std::find(listeners.begin(), listeners.end(), messageListener);

    if (it != listeners.end())
        listeners.erase(it);
}

void Instance::registerWeakReference(t_pd* ptr, pd_weak_reference* ref)
{
    lockAudioThread();
    pdWeakReferences[ptr].push_back(ref);
    unlockAudioThread();
}

void Instance::unregisterWeakReference(t_pd* ptr, pd_weak_reference* ref)
{
    lockAudioThread();
    
    auto& refs = pdWeakReferences[ptr];
    
    auto it = std::find(refs.begin(), refs.end(), ref);

    if (it != refs.end()) {
        pdWeakReferences[ptr].erase(it);
    }
    
    unlockAudioThread();
}

void Instance::clearWeakReferences(t_pd* ptr)
{
    lockAudioThread();
    for(auto* ref : pdWeakReferences[ptr])
    {
        *ref = false;
    }
    pdWeakReferences.erase(ptr);
    unlockAudioThread();
}

void Instance::enqueueFunctionAsync(std::function<void(void)> const& fn)
{
    m_function_queue.enqueue(fn);
}

void Instance::sendDirectMessage(void* object, String const& msg, std::vector<Atom>&& list)
{
    lockAudioThread();
    processSend(dmessage(this, object, String(), msg, std::move(list)));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, std::vector<Atom>&& list)
{
    lockAudioThread();
    processSend(dmessage(this, object, String(), "list", std::move(list)));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, String const& msg)
{

    lockAudioThread();
    processSend(dmessage(this, object, String(), "symbol", std::vector<Atom>(1, msg)));
    unlockAudioThread();
}

void Instance::sendDirectMessage(void* object, float const msg)
{
    lockAudioThread();
    processSend(dmessage(this, object, String(), "float", std::vector<Atom>(1, msg)));
    unlockAudioThread();
}

void Instance::sendMessagesFromQueue()
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));

    std::function<void(void)> callback;
    while (m_function_queue.try_dequeue(callback)) {
        callback();
    }
}

String Instance::getExtraInfo(File const& toOpen)
{
    String content = toOpen.loadFileAsString();
    if (content.contains("_plugdatainfo_")) {
        return content.fromFirstOccurrenceOf("_plugdatainfo_", false, false).fromFirstOccurrenceOf("[INFOSTART]", false, false).upToFirstOccurrenceOf("[INFOEND]", false, false);
    }

    return {};
}

Patch::Ptr Instance::openPatch(File const& toOpen)
{
    t_canvas* cnv = nullptr;

    String dirname = toOpen.getParentDirectory().getFullPathName().replace("\\", "/");
    auto const* dir = dirname.toRawUTF8();

    String filename = toOpen.getFileName();
    auto const* file = filename.toRawUTF8();

    setThis();

    cnv = static_cast<t_canvas*>(libpd_create_canvas(file, dir));

    return new Patch(cnv, this, true, toOpen);
}

void Instance::setThis() const
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
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
    consoleHandler.logMessage(message);
}

void Instance::logError(String const& error)
{
    consoleHandler.logError(error);
}

void Instance::logWarning(String const& warning)
{
    consoleHandler.logWarning(warning);
}

std::deque<std::tuple<String, int, int>>& Instance::getConsoleMessages()
{
    return consoleHandler.consoleMessages;
}

std::deque<std::tuple<String, int, int>>& Instance::getConsoleHistory()
{
    return consoleHandler.consoleHistory;
}

void Instance::createPanel(int type, char const* snd, char const* location)
{

    auto* obj = generateSymbol(snd)->s_thing;

    auto defaultFile = File(location);

    if (type) {
        MessageManager::callAsync(
            [this, obj, defaultFile]() mutable {
                auto constexpr folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories | FileBrowserComponent::canSelectFiles;
                openChooser = std::make_unique<FileChooser>("Open...", defaultFile, "", SettingsFile::getInstance()->wantsNativeDialog());
                openChooser->launchAsync(folderChooserFlags, [this, obj](FileChooser const& fileChooser) {
                    auto const file = fileChooser.getResult();

                    lockAudioThread();
                    String pathname = file.getFullPathName().toRawUTF8();

                // Convert slashes to backslashes
#if JUCE_WINDOWS
                    pathname = pathname.replaceCharacter('\\', '/');
#endif

                    t_atom argv[1];
                    libpd_set_symbol(argv, pathname.toRawUTF8());
                    pd_typedmess(obj, generateSymbol("callback"), 1, argv);

                    unlockAudioThread();
                });
            });
    } else {
        MessageManager::callAsync(
            [this, obj, defaultFile]() mutable {
                constexpr auto folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectDirectories | FileBrowserComponent::canSelectFiles;
                saveChooser = std::make_unique<FileChooser>("Save...", defaultFile, "", true);

                saveChooser->launchAsync(folderChooserFlags,
                    [this, obj](FileChooser const& fileChooser) {
                        const auto file = fileChooser.getResult();

                        const auto* path = file.getFullPathName().toRawUTF8();

                        t_atom argv[1];
                        libpd_set_symbol(argv, path);

                        lockAudioThread();
                        pd_typedmess(obj, generateSymbol("callback"), 1, argv);
                        unlockAudioThread();
                    });
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

bool Instance::tryLockAudioThread()
{
    if (audioLock.tryEnter()) {
        return true;
    }

    return false;
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




} // namespace pd
