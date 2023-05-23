/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ReversibleSlider : public Slider {

    bool isInverted;
    bool isVertical;

public:
    ReversibleSlider()
    {
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        setScrollWheelEnabled(false);
        getProperties().set("Style", "SliderObject");
        setVelocityModeParameters(1.0f, 1, 0.0f, false);
    }

    ~ReversibleSlider() override { }

    void setRangeFlipped(bool invert)
    {
        isInverted = invert;
    }

    void setOrientation(bool vertical)
    {
        isVertical = vertical;

        if (isVertical)
            setSliderStyle(Slider::LinearBarVertical);
        else
            setSliderStyle(Slider::LinearBar);

        resized();
    }

    void resized() override
    {
        setMouseDragSensitivity(std::max<int>(1, isVertical ? getHeight() : getWidth()));
        Slider::resized();
    }

    void mouseDown(MouseEvent const& e) override
    {
        auto normalSensitivity = std::max<int>(1, isVertical ? getHeight() : getWidth());
        auto highSensitivity = normalSensitivity * 10;

        if (ModifierKeys::getCurrentModifiersRealtime().isShiftDown()) {
            setMouseDragSensitivity(highSensitivity);
        } else {
            setMouseDragSensitivity(normalSensitivity);
        }

        Slider::mouseDown(e);
    }

    void mouseUp(MouseEvent const& e) override
    {
        setMouseDragSensitivity(std::max<int>(1, isVertical ? getHeight() : getWidth()));
        Slider::mouseUp(e);
    }

    bool isRangeFlipped()
    {
        return isInverted;
    }

    double proportionOfLengthToValue(double proportion) override
    {
        if (isInverted)
            return Slider::proportionOfLengthToValue(1.0f - proportion);
        else
            return Slider::proportionOfLengthToValue(proportion);
    };
    double valueToProportionOfLength(double value) override
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

        objectParameters.addParamFloat("Minimum", cGeneral, &min, 0.0f);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 127.0f);
        objectParameters.addParamBool("Logarithmic", cGeneral, &isLogarithmic, { "Off", "On" }, 0);
        objectParameters.addParamBool("Steady", cGeneral, &steadyOnClick, { "Jump on click", "Steady on click" }, 1);
        iemHelper.addIemParameters(objectParameters);
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
        slider.setOrientation(isVertical);

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

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("float"),
            hash("set"),
            hash("lin"),
            hash("log"),
            hash("range"),
            hash("steady"),
            hash("orientation"),
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
        case hash("orientation"): {
            if (atoms.size() >= 1) {
                isVertical = static_cast<bool>(atoms[0].getFloat());
                slider.setOrientation(isVertical);
                updateAspectRatio();
                object->updateBounds();
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

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }

    void resized() override
    {
        slider.setBounds(getLocalBounds());
    }

    float getValue()
    {
        auto* x = static_cast<t_slider*>(ptr);

        t_float fval;
        int rounded_val = (x->x_gui.x_fsf.x_finemoved) ? x->x_val : (x->x_val / 100) * 100;

        /* if rcv==snd, don't round the value to prevent bad dragging when zoomed-in */
        if (x->x_gui.x_fsf.x_snd_able && (x->x_gui.x_snd == x->x_gui.x_rcv))
            rounded_val = x->x_val;

        if (x->x_lin0_log1)
            fval = x->x_min * exp(x->x_k * (double)(rounded_val)*0.01);
        else
            fval = (double)(rounded_val)*0.01 * x->x_k + x->x_min;
        if ((fval < 1.0e-10) && (fval > -1.0e-10))
            fval = 0.0;

        return std::isfinite(fval) ? fval : 0.0f;
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

    void setSteadyOnClick(bool steady) const
    {
        static_cast<t_slider*>(ptr)->x_steady = steady;
    }

    bool getSteadyOnClick() const
    {
        return static_cast<t_slider*>(ptr)->x_steady;
    }

    void updateAspectRatio()
    {
        float width = object->getWidth();
        float height = object->getHeight();
        if (isVertical)
            object->setSize(width, height);
        else
            object->setSize(height, width);
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(::getValue<float>(min));
            updateRange();
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(::getValue<float>(max));
            updateRange();
        } else if (value.refersToSameSourceAs(isLogarithmic)) {
            setLogScale(isLogarithmic == var(true));
        } else if (value.refersToSameSourceAs(steadyOnClick)) {
            bool steady = ::getValue<bool>(steadyOnClick);
            setSteadyOnClick(steady);
            slider.setSliderSnapsToMousePosition(!steady);
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
        pd->lockAudioThread();

        auto* sym = pd->generateSymbol(log ? "log" : "lin");
        pd_typedmess(static_cast<t_pd*>(ptr), sym, 0, nullptr);
        update();

        pd->unlockAudioThread();
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
