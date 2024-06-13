/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ReversibleSlider : public Slider
    , public NVGComponent {

    bool isInverted = false;
    bool isVertical = false;

public:
    ReversibleSlider()
        : NVGComponent(this)
    {
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setTextBoxStyle(Slider::NoTextBox, 0, 0, 0);
        setScrollWheelEnabled(false);
        getProperties().set("Style", "SliderObject");
        setVelocityModeParameters(1.0f, 1, 0.0f, false);
        setRepaintsOnMouseActivity(false);
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

    void mouseEnter(MouseEvent const& e) override
    {
        getParentComponent()->getProperties().set("hover", true);
        getParentComponent()->repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        getParentComponent()->getProperties().set("hover", false);
        getParentComponent()->repaint();
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

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
    }

    double valueToProportionOfLength(double value) override
    {
        if (isInverted)
            return 1.0f - (Slider::valueToProportionOfLength(value));
        else
            return Slider::valueToProportionOfLength(value);
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat().reduced(1.0f);

        constexpr auto thumbSize = 4.0f;
        auto cornerSize = Corners::objectCornerRadius / 2.0f;

        if (isHorizontal()) {
            auto sliderPos = jmap<float>(valueToProportionOfLength(getValue()), 0.0f, 1.0f, b.getX(), b.getWidth() - thumbSize);
            auto bounds = Rectangle<float>(sliderPos, b.getY(), thumbSize, b.getHeight());

            nvgFillColor(nvg, convertColour(getLookAndFeel().findColour(Slider::trackColourId)));
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize);
            nvgFill(nvg);
        } else {

            auto sliderPos = jmap<float>(valueToProportionOfLength(getValue()), 1.0f, 0.0f, b.getY(), b.getHeight() - thumbSize);
            auto bounds = Rectangle<float>(b.getWidth(), thumbSize).translated(b.getX(), sliderPos);

            nvgFillColor(nvg, convertColour(getLookAndFeel().findColour(Slider::trackColourId)));
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize);
            nvgFill(nvg);
        }
    }
};

class SliderObject : public ObjectBase {
    bool isVertical;
    Value isLogarithmic = SynchronousValue(var(false));

    ReversibleSlider slider;

    IEMHelper iemHelper;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);
    Value steadyOnClick = SynchronousValue(false);
    Value sizeProperty = SynchronousValue();

    float value = 0.0f;

