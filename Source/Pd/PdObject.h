/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <JuceHeader.h>

#include <array>
#include <cstddef>
#include <string>

namespace pd
{
class Patch;
class Instance;

// The Pd object.
//! @details The class is a wrapper around a Pd object. The lifetime of the internal\n
//! object is not guaranteed by the class.
//! @see Instance, Patch, Gui

//!         // The type of GUI.

class Object
{
   public:
    // The compare equal operator.
    bool operator==(Object const& other) const noexcept;

    // The compare unequal operator.
    bool operator!=(Object const& other) const noexcept;

    // The name of the Object.
    std::string getName() const;
    
    static String getText(void* obj);
    // The name of the help file
    Patch getHelp() const;

    virtual void setBounds(Rectangle<int> bounds);


    // The bounds of the Object.
    virtual Rectangle<int> getBounds() const noexcept;

    void* getPointer() const noexcept
    {
        return ptr;
    }


    void toFront();

    void addUndoableAction();

    Object(void* ptr, Patch* patch, Instance* instance) noexcept;

   protected:
    void* ptr = nullptr;
    Patch* patch = nullptr;
    Instance* instance = nullptr;

    friend class Patch;
};
}  // namespace pd
