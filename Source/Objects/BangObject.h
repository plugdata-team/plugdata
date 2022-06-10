
struct BangObject final : public IEMObject
{
    uint32_t lastBang = 0;

    Value bangInterrupt = Value(100.0f);
    Value bangHold = Value(40.0f);

    bool bangState = false;

    BangObject(void* obj, Box* parent) : IEMObject(obj, parent)
    {
        bangInterrupt = static_cast<t_bng*>(ptr)->x_flashtime_break;
        bangHold = static_cast<t_bng*>(ptr)->x_flashtime_hold;

        initialise();
    }

    void checkBounds() override
    {
        // Fix aspect ratio and apply limits
        int size = jlimit(30, maxSize, box->getWidth());
        if (size != box->getHeight() || size != box->getWidth())
        {
            box->setSize(size, size);
        }
    }

    void mouseDown(const MouseEvent& e) override
    {
        startEdition();
        setValueOriginal(1);
        stopEdition();
        update();
    }

    void paint(Graphics& g) override
    {
        IEMObject::paint(g);

        const auto bounds = getLocalBounds().reduced(1).toFloat();
        const auto width = std::max(bounds.getWidth(), bounds.getHeight());

        const float circleOuter = 80.f * (width * 0.01f);
        const float circleThickness = std::max(width * 0.06f, 1.5f);

        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        g.drawEllipse(bounds.reduced(width - circleOuter), circleThickness);

        g.setColour(bangState ? getForegroundColour() : Colours::transparentWhite);

        g.fillEllipse(bounds.reduced(width - circleOuter + circleThickness));
    }

    float getValue() override
    {
        // hack to trigger off the bang if no GUI update
        if ((static_cast<t_bng*>(ptr))->x_flashed > 0)
        {
            static_cast<t_bng*>(ptr)->x_flashed = 0;
            return 1.0f;
        }
        return 0.0f;
    }

    void update() override
    {
        if (getValueOriginal() > std::numeric_limits<float>::epsilon())
        {
            bangState = true;
            repaint();

            auto currentTime = Time::getCurrentTime().getMillisecondCounter();
            auto timeSinceLast = currentTime - lastBang;

            int holdTime = bangHold.getValue();

            if (timeSinceLast < static_cast<int>(bangHold.getValue()) * 2)
            {
                holdTime = timeSinceLast / 2;
            }
            if (holdTime < bangInterrupt)
            {
                holdTime = bangInterrupt.getValue();
            }

            lastBang = currentTime;

            auto deletionChecker = SafePointer<Component>(this);
            Timer::callAfterDelay(holdTime,
                                  [deletionChecker, this]() mutable
                                  {
                                      // First check if this object still exists
                                      if (!deletionChecker) return;

                                      if (bangState)
                                      {
                                          bangState = false;
                                          repaint();
                                      }
                                  });
        }
    }

    ObjectParameters defineParameters() override
    {
        return {
            {"Interrupt", tInt, cGeneral, &bangInterrupt, {}},
            {"Hold", tInt, cGeneral, &bangHold, {}},
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(bangInterrupt))
        {
            static_cast<t_bng*>(ptr)->x_flashtime_break = bangInterrupt.getValue();
        }
        if (value.refersToSameSourceAs(bangHold))
        {
            static_cast<t_bng*>(ptr)->x_flashtime_hold = bangHold.getValue();
        }
        else
        {
            IEMObject::valueChanged(value);
        }
    }

    float getMaximum() const
    {
        return (static_cast<t_my_numbox*>(ptr))->x_max;
    }
};