public:
    SliderObject(pd::WeakReference obj, Object* object)
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
            auto minLongSide = 8;
            auto minShortSide = 8;
            if (isVertical) {
                constrainer->setMinimumSize(minShortSide, minLongSide);
            } else {
                constrainer->setMinimumSize(minLongSide, minShortSide);
            }
        };

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamFloat("Minimum", cGeneral, &min, 0.0f);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 127.0f);
        objectParameters.addParamBool("Logarithmic", cGeneral, &isLogarithmic, { "Off", "On" }, 0);
        objectParameters.addParamBool("Steady", cGeneral, &steadyOnClick, { "Jump on click", "Steady on click" }, 1);
        iemHelper.addIemParameters(objectParameters);
    }

    void update() override
    {
        auto steady = getSteadyOnClick();
        steadyOnClick = steady;
        slider.setSliderSnapsToMousePosition(!steady);

        if (auto obj = ptr.get<t_slider>()) {
            isVertical = obj->x_orientation;
            slider.setRangeFlipped(obj->x_min > obj->x_max);
            sizeProperty = Array<var> { var(obj->x_gui.x_w), var(obj->x_gui.x_h) };
        }

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
        iemHelper.updateLabel(labels, isVertical ? Point<int>(0, 2) : Point<int>(2, 0));
    }

    Rectangle<int> getPdBounds() override
    {
        if (isVertical) {
            return iemHelper.getPdBounds().expanded(0, 2).withTrimmedBottom(-1);
        } else {
            return iemHelper.getPdBounds().expanded(2, 0).withTrimmedLeft(-1);
        }
    }

    void setPdBounds(Rectangle<int> b) override
    {
        // Hsl/vsl lies to us in slider_getrect: the x/y coordintates it returns are 2 or 3 px offset from what text_xpix/text_ypix reports
        if (isVertical) {
            iemHelper.setPdBounds(b.reduced(0, 2).withTrimmedBottom(1).translated(0, -2));
        } else {
            iemHelper.setPdBounds(b.reduced(2, 0).withTrimmedLeft(1).translated(-3, 0));
        }
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

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
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
            if (numAtoms >= 2) {
                slider.setRangeFlipped(atoms[0].getFloat() > atoms[1].getFloat());
                setParameterExcludingListener(min, atoms[0].getFloat());
                setParameterExcludingListener(max, atoms[1].getFloat());
                updateRange();
            }
            break;
        }
        case hash("steady"): {
            if (numAtoms >= 1) {
                bool steady = atoms[0].getFloat();
                setParameterExcludingListener(steadyOnClick, steady);
                slider.setSliderSnapsToMousePosition(!steady);
            }
            break;
        }
        case hash("orientation"): {
            if (numAtoms >= 1) {
                isVertical = static_cast<bool>(atoms[0].getFloat());
                slider.setOrientation(isVertical);
                updateAspectRatio();
                object->updateBounds();
            }
            break;
        }
        case hash("color"): {
            iemHelper.receiveObjectMessage(symbol, atoms, numAtoms);
            getLookAndFeel().setColour(Slider::backgroundColourId, Colour::fromString(iemHelper.secondaryColour.toString()));
            getLookAndFeel().setColour(Slider::trackColourId, Colour::fromString(iemHelper.primaryColour.toString()));
            object->repaint();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms, numAtoms);
            break;
        }
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat().reduced(0.5f);
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = LookAndFeel::getDefaultLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        auto bgColour = getLookAndFeel().findColour(Slider::backgroundColourId);

        if (getProperties()["hover"])
            bgColour = brightenOrDarken(bgColour);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(bgColour), convertColour(outlineColour), Corners::objectCornerRadius);

        slider.render(nvg);
    }

    void resized() override
    {
        slider.setBounds(getLocalBounds());
    }

    void updateSizeProperty() override
    {
        if (auto iem = ptr.get<t_iemgui>()) {
            if (isVertical) {
                iem->x_w = object->getObjectBounds().getWidth() - 1;
                iem->x_h = object->getObjectBounds().getHeight() - 6;
            } else {
                iem->x_w = object->getObjectBounds().getWidth() - 6;
                iem->x_h = object->getObjectBounds().getHeight() - 1;
            }

            setParameterExcludingListener(sizeProperty, Array<var> { var(iem->x_w), var(iem->x_h) });
        }
    }

    float getValue()
    {
        if (auto slider = ptr.get<t_slider>()) {
            t_float fval;

            if (slider->x_lin0_log1)
                fval = slider->x_min * exp(slider->x_k * (double)(slider->x_val) * 0.01);
            else
                fval = (double)(slider->x_val) * 0.01 * slider->x_k + slider->x_min;
            if ((fval < 1.0e-10) && (fval > -1.0e-10))
                fval = 0.0;

            return std::isfinite(fval) ? fval : 0.0f;
        }

        return 0.0f;
    }

    float getMinimum()
    {
        if (auto slider = ptr.get<t_slider>()) {
            return slider->x_min;
        }

        return 0.0f;
    }

    float getMaximum()
    {
        if (auto slider = ptr.get<t_slider>()) {
            return slider->x_max;
        }

        return 127.0f;
    }

    void setMinimum(float value)
    {
        float min = 0.0f, max = 127.0f;
        if (auto slider = ptr.get<t_slider>()) {
            slider->x_min = value;
            min = slider->x_min;
            max = slider->x_max;
        }

        slider.setRangeFlipped(min > max);
    }

    void setMaximum(float value)
    {
        float min = 0.0f, max = 127.0f;
        if (auto slider = ptr.get<t_slider>()) {
            slider->x_max = value;
            min = slider->x_min;
            max = slider->x_max;
        }

        slider.setRangeFlipped(min > max);
    }

    void setSteadyOnClick(bool steady) const
    {
        if (auto slider = ptr.get<t_slider>()) {
            slider->x_steady = steady;
        }
    }

    bool getSteadyOnClick() const
    {
        if (auto slider = ptr.get<t_slider>()) {
            return slider->x_steady;
        }

        return false;
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
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto obj = ptr.get<t_slider>()) {
                obj->x_gui.x_w = width;
                obj->x_gui.x_h = height;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
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
        if (auto slider = ptr.get<t_slider>()) {
            return slider->x_lin0_log1;
        }

        return false;
    }

    void setLogScale(bool log)
    {
        auto* sym = pd->generateSymbol(log ? "log" : "lin");
        if (auto obj = ptr.get<t_pd>()) {
            pd_typedmess(obj.get(), sym, 0, nullptr);
        }

        update();
    }

    void setValue(float v)
    {
        sendFloatValue(v);
    }

    template<typename FloatType>
    static inline NormalisableRange<FloatType> makeLogarithmicRange(FloatType min, FloatType max)
    {
        min = std::max<FloatType>(min, max / 100000.0f);

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
