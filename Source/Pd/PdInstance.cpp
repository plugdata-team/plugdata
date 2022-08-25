/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <algorithm>

extern "C" {
#include <g_undo.h>
#include <m_imp.h>

#include "x_libpd_extra_utils.h"
#include "x_libpd_mod_utils.h"
#include "x_libpd_multi.h"
#include "z_print_util.h"
}

#include "PdInstance.h"
#include "PdPatch.h"

extern "C" {
struct pd::Instance::internal {

    static void instance_multi_bang(pd::Instance* ptr, char const* recv)
    {
        ptr->enqueueFunctionAsync([ptr, recv]() { ptr->processMessage({ String("bang"), String(recv) }); });
    }

    static void instance_multi_float(pd::Instance* ptr, char const* recv, float f)
    {
        ptr->enqueueFunctionAsync([ptr, recv, f]() mutable { ptr->processMessage({ String("float"), String(recv), std::vector<Atom>(1, { f }) }); });
    }

    static void instance_multi_symbol(pd::Instance* ptr, char const* recv, char const* sym)
    {
        ptr->enqueueFunctionAsync([ptr, recv, sym]() mutable { ptr->processMessage({ String("symbol"), String(recv), std::vector<Atom>(1, String(sym)) }); });
    }

    static void instance_multi_list(pd::Instance* ptr, char const* recv, int argc, t_atom* argv)
    {
        Message mess { String("list"), String(recv), std::vector<Atom>(argc) };
        for (int i = 0; i < argc; ++i) {
            if (argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv + i));
            else if (argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(String(atom_getsymbol(argv + i)->s_name));
        }

        ptr->enqueueFunctionAsync([ptr, mess]() mutable { ptr->processMessage(std::move(mess)); });
    }

    static void instance_multi_message(pd::Instance* ptr, char const* recv, char const* msg, int argc, t_atom* argv)
    {
        Message mess { msg, String(recv), std::vector<Atom>(argc) };
        for (int i = 0; i < argc; ++i) {
            if (argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv + i));
            else if (argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(String(atom_getsymbol(argv + i)->s_name));
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
        ptr->enqueueFunctionAsync([ptr, channel, value]() mutable { ptr->processMidiEvent({ midievent::PROGRAMCHANGE, channel, value, 0 }); });
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
        auto* concatenatedLine = ptr->printConcatBuffer;
        static int length = 0;
        concatenatedLine[length] = '\0';

        int len = (int)strlen(s);
        while (length + len >= 2048) {
            int d = 2048 - 1 - length;
            strncat(concatenatedLine, s, d);

            // Send concatenated line to PlugData!
            ptr->processPrint(String::fromUTF8(concatenatedLine));
            
            s += d;
            len -= d;
            length = 0;
            concatenatedLine[0] = '\0';
        }

        strncat(concatenatedLine, s, len);
        length += len;

        if (length > 0 && concatenatedLine[length - 1] == '\n') {
            concatenatedLine[length - 1] = '\0';

            // Send concatenated line to PlugData!
            ptr->processPrint(String::fromUTF8(concatenatedLine));

            length = 0;
        }
    }
};
}

namespace pd {

Instance::Instance(String const& symbol) : fastStringWidth(Font(14))
{
    libpd_multi_init();

    m_instance = libpd_new_instance();

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    m_midi_receiver = libpd_multi_midi_new(this, reinterpret_cast<t_libpd_multi_noteonhook>(internal::instance_multi_noteon), reinterpret_cast<t_libpd_multi_controlchangehook>(internal::instance_multi_controlchange), reinterpret_cast<t_libpd_multi_programchangehook>(internal::instance_multi_programchange),
        reinterpret_cast<t_libpd_multi_pitchbendhook>(internal::instance_multi_pitchbend), reinterpret_cast<t_libpd_multi_aftertouchhook>(internal::instance_multi_aftertouch), reinterpret_cast<t_libpd_multi_polyaftertouchhook>(internal::instance_multi_polyaftertouch),
        reinterpret_cast<t_libpd_multi_midibytehook>(internal::instance_multi_midibyte));
    m_print_receiver = libpd_multi_print_new(this, reinterpret_cast<t_libpd_multi_printhook>(internal::instance_multi_print));

    m_message_receiver = libpd_multi_receiver_new(this, "pd", reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));

    m_parameter_receiver = libpd_multi_receiver_new(this, "param", reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang), reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float), reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
        reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list), reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));
    m_atoms = malloc(sizeof(t_atom) * 512);

    // Register callback when pd's gui changes
    // Needs to be done on pd's thread
    auto gui_trigger = [](void* instance, void* target) {
        auto* pd = static_cast<t_pd*>(target);

        // redraw scalar
        if (pd && !strcmp((*pd)->c_name->s_name, "scalar")) {
            static_cast<Instance*>(instance)->receiveGuiUpdate(2);
        } else {
            static_cast<Instance*>(instance)->receiveGuiUpdate(1);
        }
    };

    auto panel_trigger = [](void* instance, int open, char const* snd, char const* location) { static_cast<Instance*>(instance)->createPanel(open, snd, location); };

    auto parameter_trigger = [](void* instance) {
        static_cast<Instance*>(instance)->receiveGuiUpdate(3);
    };

    auto synchronise_trigger = [](void* instance, void* cnv) { static_cast<Instance*>(instance)->synchroniseCanvas(cnv); };

    register_gui_triggers(static_cast<t_pdinstance*>(m_instance), this, gui_trigger, panel_trigger, synchronise_trigger, parameter_trigger);

    // HACK: create full path names for c-coded externals
    int i;
    t_class* o = pd_objectmaker;

    t_methodentry *mlist, *m;

