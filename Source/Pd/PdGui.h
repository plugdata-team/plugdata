/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include <JuceHeader.h>

#include "PdArray.h"
#include "PdAtom.h"
#include "PdObject.h"

namespace pd
{

//! @brief The Pd GUI objects.
//! @details The Instance is a wrapper for the Pd native GUI. The lifetime of the internal\n
//! GUI is not guaranteed by the class.
//! @see Instance, Patch, Object
class Gui : public Object
{
   public:
    //! @brief The type of the GUI.
    inline Type getType() const noexcept override
    {
        return type;
    }

    static Type getType(void* ptr) noexcept;

    //! @brief If the GUI is an IEM's GUI.
    bool isIEM() const noexcept
    {
        return (type != Type::Undefined) && (type < Type::Comment);
    }

    //! @brief If the GUI is an Atom GUI (AtomNumber or AtomSymbol).
    bool isAtom() const noexcept
    {
        return (type == Type::AtomNumber) || (type == Type::AtomSymbol);
    }

    //! @brief Get the font height.
    float getFontHeight() const noexcept;
    void setFontHeight(float newSize) noexcept;

    //! @brief Get the font name.
    std::string getFontName() const;

    float getMinimum() const noexcept;
    float getMaximum() const noexcept;

    void setMinimum(float value) noexcept;
    void setMaximum(float value) noexcept;

    void setSendSymbol(const String& symbol) const noexcept;
    void setReceiveSymbol(const String& symbol) const noexcept;

    String getSendSymbol() noexcept;
    String getReceiveSymbol() noexcept;

    float getValue() const noexcept;
    float getPeak() const noexcept;

    void setValue(float value) noexcept;

    size_t getNumberOfSteps() const noexcept;

    Colour getBackgroundColor() const noexcept;
    Colour getForegroundColor() const noexcept;
    Colour getLabelColour() const noexcept;

    void setLabelColour(Colour newColour) noexcept;
    void setForegroundColour(Colour newColour) noexcept;
    void setBackgroundColour(Colour newColour) noexcept;

    std::string getSymbol() const noexcept;

    void setSymbol(std::string const& value) noexcept;

    void click() noexcept;

    Rectangle<int> getBounds() const noexcept override;
    void setSize(int w, int h) override;

    bool jumpOnClick() const noexcept;

    bool isLogScale() const noexcept;
    void setLogScale(bool log) noexcept;

    Array getArray() const noexcept;

    String getLabelText() const noexcept;
    Point<int> getLabelPosition(Rectangle<int> bounds) const noexcept;

    void setLabelText(String newText) noexcept;
    void setLabelPosition(Point<int> bounds) noexcept;
    void setLabelPosition(int wherelabel) noexcept;

    Patch getPatch() const noexcept;

    std::vector<Atom> getList() const noexcept;

    void setList(std::vector<Atom> const& value) noexcept;
    
    void startEditingProperties();
    void finishEditingProperties();
    
    void* getObjectState();
    
    void* lastProperties = nullptr;

    Gui(void* ptr, Patch* patch, Instance* instance) noexcept;

   private:
    Type type = Type::Undefined;
    friend class Patch;
};

}  // namespace pd
