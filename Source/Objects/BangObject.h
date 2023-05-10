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
        onConstrainerCreate = [this]() {
            constrainer->setFixedAspectRatio(1);
        };
    }

    void update() override
    {
        auto* bng = static_cast<t_bng*>(ptr);
        bangInterrupt = bng->x_flashtime_break;
        bangHold = bng->x_flashtime_hold;

        iemHelper.update();
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

    void toggleObject(Point<int> position) override
    {
        if (!alreadyBanged) {
            pd->enqueueFunction([this]() {
                if (cnv->patch.objectWasDeleted(ptr))
                    return;

                startEdition();
                pd_bang(static_cast<t_pd*>(ptr));
                stopEdition();
            });

            trigger();
            alreadyBanged = true;
        }
    }

    void untoggleObject() override
    {
        alreadyBanged = false;
    }

    void mouseDown(MouseEvent const& e) override
    {
        pd->enqueueFunction([this]() {
            if (cnv->patch.objectWasDeleted(ptr))
                return;

            startEdition();
            pd_bang(static_cast<t_pd*>(ptr));
            stopEdition();
        });

        // Make sure we don't re-click with an accidental drag
        alreadyBanged = true;
        trigger();
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);

        auto const bounds = getLocalBounds().reduced(1).toFloat();
        auto const width = std::max(bounds.getWidth(), bounds.getHeight());

        float const circleOuter = 80.f * (width * 0.01f);
        float const circleThickness = std::max(width * 0.06f, 1.5f);

        g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));
        g.drawEllipse(bounds.reduced(width - circleOuter), circleThickness);

        if (bangState) {
            g.setColour(iemHelper.getForegroundColour());
            g.fillEllipse(bounds.reduced(width - circleOuter + circleThickness));
        }
    }

    void trigger()
    {
        if (bangState)
            return;

        bangState = true;
        repaint();

        auto currentTime = Time::getCurrentTime().getMillisecondCounter();
        auto timeSinceLast = currentTime - lastBang;

        int holdTime = bangHold.getValue();

        if (timeSinceLast < getValue<int>(bangHold) * 2) {
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

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = {
            makeObjectParam("Minimum flash time", tInt, cGeneral, &bangInterrupt, {} ),
            makeObjectParam("Maximum flash time", tInt, cGeneral, &bangHold, {} )
        };

        iemHelper.addIemParameters(&allParameters);

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

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("anything")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("bang"):
        case hash("list"):
            trigger();
            break;
        case hash("flashtime"): {
            if (atoms.size() > 0)
                setParameterExcludingListener(bangInterrupt, atoms[0].getFloat());
            if (atoms.size() > 1)
                setParameterExcludingListener(bangHold, atoms[1].getFloat());
            break;
        }
        default: {
            bool wasIemMessage = iemHelper.receiveObjectMessage(symbol, atoms);
            if (!wasIemMessage) {
                trigger();
            }
            break;
        }
        }
    }
};
