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

namespace pd
{
    // ==================================================================================== //
    //                                      OBJECT                                          //
    // ==================================================================================== //
    
    Object::Object(void* ptr, void* patch, Instance* instance) noexcept :
    m_ptr(ptr), m_patch(patch), m_instance(instance)
    {
    }
    
    std::string Object::getText() const
    {
        if(m_ptr)
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
            libpd_get_object_bounds(m_patch, m_ptr, &x, &y, &w, &h);
            
            t_canvas const* cnv = static_cast<t_canvas*>(m_patch);
            if(cnv != nullptr)
            {
                x -= cnv->gl_xmargin;
                y -= cnv->gl_ymargin;
            }

            return {x, y, w, h};
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
}



