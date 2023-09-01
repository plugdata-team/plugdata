/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

class DraggableNumber : public Label
    , public Label::Listener {
public:
    enum DragMode {
        Regular,
        Integer,
        Logarithmic
    };

protected:
    double dragValue = 0.0;
    int decimalDrag = 0;
    int hoveredDecimal = -1;
    Rectangle<float> hoveredDecimalPosition;

    double lastValue = 0.0;

    bool isMinLimited = false, isMaxLimited = false;
    DragMode dragMode = Regular;
    double logarithmicHeight = 256.0f;
    int lastLogarithmicDragPosition = 0;
    double min = 0.0, max = 0.0;

    bool resetOnCommandClick = false;
    bool wasReset = false;
    double valueToResetTo = 0.0;
    double valueToRevertTo = 0.0;

    GlyphArrangement currentGlyphs;

public:
    std::function<void(double)> onValueChange = [](double) {};
    std::function<void()> dragStart = []() {};
    std::function<void()> dragEnd = []() {};

    explicit DraggableNumber(bool integerDrag)
        : dragMode(integerDrag ? Integer : Regular)
    {
        setWantsKeyboardFocus(true);
        addListener(this);
        setFont(Fonts::getTabularNumbersFont().withHeight(14.0f));
        setBufferedToImage(true);
    }

    void labelTextChanged(Label* labelThatHasChanged) override {};

    void editorShown(Label* l, TextEditor& editor) override
    {
        dragStart();
    }

    void editorHidden(Label* l, TextEditor& editor) override
    {
        auto newValue = editor.getText().getDoubleValue();
        setValue(newValue, dontSendNotification);
        dragEnd();
    }

    void setEditableOnClick(bool editable)
    {
        setEditable(editable, editable);
        setInterceptsMouseClicks(true, true);
    }

    void setMaximum(double maximum)
    {
        isMaxLimited = true;
        max = maximum;
    }

    void setMinimum(double minimum)
    {
        isMinLimited = true;
        min = minimum;
    }

    void setLogarithmicHeight(double logHeight)
    {
        logarithmicHeight = logHeight;
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

        if (!isEditableOnSingleClick() && !getCurrentTextEditor() && key.isKeyCode(KeyPress::upKey)) {
            setValue(getText().getDoubleValue() + 1.0);
            return true;
        }
        if (!isEditableOnSingleClick() && !getCurrentTextEditor() && key.isKeyCode(KeyPress::downKey)) {
            setValue(getText().getDoubleValue() - 1.0);
            return true;
        }

        return false;
    }

    void setValue(double newValue, NotificationType notification = sendNotification)
    {
        wasReset = false;

        newValue = limitValue(newValue);

        if (lastValue != newValue) {
            lastValue = newValue;

            setText(String(newValue, 8), notification);
            onValueChange(newValue);
        }
    }

    double getValue() const
    {
        return lastValue;
    }

    void setResetEnabled(bool enableReset)
    {
        resetOnCommandClick = enableReset;
    }

    void setResetValue(double resetValue)
    {
        valueToResetTo = resetValue;
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

        hoveredDecimal = -1;
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        bool command = e.mods.isCommandDown();
        bool shift = e.mods.isShiftDown();

        if (command && resetOnCommandClick) {
            if (wasReset) {
                setValue(valueToRevertTo);
            } else {
                valueToRevertTo = lastValue;
                setValue(valueToResetTo);
                wasReset = true;
            }
        }

        dragValue = getText().getDoubleValue();

        auto const textArea = getBorderSize().subtractedFrom(getLocalBounds());

        GlyphArrangement glyphs;
        glyphs.addFittedText(getFont(), formatNumber(dragValue), textArea.getX(), 0., textArea.getWidth(), getHeight(), 1, getMinimumHorizontalScale());

        if (dragMode != Regular) {
            decimalDrag = 0;
            lastLogarithmicDragPosition = e.y;
            return;
        }

        decimalDrag = getDecimalAtPosition(e.getMouseDownX());

        dragStart();
    }

    void setDragMode(DragMode newDragMode)
    {
        dragMode = newDragMode;
    }

    int getDecimalAtPosition(int x, Rectangle<float>* position = nullptr)
    {
        auto const textArea = getBorderSize().subtractedFrom(getLocalBounds());

        // For integer or logarithmic drag mode, draw the highlighted area around the whole number
        if (dragMode != Regular) {
            auto text = dragMode == Integer ? getText().upToFirstOccurrenceOf(".", false, false) : String(getText().getDoubleValue());

            GlyphArrangement glyphs;
            glyphs.addFittedText(getFont(), text, textArea.getX(), 0., 99999, getHeight(), 1, 1.0f);
            auto glyphsBounds = glyphs.getBoundingBox(0, glyphs.getNumGlyphs(), false);
            if (x < glyphsBounds.getRight()) {
                if (position)
                    *position = glyphsBounds;
                return 0;
            } else {
                if (position)
                    *position = Rectangle<float>();
                return -1;
            }
        }

        GlyphArrangement glyphs;
        auto formattedNumber = formatNumber(getText().getDoubleValue());
        auto fullNumber = formattedNumber + String("000000");
        glyphs.addFittedText(getFont(), fullNumber, textArea.getX(), 0., 99999, getHeight(), 1, 1.0f);
        int draggedDecimal = -1;

        int decimalPointPosition = 0;
        bool afterDecimalPoint = false;
        for (int i = 0; i < glyphs.getNumGlyphs(); ++i) {
            auto const& glyph = glyphs.getGlyph(i);

            bool isDecimalPoint = glyph.getCharacter() == '.';
            if (isDecimalPoint) {
                decimalPointPosition = i;
                afterDecimalPoint = true;
            }

            if (x <= glyph.getRight()) {
                draggedDecimal = isDecimalPoint ? 0 : i - decimalPointPosition;

                auto glyphBounds = glyph.getBounds();
                if (!afterDecimalPoint) {
                    continue;
                }
                if (isDecimalPoint) {
                    glyphBounds = Rectangle<float>();
                    for (int j = 0; j < i; j++) {
                        glyphBounds = glyphBounds.getUnion(glyphs.getGlyph(j).getBounds());
                    }
                }
                if (position)
                    *position = glyphBounds;

                break;
            }
        }

        return draggedDecimal;
    }

    void paint(Graphics& g) override
    {
        if (hoveredDecimal >= 0) {
            // TODO: make this colour Id configurable?
            g.setColour(findColour(ComboBox::outlineColourId).withAlpha(isMouseButtonDown() ? 0.5f : 0.3f));
            PlugDataLook::fillSmoothedRectangle(g, hoveredDecimalPosition, 2.5f);
        }

        if (!isBeingEdited()) {
            auto textArea = getBorderSize().subtractedFrom(getLocalBounds()).toFloat();
            // g.drawText(formatNumber(getText().getDoubleValue(), decimalDrag), textArea, Justification::centredLeft);

            auto numberText = formatNumber(getText().getDoubleValue(), decimalDrag);
            auto extraNumberText = String();
            auto numDecimals = numberText.fromFirstOccurrenceOf(".", false, false).length();
            for (int i = 0; i < std::min(hoveredDecimal - decimalDrag, 7 - numDecimals); ++i)
                extraNumberText += "0";

            auto numberTextLength = getFont().getStringWidthFloat(numberText);

            g.setFont(getFont());
            g.setColour(findColour(Label::textColourId));
            g.drawText(numberText, textArea, Justification::centredLeft);

            if (dragMode == Regular) {
                g.setColour(findColour(Label::textColourId).withAlpha(0.4f));
                g.drawText(extraNumberText, textArea.withTrimmedLeft(numberTextLength), Justification::centredLeft);
            }
        }
    }

    void updateHoverPosition(int x)
    {
        int oldHoverPosition = hoveredDecimal;
        hoveredDecimal = getDecimalAtPosition(x, &hoveredDecimalPosition);

        if (oldHoverPosition != hoveredDecimal) {
            repaint();
        }
    }

    void mouseMove(MouseEvent const& e) override
    {
        updateHoverPosition(e.x);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        updateHoverPosition(e.getMouseDownX());

        // Hide cursor and set unbounded mouse movement
        setMouseCursor(MouseCursor::NoCursor);
        updateMouseCursor();

        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.enableUnboundedMouseMovement(true, true);

        if (dragMode == Logarithmic) {
            double logMin = min;
            double logMax = max;

            if ((logMin == 0.0) && (logMax == 0.0))
                logMax = 1.0;
            if (logMax > 0.0) {
                if (logMin <= 0.0)
                    logMin = 0.01 * logMax;
            } else {
                if (logMin > 0.0)
                    logMax = 0.01 * logMin;
            }

            double dy = lastLogarithmicDragPosition - e.y;
            double k = exp(log(logMax / logMin) / std::max(logarithmicHeight, 10.0));
            double factor = pow(k, dy);
            setValue(std::clamp(getValue(), logMin, logMax) * factor);

            lastLogarithmicDragPosition = e.y;
        } else {
            int const decimal = decimalDrag + e.mods.isShiftDown();
            double const increment = (decimal == 0) ? 1. : (1. / std::pow(10., decimal));
            double const deltaY = (e.y - e.mouseDownPosition.y) * 0.7;

            // truncate value and set
            double newValue = dragValue + (increment * -deltaY);

            if (decimal > 0) {
                int const sign = (newValue > 0) ? 1 : -1;
                unsigned long long ui_temp = (newValue * std::pow(10, decimal)) * sign;
                newValue = (((long double)ui_temp) / std::pow(10, decimal) * sign);
            } else {
                newValue = static_cast<int64_t>(newValue);
            }

            setValue(newValue);
        }
    }

    double limitValue(double valueToLimit) const
    {
        if (min == 0.0 && max == 0.0)
            return valueToLimit;

        if (isMinLimited)
            valueToLimit = std::max(valueToLimit, min);
        if (isMaxLimited)
            valueToLimit = std::min(valueToLimit, max);

        return valueToLimit;
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        repaint();

        // Show cursor again
        setMouseCursor(MouseCursor::NormalCursor);
        updateMouseCursor();

        // Reset mouse position to where it was first clicked and disable unbounded movement
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.setScreenPosition(e.getMouseDownScreenPosition().toFloat());
        mouseSource.enableUnboundedMouseMovement(false);
        dragEnd();

        if (!e.mouseWasDraggedSinceMouseDown()) {
            Label::mouseUp(e);
        }
    }

    static String formatNumber(double value, int precision = -1)
    {
        auto text = String(value, precision == -1 ? 8 : precision);

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
        : DraggableNumber(true)
    {
        setEditableOnClick(true);
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (isBeingEdited())
            return;

        repaint();

        auto [numberStart, numberEnd, numberValue] = getListItemAtPosition(e.x);

        numberStartIdx = numberStart;
        numberEndIdx = numberEnd;
        dragValue = numberValue;

        targetFound = numberStart != -1;
        if (targetFound) {
            dragStart();
        }
    }

    void mouseMove(MouseEvent const& e) override
    {
        updateListHoverPosition(e.x);
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (isBeingEdited() || !targetFound)
            return;

        updateListHoverPosition(e.x);

        // Hide cursor and set unbounded mouse movement
        setMouseCursor(MouseCursor::NoCursor);
        updateMouseCursor();

        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.enableUnboundedMouseMovement(true, true);

        double const increment = 1.;
        double const deltaY = (e.y - e.mouseDownPosition.y) * 0.7;

        double newValue = dragValue + std::floor(increment * -deltaY);

        newValue = limitValue(newValue);

        int length = numberEndIdx - numberStartIdx;

        auto replacement = String();
        replacement << newValue;

        auto newText = getText().replaceSection(numberStartIdx, length, replacement);

        // In case the length of the number changes
        if (length != replacement.length()) {
            numberEndIdx = replacement.length() + numberStartIdx;
        }

        setText(newText, dontSendNotification);
        onValueChange(0);
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
        if (hoveredDecimal >= 0) {
            // TODO: make this colour Id configurable?
            g.setColour(findColour(ComboBox::outlineColourId).withAlpha(isMouseButtonDown() ? 0.5f : 0.3f));
            PlugDataLook::fillSmoothedRectangle(g, hoveredDecimalPosition, 2.5f);
        }

        if (!isBeingEdited()) {
            g.setColour(findColour(Label::textColourId));
            g.setFont(getFont());

            auto textArea = getBorderSize().subtractedFrom(getLocalBounds());
            g.drawText(getText(), textArea, Justification::centredLeft, false);
        }
    }

    void editorHidden(Label* l, TextEditor& editor) override
    {
        setText(editor.getText().trimEnd(), dontSendNotification);
        onValueChange(0);
        dragEnd();
    }

    void updateListHoverPosition(int x)
    {
        int oldHoverPosition = hoveredDecimal;

        Rectangle<float> position;
        auto [numberStart, numberEnd, numberValue] = getListItemAtPosition(x, &hoveredDecimalPosition);

        hoveredDecimal = numberStart;

        if (oldHoverPosition != hoveredDecimal) {
            repaint();
        }
    }

    std::tuple<int, int, double> getListItemAtPosition(int x, Rectangle<float>* position = nullptr)
    {
        auto const textArea = getBorderSize().subtractedFrom(getBounds());

        GlyphArrangement glyphs;
        glyphs.addFittedText(getFont(), getText(), textArea.getX(), 0., textArea.getWidth(), textArea.getHeight(), Justification::centredLeft, 1, getMinimumHorizontalScale());

        auto text = getText();
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
                    if (number.containsOnly("0123456789.-") && x >= startGlyph.getLeft() && x <= endGlyph.getRight()) {

                        if (position)
                            *position = glyphs.getBoundingBox(i, j - i, false).translated(0, 2);
                        return { i, j, number.getDoubleValue() };
                    }

                    // Move start to end of current item
                    i = j;
                    break;
                }
            }
        }

        return { -1, -1, 0.0f };
    }
};
