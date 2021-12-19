/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "PdObject.hpp"
#include "PdArray.hpp"
#include "PdAtom.hpp"
#include <JuceHeader.h>

namespace pd
{
    class Label;
    
    // ==================================================================================== //
    //                                          GUI                                         //
    // ==================================================================================== //
    
    //! @brief The Pd GUI objects.
    //! @details The Instance is a wrapper for the Pd native GUI. The lifetime of the internal\n
    //! GUI is not guaranteed by the class.
    //! @see Instance, Patch, Object
    class Gui : public Object
    {
    public:

        
        //! @brief The default constructor
        Gui() noexcept = default;
        
        //! @brief The copy constructor.
        Gui(const Gui& other) noexcept = default;
        
        //! @brief The copy operator.
        Gui& operator=(Gui const& other) noexcept = default;
        
        //! @brief The destructor.
        ~Gui() noexcept = default;
        
        
        //! @brief The type of the GUI.
        inline Type getType() const noexcept override
        {
            return m_type;
        }
        
        static Type getType(void* ptr, std::string obj_text) noexcept;
        
        //! @brief If the GUI is an IEM's GUI.
        bool isIEM() const noexcept
        {
            return (m_type != Type::Undefined) && (m_type < Type::Comment);
        }
        
        //! @brief If the GUI is an Atom GUI (AtomNumber or AtomSymbol).
        bool isAtom() const noexcept
        {
            return (m_type == Type::AtomNumber) || (m_type == Type::AtomSymbol);
        }
        
        //! @brief Get the font height.
        float getFontHeight() const noexcept;
            
        //! @brief Get the font name.
        std::string getFontName() const;
        
        float getMinimum() const noexcept;
        
        float getMaximum() const noexcept;
        
        float getValue() const noexcept;
        
        void setValue(float value) noexcept;
        
        size_t getNumberOfSteps() const noexcept;
        
        unsigned int getBackgroundColor() const noexcept;
        
        unsigned int getForegroundColor() const noexcept;
        
        std::string getSymbol() const noexcept;
        
        void setSymbol(std::string const& value) noexcept;
        
        void click() noexcept;
        
        std::array<int, 4> getBounds() const noexcept override;
        
        bool jumpOnClick() const noexcept;
        
        bool isLogScale() const noexcept;
        
        Array getArray() const noexcept;
        
        Label getLabel() const noexcept;
            
        Patch getPatch() const noexcept;
        
        std::vector<Atom> getList() const noexcept;
        
        void setList(std::vector<Atom> const& value) noexcept;
        
        Gui(void* ptr, Patch* patch, Instance* instance) noexcept;
    private:

        std::string last_symbol;
    
        Type m_type = Type::Undefined;
        friend class Patch;
    };
    
    // ==================================================================================== //
    //                                      LABEL                                           //
    // ==================================================================================== //
    
    class Label
    {
    public:
        Label() noexcept;
        Label(Label const& other) noexcept;
        Label(std::string const& text, unsigned int color, int x, int y, std::string const& fontname, float fontheight) noexcept;
        
        std::string getText() const noexcept { return m_text; }
        unsigned int getColor() const noexcept { return m_color; }
        std::array<int, 2> getPosition() const noexcept { return m_position; }
        
        //! @brief Get the font height.
        float getFontHeight() const noexcept;
        
        //! @brief Get the font name.
        std::string getFontName() const;
    private:
        Label(Gui const& gui) noexcept;
        std::string const        m_text;
        unsigned int const       m_color;
        std::array<int, 2> const m_position;
        std::string              m_font_name;
        float                    m_font_height;
        friend class Gui;
    };
}

