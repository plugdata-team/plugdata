

struct SliderComponent : public GUIComponent
{
    bool isVertical;
    Value isLogarithmic = Value(var(false));

    Slider slider;

    SliderComponent(bool vertical, const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        isVertical = vertical;
        addAndMakeVisible(slider);

        isLogarithmic = gui.isLogScale();
        
        slider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        
        
        if (vertical)
            slider.setSliderStyle(Slider::LinearBarVertical);
        else
            slider.setSliderStyle(Slider::LinearBar);
        

        slider.setRange(0., 1., 0.001);
        slider.setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        slider.setScrollWheelEnabled(false);

        slider.setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);

        slider.setValue(getValueScaled());

        slider.onDragStart = [this]() { startEdition(); };

        slider.onValueChange = [this]()
        {
            const float val = slider.getValue();
            if (gui.isLogScale())
            {
                float minValue = static_cast<float>(min.getValue());
                float maxValue = static_cast<float>(max.getValue());
                float minimum = minValue == 0.0f ? std::numeric_limits<float>::epsilon() : minValue;
                setValueOriginal(exp(val * log(maxValue / minimum)) * minimum);
            }
            else
            {
                setValueScaled(val);
            }
        };

        slider.onDragEnd = [this]() { stopEdition(); };

        initialise(newObject);
    }

    void checkBoxBounds() override
    {
        // Apply size limits
        int w = jlimit(isVertical ? 23 : 50, maxSize, box->getWidth());
        int h = jlimit(isVertical ? 77 : 25, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    void resized() override
    {
        slider.setBounds(getLocalBounds());
    }

    void update() override
    {
        slider.setValue(getValueScaled(), dontSendNotification);
    }

    ObjectParameters defineParameters() override
    {
        return {
            {"Minimum", tFloat, cGeneral, &min, {}},
            {"Maximum", tFloat, cGeneral, &max, {}},
            {"Logarithmic", tBool, cGeneral, &isLogarithmic, {"off", "on"}},
        };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min))
        {
            gui.setMinimum(static_cast<float>(min.getValue()));
        }
        else if (value.refersToSameSourceAs(max))
        {
            gui.setMaximum(static_cast<float>(max.getValue()));
        }
        else if (value.refersToSameSourceAs(isLogarithmic))
        {
            gui.setLogScale(isLogarithmic == var(true));
            min = gui.getMinimum();
            max = gui.getMaximum();
        }
        else
        {
            GUIComponent::valueChanged(value);
        }
    }
};