#if PDINSTANCE
    mlist = o->c_methods[pd_this->pd_instanceno];
#else
    mlist = o->c_methods;
#endif

    bool insideElse = false;
    bool insideCyclone = false;

    std::vector<std::tuple<String, t_newmethod, std::array<t_atomtype, 6>>> newMethods;

    // First find all the objects that need a full path and put them in a list
    // Adding new entries while iterating over them is a bad idea
    for (i = o->c_nmethod, m = mlist; i--; m++) {
        String name(m->me_name->s_name);

        if (name == "accum") {
            insideCyclone = true;
        }
        if (name == "above~") {
            insideElse = true;
        }

        if ((insideCyclone || insideElse) && !(name.contains("cyclone") || name.contains("else") || name == "Pow~" || name == "del~")) {
            auto newName = insideCyclone ? "cyclone/" + name : "else/" + name;

            std::array<t_atomtype, 6> args;
            for (int n = 0; n < 6; n++) {
                args[n] = static_cast<t_atomtype>(m->me_arg[n]);
            }

            auto* method = reinterpret_cast<t_newmethod>(m->me_fun);

            newMethods.push_back({ newName, method, args });
        }
        if (name == "zerox~") {
            insideCyclone = false;
        }
        if (name == "zerocross~") {
            insideElse = false;
        }
    }

    // Then create aliases for all these objects
    // We seperate this process in two parts because adding new objects while looping through objects causes problems
    for (auto [name, method, args] : newMethods) {
        class_addcreator(method, gensym(name.toRawUTF8()), args[0], args[1], args[2], args[3], args[4], args[5]);
    }

    libpd_set_verbose(0);

    setThis();
}

Instance::~Instance()
{
    pd_free(static_cast<t_pd*>(m_message_receiver));
    pd_free(static_cast<t_pd*>(m_midi_receiver));
    pd_free(static_cast<t_pd*>(m_print_receiver));
    pd_free(static_cast<t_pd*>(m_parameter_receiver));

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_free_instance(static_cast<t_pdinstance*>(m_instance));
}

int Instance::getBlockSize() const
{
    return libpd_blocksize();
}

void Instance::prepareDSP(int const nins, int const nouts, double const samplerate, int const blockSize)
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_init_audio(nins, nouts, static_cast<int>(samplerate));
    continuityChecker.prepare(samplerate, blockSize, std::max(nins, nouts));
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
    if (!m_instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_bang(receiver);
}

void Instance::sendFloat(char const* receiver, float const value) const
{
    if (!m_instance)
        return;

    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    libpd_float(receiver, value);
}

