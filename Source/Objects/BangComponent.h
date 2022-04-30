
struct BangComponent : public GUIComponent
{
    uint32_t lastBang = 0;

    Value bangInterrupt = Value(100.0f);
    Value bangHold = Value(40.0f);

    // TODO: fix in LookAndFeel!
    struct BangButton : public TextButton
    {
        Box* box;

        BangButton(Box* parent) : box(parent){};

        void paint(Graphics& g) override
        {
            const auto bounds = getLocalBounds().reduced(1).toFloat();
            const auto width = std::max(bounds.getWidth(), bounds.getHeight());

            const float circleOuter = 80.f * (width * 0.01f);
            const float circleThickness = std::max(8.f * (width * 0.01f), 2.0f);

            g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
            g.drawEllipse(bounds.reduced(width - circleOuter), circleThickness);

            g.setColour(getToggleState() ? findColour(TextButton::buttonOnColourId) : Colours::transparentWhite);

            g.fillEllipse(bounds.reduced(width - circleOuter + circleThickness));
        }
    };

    BangButton bangButton;

    BangComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject), bangButton(parent)
    {
        addAndMakeVisible(bangButton);

        bangButton.setTriggeredOnMouseDown(true);
        bangButton.setName("pd:bang");

        bangButton.onClick = [this]()
        {
            startEdition();
            setValueOriginal(1);
            stopEdition();
            update();  // ?
        };

        bangInterrupt = static_cast<t_bng*>(gui.getPointer())->x_flashtime_break;
        bangHold = static_cast<t_bng*>(gui.getPointer())->x_flashtime_hold;

        initialise(newObject);
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

    void update() override
    {
        if (getValueOriginal() > std::numeric_limits<float>::epsilon())
        {
            bangButton.setToggleState(true, dontSendNotification);

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

            auto button = SafePointer<TextButton>(&bangButton);
            Timer::callAfterDelay(holdTime,
                                  [button]() mutable
                                  {
                                      if (!button) return;
                                      button->setToggleState(false, dontSendNotification);
                                      if (button->isDown())
                                      {
                                          button->setState(Button::ButtonState::buttonNormal);
                                      }
                                  });
        }
    }

    void resized() override
    {
        bangButton.setBounds(getLocalBounds().reduced(1));
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
            static_cast<t_bng*>(gui.getPointer())->x_flashtime_break = bangInterrupt.getValue();
        }
        if (value.refersToSameSourceAs(bangHold))
        {
            static_cast<t_bng*>(gui.getPointer())->x_flashtime_hold = bangHold.getValue();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
};
