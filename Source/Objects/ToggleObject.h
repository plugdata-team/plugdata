/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ToggleObject final : public ObjectBase {
    bool toggleState = false;
    bool alreadyToggled = false;
    Value nonZero;

    float value = 0.0f;

    IEMHelper iemHelper;

public:
    ToggleObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
    {
        onConstrainerCreate = [this]() {
            constrainer->setFixedAspectRatio(1);
        };
    }

    bool hideInlets() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(label);
    }

    Rectangle<int> getPdBounds() override
    {
        return iemHelper.getPdBounds();
    }

    void setPdBounds(Rectangle<int> b) override
    {
        iemHelper.setPdBounds(b);
    }

    void update() override
    {
        nonZero = static_cast<t_toggle*>(ptr)->x_nonzero;
        iemHelper.update();

        value = getValue();
        setToggleStateFromFloat(value);
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);

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
            auto newValue = value != 0 ? 0 : ::getValue<float>(nonZero);
            sendToggleValue(newValue);
            setToggleStateFromFloat(newValue);
            stopEdition();
            alreadyToggled = true;
        }
    }

    void sendToggleValue(int newValue)
    {
        pd->enqueueFunction([ptr = this->ptr, pd = this->pd, patch = &cnv->patch, newValue]() {
            if (patch->objectWasDeleted(ptr))
                return;

            t_atom atom;
            SETFLOAT(&atom, newValue);
            pd_typedmess(static_cast<t_pd*>(ptr), pd->generateSymbol("set"), 1, &atom);

            auto* iem = static_cast<t_iemgui*>(ptr);
            outlet_float(iem->x_obj.ob_outlet, newValue);
            if (iem->x_fsf.x_snd_able && iem->x_snd->s_thing)
                pd_float(iem->x_snd->s_thing, newValue);
        });
    }

    void untoggleObject() override
    {
        alreadyToggled = false;
        repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        startEdition();
        auto newValue = value != 0 ? 0 : ::getValue<float>(nonZero);
        sendToggleValue(newValue);
        setToggleStateFromFloat(newValue);
        stopEdition();

        // Make sure we don't re-toggle with an accidental drag
        alreadyToggled = true;
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = {
            { "Non-zero value", tFloat, cGeneral, &nonZero, {} }
        };

        auto iemParameters = iemHelper.getParameters();
        allParameters.insert(allParameters.end(), iemParameters.begin(), iemParameters.end());

        return allParameters;
    }

    void setToggleStateFromFloat(float newValue)
    {
        value = newValue;
        toggleState = std::abs(newValue) > std::numeric_limits<float>::epsilon();
        repaint();
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("bang"),
            hash("float"),
            hash("nonzero"),
            IEMGUI_MESSAGES
        };
    }

    void receiveObjectMessage(hash32 const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (symbol) {
        case hash("bang"): {
            value = !value;
            setToggleStateFromFloat(value);
            break;
        }
        case hash("float"):
        case hash("set"): {
            value = atoms[0].getFloat();
            setToggleStateFromFloat(value);
            break;
        }
        case hash("nonzero"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(nonZero, atoms[0].getFloat());
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(nonZero)) {
            float val = nonZero.getValue();
            static_cast<t_toggle*>(ptr)->x_nonzero = val;
        } else {
            iemHelper.valueChanged(value);
        }
    }

    float getValue()
    {
        return (static_cast<t_toggle*>(ptr))->x_on;
    }
};
