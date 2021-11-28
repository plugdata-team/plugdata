/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

namespace pd
{
    // ==================================================================================== //
    //                                      ATOM                                            //
    // ==================================================================================== //
    
    //! @brief The Pd atom.
    //! @details The class is a copy of the Pd atom.
    //! @see Instance, Gui
    class Atom
    {
    public:
        //! @brief The default constructor.
        inline Atom() : type(FLOAT), value(0), symbol() {}
        
        //! @brief The float constructor.
        inline Atom(const float val) : type(FLOAT), value(val), symbol() {}
        
        //! @brief The string constructor.
        inline Atom(const std::string& sym) : type(SYMBOL), value(0), symbol(sym) {}
        
        //! @brief The c-string constructor.
        inline Atom(const char* sym) : type(SYMBOL), value(0), symbol(sym) {}
        
        //! @brief Check if the atom is a float.
        inline bool isFloat() const noexcept { return type == FLOAT; }
        
        //! @brief Check if the atom is a string.
        inline bool isSymbol() const noexcept { return type == SYMBOL; }
        
        //! @brief Get the float value.
        inline float getFloat() const noexcept { return value; }
        
        //! @brief Get the string.
        inline std::string const& getSymbol() const noexcept { return symbol; }
        
        //! @brief Compare two atoms.
        inline bool operator==(Atom const& other) const noexcept
        {
            if(type == SYMBOL) { return other.type == SYMBOL && symbol == other.symbol; }
            else { return other.type == FLOAT && value == other.value; }
        }
    private:
        enum Type
        {
            FLOAT,
            SYMBOL
        };
        Type        type = FLOAT;
        float       value = 0;
        std::string symbol;
    };
}