void Instance::sendSymbol(char const* receiver, char const* symbol) const
{
    if (!m_instance)
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

void Instance::sendMessage(char const* receiver, char const* msg, std::vector<Atom> const& list) const
{
    if (!static_cast<t_pdinstance*>(m_instance))
        return;

    auto* argv = static_cast<t_atom*>(m_atoms);
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
    for (size_t i = 0; i < list.size(); ++i) {
        if (list[i].isFloat())
            libpd_set_float(argv + i, list[i].getFloat());
        else
            libpd_set_symbol(argv + i, list[i].getSymbol().toRawUTF8());
    }
    libpd_message(receiver, msg, static_cast<int>(list.size()), argv);
}

void Instance::processMessage(Message mess)
{
    if (mess.destination == "param") {
        int index = mess.list[0].getFloat();
        float value = std::clamp(mess.list[1].getFloat(), 0.0f, 1.0f);
        receiveParameter(index, value);
    } else if (mess.selector == "bang")
        receiveBang(mess.destination);
    else if (mess.selector == "float")
        receiveFloat(mess.destination, mess.list[0].getFloat());
    else if (mess.selector == "symbol")
        receiveSymbol(mess.destination, mess.list[0].getSymbol());
    else if (mess.selector == "list")
        receiveList(mess.destination, mess.list);
    else if (mess.selector == "dsp")
        receiveDSPState(mess.list[0].getFloat());
    else
        receiveMessage(mess.destination, mess.selector, mess.list);
}

void Instance::processMidiEvent(midievent event)
{
    if (event.type == midievent::NOTEON)
        receiveNoteOn(event.midi1 + 1, event.midi2, event.midi3);
    else if (event.type == midievent::CONTROLCHANGE)
        receiveControlChange(event.midi1 + 1, event.midi2, event.midi3);
    else if (event.type == midievent::PROGRAMCHANGE)
        receiveProgramChange(event.midi1 + 1, event.midi2);
    else if (event.type == midievent::PITCHBEND)
        receivePitchBend(event.midi1 + 1, event.midi2);
    else if (event.type == midievent::AFTERTOUCH)
        receiveAftertouch(event.midi1 + 1, event.midi2);
    else if (event.type == midievent::POLYAFTERTOUCH)
        receivePolyAftertouch(event.midi1 + 1, event.midi2, event.midi3);
    else if (event.type == midievent::MIDIBYTE)
        receiveMidiByte(event.midi1, event.midi2);
}

void Instance::processPrint(String print)
{
    MessageManager::callAsync([this, print]() mutable {
        receivePrint(print);
    });
}

void Instance::processSend(dmessage mess)
{
    if (mess.object && !mess.list.empty()) {
        if (mess.selector == "list") {
            auto* argv = static_cast<t_atom*>(m_atoms);
            for (size_t i = 0; i < mess.list.size(); ++i) {
                if (mess.list[i].isFloat())
                    SETFLOAT(argv + i, mess.list[i].getFloat());
                else if (mess.list[i].isSymbol()) {
                    sys_lock();
                    SETSYMBOL(argv + i, gensym(mess.list[i].getSymbol().toRawUTF8()));
                    sys_unlock();
                } else
                    SETFLOAT(argv + i, 0.0);
            }
            sys_lock();
            pd_list(static_cast<t_pd*>(mess.object), gensym("list"), static_cast<int>(mess.list.size()), argv);
            sys_unlock();
        } else if (mess.selector == "float" && mess.list[0].isFloat()) {
            sys_lock();
            pd_float(static_cast<t_pd*>(mess.object), mess.list[0].getFloat());
            sys_unlock();
        } else if (mess.selector == "symbol") {
            sys_lock();
            pd_symbol(static_cast<t_pd*>(mess.object), gensym(mess.list[0].getSymbol().toRawUTF8()));
            sys_unlock();
        }
    } else {
        sendMessage(mess.destination.toRawUTF8(), mess.selector.toRawUTF8(), mess.list);
    }
}

void Instance::enqueueFunction(std::function<void(void)> const& fn)
{
    // This should be the way to do it, but it currently causes some issues
    // By calling fn directly we fix these issues at the cost of possible thread unsafety
    m_function_queue.enqueue(fn);

    // Checks if it can be performed immediately
    messageEnqueued();
}

void Instance::enqueueFunctionAsync(std::function<void(void)> const& fn)
{
    // This should be the way to do it, but it currently causes some issues
    // By calling fn directly we fix these issues at the cost of possible thread unsafety
    m_function_queue.enqueue(fn);
}

void Instance::enqueueMessages(String const& dest, String const& msg, std::vector<Atom>&& list)
{
    enqueueFunction([this, dest, msg, list]() mutable { processSend(dmessage { nullptr, dest, msg, std::move(list) }); });
}

void Instance::enqueueDirectMessages(void* object, std::vector<Atom> const& list)
{
    enqueueFunction([this, object, list]() mutable { processSend(dmessage { object, String(), "list", list }); });
}

void Instance::enqueueDirectMessages(void* object, String const& msg)
{
    enqueueFunction([this, object, msg]() mutable { processSend(dmessage { object, String(), "symbol", std::vector<Atom>(1, msg) }); });
}

void Instance::enqueueDirectMessages(void* object, float const msg)
{
    enqueueFunction([this, object, msg]() mutable { processSend(dmessage { object, String(), "float", std::vector<Atom>(1, msg) }); });
}

void Instance::waitForStateUpdate()
{
    // No action needed
    if (m_function_queue.size_approx() == 0) {
        return;
    }

    //  Append signal to resume thread at the end of the queue
    //  This will make sure that any actions we performed are definitely finished now
    //  If it can aquire a lock, it will dequeue all action immediately
    enqueueFunction([this]() { updateWait.signal(); });

    updateWait.wait();
}

void Instance::sendMessagesFromQueue()
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));

    std::function<void(void)> callback;
    while (m_function_queue.try_dequeue(callback)) {
        callback();
    }
}

