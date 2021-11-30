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
    }
    
    bool Patch::isGraph() const noexcept
    {
        if(m_ptr)
        {
            return static_cast<bool>(getPointer()->gl_isgraph);
        }
        return false;
    }
    
    std::array<int, 4> Patch::getBounds() const noexcept
    {
        if(m_ptr)
        {
            t_canvas const* cnv = getPointer();
            if(cnv->gl_isgraph)
            {
                return {cnv->gl_xmargin, cnv->gl_ymargin, cnv->gl_pixwidth - 2, cnv->gl_pixheight - 2};
            }
        }
        return {0, 0, 0, 0};
    }

    std::vector<Object> Patch::getObjects(bool only_gui) noexcept
    {
        if(m_ptr)
        {
            std::vector<Object> objects;
            t_canvas const* cnv = getPointer();
            
            for(t_gobj *y = cnv->gl_list; y; y = y->g_next)
            {
                Object object(static_cast<void*>(y), m_ptr, m_instance);
                
                if(only_gui) {
                    Gui gui(static_cast<void*>(y), m_ptr, m_instance);
                    if(gui.getType() != Gui::Type::Undefined)
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


t_pd* Patch::createGraphOnParent() {
    m_instance->setThis();
    t_pd* pdobject = libpd_creategraphonparent(static_cast<t_pd*>(m_ptr));
    
    return pdobject;
}

t_pd* Patch::createGraph(String name, int size)
{
    m_instance->setThis();
    t_pd* pdobject = libpd_creategraph(static_cast<t_pd*>(m_ptr), name.toRawUTF8(), size);
    
    // Cant enqueue this...
    
    return pdobject;
}

t_pd* Patch::createObject(String name, int x, int y)
{
    
    if(!m_ptr) return nullptr;
    
    StringArray tokens;
    tokens.addTokens(name, " ", "");
    

    if(tokens[0] == "graph" && tokens.size() == 3) {
        return createGraph(tokens[1], tokens[2].getIntValue());
    }
    else if(tokens[0] == "graph") {
        return createGraphOnParent();
    }
    
    
    t_symbol* typesymbol = gensym("obj");
    
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
    
    // Cant enqueue this...
    
    m_instance->setThis();
    t_pd* pdobject = libpd_createobj(static_cast<t_pd*>(m_ptr), typesymbol, argc, argv.data());
    
    return pdobject;
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
        libpd_duplicate(getPointer());
    });
}


void Patch::selectObject(t_pd* x) {
    m_instance->enqueueFunction([this, x]() {
        glist_select(getPointer(), &(pd_checkobject(x)->te_g));
    });
}

void Patch::deselectAll() {
    m_instance->enqueueFunction([this]() {
        glist_noselect(getPointer());
    });
}

void Patch::removeObject(t_pd* obj)
{
    if(!obj || !m_ptr) return;
    
    m_instance->enqueueFunction([this, obj](){
        m_instance->setThis();
        libpd_removeobj(getPointer(), &pd_checkobject(obj)->te_g);
    });
    
}

bool Patch::createConnection(t_pd* src, int nout, t_pd* sink, int nin)
{
    if(!src || !sink || !m_ptr) return false;
    

    
    m_instance->enqueueFunction([this, src, nout, sink, nin]() mutable {
        
        bool can_connect = libpd_canconnect(getPointer(), pd_checkobject(src), nout, pd_checkobject(sink), nin);
        
        if(!can_connect) return;
        
        m_instance->setThis();
        libpd_createconnection(getPointer(), pd_checkobject(src), nout, pd_checkobject(sink), nin);
    });
    

    
    return true;
}

void Patch::removeConnection(t_pd* src, int nout, t_pd*sink, int nin)
{
    if(!src || !sink || !m_ptr) return;
    
    m_instance->enqueueFunction([this, src, nout, sink, nin]() mutable {
        m_instance->setThis();
        libpd_removeconnection(getPointer(), pd_checkobject(src), nout, pd_checkobject(sink), nin);
    });
    
    

}


t_pd* Patch::renameObject(t_pd* obj, String name) {
    if(!obj || !m_ptr) return nullptr;
    
    // Cant use the queue for this...
    
    m_instance->setThis();
    
    libpd_renameobj(getPointer(), &pd_checkobject(obj)->te_g, name.toRawUTF8(), name.length());
    
    return static_cast<t_pdinstance*>(m_instance->m_instance)->pd_newest;
}

void Patch::moveObject(t_pd* obj, int x, int y) {
    if(!obj || !m_ptr) return;
    
    m_instance->enqueueFunction([this, obj, x, y]() mutable {
        m_instance->setThis();
        libpd_moveobj(getPointer(), &pd_checkobject(obj)->te_g, x, y);
    });
    
}

void Patch::removeSelection() {
    m_instance->enqueueFunction([this]() mutable {
        libpd_canvas_doclear(getPointer());
    });
}

void Patch::undo() {
    m_instance->enqueueFunction([this]() {
        m_instance->setThis();
        libpd_undo(getPointer());
    });
}

void Patch::redo() {
    m_instance->enqueueFunction([this]() {
        m_instance->setThis();
        libpd_redo(getPointer());
    });
}

}



