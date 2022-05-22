

struct ToggleObject : public IEMObject
{
    bool toggleState = false;
    Value nonZero;

    ToggleObject(void* obj, Box* parent) : IEMObject(obj, parent)
    {
        nonZero = static_cast<t_toggle*>(ptr)->x_nonzero;
        initialise();
    }

    void paint(Graphics& g) override
    {
        auto backgroundColour = getBackgroundColour();
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

        auto toggledColour = getForegroundColour();
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
            static_cast<t_toggle*>(ptr)->x_nonzero = val;
        }
        else
        {
            GUIObject::valueChanged(value);
        }
    }

    float getValue() override
    {
        return (static_cast<t_toggle*>(ptr))->x_on;
    }

    void update() override
    {
        toggleState = getValueOriginal() > std::numeric_limits<float>::epsilon();
        repaint();
    }
};
