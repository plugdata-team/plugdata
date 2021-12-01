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
    std::string getText() const;
    
    //! @brief The name of the Object.
    std::string getName() const;
    
    //! @brief The bounds of the Object.
    virtual std::array<int, 4> getBounds() const noexcept;
    
    void* getPointer() {
        return m_ptr;
    }
    
protected:
    Object(void* ptr, Patch* patch, Instance* instance) noexcept;
    
    void*   m_ptr   = nullptr;
    Patch*   m_patch = nullptr;
    Instance* m_instance = nullptr ;
    
    friend class Patch;
};
}

