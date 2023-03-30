/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ReversibleSlider : public Slider {
    
    bool isInverted;
    
public:
    ReversibleSlider() {
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        setScrollWheelEnabled(false);
        getProperties().set("Style", "SliderObject");
        setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
    }
    
    ~ReversibleSlider() {}

    void setRangeFlipped(bool invert)
    {
        isInverted = invert;
    }

    bool isRangeFlipped()
    {
        return isInverted;
    }

    double proportionOfLengthToValue(double proportion)
    {
        if (isInverted)
            return Slider::proportionOfLengthToValue(1.0f - proportion);
        else
            return Slider::proportionOfLengthToValue(proportion);
    };
    double valueToProportionOfLength(double value)
    {
        if (isInverted)
            return 1.0f - (Slider::valueToProportionOfLength(value));
        else
            return Slider::valueToProportionOfLength(value);
    };
};


class SliderObject : public ObjectBase {
    bool isVertical;
    Value isLogarithmic = Value(var(false));

    ReversibleSlider slider;

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
        addAndMakeVisible(slider);

        slider.onDragStart = [this]() {
            startEdition();
            const float val = slider.getValue();
            setValue(val);
        };

        slider.onValueChange = [this]() {
            const float val = slider.getValue();
            setValue(val);
        };

        slider.onDragEnd = [this]() {
            stopEdition();
        };

        onConstrainerCreate = [this]() {
            auto minLongSide = this->object->minimumSize * 2;
            auto minShortSide = this->object->minimumSize;
            if (isVertical) {
                constrainer->setMinimumSize(minShortSide, minLongSide);
            } else {
                constrainer->setMinimumSize(minLongSide, minShortSide);
            }
        };
    }

    void update() override
    {
        isVertical = static_cast<t_slider*>(ptr)->x_orientation;

        auto steady = getSteadyOnClick();
        steadyOnClick = steady;
        slider.setSliderSnapsToMousePosition(!steady);

        slider.setRangeFlipped((static_cast<t_slider*>(ptr)->x_min) > (static_cast<t_slider*>(ptr)->x_max));

        min = getMinimum();
        max = getMaximum();

        updateRange();

        auto currentValue = getValue();
        value = currentValue;
        slider.setValue(currentValue, dontSendNotification);

        isLogarithmic = isLogScale();

        if (isVertical)
            slider.setSliderStyle(Slider::LinearBarVertical);
        else
            slider.setSliderStyle(Slider::LinearBar);

        iemHelper.update();

        getLookAndFeel().setColour(Slider::backgroundColourId, Colour::fromString(iemHelper.secondaryColour.toString()));
        getLookAndFeel().setColour(Slider::trackColourId, Colour::fromString(iemHelper.primaryColour.toString()));
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
        return iemHelper.getPdBounds().expanded(2, 0).withTrimmedLeft(-1);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        iemHelper.setPdBounds(b.reduced(2, 0).withTrimmedLeft(1));
    }

    void updateRange()
    {
        if (isLogScale()) {
            if (slider.isRangeFlipped())
                slider.setNormalisableRange(makeLogarithmicRange<double>(getMaximum(), getMinimum()));
            else
                slider.setNormalisableRange(makeLogarithmicRange<double>(getMinimum(), getMaximum()));
        } else {
            if (slider.isRangeFlipped())
                slider.setRange(getMaximum(), getMinimum(), std::numeric_limits<float>::epsilon());
            else
                slider.setRange(getMinimum(), getMaximum(), std::numeric_limits<float>::epsilon());
        }
    }

    std::vector<hash32> getAllMessages() override {
        return {
            hash("float"),
            hash("set"),
            hash("lin"),
            hash("log"),
            hash("range"),
            hash("steady"),
            IEMGUI_MESSAGES
        };
    }
    
    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("set"): {
            value = atoms[0].getFloat();
            slider.setValue(value, dontSendNotification);
            break;
        }
        case hash("lin"): {
            setParameterExcludingListener(isLogarithmic, false);
            updateRange();
            break;
        }
        case hash("log"): {
            setParameterExcludingListener(isLogarithmic, true);
            updateRange();
            break;
        }
        case hash("range"): {
            if (atoms.size() >= 2) {
                slider.setRangeFlipped(atoms[0].getFloat() > atoms[1].getFloat());
                setParameterExcludingListener(min, atoms[0].getFloat());
                setParameterExcludingListener(max, atoms[1].getFloat());
                updateRange();
            }
            break;
        }
        case hash("steady"): {
            if (atoms.size() >= 1) {
                bool steady = atoms[0].getFloat();
                setParameterExcludingListener(steadyOnClick, steady);
                slider.setSliderSnapsToMousePosition(!steady);
            }
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }

        // Update the colours of the actual slider
        if (hash(symbol) == hash("color")) {
            getLookAndFeel().setColour(Slider::backgroundColourId, Colour::fromString(iemHelper.secondaryColour.toString()));
            getLookAndFeel().setColour(Slider::trackColourId, Colour::fromString(iemHelper.primaryColour.toString()));
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void resized() override
    {
        slider.setBounds(getLocalBounds());

        // TODO: we would also want to have a high precision mode, use keypress to change sensitivity etc
        // Currently we set the sensitivity to 1:1 of current slider size
        slider.setMouseDragSensitivity(isVertical ? slider.getHeight() : slider.getWidth());
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = {
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },
            { "Logarithmic", tBool, cGeneral, &isLogarithmic, { "Off", "On" } },
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
        slider.setRangeFlipped(static_cast<t_slider*>(ptr)->x_min > static_cast<t_slider*>(ptr)->x_max);
    }

    void setMaximum(float value)
    {
        static_cast<t_slider*>(ptr)->x_max = value;
        slider.setRangeFlipped(static_cast<t_slider*>(ptr)->x_min > static_cast<t_slider*>(ptr)->x_max);
    }

    bool getSteadyOnClick() const
    {
        return static_cast<t_slider*>(ptr)->x_steady;
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
            updateRange();
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
            updateRange();
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

    void setValue(float v)
    {
        sendFloatValue(v);
    }

    template<typename FloatType>
    static inline NormalisableRange<FloatType> makeLogarithmicRange(FloatType min, FloatType max)
    {
        min = std::max<float>(min, max / 100.0f);

        return NormalisableRange<FloatType>(
            min, max,
            [](FloatType min, FloatType max, FloatType linVal) {
                return std::pow(10.0f, (std::log10(max / min) * linVal + std::log10(min)));
            },
            [](FloatType min, FloatType max, FloatType logVal) {
                return (std::log10(logVal / min) / std::log10(max / min));
            });
    }
};
