/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <string>
#include <array>
#include <cstddef>

namespace pd
{
class Patch;
class Instance;
// ==================================================================================== //
//                                      OBJECT                                          //
// ==================================================================================== //

//! @brief The Pd object.
//! @details The class is a wrapper around a Pd object. The lifetime of the internal\n
//! object is not guaranteed by the class.
//! @see Instance, Patch, Gui


//!         //! @brief The type of GUI.
enum class Type : size_t
{
    Undefined        = 0,
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
};

class Object
{
public:
    
    //! @brief The default constructor.
    Object() noexcept = default;
    
    //! @brief The copy constructor.
    Object(Object const& other) noexcept = default;
    
    //! @brief The copy operator.
    Object& operator=(Object const& other) noexcept = default;
    
    //! @brief The compare equal operator.
    bool operator==(Object const& other) const noexcept;
    
    //! @brief The compare unequal operator.
    bool operator!=(Object const& other) const noexcept;
    
    //! @brief The destructor.
    virtual ~Object() noexcept = default;
    
    //! @brief The text of the Object.
    std::string getText();
    
    //! @brief The name of the Object.
    std::string getName() const;
    
    //! @brief The name of the help file
    std::string getHelp() const;
    
    virtual inline Type getType() const noexcept {
        return Type::Undefined;
    }
    
    //! @brief The bounds of the Object.
    virtual std::array<int, 4> getBounds() const noexcept;
    
    void* getPointer() const noexcept{
        return m_ptr;
    }
    
    int getNumInlets() noexcept;
    int getNumOutlets() noexcept;
    
    bool isSignalInlet(int idx) noexcept;
    bool isSignalOutlet(int idx) noexcept;
    
    Object(void* ptr, Patch* patch, Instance* instance) noexcept;
    
protected:
    void*   m_ptr   = nullptr;
    Patch*   m_patch = nullptr;
    Instance* m_instance = nullptr ;
    
    friend class Patch;
};
}

