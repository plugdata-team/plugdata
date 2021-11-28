/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "PdGui.hpp"
#include "x_libpd_mod_utils.h"
#include <array>
#include <vector>
#include <JuceHeader.h>

namespace pd
{
    class Instance;
    // ==================================================================================== //
    //                                          PATCHER                                     //
    // ==================================================================================== //
    
    //! @brief The Pd patch.
    //! @details The class is a wrapper around a Pd patch. The lifetime of the internal patch\n
    //! is not guaranteed by the class.
    //! @see Instance, Object, Gui
    class Patch
    {
    public:
        //! @brief The default constructor.
        Patch() noexcept = default;
        
        //! @brief The copy constructor.
        Patch(const Patch&) noexcept = default;
        
        //! @brief The copy operator.
        Patch& operator=(const Patch& other) noexcept = default;
        
        //! @brief The destructor.
        ~Patch() noexcept = default;
        
        //! @brief Gets if the patch is Graph On Parent.
        bool isGraph() const noexcept;
        
        //! @brief Gets the bounds of the patch.
        std::array<int, 4> getBounds() const noexcept;
        
        //! @brief Gets the GUI objects of the patch.
        std::vector<Gui> getGuis() noexcept;
        std::vector<Object> getObjects() noexcept;
        
        String getCanvasContent() {
            
            char* buf;
            int bufsize;
            
            
            libpd_getcontent(static_cast<t_canvas*>(m_ptr), &buf, &bufsize);
            
            return String(buf, bufsize);
            
        }
        
    private:
        Patch(void* ptr, Instance* instance) noexcept;
        
        void*     m_ptr      = nullptr;
        Instance* m_instance = nullptr;
        
        friend class Instance;
        friend class Gui;
    };
}
