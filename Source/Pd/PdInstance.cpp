/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


#include <algorithm>

extern "C"
{
#include <g_undo.h>
#include "x_libpd_multi.h"
#include "x_libpd_extra_utils.h"
#include "x_libpd_mod_utils.h"
}

#include "PdInstance.hpp"
#include "PdPatch.hpp"


extern "C"
{

struct _instanceeditor
{
    t_binbuf *copy_binbuf;
    char *canvas_textcopybuf;
    int canvas_textcopybufsize;
    t_undofn canvas_undo_fn;         /* current undo function if any */
    int canvas_undo_whatnext;        /* whether we can now UNDO or REDO */
    void *canvas_undo_buf;           /* data private to the undo function */
    t_canvas *canvas_undo_canvas;    /* which canvas we can undo on */
    const char *canvas_undo_name;
    int canvas_undo_already_set_move;
    double canvas_upclicktime;
    int canvas_upx, canvas_upy;
    int canvas_find_index, canvas_find_wholeword;
    t_binbuf *canvas_findbuf;
    int paste_onset;
    t_canvas *paste_canvas;
    t_glist *canvas_last_glist;
    int canvas_last_glist_x, canvas_last_glist_y;
    t_canvas *canvas_cursorcanvaswas;
    unsigned int canvas_cursorwas;
};

struct pd::Instance::internal
{
    static void instance_multi_bang(pd::Instance* ptr, const char *recv)
    {
        ptr->m_message_queue.try_enqueue({std::string("bang"), std::string(recv)});
    }
    
    static void instance_multi_float(pd::Instance* ptr, const char *recv, float f)
    {
        ptr->m_message_queue.try_enqueue({std::string("float"), std::string(recv), std::vector<Atom>(1, f)});
    }
    
    static void instance_multi_symbol(pd::Instance* ptr, const char *recv, const char *sym)
    {
        ptr->m_message_queue.try_enqueue({std::string("symbol"), std::string(recv), std::vector<Atom>(1, std::string(sym))});
    }
    
    static void instance_multi_list(pd::Instance* ptr, const char *recv, int argc, t_atom *argv)
    {
        Message mess{std::string("list"), std::string(recv), std::vector<Atom>(argc)};
        for(int i = 0; i < argc; ++i)
        {
            if(argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv+i));
            else if(argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(std::string(atom_getsymbol(argv+i)->s_name));
        }
        ptr->m_message_queue.try_enqueue(std::move(mess));
    }
    
    static void instance_multi_message(pd::Instance* ptr, const char *recv, const char *msg, int argc, t_atom *argv)
    {
        Message mess{msg, std::string(recv), std::vector<Atom>(argc)};
        for(int i = 0; i < argc; ++i)
        {
            if(argv[i].a_type == A_FLOAT)
                mess.list[i] = Atom(atom_getfloat(argv+i));
            else if(argv[i].a_type == A_SYMBOL)
                mess.list[i] = Atom(std::string(atom_getsymbol(argv+i)->s_name));
        }
        ptr->m_message_queue.try_enqueue(std::move(mess));
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////
    
    static void instance_multi_noteon(pd::Instance* ptr, int channel, int pitch, int velocity)
    {
        ptr->m_midi_queue.try_enqueue({midievent::NOTEON, channel, pitch, velocity});
    }
    
    static void instance_multi_controlchange(pd::Instance* ptr, int channel, int controller, int value)
    {
        ptr->m_midi_queue.try_enqueue({midievent::CONTROLCHANGE, channel, controller, value});
    }
    
    static void instance_multi_programchange(pd::Instance* ptr, int channel, int value)
    {
        ptr->m_midi_queue.try_enqueue({midievent::PROGRAMCHANGE, channel, value, 0});
    }
    
    static void instance_multi_pitchbend(pd::Instance* ptr, int channel, int value)
    {
        ptr->m_midi_queue.try_enqueue({midievent::PITCHBEND, channel, value, 0});
    }
    
    static void instance_multi_aftertouch(pd::Instance* ptr, int channel, int value)
    {
        ptr->m_midi_queue.try_enqueue({midievent::AFTERTOUCH, channel, value, 0});
    }
    
    static void instance_multi_polyaftertouch(pd::Instance* ptr, int channel, int pitch, int value)
    {
        ptr->m_midi_queue.try_enqueue({midievent::POLYAFTERTOUCH, channel, pitch, value});
    }
    
    static void instance_multi_midibyte(pd::Instance* ptr, int port, int byte)
    {
        ptr->m_midi_queue.try_enqueue({midievent::MIDIBYTE, port, byte, 0});
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////
    
    static void instance_multi_print(pd::Instance* ptr, char const* s)
    {
        ptr->m_print_queue.try_enqueue(std::string(s));
    }
};

}

namespace pd
{
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

Instance::Instance(std::string const& symbol)
{
    libpd_multi_init();
    m_instance = libpd_new_instance();
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    m_midi_receiver = libpd_multi_midi_new(this,
                                           reinterpret_cast<t_libpd_multi_noteonhook>(internal::instance_multi_noteon),
                                           reinterpret_cast<t_libpd_multi_controlchangehook>(internal::instance_multi_controlchange),
                                           reinterpret_cast<t_libpd_multi_programchangehook>(internal::instance_multi_programchange),
                                           reinterpret_cast<t_libpd_multi_pitchbendhook>(internal::instance_multi_pitchbend),
                                           reinterpret_cast<t_libpd_multi_aftertouchhook>(internal::instance_multi_aftertouch),
                                           reinterpret_cast<t_libpd_multi_polyaftertouchhook>(internal::instance_multi_polyaftertouch),
                                           reinterpret_cast<t_libpd_multi_midibytehook>(internal::instance_multi_midibyte));
    m_print_receiver = libpd_multi_print_new(this,
                                             reinterpret_cast<t_libpd_multi_printhook>(internal::instance_multi_print));
    
    m_message_receiver[0] = libpd_multi_receiver_new(this, symbol.c_str(),
                                                     reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang),
                                                     reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float),
                                                     reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
                                                     reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list),
                                                     reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message));
    m_atoms = malloc(sizeof(t_atom) * 512);
    
    libpd_set_verbose(0);
    setThis();
}

