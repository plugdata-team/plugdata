#pragma once
#include <JuceHeader.h>

struct DraggableNumber : public Label {

    float dragValue = 0.0f;
    bool shift = false;
    int decimalDrag = 0;
    int numDecimalsToShow = 0;

    float lastValue = 0.0f;

    bool isMinLimited = false, isMaxLimited = false;
    float min, max;

    std::function<void(float)> valueChanged = [](float) {};
    std::function<void()> dragStart = []() {};
    std::function<void()> dragEnd = []() {};

    DraggableNumber()
    {
        setWantsKeyboardFocus(true);
    }

    ~DraggableNumber()
    {
    }

    void setEditableOnClick(bool editable)
    {
        setEditable(true, false);
        setInterceptsMouseClicks(true, true);
    }

    void setMaximum(float maximum)
    {
        isMaxLimited = true;
        max = maximum;
    }

    void setMinimum(float minimum)
    {
        isMinLimited = true;
        min = minimum;
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (isEditable())
            return false;
        // Otherwise it might catch a shortcut
        if (key.getModifiers().isCommandDown())
            return false;

        auto chr = key.getTextCharacter();

        if (!getCurrentTextEditor() && ((chr >= '0' && chr <= '9') || chr == '+' || chr == '-' || chr == '.')) {
            showEditor();
            auto* editor = getCurrentTextEditor();

            auto text = String();
            text += chr;
            editor->setText(text);
            editor->moveCaretToEnd(false);
            return true;
        }

        if (!isEditableOnSingleClick() && !getCurrentTextEditor() && key.getKeyCode() == KeyPress::upKey) {
            setValue(getText().getFloatValue() + 1.0f);
            return true;
        }
        if (!isEditableOnSingleClick() && !getCurrentTextEditor() && key.getKeyCode() == KeyPress::downKey) {
            setValue(getText().getFloatValue() - 1.0f);
            return true;
        }

        return false;
    }

    void setValue(float newValue)
    {
        if (lastValue != newValue) {
            lastValue = newValue;
            setText(String(newValue), sendNotification);
            valueChanged(newValue);
        }
    }

    // Make sure mouse cursor gets reset, sometimes this doesn't happen automatically
    void mouseEnter(MouseEvent const& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
    }

    void mouseExit(MouseEvent const& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            return;

        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        shift = e.mods.isShiftDown();
        dragValue = getText().getFloatValue();

        auto const textArea = getBorderSize().subtractedFrom(getLocalBounds());

        GlyphArrangement glyphs;
        glyphs.addFittedText(getFont(), formatNumber(dragValue), textArea.getX(), 0., textArea.getWidth(), getHeight(), Justification::centredLeft, 1, getMinimumHorizontalScale());

        float decimalX = getWidth();
        for (int i = 0; i < glyphs.getNumGlyphs(); ++i) {
            auto const& glyph = glyphs.getGlyph(i);
            if (glyph.getCharacter() == '.') {
                decimalX = glyph.getRight();
            }
        }

        bool const isDraggingDecimal = e.x > decimalX;

        decimalDrag = isDraggingDecimal ? 6 : 0;

        if (isDraggingDecimal) {
            GlyphArrangement decimalsGlyph;
            static const String decimalStr("000000");

            decimalsGlyph.addFittedText(getFont(), decimalStr, decimalX, 0, getWidth(), getHeight(), Justification::centredLeft, 1, getMinimumHorizontalScale());

            for (int i = 0; i < decimalsGlyph.getNumGlyphs(); ++i) {
                auto const& glyph = decimalsGlyph.getGlyph(i);
                if (e.x <= glyph.getRight()) {
                    decimalDrag = i + 1;
                    break;
                }
            }
        }

        numDecimalsToShow = decimalDrag;

        dragStart();
    }

