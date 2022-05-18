

struct ToggleComponent : public GUIComponent
{
    bool toggleState = false;
    Value nonZero;

    ToggleComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        nonZero = static_cast<t_toggle*>(gui.getPointer())->x_nonzero;
        initialise(newObject);
    }
    
    void paint(Graphics& g) override
    {
        auto backgroundColour = gui.getBackgroundColour();
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);
        
        auto toggledColour = gui.getForegroundColour();
        auto untoggledColour = toggledColour.interpolatedWith(backgroundColour, 0.8f);
        g.setColour(toggleState ? toggledColour : untoggledColour);

        auto crossBounds = getLocalBounds().reduced(6).toFloat();

        if (getWidth() < 20)
        {
            crossBounds = crossBounds.expanded(20 - getWidth());
        }

        const auto max = std::max(crossBounds.getWidth(), crossBounds.getHeight());
        const auto strokeWidth = std::max(max * 0.15f, 2.0f);

        g.drawLine({crossBounds.getTopLeft(), crossBounds.getBottomRight()}, strokeWidth);
        g.drawLine({crossBounds.getBottomLeft(), crossBounds.getTopRight()}, strokeWidth);
    }
    
    void mouseDown(const MouseEvent& e) override
    {
        startEdition();
        auto newValue = getValueOriginal() != 0 ? 0 : static_cast<float>(nonZero.getValue());
        setValueOriginal(newValue);
        toggleState = newValue;
        stopEdition();
        
        repaint();
    }

    void checkBoxBounds() override
    {
        // Fix aspect ratio and apply limits
        int size = jlimit(30, maxSize, box->getWidth());
        if (size != box->getHeight() || size != box->getWidth())
        {
            box->setSize(size, size);
        }
    }

    ObjectParameters defineParameters() override
    {
        return {
            {"Non-zero value", tInt, cGeneral, &nonZero, {}},
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(nonZero))
        {
            float val = nonZero.getValue();
            max = val;
            static_cast<t_toggle*>(gui.getPointer())->x_nonzero = val;
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }

    void update() override
    {
        toggleState = getValueOriginal() > std::numeric_limits<float>::epsilon();
        repaint();
    }
};
