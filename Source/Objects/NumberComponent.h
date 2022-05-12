
struct NumberComponent : public GUIComponent
{
    float dragValue = 0.0f;
    bool shift = false;
    int decimalDrag = 0;

    Point<float> lastDragPos;

    NumberComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        input.onEditorShow = [this]()
        {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            if (!gui.isAtom())
            {
                editor->setBorder({0, 10, 0, 0});
            }

            if (editor != nullptr)
            {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]()
        {
            setValueOriginal(input.getText().getFloatValue());
            stopEdition();
        };

        if (!gui.isAtom())
        {
            input.setBorderSize({1, 15, 1, 1});
        }

        addAndMakeVisible(input);

        input.setText(formatNumber(getValueOriginal()), dontSendNotification);

        initialise(newObject);

        input.setEditable(true, false);

        addMouseListener(this, true);
    }

    void checkBoxBounds() override
    {
        // Apply size limits
        int w = jlimit(30, maxSize, box->getWidth());
        int h = jlimit(Box::height - 12, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        input.setFont(getHeight() - 6);
    }

    String formatNumber(float value)
    {
        String text;
        text << value;

        while (text.length() > 1 && input.getFont().getStringWidth(text) > getWidth() - 5)
        {
            text = text.substring(0, text.length() - 2);
        }
        if (!text.containsChar('.')) text << '.';

        return text;
    }

    void update() override
    {
        input.setText(formatNumber(getValueOriginal()), dontSendNotification);
    }

    void mouseDown(const MouseEvent& e) override
    {
        GUIComponent::mouseDown(e);
        if (input.isBeingEdited()) return;

        
        // Hide cursor and set unbounded mouse movement
        input.setMouseCursor(MouseCursor::NoCursor);
        input.updateMouseCursor();
        
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.setScreenPosition(e.getMouseDownScreenPosition().toFloat());
        mouseSource.enableUnboundedMouseMovement(true, true);
        
        startEdition();
        shift = e.mods.isShiftDown();
        dragValue = input.getText().getFloatValue();
        

        lastDragPos = e.position;

        const auto textArea = input.getBorderSize().subtractedFrom(input.getBounds());

        GlyphArrangement glyphs;
        glyphs.addFittedText(input.getFont(), input.getText(), textArea.getX(), 0., textArea.getWidth(), getHeight(), Justification::centredLeft, 1, input.getMinimumHorizontalScale());

        double decimalX = getWidth();
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

            decimalsGlyph.addFittedText(input.getFont(), decimalStr, decimalX, 0, getWidth(), getHeight(), Justification::centredLeft, 1, input.getMinimumHorizontalScale());

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
    }

    void mouseUp(const MouseEvent& e) override
    {
        if (input.isBeingEdited()) return;

        // Show cursor again
        input.setMouseCursor(MouseCursor::NormalCursor);
        input.updateMouseCursor();
        
        // Reset mouse position to where it was first clicked and disable unbounded movement
        auto mouseSource = Desktop::getInstance().getMainMouseSource();
        mouseSource.setScreenPosition(e.getMouseDownScreenPosition().toFloat());
        mouseSource.enableUnboundedMouseMovement(false);
        stopEdition();
    }

    void mouseDrag(const MouseEvent& e) override
    {
        if (input.isBeingEdited()) return;

        const int decimal = decimalDrag + e.mods.isShiftDown();
        const float increment = (decimal == 0) ? 1. : (1. / std::pow(10., decimal));
        const float deltaY = e.y - lastDragPos.y;
        lastDragPos = e.position;

        dragValue += increment * -deltaY;
        
        // truncate value and set
        double newValue = dragValue;

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

        setValueOriginal(newValue);

        input.setText(formatNumber(getValueOriginal()), NotificationType::dontSendNotification);
    }

    ObjectParameters defineParameters() override
    {
        return {{"Minimum", tFloat, cGeneral, &min, {}}, {"Maximum", tFloat, cGeneral, &max, {}}};
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        }
        else if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        }
        else if (gui.isAtom() && value.refersToSameSourceAs(labelHeight))
        {
            updateLabel();
            box->updateBounds(false);  // update box size based on new font
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        GUIComponent::paintOverChildren(g);

        if (gui.isAtom()) return;

        const int indent = 9;

        const Rectangle<int> iconBounds = getLocalBounds().withWidth(indent - 4).withHeight(getHeight() - 8).translated(4, 4);

        Path corner;

        corner.addTriangle(iconBounds.getTopLeft().toFloat(), iconBounds.getTopRight().toFloat() + Point<float>(0, (iconBounds.getHeight() / 2.)), iconBounds.getBottomLeft().toFloat());

        g.setColour(Colour(gui.getForegroundColour()));
        g.fillPath(corner);
    }
};
