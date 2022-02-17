/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace pd
{
class Patch;
class Instance;

//! @brief The Pd object.
//! @details The class is a wrapper around a Pd object. The lifetime of the internal\n
//! object is not guaranteed by the class.
//! @see Instance, Patch, Gui

//!         //! @brief The type of GUI.
enum class Type : size_t
{
    Undefined = 0,
    HorizontalSlider,
    VerticalSlider,
    Toggle,
    Number,
    HorizontalRadio,
    VerticalRadio,
    Bang,
    Panel,
    VuMeter,
    Comment,
    AtomNumber,
    AtomSymbol,
    AtomList,
    Array,
    GraphOnParent,
    Message,
    Subpatch,
    Mousepad,
    Mouse,
    Keyboard,
    Invalid
};

class Object
{
   public:
    //! @brief The compare equal operator.
    bool operator==(Object const& other) const noexcept;

    //! @brief The compare unequal operator.
    bool operator!=(Object const& other) const noexcept;

    //! @brief The text of the Object.
    std::string getText();

    //! @brief The name of the Object.
    std::string getName() const;

    //! @brief The name of the help file
    Patch getHelp() const;

    virtual void setSize(int width, int height);

    int getWidth() const;

    virtual inline Type getType() const noexcept
    {
        return Type::Undefined;
    }

    //! @brief The bounds of the Object.
    virtual std::array<int, 4> getBounds() const noexcept;

    void* getPointer() const noexcept
    {
        return ptr;
    }

    int getNumInlets() noexcept;
    int getNumOutlets() noexcept;

    bool isSignalInlet(int idx) noexcept;
    bool isSignalOutlet(int idx) noexcept;

    Object(void* ptr, Patch* patch, Instance* instance) noexcept;

   protected:
    void* ptr = nullptr;
    Patch* patch = nullptr;
    Instance* instance = nullptr;

    friend class Patch;
};
}  // namespace pd
