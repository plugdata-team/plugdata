/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class BangObject final : public ObjectBase {
    uint32_t lastBang = 0;

    Value bangInterrupt = Value(100.0f);
    Value bangHold = Value(40.0f);

    bool bangState = false;
    bool alreadyBanged = false;

    IEMHelper iemHelper;

public:
    BangObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , iemHelper(obj, parent, this)
    {
        auto* bng = static_cast<t_bng*>(ptr);
        bangInterrupt = bng->x_flashtime_break;
        bangHold = bng->x_flashtime_hold;

        parent->constrainer->setFixedAspectRatio(1);
    }

    void initialiseParameters() override
    {
        iemHelper.initialiseParameters();
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

    void updateBounds() override
    {
        iemHelper.updateBounds();
    }

    void applyBounds() override
    {
        iemHelper.applyBounds(object->getObjectBounds());
    }

    void toggleObject(Point<int> position) override
    {
        if (!alreadyBanged) {
            startEdition();
            sendFloatValue(1.0f);
            stopEdition();
            update();
            alreadyBanged = true;
        }
    }

    void untoggleObject() override
    {
        alreadyBanged = false;
    }

    void mouseDown(MouseEvent const& e) override
    {
        startEdition();
        sendFloatValue(1.0f);
        stopEdition();

        // Make sure we don't re-click with an accidental drag
        alreadyBanged = true;
        update();
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);

        auto const bounds = getLocalBounds().reduced(1).toFloat();
        auto const width = std::max(bounds.getWidth(), bounds.getHeight());

        float const circleOuter = 80.f * (width * 0.01f);
        float const circleThickness = std::max(width * 0.06f, 1.5f);

        g.setColour(object->findColour(PlugDataColour::objectOutlineColourId));
        g.drawEllipse(bounds.reduced(width - circleOuter), circleThickness);

        if (bangState) {
            g.setColour(iemHelper.getForegroundColour());
            g.fillEllipse(bounds.reduced(width - circleOuter + circleThickness));
        }
    }

    float getValue()
    {
        // hack to trigger off the bang if no GUI update
        if ((static_cast<t_bng*>(ptr))->x_flashed > 0) {
            static_cast<t_bng*>(ptr)->x_flashed = 0;
            return 1.0f;
        }
        return 0.0f;
    }

    void update()
    {
        if (getValue() > std::numeric_limits<float>::epsilon()) {
            bangState = true;
            repaint();

            auto currentTime = Time::getCurrentTime().getMillisecondCounter();
            auto timeSinceLast = currentTime - lastBang;

            int holdTime = bangHold.getValue();

            if (timeSinceLast < static_cast<int>(bangHold.getValue()) * 2) {
                holdTime = timeSinceLast / 2;
            }
            if (holdTime < bangInterrupt) {
                holdTime = bangInterrupt.getValue();
            }

            lastBang = currentTime;

            auto deletionChecker = SafePointer(this);
            Timer::callAfterDelay(holdTime,
                [deletionChecker, this]() mutable {
                    // First check if this object still exists
                    if (!deletionChecker)
                        return;

                    if (bangState) {
                        bangState = false;
                        repaint();
                    }
                });
        }
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = {
            { "Minimum flash time", tInt, cGeneral, &bangInterrupt, {} },
            { "Maximum flash time", tInt, cGeneral, &bangHold, {} },
        };

        auto iemParameters = iemHelper.getParameters();
        allParameters.insert(allParameters.end(), iemParameters.begin(), iemParameters.end());

        return allParameters;
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(bangInterrupt)) {
            static_cast<t_bng*>(ptr)->x_flashtime_break = bangInterrupt.getValue();
        }
        if (value.refersToSameSourceAs(bangHold)) {
            static_cast<t_bng*>(ptr)->x_flashtime_hold = bangHold.getValue();
        } else {
            iemHelper.valueChanged(value);
        }
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("bang"):
        case hash("list"):
            update();
            break;
        case hash("flashtime"): {
            if (atoms.size() > 0)
                setParameterExcludingListener(bangInterrupt, atoms[0].getFloat());
            if (atoms.size() > 1)
                setParameterExcludingListener(bangHold, atoms[1].getFloat());
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }
};