    void paint(Graphics& g) override
    {
        if (!isBeingEdited()) {
            g.setColour(findColour(Label::textColourId));
            g.setFont(getFont());

            auto textArea = getBorderSize().subtractedFrom(getLocalBounds());
            g.drawText(formatNumber(getText().getFloatValue(), decimalDrag), textArea, Justification::left, false);
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        // Hide cursor and set unbounded mouse movement
        setMouseCursor(MouseCursor::NoCursor);
        updateMouseCursor();

        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.enableUnboundedMouseMovement(true, true);

        int const decimal = decimalDrag + e.mods.isShiftDown();
        float const increment = (decimal == 0) ? 1. : (1. / std::pow(10., decimal));
        float const deltaY = (e.y - e.mouseDownPosition.y) * 0.7f;

        // truncate value and set
        float newValue = dragValue + (increment * -deltaY);

        if (decimal > 0) {
            int const sign = (newValue > 0) ? 1 : -1;
            unsigned int ui_temp = (newValue * std::pow(10, decimal)) * sign;
            newValue = (((float)ui_temp) / std::pow(10, decimal) * sign);
        } else {
            newValue = static_cast<int64_t>(newValue);
        }

        if (isMinLimited && min < max)
            newValue = std::max(newValue, min);
        if (isMaxLimited && max > min)
            newValue = std::min(newValue, max);

        setText(String(newValue), dontSendNotification);

        numDecimalsToShow = decimal;

        setValue(newValue);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        // Show cursor again
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();

        // Reset mouse position to where it was first clicked and disable unbounded movement
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.setScreenPosition(e.getMouseDownScreenPosition().toFloat());
        mouseSource.enableUnboundedMouseMovement(false);
        dragEnd();

        Label::mouseUp(e);
    }

    String formatNumber(float value, int precision = -1)
    {
        auto text = String(value, precision);

        if (!text.containsChar('.'))
            text << '.';

        return text;
    }
};

struct DraggableListNumber : public DraggableNumber {
    int numberStartIdx = 0;
    int numberEndIdx = 0;

    bool targetFound = false;

    explicit DraggableListNumber()
    {
        setEditableOnClick(true);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        shift = e.mods.isShiftDown();

        auto const textArea = getBorderSize().subtractedFrom(getBounds());

        GlyphArrangement glyphs;
        glyphs.addFittedText(getFont(), getText(), textArea.getX(), 0., textArea.getWidth(), textArea.getHeight(), Justification::centredLeft, 1, getMinimumHorizontalScale());

        auto text = getText();
        targetFound = false;
        // Loop to find start of item
        for (int i = 0; i < glyphs.getNumGlyphs(); i++) {
            auto const& startGlyph = glyphs.getGlyph(i);

            // Don't start at whitespace
            if (startGlyph.isWhitespace())
                continue;

            // Loop from start to find end of item
            for (int j = i; j < glyphs.getNumGlyphs(); j++) {
                auto const& endGlyph = glyphs.getGlyph(j);

                // End of item when we find whitespace or end of message
                if (endGlyph.isWhitespace() || j == glyphs.getNumGlyphs() - 1) {
                    if (j == glyphs.getNumGlyphs() - 1)
                        j++;
                    auto number = text.substring(i, j);

                    // Check if item is a number and if mouse clicked on it
                    if (number.containsOnly("0123456789.-") && e.x >= startGlyph.getLeft() && e.x <= endGlyph.getRight()) {
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
            if (targetFound)
                break;
        }

        if (!targetFound)
            return;

        dragStart();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (isBeingEdited() || !targetFound)
            return;

        // Hide cursor and set unbounded mouse movement
        setMouseCursor(MouseCursor::NoCursor);
        updateMouseCursor();

        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.enableUnboundedMouseMovement(true, true);

        int const decimal = decimalDrag + e.mods.isShiftDown();
        float const increment = 1.;
        float const deltaY = (e.y - e.mouseDownPosition.y) * 0.7f;

        // lastDragPos = e.position;

        float newValue = dragValue + std::floor(increment * -deltaY);

        int length = numberEndIdx - numberStartIdx;

        auto replacement = String();
        replacement << newValue;

        auto newText = getText().replaceSection(numberStartIdx, length, replacement);

        // In case the length of the number changes
        if (length != replacement.length()) {
            numberEndIdx = replacement.length() + numberStartIdx;
        }

        setText(newText, dontSendNotification);
        valueChanged(0);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (isBeingEdited() || !targetFound)
            return;

        // Show cursor again
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();

        // Reset mouse position to where it was first clicked and disable unbounded movement
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.setScreenPosition(e.getMouseDownScreenPosition().toFloat());
        mouseSource.enableUnboundedMouseMovement(false);
        dragEnd();
    }

    void paint(Graphics& g) override
    {
        if (!isBeingEdited()) {
            g.setColour(findColour(Label::textColourId));
            g.setFont(getFont());

            auto textArea = getBorderSize().subtractedFrom(getLocalBounds());
            g.drawText(getText(), textArea, Justification::left, false);
        }
    }
};
