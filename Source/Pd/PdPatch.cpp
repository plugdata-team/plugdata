/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdPatch.hpp"
#include "PdObject.hpp"
#include "PdGui.hpp"
#include "PdInstance.hpp"

extern "C"
{
#include <z_libpd.h>
#include <m_pd.h>
#include <g_canvas.h>
#include "x_libpd_multi.h"

}

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
namespace pd
{    
// ==================================================================================== //
//                                          PATCHER                                     //
// ==================================================================================== //



Patch::Patch(void* ptr, Instance* instance) noexcept :
m_ptr(ptr), m_instance(instance)
{
    if(ptr) {
        setZoom(1);
    }
    
}

std::array<int, 4> Patch::getBounds() const noexcept
{
    if(m_ptr)
    {
        t_canvas const* cnv = getPointer();
        
        if(cnv->gl_isgraph)
        {
            return {int(cnv->gl_xmargin * pd::Patch::zoom) + 4, int(cnv->gl_ymargin * pd::Patch::zoom) + 4, cnv->gl_pixwidth, cnv->gl_pixheight};
        }
    }
    return {0, 0, 0, 0};
}

void Patch::setCurrent() {
    
    m_instance->setThis();
    
    
    m_instance->canvasLock.lock();
    canvas_setcurrent(getPointer());
    canvas_vis(getPointer(), 1.);
    m_instance->canvasLock.unlock();
    
    
}

t_canvas* Patch::getCurrent()
{
       
    Instance::canvasLock.lock();
    auto* current = canvas_getcurrent();
    Instance::canvasLock.unlock();
    return current;
}

std::vector<Object> Patch::getObjects(bool only_gui) noexcept
{
    if(m_ptr)
    {
        std::vector<Object> objects;
        t_canvas const* cnv = getPointer();
        
        for(t_gobj *y = cnv->gl_list; y; y = y->g_next)
        {
            Object object(static_cast<void*>(y), this, m_instance);
            
            if(only_gui) {
                Gui gui(static_cast<void*>(y), this, m_instance);
                if(gui.getType() != Type::Undefined)
                {
                    objects.push_back(object);
                }
            }
            else {
                objects.push_back(object);
            }
            
            
        }
        return objects;
    }
    return std::vector<Object>();
}


std::unique_ptr<Object> Patch::createGraphOnParent(int x, int y) {
    
    
    t_pd* pdobject = nullptr;
    m_instance->enqueueFunction([this, x, y, &pdobject]() mutable {
        m_instance->setThis();
        pdobject = libpd_creategraphonparent(static_cast<t_canvas*>(m_ptr), x, y);
    });
    
    m_instance->waitForStateUpdate();
    
    
    assert(pdobject);
    
    return std::make_unique<Gui>(pdobject, this, m_instance);
}

std::unique_ptr<Object> Patch::createGraph(String name, int size, int x, int y)
{
    t_pd* pdobject = nullptr;
    m_instance->enqueueFunction([this, name, size, x, y, &pdobject]() mutable {
        m_instance->setThis();
        pdobject = libpd_creategraph(static_cast<t_canvas*>(m_ptr), name.toRawUTF8(), size, x, y);
    });
    
    m_instance->waitForStateUpdate();
    
    assert(pdobject);
    
    return std::make_unique<Gui>(pdobject, this, m_instance);
}

std::unique_ptr<Object> Patch::createObject(String name, int x, int y)
{
    
    if(!m_ptr) return nullptr;
    
    x /= zoom;
    y /= zoom;
    
    StringArray tokens;
    tokens.addTokens(name, " ", "");
    
    if(tokens[0] == "graph" && tokens.size() == 3) {
        return createGraph(tokens[1], tokens[2].getIntValue(), x, y);
    }
    else if(tokens[0] == "graph") {
        return createGraphOnParent(x, y);
    }
    
    t_symbol* typesymbol = gensym("obj");
    
    if(tokens[0] == "msg") {
        typesymbol = gensym("msg");
        tokens.remove(0);
    }
    if(tokens[0] == "comment") {
        typesymbol = gensym("text");
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
    t_pd* pdobject = nullptr;
    m_instance->enqueueFunction([this, argc, argv, typesymbol, &pdobject]() mutable {
        m_instance->setThis();
        pdobject = libpd_createobj(static_cast<t_canvas*>(m_ptr), typesymbol, argc, argv.data());
    });
    
    
    
    while(!pdobject) {
        m_instance->waitForStateUpdate();
    }
    
    assert(pdobject);
    
    bool is_gui = Gui::getType(pdobject, name.toStdString()) != Type::Undefined;
    
    if(is_gui) {
        return std::make_unique<Gui>(pdobject, this, m_instance);
    }
    else {
        return std::make_unique<Object>(pdobject, this, m_instance);
    }
}


void Patch::copy() {
    m_instance->enqueueFunction([this]() {
        libpd_copy(getPointer());
    });
}

void Patch::paste() {
    m_instance->enqueueFunction([this]() {
        libpd_paste(getPointer());
    });
}

void Patch::duplicate() {
    m_instance->enqueueFunction([this]() {
        setCurrent();
        libpd_duplicate(getPointer());
    });
}


void Patch::selectObject(Object* obj)  {
    m_instance->enqueueFunction([this, obj]() {
        glist_select(getPointer(), &checkObject(obj)->te_g);
    });
}

void Patch::deselectAll() {
    m_instance->enqueueFunction([this]() {
        glist_noselect(getPointer());
        EDITOR->canvas_undo_already_set_move = 0;
    });
}

void Patch::removeObject(Object* obj)
{
    if(!obj || !m_ptr) return;
    
    m_instance->enqueueFunction([this, obj](){
        m_instance->setThis();
        libpd_removeobj(getPointer(), &checkObject(obj)->te_g);
    });
    
}

bool Patch::canConnect(Object* src, int nout, Object* sink, int nin) {
    
    bool can_connect = false;
    
    m_instance->enqueueFunction([this, &can_connect, src, nout, sink, nin]() mutable {
        
        can_connect = libpd_canconnect(getPointer(), checkObject(src), nout, checkObject(sink), nin);
    });
                                
    m_instance->waitForStateUpdate();

    return can_connect;
}

bool Patch::createConnection(Object* src, int nout, Object* sink, int nin)
{
    if(!src || !sink || !m_ptr) return false;
    
    bool can_connect = false;
    
    m_instance->enqueueFunction([this, &can_connect, src, nout, sink, nin]() mutable {
        
        can_connect = libpd_canconnect(getPointer(), checkObject(src), nout, checkObject(sink), nin);
        
        if(!can_connect) return;
        
        m_instance->setThis();
        
        libpd_createconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
    });
    
    m_instance->waitForStateUpdate();
    
    
    return can_connect;
}

void Patch::removeConnection(Object* src, int nout, Object* sink, int nin)
{
    if(!src || !sink || !m_ptr) return;
    
    m_instance->enqueueFunction([this, src, nout, sink, nin]() mutable {
        m_instance->setThis();
        
        libpd_removeconnection(getPointer(), checkObject(src), nout, checkObject(sink), nin);
    });
    
    
    
}


std::unique_ptr<Object> Patch::renameObject(Object* obj, String name) {
    if(!obj || !m_ptr) return nullptr;
    
    // Cant use the queue for this...
    
    setCurrent();
    
    libpd_renameobj(getPointer(), &checkObject(obj)->te_g, name.toRawUTF8(), name.length());
    
    setCurrent();
    // This only works if pd always recreats the object
    // TODO: find out if thats always the case
    
    auto gui = Gui(libpd_newest(getPointer()), this, m_instance);
    if(gui.getType() == Type::Undefined) {
        return std::make_unique<Object>(libpd_newest(getPointer()), this, m_instance);
    }
    else {
        return std::make_unique<Gui>(gui);
    }
    
   
}

void Patch::moveObjects(std::vector<Object*> objects, int dx, int dy) {
    //if(!obj || !m_ptr) return;
    
    m_instance->enqueueFunction([this, objects, dx, dy]() mutable {
        setCurrent();
        
        for(auto* obj : objects) {
            if(!obj) continue;
            glist_select(getPointer(), &checkObject(obj)->te_g);
        }
        
        libpd_moveselection(getPointer(), dx / zoom, dy / zoom);
        
        glist_noselect(getPointer());
        EDITOR->canvas_undo_already_set_move = 0;
        setCurrent();
    });
}


void Patch::removeSelection() {
    m_instance->enqueueFunction([this]() mutable {
        setCurrent();
        libpd_removeselection(getPointer());
    });
    
}

void Patch::undo() {
    m_instance->enqueueFunction([this]() {
        setCurrent();
        glist_noselect(getPointer());
        EDITOR->canvas_undo_already_set_move = 0;
        

        libpd_undo(getPointer());

        
        setCurrent();
    });
}

void Patch::redo() {
    m_instance->enqueueFunction([this]() {
        setCurrent();
        glist_noselect(getPointer());
        EDITOR->canvas_undo_already_set_move = 0;
        
        libpd_redo(getPointer());
        
        setCurrent();
    });
}

void Patch::setZoom(int zoom)
{
    t_atom arg;
    SETFLOAT(&arg, zoom);
    pd_typedmess((t_pd*)getPointer(), gensym("zoom"), 2, &arg);
}

t_object* Patch::checkObject(Object* obj) const noexcept {
    return pd_checkobject(static_cast<t_pd*>(obj->getPointer()));
}

void Patch::keyPress(int keycode, int shift)
{
    t_atom args[3];
    
    SETFLOAT(args, 1);
    SETFLOAT(args + 1, keycode);
    SETFLOAT(args + 2, shift);
    
    pd_typedmess((t_pd*)getPointer(), gensym("key"), 3, args);
    
}

               
}



