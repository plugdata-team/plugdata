/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ReversibleKnob : public Slider {
    
    bool isInverted;
    
public:
    ReversibleKnob() {
        setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        setScrollWheelEnabled(false);
        getProperties().set("Style", "SliderObject");
        setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
    }
    
    ~ReversibleKnob() {}

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


class KnobObject : public ObjectBase {
    
    struct t_fake_knb
    {
        t_iemgui x_gui;
        float    x_pos; /* 0-1 normalized position */
        int      x_arc_width;
        int      x_start_angle;
        int      x_end_angle;
        int      x_ticks;
        double   x_min;
        double   x_max;
        t_float  x_fval;
        int      x_acol;
        t_symbol *x_move_mode; /* "x","y", "xy" or "angle" */
        unsigned int      x_lin0_log1:1;
        unsigned int      x_wiper_visible:1;
        unsigned int      x_arc_visible:1;
        unsigned int      x_center_visible:1;
    };
    
    Value isLogarithmic = Value(var(false));

    ReversibleKnob knob;

    IEMHelper iemHelper;

    Value min = Value(0.0f);
    Value max = Value(0.0f);

    float value = 0.0f;

public:
    KnobObject(void* obj, Object* object)
        : ObjectBase(obj, object)
        , iemHelper(obj, object, this)
    {
        addAndMakeVisible(knob);

        knob.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

        knob.onDragStart = [this]() {
            startEdition();
            const float val = knob.getValue();
            setValue(val);
        };

        knob.onValueChange = [this]() {
            const float val = knob.getValue();
            setValue(val);
        };

        knob.onDragEnd = [this]() {
            stopEdition();
        };

        onConstrainerCreate = [this]() {
            constrainer->setFixedAspectRatio(1.0f);
            constrainer->setMinimumSize(this->object->minimumSize, this->object->minimumSize);
        };
    }

    void update() override
    {
        knob.setRangeFlipped((static_cast<t_fake_knb*>(ptr)->x_min) > (static_cast<t_fake_knb*>(ptr)->x_max));

        min = getMinimum();
        max = getMaximum();

        updateRange();

        auto currentValue = getValue();
        value = currentValue;
        knob.setValue(currentValue, dontSendNotification);

        isLogarithmic = isLogScale();

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
            if (knob.isRangeFlipped())
                knob.setNormalisableRange(makeLogarithmicRange<double>(getMaximum(), getMinimum()));
            else
                knob.setNormalisableRange(makeLogarithmicRange<double>(getMinimum(), getMaximum()));
        } else {
            if (knob.isRangeFlipped())
                knob.setRange(getMaximum(), getMinimum(), std::numeric_limits<float>::epsilon());
            else
                knob.setRange(getMinimum(), getMaximum(), std::numeric_limits<float>::epsilon());
        }
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("set"): {
            value = atoms[0].getFloat();
            knob.setValue(value, dontSendNotification);
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
                knob.setRangeFlipped(atoms[0].getFloat() > atoms[1].getFloat());
                setParameterExcludingListener(min, atoms[0].getFloat());
                setParameterExcludingListener(max, atoms[1].getFloat());
                updateRange();
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
        knob.setBounds(getLocalBounds());

    }

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = {
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },
            { "Logarithmic", tBool, cGeneral, &isLogarithmic, { "Off", "On" } },
        };

        auto iemParameters = iemHelper.getParameters();
        allParameters.insert(allParameters.end(), iemParameters.begin(), iemParameters.end());

        return allParameters;
    }

    float getValue()
    {
        return static_cast<t_fake_knb*>(ptr)->x_fval;
    }

    float getMinimum()
    {
        return static_cast<t_fake_knb*>(ptr)->x_min;
    }

    float getMaximum()
    {
        return static_cast<t_fake_knb*>(ptr)->x_max;
    }

    void setMinimum(float value)
    {
        static_cast<t_fake_knb*>(ptr)->x_min = value;
        knob.setRangeFlipped(static_cast<t_fake_knb*>(ptr)->x_min > static_cast<t_fake_knb*>(ptr)->x_max);
    }

    void setMaximum(float value)
    {
        static_cast<t_fake_knb*>(ptr)->x_max = value;
        knob.setRangeFlipped(static_cast<t_fake_knb*>(ptr)->x_min > static_cast<t_fake_knb*>(ptr)->x_max);
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
        }
        else {
            iemHelper.valueChanged(value);
        }
    }

    bool isLogScale() const
    {
        return static_cast<t_fake_knb*>(ptr)->x_lin0_log1;
    }

    void setLogScale(bool log)
    {
        static_cast<t_fake_knb*>(ptr)->x_lin0_log1 = log;
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
