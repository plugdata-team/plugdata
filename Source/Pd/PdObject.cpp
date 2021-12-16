/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdObject.hpp"
#include "PdInstance.hpp"
extern "C"
{
#include <z_libpd.h>
#include <m_pd.h>
#include <g_canvas.h>
#include "x_libpd_extra_utils.h"
}

struct _outconnect
{
    struct _outconnect *oc_next;
    t_pd *oc_to;
};


struct _outlet
{
    t_object *o_owner;
    struct _outlet *o_next;
    t_outconnect *o_connections;
    t_symbol *o_sym;
};

namespace pd
{
// ==================================================================================== //
//                                      OBJECT                                          //
// ==================================================================================== //

Object::Object(void* ptr, Patch* patch, Instance* instance) noexcept :
m_ptr(ptr), m_patch(patch), m_instance(instance)
{
    
}

std::string Object::getText()
{
    if(m_ptr && m_patch->checkObject(this))
    {
        char* text = nullptr;
        int size = 0;
        m_instance->setThis();
        libpd_get_object_text(m_ptr, &text, &size);
        if(text && size)
        {
            std::string txt(text, size);
            freebytes(static_cast<void*>(text), static_cast<size_t>(size) * sizeof(char));
            return txt;
        }
    }
    return std::string();
}

std::string Object::getName() const
{
    if(m_ptr)
    {
        char const* name = libpd_get_object_class_name(m_ptr);
        if(name)
        {
            return std::string(name);
        }
    }
    return std::string();
}

std::array<int, 4> Object::getBounds() const noexcept
{
    if(m_ptr)
    {
        int x = 0, y = 0, w = 0, h = 0;
        m_instance->setThis();
        
        libpd_get_object_bounds(m_patch->getPointer(), m_ptr, &x, &y, &w, &h);
        
        t_canvas const* cnv = m_patch->getPointer();
        if(cnv != nullptr)
        {
            //x -= cnv->gl_xmargin;
            //y -= cnv->gl_ymargin;
        }
        
        return {x * Patch::zoom, y * Patch::zoom, w, h};
    }
    return {0, 0, 0, 0};
}





bool Object::operator==(Object const& other) const noexcept
{
    return m_ptr == other.m_ptr;
}

bool Object::operator!=(Object const& other) const noexcept
{
    return m_ptr != other.m_ptr;
}

int Object::getNumInlets() noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_ninlets(checked);
    }
    
    return 0;
}
int Object::getNumOutlets() noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_noutlets(checked);
    }
    
    return 0;
}

bool Object::isSignalInlet(int idx) noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_issignalinlet(checked, idx);
    }
    
    return 0;
}
bool Object::isSignalOutlet(int idx) noexcept {
    if(auto* checked = m_patch->checkObject(this)) {
        return libpd_issignaloutlet(checked, idx);
    }
    
    return 0;
}

}


