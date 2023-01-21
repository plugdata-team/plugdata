/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

struct SliderObject : public IEMObject {
    bool isVertical;
    Value isLogarithmic = Value(var(false));

    Slider slider;

    SliderObject(void* obj, Object* parent)
        : IEMObject(obj, parent)
    {
        isVertical = static_cast<t_slider*>(obj)->x_orientation;
        addAndMakeVisible(slider);

        min = getMinimum();
        max = getMaximum();

        isLogarithmic = isLogScale();

        slider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

        if (isVertical)
            slider.setSliderStyle(Slider::LinearBarVertical);
        else
            slider.setSliderStyle(Slider::LinearBar);

        slider.setRange(0., 1., 0.001);
        slider.setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        slider.setScrollWheelEnabled(false);
        slider.setName("object:slider");

        slider.setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);

        slider.setValue(getValueScaled());

        slider.onDragStart = [this]() {
            draggingSlider = true;
            startEdition();
        };

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

        slider.onDragEnd = [this]() {
            stopEdition();
            draggingSlider = false;
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        if(symbol == "float") {
            value = atoms[0].getFloat();
            
            float maxValue = static_cast<float>(max.getValue());
            float minValue = static_cast<float>(min.getValue()) == 0.0f ? std::numeric_limits<float>::epsilon() : static_cast<float>(min.getValue());

            auto scaledValue = isLogScale() ? std::log(value / minValue) / std::log(maxValue / minValue) : getValueScaled();

            if (!std::isfinite(scaledValue))
                scaledValue = 0.0f;

            slider.setValue(scaledValue, dontSendNotification);
        }

        if (symbol == "lin") {
            setParameterExcludingListener(isLogarithmic, false);
        } else if (symbol == "log") {
            setParameterExcludingListener(isLogarithmic, true);
        } else if (symbol == "range" && atoms.size() >= 2) {
            setParameterExcludingListener(min, atoms[0].getFloat());
            setParameterExcludingListener(max, atoms[1].getFloat());
        } else {
            IEMObject::receiveObjectMessage(symbol, atoms);
        }
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
        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    void resized() override
    {
        slider.setBounds(getLocalBounds());
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
        return static_cast<t_slider*>(ptr)->x_fval;
    }

    float getMinimum()
    {
        return static_cast<t_slider*>(ptr)->x_min;
    }

    float getMaximum()
    {
        return static_cast<t_slider*>(ptr)->x_max;
    }

    void setMinimum(float value)
    {
        static_cast<t_slider*>(ptr)->x_min = value;
    }

    void setMaximum(float value)
    {
        static_cast<t_slider*>(ptr)->x_max = value;
    }

    bool jumpOnClick() const
    {
        return !static_cast<t_slider*>(ptr)->x_steady;
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

            if (static_cast<float>(min.getValue()) == 0.0f && static_cast<bool>(isLogarithmic.getValue())) {
                min = std::numeric_limits<float>::epsilon();
                setMinimum(std::numeric_limits<float>::epsilon());
            }
        } else {
            IEMObject::valueChanged(value);
        }
    }

    bool isLogScale() const
    {
        return static_cast<t_slider*>(ptr)->x_lin0_log1;
    }

    void setLogScale(bool log)
    {
        static_cast<t_slider*>(ptr)->x_lin0_log1 = log;
    }
    
    float getValueScaled() const
    {
        auto minimum = static_cast<float>(min.getValue());
        auto maximum = static_cast<float>(max.getValue());

        return (minimum < maximum) ? (value - minimum) / (maximum - minimum) : 1.f - (value - maximum) / (minimum - maximum);
    }

    void setValueScaled(float v)
    {
        auto minimum = static_cast<float>(min.getValue());
        auto maximum = static_cast<float>(max.getValue());

        value = (minimum < maximum) ? std::max(std::min(v, 1.f), 0.f) * (maximum - minimum) + minimum : (1.f - std::max(std::min(v, 1.f), 0.f)) * (minimum - maximum) + maximum;
        setValue(value);
    }
    
    void setValueOriginal(float v)
    {
        auto minimum = static_cast<float>(min.getValue());
        auto maximum = static_cast<float>(max.getValue());

        if (minimum != maximum || minimum != 0 || maximum != 0) {
            v = (minimum < maximum) ? std::max(std::min(v, maximum), minimum) : std::max(std::min(v, minimum), maximum);
        }

        setValue(value);
    }
};
