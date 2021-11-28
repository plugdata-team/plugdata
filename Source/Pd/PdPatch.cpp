/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PdPatch.hpp"
#include "PdObject.hpp"
#include "PdGui.hpp"

extern "C"
{
#include <z_libpd.h>
#include <m_pd.h>
#include <g_canvas.h>
#include "x_libpd_multi.h"
}

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
            return static_cast<bool>(static_cast<t_canvas*>(m_ptr)->gl_isgraph);
        }
        return false;
    }
    
    std::array<int, 4> Patch::getBounds() const noexcept
    {
        if(m_ptr)
        {
            t_canvas const* cnv = static_cast<t_canvas*>(m_ptr);
            if(cnv->gl_isgraph)
            {
                return {cnv->gl_xmargin, cnv->gl_ymargin, cnv->gl_pixwidth - 2, cnv->gl_pixheight - 2};
            }
        }
        return {0, 0, 0, 0};
    }
    
    std::vector<Gui> Patch::getGuis() noexcept
    {
        if(m_ptr)
        {
            std::vector<Gui> objects;
            t_canvas const* cnv = static_cast<t_canvas*>(m_ptr);
            for(t_gobj *y = cnv->gl_list; y; y = y->g_next)
            {
                Gui gui(static_cast<void*>(y), m_ptr, m_instance);
                if(gui.getType() != Gui::Type::Undefined)
                {
                    objects.push_back(gui);
                }
            }
            return objects;
        }
        return std::vector<Gui>();
    }

    std::vector<Object> Patch::getObjects() noexcept
    {
        if(m_ptr)
        {
            std::vector<Object> objects;
            t_canvas const* cnv = static_cast<t_canvas*>(m_ptr);
            
            for(t_gobj *y = cnv->gl_list; y; y = y->g_next)
            {
                Object object(static_cast<void*>(y), m_ptr, m_instance);
                objects.push_back(object);
            }
            return objects;
        }
        return std::vector<Object>();
    }
}



