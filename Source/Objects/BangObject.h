/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct BangObject final : public IEMObject {
    uint32_t lastBang = 0;

    Value bangInterrupt = Value(100.0f);
    Value bangHold = Value(40.0f);

    bool bangState = false;
    bool alreadyBanged = false;

    BangObject(void* obj, Object* parent)
        : IEMObject(obj, parent)
    {
        auto* bng = static_cast<t_bng*>(ptr);
        bangInterrupt = bng->x_flashtime_break;
        bangHold = bng->x_flashtime_hold;
    }

    void checkBounds() override
    {
        // Fix aspect ratio and apply limits
        int size = jlimit(30, maxSize, object->getWidth());
        if (size != object->getHeight() || size != object->getWidth()) {
            object->setSize(size, size);
        }
    }
    void toggleObject(Point<int> position) override
    {
        if (!alreadyBanged) {
            startEdition();
            setValueOriginal(1);
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
        setValueOriginal(1);
        stopEdition();

        // Make sure we don't re-click with an accidental drag
        alreadyBanged = true;

        update();
    }

    void paint(Graphics& g) override
    {
        IEMObject::paint(g);

        auto const bounds = getLocalBounds().reduced(1).toFloat();
        auto const width = std::max(bounds.getWidth(), bounds.getHeight());

        float const circleOuter = 80.f * (width * 0.01f);
        float const circleThickness = std::max(width * 0.06f, 1.5f);


        g.setColour(object->findColour(PlugDataColour::objectOutlineColourId));
        g.drawEllipse(bounds.reduced(width - circleOuter), circleThickness);

        if (bangState) {
            g.setColour(getForegroundColour());
            g.fillEllipse(bounds.reduced(width - circleOuter + circleThickness));
        }
    }

    float getValue() override
    {
        // hack to trigger off the bang if no GUI update
        if ((static_cast<t_bng*>(ptr))->x_flashed > 0) {
            static_cast<t_bng*>(ptr)->x_flashed = 0;
            return 1.0f;
        }
        return 0.0f;
    }

    void update() override
    {
        if (getValueOriginal() > std::numeric_limits<float>::epsilon()) {
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

    ObjectParameters defineParameters() override
    {
        return {
            { "Interrupt", tInt, cGeneral, &bangInterrupt, {} },
            { "Hold", tInt, cGeneral, &bangHold, {} },
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(bangInterrupt)) {
            static_cast<t_bng*>(ptr)->x_flashtime_break = bangInterrupt.getValue();
        }
        if (value.refersToSameSourceAs(bangHold)) {
            static_cast<t_bng*>(ptr)->x_flashtime_hold = bangHold.getValue();
        } else {
            IEMObject::valueChanged(value);
        }
    }
};