Instance::~Instance()
{
    closePatch();
    for(int i = 0; i < m_message_receiver.size(); i++)
        pd_free((t_pd *)m_message_receiver[i]);
    
    pd_free((t_pd *)m_midi_receiver);
    pd_free((t_pd *)m_print_receiver);
    
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_free_instance(static_cast<t_pdinstance *>(m_instance));
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

int Instance::getBlockSize() const noexcept
{
    return libpd_blocksize();
}

void Instance::addListener(const char* sym)
{
    
    m_message_receiver.push_back(libpd_multi_receiver_new(this, sym,
                                                          reinterpret_cast<t_libpd_multi_banghook>(internal::instance_multi_bang),
                                                          reinterpret_cast<t_libpd_multi_floathook>(internal::instance_multi_float),
                                                          reinterpret_cast<t_libpd_multi_symbolhook>(internal::instance_multi_symbol),
                                                          reinterpret_cast<t_libpd_multi_listhook>(internal::instance_multi_list),
                                                          reinterpret_cast<t_libpd_multi_messagehook>(internal::instance_multi_message)));
    
}

void Instance::prepareDSP(const int nins, const int nouts, const double samplerate)
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_init_audio(nins, nouts, (int)samplerate);
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
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_set_float(&av, 0.f);
    libpd_message("pd", "dsp", 1, &av);
}

void Instance::performDSP(float const* inputs, float* outputs)
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_process_raw(inputs, outputs);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void Instance::sendNoteOn(const int channel, const int pitch, const int velocity) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_noteon(channel-1, pitch, velocity);
}

void Instance::sendControlChange(const int channel, const int controller, const int value) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_controlchange(channel-1, controller, value);
}

void Instance::sendProgramChange(const int channel, const int value) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_programchange(channel-1, value);
}

void Instance::sendPitchBend(const int channel, const int value) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_pitchbend(channel-1, value);
}

void Instance::sendAfterTouch(const int channel, const int value) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_aftertouch(channel-1, value);
}

void Instance::sendPolyAfterTouch(const int channel, const int pitch, const int value) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_polyaftertouch(channel-1, pitch, value);
}

void Instance::sendSysEx(const int port, const int byte) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_sysex(port, byte);
}

void Instance::sendSysRealTime(const int port, const int byte) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_sysrealtime(port, byte);
}

