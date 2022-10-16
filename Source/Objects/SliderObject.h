

struct SliderObject : public IEMObject {
    bool isVertical;
    Value isLogarithmic = Value(var(false));

    Slider slider;

    SliderObject(bool vertical, void* obj, Object* parent)
        : IEMObject(obj, parent)
    {
        isVertical = vertical;
        addAndMakeVisible(slider);

        min = getMinimum();
        max = getMaximum();

        isLogarithmic = isLogScale();

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

        slider.onValueChange = [this]() {
            const float val = slider.getValue();
            if (isLogScale()) {
                float minValue = static_cast<float>(min.getValue());
                float maxValue = static_cast<float>(max.getValue());
                float minimum = minValue == 0.0f ? std::numeric_limits<float>::epsilon() : minValue;
                setValueOriginal(exp(val * log(maxValue / minimum)) * minimum);
            } else {
                setValueScaled(val);
            }
        };

        slider.onDragEnd = [this]() { stopEdition(); };
    }

    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(isVertical ? 23 : 50, maxSize, object->getWidth());
        int h = jlimit(isVertical ? 77 : 25, maxSize, object->getHeight());

        if (w != object->getWidth() || h != object->getHeight()) {
            object->setSize(w, h);
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        auto outlineColour = object->findColour(cnv->isSelected(object) && !cnv->isGraph ? PlugDataColour::canvasActiveColourId : PlugDataColour::outlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f, 1.0f);
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
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },
            { "Logarithmic", tBool, cGeneral, &isLogarithmic, { "off", "on" } },
        };
    }

    float getValue() override
    {
        return isVertical ? (static_cast<t_vslider*>(ptr))->x_fval : (static_cast<t_hslider*>(ptr))->x_fval;
    }

    float getMinimum()
    {
        return isVertical ? (static_cast<t_vslider*>(ptr))->x_min : (static_cast<t_hslider*>(ptr))->x_min;
    }

    float getMaximum()
    {
        return isVertical ? (static_cast<t_vslider*>(ptr))->x_max : (static_cast<t_hslider*>(ptr))->x_max;
    }

    void setMinimum(float value)
    {
        if (isVertical) {
            static_cast<t_vslider*>(ptr)->x_min = value;
        } else {
            static_cast<t_hslider*>(ptr)->x_min = value;
        }
    }

    void setMaximum(float value)
    {
        if (isVertical) {
            static_cast<t_vslider*>(ptr)->x_max = value;
        } else {
            static_cast<t_hslider*>(ptr)->x_max = value;
        }
    }

    bool jumpOnClick() const
    {
        return isVertical ? (static_cast<t_vslider*>(ptr))->x_steady == 0 : (static_cast<t_hslider*>(ptr))->x_steady == 0;
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
        } else if (value.refersToSameSourceAs(isLogarithmic)) {
            setLogScale(isLogarithmic == var(true));
            min = getMinimum();
            max = getMaximum();
        } else {
            IEMObject::valueChanged(value);
        }
    }

    bool isLogScale() const
    {
        return isVertical ? (static_cast<t_hslider*>(ptr))->x_lin0_log1 != 0 : (static_cast<t_vslider*>(ptr))->x_lin0_log1 != 0;
    }

    void setLogScale(bool log)
    {
        if (isVertical) {
            static_cast<t_vslider*>(ptr)->x_lin0_log1 = log;
        } else {
            static_cast<t_hslider*>(ptr)->x_lin0_log1 = log;
        }
    }
};
