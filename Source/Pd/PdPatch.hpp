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
    
    //! @brief The compare equal operator.
    bool operator==(Patch const& other) const noexcept {
        return getPointer() == other.getPointer();
    }
    
    //! @brief The copy operator.
    Patch& operator=(const Patch& other) noexcept = default;
    
    //! @brief The destructor.
    ~Patch() noexcept = default;
    
    //! @brief Gets the bounds of the patch.
    std::array<int, 4> getBounds() const noexcept;
    
    std::unique_ptr<Object> createGraph(String name, int size, int x, int y);
    std::unique_ptr<Object> createGraphOnParent(int x, int y);
    
    std::unique_ptr<Object> createObject(String name, int x, int y);
    void removeObject(Object* obj);
    std::unique_ptr<Object> renameObject(Object* obj, String name);
   
    void moveObjects (std::vector<Object*>, int x, int y);
    
    void removeSelection();
    
    void selectObject(Object*);
    void deselectAll();
    
    void copy();
    void paste();
    void duplicate();
    
    void undo();
    void redo();
    
    enum GroupUndoType
    {
        Remove = 0,
        Move
    };


    void setCurrent();
    
    bool createConnection(Object* src, int nout, Object* sink, int nin);
    void removeConnection(Object* src, int nout, Object*sink, int nin);
    
    
    t_canvas* getPointer() const {
        return static_cast<t_canvas*>(m_ptr);
    }
    
    //! @brief Gets the objects of the patch.
    std::vector<Object> getObjects(bool only_gui = false) noexcept;
    
    String getCanvasContent() {
        
        char* buf;
        int bufsize;
        
        
        libpd_getcontent(static_cast<t_canvas*>(m_ptr), &buf, &bufsize);
        
        return String(buf, bufsize);
        
    }

    
private:
    Patch(void* ptr, Instance* instance) noexcept;
    
    t_object* checkObject(Object* obj) const noexcept;
    
    void*     m_ptr      = nullptr;
    Instance* m_instance = nullptr;
    
    friend class Instance;
    friend class Gui;
    friend class Object;
};
}