void Instance::sendMidiByte(const int port, const int byte) const
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_midibyte(port, byte);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void Instance::sendBang(const char* receiver) const
{
    if(!m_instance)
        return;
    
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_bang(receiver);
}

void Instance::sendFloat(const char* receiver, float const value) const
{
    if(!m_instance)
        return;
    
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_float(receiver, value);
}

void Instance::sendSymbol(const char* receiver, const char* symbol) const
{
    if(!m_instance)
        return;
    
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    libpd_symbol(receiver, symbol);
}

void Instance::sendList(const char* receiver, const std::vector<Atom>& list) const
{
    if(!static_cast<t_pdinstance *>(m_instance))
        return;
    
    t_atom* argv = static_cast<t_atom*>(m_atoms);
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    for(size_t i = 0; i < list.size(); ++i)
    {
        if(list[i].isFloat())
            libpd_set_float(argv+i, list[i].getFloat());
        else
            libpd_set_symbol(argv+i, list[i].getSymbol().c_str());
    }
    libpd_list(receiver, (int)list.size(), argv);
}

void Instance::sendMessage(const char* receiver, const char* msg, const std::vector<Atom>& list) const
{
    if(!static_cast<t_pdinstance *>(m_instance))
        return;
    
    t_atom* argv = static_cast<t_atom*>(m_atoms);
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    for(size_t i = 0; i < list.size(); ++i)
    {
        if(list[i].isFloat())
            libpd_set_float(argv+i, list[i].getFloat());
        else
            libpd_set_symbol(argv+i, list[i].getSymbol().c_str());
    }
    libpd_message(receiver, msg, (int)list.size(), argv);
}

void Instance::processMessages()
{
    Message mess;
    while(m_message_queue.try_dequeue(mess))
    {
        if(mess.selector == "bang")
            receiveBang(mess.destination);
        else if(mess.selector == "float")
            receiveFloat(mess.destination, mess.list[0].getFloat());
        else if(mess.selector == "symbol")
            receiveSymbol(mess.destination, mess.list[0].getSymbol());
        else if(mess.selector == "list")
            receiveList(mess.destination, mess.list);
        else
            receiveMessage(mess.destination, mess.selector, mess.list);
    }
}

void Instance::processMidi()
{
    midievent event;
    while(m_midi_queue.try_dequeue(event))
    {
        if(event.type == midievent::NOTEON)
            receiveNoteOn(event.midi1+1, event.midi2, event.midi3);
        else if(event.type == midievent::CONTROLCHANGE)
            receiveControlChange(event.midi1+1, event.midi2, event.midi3);
        else if(event.type == midievent::PROGRAMCHANGE)
            receiveProgramChange(event.midi1+1, event.midi2);
        else if(event.type == midievent::PITCHBEND)
            receivePitchBend(event.midi1+1, event.midi2);
        else if(event.type == midievent::AFTERTOUCH)
            receiveAftertouch(event.midi1+1, event.midi2);
        else if(event.type == midievent::POLYAFTERTOUCH)
            receivePolyAftertouch(event.midi1+1, event.midi2, event.midi3);
        else if(event.type == midievent::MIDIBYTE)
            receiveMidiByte(event.midi1, event.midi2);
    }
}

void Instance::processPrints()
{
    std::string temp;
    std::string print;
    while(m_print_queue.try_dequeue(print))
    {
        if(print.size() && print.back() == '\n')
        {
            while(print.size() && (print.back() == '\n' || print.back() == ' ')) {
                print.pop_back();
            }
            temp += print;
            receivePrint(temp);
            temp.clear();
        }
        else
        {
            temp += print;
        }
    }
}

void Instance::enqueueFunction(std::function<void(void)> fn) {
    m_function_queue.try_enqueue(fn);
}

void Instance::enqueueMessages(const std::string& dest, const std::string& msg, std::vector<Atom>&& list)
{
    m_send_queue.try_enqueue(dmessage{nullptr, dest, msg, std::move(list)});
    messageEnqueued();
}



void Instance::enqueueDirectMessages(void* object, const std::string& msg)
{
    m_send_queue.try_enqueue(dmessage{object, std::string(), std::string(), std::vector<Atom>(1, msg)});
    messageEnqueued();
}

void Instance::enqueueDirectMessages(void* object, const float msg)
{
    m_send_queue.try_enqueue(dmessage{object, std::string(), std::string(), std::vector<Atom>(1, msg)});
    messageEnqueued();
}

