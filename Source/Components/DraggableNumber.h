#pragma once
#include <JuceHeader.h>

struct DraggableNumber : public MouseListener
{
    
    float dragValue = 0.0f;
    bool shift = false;
    int decimalDrag = 0;
    
    Label& label;
    std::function<void(float)> valueChanged = [](float){};
    std::function<void()> dragStart = [](){};
    std::function<void()> dragEnd = [](){};
    
    DraggableNumber(Label& labelToAttachTo) : label(labelToAttachTo) {
        label.addMouseListener(this, false);
    }
    
    ~DraggableNumber() {
        label.removeMouseListener(this);
    }
    
    void mouseDown(const MouseEvent& e) {
        if (label.isBeingEdited()) return;
        
        shift = e.mods.isShiftDown();
        dragValue = label.getText().getFloatValue();

        const auto textArea = label.getBorderSize().subtractedFrom(label.getBounds());

        GlyphArrangement glyphs;
        glyphs.addFittedText(label.getFont(), label.getText(), textArea.getX(), 0., textArea.getWidth(), label.getHeight(), Justification::centredLeft, 1, label.getMinimumHorizontalScale());

        double decimalX = label.getWidth();
        for (int i = 0; i < glyphs.getNumGlyphs(); ++i)
        {
            auto const& glyph = glyphs.getGlyph(i);
            if (glyph.getCharacter() == '.')
            {
                decimalX = glyph.getRight();
            }
        }

        const bool isDraggingDecimal = e.x > decimalX;

        decimalDrag = isDraggingDecimal ? 6 : 0;

        if (isDraggingDecimal)
        {
            GlyphArrangement decimalsGlyph;
            static const String decimalStr("000000");

            decimalsGlyph.addFittedText(label.getFont(), decimalStr, decimalX, 0, label.getWidth(), label.getHeight(), Justification::centredLeft, 1, label.getMinimumHorizontalScale());

            for (int i = 0; i < decimalsGlyph.getNumGlyphs(); ++i)
            {
                auto const& glyph = decimalsGlyph.getGlyph(i);
                if (e.x <= glyph.getRight())
                {
                    decimalDrag = i + 1;
                    break;
                }
            }
        }
        
        dragStart();

    }
    
    void mouseDrag(const MouseEvent& e) {
        if (label.isBeingEdited()) return;
        
        // Hide cursor and set unbounded mouse movement
        label.setMouseCursor(MouseCursor::NoCursor);
        label.updateMouseCursor();
        
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.enableUnboundedMouseMovement(true, true);

        const int decimal = decimalDrag + e.mods.isShiftDown();
        const float increment = (decimal == 0) ? 1. : (1. / std::pow(10., decimal));
        const float deltaY = (e.y - e.mouseDownPosition.y) * 0.7f;
        
        // truncate value and set
        double newValue = dragValue + (increment * -deltaY);

        if (decimal > 0)
        {
            const int sign = (newValue > 0) ? 1 : -1;
            unsigned int ui_temp = (newValue * std::pow(10, decimal)) * sign;
            newValue = (((double)ui_temp) / std::pow(10, decimal) * sign);
        }
        else
        {
            newValue = static_cast<int64_t>(newValue);
        }
        
        label.setText(formatNumber(newValue), dontSendNotification);
        valueChanged(newValue);
    }
    
    void mouseUp(const MouseEvent& e) {
        if (label.isBeingEdited()) return;

        // Show cursor again
        label.setMouseCursor(MouseCursor::NormalCursor);
        label.updateMouseCursor();
        
        // Reset mouse position to where it was first clicked and disable unbounded movement
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.setScreenPosition(e.getMouseDownScreenPosition().toFloat());
        mouseSource.enableUnboundedMouseMovement(false);
        dragEnd();
    }
    
    
    String formatNumber(float value)
    {
        String text;
        text << value;

        while (text.length() > 1 && label.getFont().getStringWidth(text) > label.getWidth() - 5)
        {
            text = text.substring(0, text.length() - 2);
        }
        if (!text.containsChar('.')) text << '.';

        return text;
    }
    
};

struct DraggableListNumber : public DraggableNumber
{
    int numberStartIdx = 0;
    int numberEndIdx = 0;
    
    bool targetFound = false;
    
    DraggableListNumber(Label& label) : DraggableNumber(label) {
    }
    
    void mouseDown(const MouseEvent& e) {
        if (label.isBeingEdited()) return;

        shift = e.mods.isShiftDown();

        const auto textArea = label.getBorderSize().subtractedFrom(label.getBounds());
        
        GlyphArrangement glyphs;
        glyphs.addFittedText(label.getFont(), label.getText(), textArea.getX(), 0., textArea.getWidth(), textArea.getHeight(), Justification::centredLeft, 1, label.getMinimumHorizontalScale());
        
        auto text = label.getText();
        targetFound = false;
        // Loop to find start of item
        for(int i = 0; i < glyphs.getNumGlyphs(); i++) {
            const auto& startGlyph = glyphs.getGlyph(i);
            
            // Don't start at whitespace
            if(startGlyph.isWhitespace()) continue;
            
            // Loop from start to find end of item
            for(int j = i; j < glyphs.getNumGlyphs(); j++) {
                const auto& endGlyph = glyphs.getGlyph(j);
                
                // End of item when we find whitespace or end of message
                if(endGlyph.isWhitespace() || j == glyphs.getNumGlyphs() - 1) {
                    auto number = text.substring(i, j);

                    // Check if item is a number and if mouse clicked on it
                    if(number.containsOnly("0123456789.-") && e.x >= startGlyph.getLeft() && e.x <= endGlyph.getRight())
                    {
                        numberStartIdx = i;
                        numberEndIdx = j;
                        dragValue = number.getFloatValue();
                        targetFound = true;
                    }
                    
                    // Move start to end of current item
                    i = j;
                    break;
                }
            }
            if(targetFound) break;
        }
        
        if(!targetFound) return;

        dragStart();

    }
    
    void mouseDrag(const MouseEvent& e) {
        if (label.isBeingEdited() || !targetFound) return;
        
        
        // Hide cursor and set unbounded mouse movement
        label.setMouseCursor(MouseCursor::NoCursor);
        label.updateMouseCursor();
        
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.enableUnboundedMouseMovement(true, true);

        //const int decimal = decimalDrag + e.mods.isShiftDown();
        const float increment =  1.;
        const float deltaY = (e.y - e.mouseDownPosition.y) * 0.7f;
        
        //lastDragPos = e.position;

        double newValue = dragValue + std::floor(increment * -deltaY);
        
        int length = numberEndIdx - numberStartIdx;
        
        auto replacement = String();
        replacement << newValue;
        
        auto newText = label.getText().replaceSection(numberStartIdx, length, replacement);
        
        // In case the length of the number changes
        if(length != replacement.length()) {
            numberEndIdx = replacement.length() + numberStartIdx;
        }

        label.setText(newText, dontSendNotification);
        valueChanged(0);
    }
    
    void mouseUp(const MouseEvent& e) {
        if (label.isBeingEdited() || !targetFound) return;

        // Show cursor again
        label.setMouseCursor(MouseCursor::NormalCursor);
        label.updateMouseCursor();
        
        // Reset mouse position to where it was first clicked and disable unbounded movement
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.setScreenPosition(e.getMouseDownScreenPosition().toFloat());
        mouseSource.enableUnboundedMouseMovement(false);
        dragEnd();
    }
};
