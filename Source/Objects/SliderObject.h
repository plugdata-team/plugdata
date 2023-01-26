/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SliderObject : public ObjectBase {
    bool isVertical;
    Value isLogarithmic = Value(var(false));

    Slider slider;

    IEMHelper iemHelper;

    Value min = Value(0.0f);
    Value max = Value(0.0f);
    Value steadyOnClick = Value(false);

    float value = 0.0f;

public:
    SliderObject(void* obj, Object* object)
        : ObjectBase(obj, object)
        , iemHelper(obj, object, this)
    {
        isVertical = static_cast<t_slider*>(obj)->x_orientation;
        addAndMakeVisible(slider);

        auto steady = getSteadyOnClick();
        steadyOnClick = steady;
        slider.setSliderSnapsToMousePosition(!steady);

        min = getMinimum();
        max = getMaximum();

        value = getValue();

        isLogarithmic = isLogScale();

        slider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

        if (isVertical)
            slider.setSliderStyle(Slider::LinearBarVertical);
        else
            slider.setSliderStyle(Slider::LinearBar);

        slider.setRange(0., 1., 0.000001);
        slider.setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        slider.setScrollWheelEnabled(false);
        slider.getProperties().set("Style", "SliderObject");

        slider.setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);

        slider.setValue(getValueScaled());

        slider.onDragStart = [this]() {
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
        };
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(label);
    }

    void initialiseParameters() override
    {
        iemHelper.initialiseParameters();
    }

    void updateBounds() override
    {
        iemHelper.updateBounds();
    }

    void applyBounds() override
    {
        iemHelper.applyBounds();
    }

    void updateRange()
    {
        min = getMinimum();
        max = getMaximum();

        if (static_cast<float>(min.getValue()) == 0.0f && static_cast<bool>(isLogarithmic.getValue())) {
            min = std::numeric_limits<float>::epsilon();
            setMinimum(std::numeric_limits<float>::epsilon());
        }
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        if (symbol == "float") {
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
            updateRange();

        } else if (symbol == "log") {

            setParameterExcludingListener(isLogarithmic, true);
            updateRange();

        } else if (symbol == "range" && atoms.size() >= 2) {
            setParameterExcludingListener(min, atoms[0].getFloat());
            setParameterExcludingListener(max, atoms[1].getFloat());
            updateRange();
        } else if (symbol == "steady" && atoms.size() >= 1) {
            bool steady = atoms[0].getFloat();
            setParameterExcludingListener(steadyOnClick, steady);
            slider.setSliderSnapsToMousePosition(!steady);
        } else {
            iemHelper.receiveObjectMessage(symbol, atoms);
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

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
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

        // TODO we would also want to have a high precision mode, use keypress to change sensitivity etc
        // Currently we set the sensitivity to 1:1 of current slider size
        int sliderLength;
        if (isVertical) {
            sliderLength = slider.getHeight();
        } else {
            sliderLength = slider.getWidth();
        }
        slider.setMouseDragSensitivity(sliderLength);
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = {
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },
            { "Logarithmic", tBool, cGeneral, &isLogarithmic, { "off", "on" } },
            { "Steady", tBool, cGeneral, &steadyOnClick, { "Jump on click", "Steady on click" } }
        };

        auto iemParameters = iemHelper.getParameters();
        allParameters.insert(allParameters.end(), iemParameters.begin(), iemParameters.end());

        return allParameters;
    }

    float getValue()
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

    bool getSteadyOnClick() const
    {
        return static_cast<t_slider*>(ptr)->x_steady;
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
        } else if (value.refersToSameSourceAs(isLogarithmic)) {
            setLogScale(isLogarithmic == var(true));
            updateRange();
        } else if (value.refersToSameSourceAs(steadyOnClick)) {
            slider.setSliderSnapsToMousePosition(!static_cast<bool>(steadyOnClick.getValue()));
        } else {
            iemHelper.valueChanged(value);
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
    }

    void setValueOriginal(float v)
    {
        auto minimum = static_cast<float>(min.getValue());
        auto maximum = static_cast<float>(max.getValue());

        if (minimum != maximum || minimum != 0 || maximum != 0) {
            value = (minimum < maximum) ? std::max(std::min(v, maximum), minimum) : std::max(std::min(v, minimum), maximum);
        } else {
            value = v;
        }
    }
};