void Instance::dequeueMessages()
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    dmessage mess;
    
    std::function<void(void)> callback;
    
    while(m_function_queue.try_dequeue(callback))
    {
        sys_lock();
        callback();
        sys_unlock();
    }
    
    while(m_send_queue.try_dequeue(mess))
    {
        if(mess.object && !mess.list.empty())
        {
            if(mess.list[0].isFloat())
            {
                sys_lock();
                pd_float(static_cast<t_pd *>(mess.object), mess.list[0].getFloat());
                sys_unlock();
            }
            else if (!mess.selector.empty()) {
                int argc = mess.list.size();
                t_atom argv[argc];
                
                for(int i = 0; i < argc; i++) {
                    if(mess.list[i].isFloat()) {
                        SETFLOAT(argv + i, mess.list[i].getFloat());
                    }
                    else if (mess.list[i].isSymbol()) {
                        SETSYMBOL(argv + i, gensym(mess.list[i].getSymbol().c_str()));
                    }
                }
                pd_typedmess(static_cast<t_pd *>(mess.object), gensym(mess.selector.c_str()), argc, argv);
            }
            else {
                sys_lock();
                pd_symbol(static_cast<t_pd *>(mess.object), gensym(mess.list[0].getSymbol().c_str()));
                sys_unlock();
            }
        }
        else
        {
            sendMessage(mess.destination.c_str(), mess.selector.c_str(), mess.list);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////



void Instance::openPatch(std::string const& path, std::string const& name)
{
    closePatch();
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
    m_patch = libpd_create_canvas(name.c_str(), path.c_str());
    setThis();
    
}

void Instance::closePatch()
{
    if(m_patch)
    {
        libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
        libpd_closefile(m_patch);
        m_patch = nullptr;
    }
}

Patch Instance::getPatch()
{
    return Patch(m_patch, this);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

Array Instance::getArray(std::string const& name)
{
    return Array(name, m_instance);
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void Instance::setThis()
{
    libpd_set_instance(static_cast<t_pdinstance *>(m_instance));
}

void Instance::stringToAtom(String name, int& argc, t_atom& target)
{
    /*

    for(int i = 0; i < argc; i++) {
        if(tokens[i].containsOnly("0123456789.-") && tokens[i] != "-") {
            SETFLOAT(argv + i + 2, tokens[i].getFloatValue());
        }
        else {
            SETSYMBOL(argv + i + 2, gensym(tokens[i].toRawUTF8()));
        }
    }
    */
}
t_pd* Instance::createGraphOnParent() {
    // TODO: implement this
    return nullptr;
}

t_pd* Instance::createGraph(String name, int size)
{
    int argc = 4;
    
    auto argv = std::vector<t_atom>(argc);
    
    SETSYMBOL(argv.data(), gensym(name.toRawUTF8()));
    
    // Set position
    SETFLOAT(argv.data() + 1, size);
    SETFLOAT(argv.data() + 2, 1);
    SETFLOAT(argv.data() + 3, 0);

    setThis();
    t_pd* pdobject = libpd_creategraph(static_cast<t_pd*>(m_patch), argc, argv.data());
    
    return pdobject;
}

t_pd* Instance::createObject(String name, int x, int y)
{
    
    if(!m_patch) return nullptr;
    
    StringArray tokens;
    tokens.addTokens(name, " ", "");
    
    if(tokens[0] == "graph" && tokens.size() == 3) {
        return createGraph(tokens[1], tokens[2].getIntValue());
    }
    
    if(tokens[0] == "sub")  {
        return createGraphOnParent();
    }
    
    t_symbol* typesymbol =  gensym("obj");
    
    if(tokens[0] == "msg") {
        typesymbol = gensym("msg");
        tokens.remove(0);
    }
    if(tokens[0] == "floatatom") {
        typesymbol = gensym("floatatom");
        tokens.remove(0);
    }
    if(tokens[0] == "symbolatom") {
        typesymbol = gensym("symbolatom");
        tokens.remove(0);
    }

    int argc = tokens.size() + 2;
    
    auto argv = std::vector<t_atom>(argc);
    
    // Set position
    SETFLOAT(argv.data(), (float)x);
    SETFLOAT(argv.data() + 1, (float)y);
    
    for(int i = 0; i < tokens.size(); i++) {
        if(tokens[i].containsOnly("0123456789.-") && tokens[i] != "-") {
            SETFLOAT(argv.data() + i + 2, tokens[i].getFloatValue());
        }
        else {
            SETSYMBOL(argv.data() + i + 2, gensym(tokens[i].toRawUTF8()));
        }
    }
    
    
    setThis();
    t_pd* pdobject = libpd_createobj(static_cast<t_pd*>(m_patch), typesymbol, argc, argv.data());
    
    return pdobject;
}


void Instance::removeObject(t_pd* obj)
{
    if(!obj || !m_patch) return;
    // not sure if this works
    //canvas_deletelinesfor(static_cast<t_pd*>(m_patch), static_cast<t_text*>(pd_checkobject(obj)));
    setThis();
    libpd_removeobj(static_cast<t_canvas*>(m_patch), &pd_checkobject(obj)->te_g);
    
}

bool Instance::createConnection(t_pd* src, int nout, t_pd* sink, int nin)
{
    if(!src || !sink || !m_patch) return false;
    
    bool can_connect = libpd_canconnect(static_cast<t_canvas*>(m_patch), pd_checkobject(src), nout, pd_checkobject(sink), nin);
    
    if(!can_connect) return false;
    
    setThis();
    libpd_createconnection(static_cast<t_canvas*>(m_patch), pd_checkobject(src), nout, pd_checkobject(sink), nin);
    
    return true;
}

void Instance::removeConnection(t_pd* src, int nout, t_pd*sink, int nin)
{
    if(!src || !sink || !m_patch) return;
    setThis();
    libpd_removeconnection(static_cast<t_canvas*>(m_patch), pd_checkobject(src), nout, pd_checkobject(sink), nin);
}


t_pd* Instance::renameObject(t_pd* obj, String name) {
    if(!obj || !m_patch) return nullptr;
    setThis();
    
    libpd_renameobj(static_cast<t_canvas*>(m_patch), &pd_checkobject(obj)->te_g, name.toRawUTF8(), name.length());
    
    
    
    return static_cast<t_pdinstance*>(m_instance)->pd_newest;
    
    
    

}
void Instance::moveObject(t_pd* obj, int x, int y) {
    if(!obj || !m_patch) return;
    setThis();
    libpd_moveobj(static_cast<t_canvas*>(m_patch), &pd_checkobject(obj)->te_g, x, y);
    
}

void Instance::undo() {
    setThis();

    auto* instance = static_cast<_pdinstance*>(m_instance);
    
    instance->pd_gui->i_editor->canvas_undo_already_set_move = 0;
    
    pd_typedmess(static_cast<t_pd*>(m_patch), gensym("undo"), 0, nullptr);
}

void Instance::redo() {
    setThis();
    
    //auto* instance = static_cast<_pdinstance*>(m_instance);
    //instance->pd_gui->i_editor->canvas_undo_already_set_move = 0;
    pd_typedmess(static_cast<t_pd*>(m_patch), gensym("redo"), 0, nullptr);
}

bool Instance::checkState(String pdstate) {
    /*
    StringArray currentstate;
    currentstate.addTokens(getCanvasContent(), ";\n", "");
    
    StringArray newstate;
    newstate.addTokens(pdstate, ";\n", "");
    
    for(auto line : currentstate) {
        StringArray singleline;
        singleline.addTokens(line, " ", "");
        if(!newstate.contains(line)) {
            if(singleline[1] == "obj") {
                // DELETE OBJECT FROM PD PATCH
            else if(singleline[1] == "connect") {
                // DELETE CONNECTION FROM PD PATCH
            
        }
    }
    
    for(auto line : newstate) {
        StringArray singleline;
        singleline.addTokens(line, " ", "");
        if(!currentstate.contains(line)) {
            if(singleline[1] == "obj") {
                // ADD OBJECT FROM PD PATCH
            else if(singleline[1] == "connect") {
                // ADD CONNECTION FROM PD PATCH
                
            }
        }
    }
    
    */
}

String Instance::getCanvasContent() {
    
    char* buf;
    int bufsize;
    
    libpd_getcontent(static_cast<t_canvas*>(m_patch), &buf, &bufsize);
    
    return String(buf, bufsize);
    
}

}