Patch Instance::openPatch(File const& toOpen)
{
    t_canvas* cnv = nullptr;

    bool done = false;
    enqueueFunction(
        [this, toOpen, &cnv, &done]() mutable {
            String dirname = toOpen.getParentDirectory().getFullPathName();
            const auto* dir = dirname.toRawUTF8();

            String filename = toOpen.getFileName();
            const auto* file = filename.toRawUTF8();

            setThis();

            cnv = static_cast<t_canvas*>(libpd_create_canvas(file, dir));
            done = true;
        });

    while (!done) {
        waitForStateUpdate();
    }

    return Patch(cnv, this, toOpen);
}

void Instance::setThis()
{
    libpd_set_instance(static_cast<t_pdinstance*>(m_instance));
}

void Instance::createPanel(int type, char const* snd, char const* location)
{

    auto* obj = gensym(snd)->s_thing;

    auto defaultFile = File(location);

    if (type) {
        MessageManager::callAsync(
            [this, obj, defaultFile]() mutable {
                auto constexpr folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories | FileBrowserComponent::canSelectFiles;
                openChooser = std::make_unique<FileChooser>("Open...", defaultFile, "", true);
                openChooser->launchAsync(folderChooserFlags,
                    [this, obj](FileChooser const& fileChooser) {
                        auto const file = fileChooser.getResult();
                        enqueueFunction(
                            [obj, file]() mutable {
                                String pathname = file.getFullPathName().toRawUTF8();

                    // Convert slashes to backslashes
#if JUCE_WINDOWS
                                pathname = pathname.replaceCharacter('\\', '/');
#endif

                                t_atom argv[1];
                                libpd_set_symbol(argv, pathname.toRawUTF8());
                                pd_typedmess(obj, gensym("callback"), 1, argv);
                            });
                    });
            });
    } else {
        MessageManager::callAsync(
            [this, obj, defaultFile]() mutable {
                auto constexpr folderChooserFlags = FileBrowserComponent::saveMode | FileBrowserComponent::canSelectDirectories | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting;
                saveChooser = std::make_unique<FileChooser>("Save...", defaultFile, "", true);

                saveChooser->launchAsync(folderChooserFlags,
                    [this, obj](FileChooser const& fileChooser) {
                        auto const file = fileChooser.getResult();
                        enqueueFunction(
                            [obj, file]() mutable {
                                const auto* path = file.getFullPathName().toRawUTF8();

                                t_atom argv[1];
                                libpd_set_symbol(argv, path);
                                pd_typedmess(obj, gensym("callback"), 1, argv);
                            });
                    });
            });
    }
}

void Instance::logMessage(String const& message)
{
    
    consoleMessages.emplace_back(message, 0, fastStringWidth.getStringWidth(message) + 12);

    if (consoleMessages.size() > 800)
        consoleMessages.pop_front();

    updateConsole();
}

void Instance::logError(String const& error)
{
    consoleMessages.emplace_back(error, 1, fastStringWidth.getStringWidth(error) + 12);

    if (consoleMessages.size() > 800)
        consoleMessages.pop_front();

    updateConsole();
}
} // namespace pd
