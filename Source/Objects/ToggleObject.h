

struct ToggleObject final : public IEMObject {
    bool toggleState = false;
    bool alreadyToggled = false;
    Value nonZero;

    ToggleObject(void* obj, Object* parent)
        : IEMObject(obj, parent)
    {
    }

    void updateParameters() override
    {
        nonZero = static_cast<t_toggle*>(ptr)->x_nonzero;
        IEMObject::updateParameters();
    }

    void paint(Graphics& g) override
    {
        IEMObject::paint(g);
        
        auto toggledColour = getForegroundColour();
        auto untoggledColour = getBackgroundColour();
        g.setColour(toggleState ? toggledColour : untoggledColour);

        auto crossBounds = getLocalBounds().reduced(6).toFloat();

        if (getWidth() < 20) {
            crossBounds = crossBounds.expanded(20 - getWidth());
        }

        auto const max = std::max(crossBounds.getWidth(), crossBounds.getHeight());
        auto const strokeWidth = std::max(max * 0.15f, 2.0f);

        g.drawLine({ crossBounds.getTopLeft(), crossBounds.getBottomRight() }, strokeWidth);
        g.drawLine({ crossBounds.getBottomLeft(), crossBounds.getTopRight() }, strokeWidth);
    }

    void toggleObject(Point<int> position) override
    {
        if (!alreadyToggled) {
            startEdition();
            auto newValue = getValueOriginal() != 0 ? 0 : static_cast<float>(nonZero.getValue());
            setValueOriginal(newValue);
            toggleState = newValue;
            stopEdition();
            alreadyToggled = true;
        }
    }

    void untoggleObject() override
    {
        alreadyToggled = false;
    }

    void mouseDown(MouseEvent const& e) override
    {
        startEdition();
        auto newValue = getValueOriginal() != 0 ? 0 : static_cast<float>(nonZero.getValue());
        setValueOriginal(newValue);
        toggleState = newValue;
        stopEdition();

        // Make sure we don't re-toggle with an accidental drag
        alreadyToggled = true;

        repaint();
    }

    void checkBounds() override
    {
        // Fix aspect ratio and apply limits
        int size = jlimit(30, maxSize, object->getWidth());
        if (size != object->getHeight() || size != object->getWidth()) {
            object->setSize(size, size);
        }
    }

    ObjectParameters defineParameters() override
    {
        return {
            { "Non-zero value", tInt, cGeneral, &nonZero, {} },
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(nonZero)) {
            float val = nonZero.getValue();
            max = val;
            static_cast<t_toggle*>(ptr)->x_nonzero = val;
        } else {
            IEMObject::valueChanged(value);
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
