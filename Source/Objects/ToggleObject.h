/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct ToggleObject final : public ObjectBase {
    bool toggleState = false;
    bool alreadyToggled = false;
    Value nonZero;
    
    IEMHelper iemHelper;

    ToggleObject(void* ptr, Object* object)
    : ObjectBase(ptr, object)
    , iemHelper(ptr, object, this)
    {
    }
    
    void updateBounds() override
    {
        iemHelper.updateBounds();
    }
    
    void applyBounds() override
    {
        iemHelper.applyBounds();
    }

    void updateParameters() override
    {
        nonZero = static_cast<t_toggle*>(ptr)->x_nonzero;
        iemHelper.updateParameters();
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);

        auto toggledColour = iemHelper.getForegroundColour();
        auto untoggledColour = toggledColour.interpolatedWith(iemHelper.getBackgroundColour(), 0.8f);
        g.setColour(toggleState ? toggledColour : untoggledColour);

        auto crossBounds = getLocalBounds().reduced((getWidth() * 0.08f) + 4.5f).toFloat();

        if (getWidth() < 18) {
            crossBounds = getLocalBounds().toFloat().reduced(3.5f);
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
            auto newValue = value != 0 ? 0 : static_cast<float>(nonZero.getValue());
            setValue(newValue);
            toggleState = newValue;
            stopEdition();
            alreadyToggled = true;

            repaint();
        }
    }

    void untoggleObject() override
    {
        alreadyToggled = false;
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        startEdition();
        auto newValue = value != 0 ? 0 : static_cast<float>(nonZero.getValue());
        setValue(newValue);
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

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = {
            { "Non-zero value", tInt, cGeneral, &nonZero, {} }
        };
           
        auto iemParameters = iemHelper.getParameters();
        allParameters.insert(allParameters.end(), iemParameters.begin(), iemParameters.end());
        
        return allParameters;
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        if(symbol == "bang") {
            value = !value;
            toggleState = value > std::numeric_limits<float>::epsilon();
            repaint();
        }
        if(symbol == "float") {
            value = atoms[0].getFloat();
            toggleState = value > std::numeric_limits<float>::epsilon();
            repaint();
        }
        if (symbol == "nonzero" && atoms.size() >= 1) {
            setParameterExcludingListener(nonZero, atoms[0].getFloat());
        } else {
            iemHelper.receiveObjectMessage(symbol, atoms);
        }
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(nonZero)) {
            float val = nonZero.getValue();
            max = val;
            static_cast<t_toggle*>(ptr)->x_nonzero = val;
        } else {
            iemHelper.valueChanged(value);
        }
    }

    float getValue() override
    {
        return (static_cast<t_toggle*>(ptr))->x_on;
    }
};
