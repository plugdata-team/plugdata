

struct ToggleComponent : public GUIComponent
{
    struct Toggle : public TextButton
    {
        std::function<void()> onMouseDown;

        void paint(Graphics& g) override
        {
            float width = getWidth() - 4;
            const float rectangleOuter = width * 0.70f;
            const float rectangleThickness = std::max(width * 0.04f, 1.5f);
            
            auto toggleBounds = getLocalBounds().toFloat().reduced(width - rectangleOuter);
            
            
            if(getToggleState()) {
                g.setColour(findColour(TextButton::buttonOnColourId));
                g.fillRoundedRectangle(toggleBounds, 2.0f);
            }
            
            g.setColour(findColour(PlugDataColour::canvasOutlineColourId));
            g.drawRoundedRectangle(toggleBounds, 2.0f, rectangleThickness);
        }

        void mouseDown(const MouseEvent& e) override
        {
            TextButton::mouseDown(e);
            onMouseDown();
        }
    };

    Value nonZero;
    Toggle toggleButton;

    ToggleComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        addAndMakeVisible(toggleButton);
        toggleButton.setToggleState(getValueOriginal(), dontSendNotification);
        toggleButton.setConnectedEdges(12);
        toggleButton.setName("pd:toggle");

        toggleButton.onMouseDown = [this]()
        {
            startEdition();
            auto newValue = getValueOriginal() != 0 ? 0 : static_cast<float>(nonZero.getValue());
            setValueOriginal(newValue);
            toggleButton.setToggleState(newValue, dontSendNotification);
            stopEdition();

            update();
        };

        nonZero = static_cast<t_toggle*>(gui.getPointer())->x_nonzero;

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

    void resized() override
    {
        toggleButton.setBounds(getLocalBounds());
    }

    void update() override
    {
        toggleButton.setToggleState((getValueOriginal() > std::numeric_limits<float>::epsilon()), dontSendNotification);
    }
};
