/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class ReversibleSlider final : public Slider
    , public NVGComponent {

    bool isInverted : 1 = false;
    bool isVertical : 1 = false;
    bool shiftIsDown : 1 = false;
    bool isZeroRange : 1 = false;
    float zeroRangeValue = 0.0f;
    NVGcolor trackColour;

public:
    ReversibleSlider()
        : NVGComponent(this)
    {
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
        setScrollWheelEnabled(false);
        setVelocityModeParameters(1.0f, 1, 0.0f, false);
        setRepaintsOnMouseActivity(false);
    }

    ~ReversibleSlider() override { }

    float getCurrentValue() const
    {
        if (isZeroRange)
            return zeroRangeValue;

        return getValue();
    }

    void setCurrentValue(float const value)
    {
        if (isZeroRange) {
            return;
        }
        setValue(value, dontSendNotification);
    }

    void updateRange(float const min, float const max)
    {
        if (approximatelyEqual(min, max)) {
            setRange(0.0, 1.0, std::numeric_limits<float>::epsilon());
            isZeroRange = true;
            zeroRangeValue = min;
            setValue(0.0f);
            return;
        }

        isZeroRange = false;

        setRange(min, max, std::numeric_limits<float>::epsilon());
        repaint();
    }

    void setRangeFlipped(bool const invert)
    {
        isInverted = invert;
    }

    void setOrientation(bool const vertical)
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
        if (!e.mods.isLeftButtonDown())
            return;

        auto const normalSensitivity = std::max<int>(1, isVertical ? getHeight() : getWidth());
        auto const highSensitivity = normalSensitivity * 10;
        if (ModifierKeys::getCurrentModifiersRealtime().isShiftDown()) {
            setMouseDragSensitivity(highSensitivity);
            shiftIsDown = true;
        } else {
            setMouseDragSensitivity(normalSensitivity);
        }

        Slider::mouseDown(e);

        auto const snaps = getSliderSnapsToMousePosition();
        if (snaps && shiftIsDown) {
            setSliderSnapsToMousePosition(false); // hack to make jump-on-click work the same with high-accuracy mode as in Pd
            Slider::mouseDown(e);
            setSliderSnapsToMousePosition(true);
        }
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        auto const snaps = getSliderSnapsToMousePosition();
        if (snaps && shiftIsDown)
            setSliderSnapsToMousePosition(false); // We disable this temporarily, otherwise it breaks high accuracy mode
        Slider::mouseDrag(e);
        if (snaps && shiftIsDown)
            setSliderSnapsToMousePosition(true);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown())
            return;

        setMouseDragSensitivity(std::max<int>(1, isVertical ? getHeight() : getWidth()));
        Slider::mouseUp(e);
        shiftIsDown = false;
    }

    bool isRangeFlipped() const
    {
        return isInverted;
    }

    double proportionOfLengthToValue(double const proportion) override
    {
        if (isInverted)
            return Slider::proportionOfLengthToValue(1.0f - proportion);
        return Slider::proportionOfLengthToValue(proportion);
    }

    double valueToProportionOfLength(double const value) override
    {
        if (isInverted)
            return 1.0f - Slider::valueToProportionOfLength(value);
        return Slider::valueToProportionOfLength(value);
    }

    void setTrackColour(Colour const& c)
    {
        trackColour = nvgColour(c);
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat().reduced(1.0f);

        constexpr auto thumbSize = 4.0f;
        auto const cornerSize = Corners::objectCornerRadius / 2.0f;

        Rectangle<float> bounds;
        if (isHorizontal()) {
            auto const sliderPos = jmap<float>(valueToProportionOfLength(getValue()), 0.0f, 1.0f, b.getX(), b.getWidth() - thumbSize);
            bounds = Rectangle<float>(sliderPos, b.getY(), thumbSize, b.getHeight());
        } else {
            auto const sliderPos = jmap<float>(valueToProportionOfLength(getValue()), 1.0f, 0.0f, b.getY(), b.getHeight() - thumbSize);
            bounds = Rectangle<float>(b.getWidth(), thumbSize).translated(b.getX(), sliderPos);
        }
        nvgFillColor(nvg, trackColour);
        nvgFillRoundedRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), cornerSize);
    }

    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override { return nullptr; };
};

class SliderObject final : public ObjectBase {
    bool isVertical;
    Value isLogarithmic = SynchronousValue(var(false));

    ReversibleSlider slider;

    IEMHelper iemHelper;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);
    Value steadyOnClick = SynchronousValue(false);
    Value sizeProperty = SynchronousValue();

    NVGcolor backgroundColour;

    float value = 0.0f;

public:
    SliderObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
        , iemHelper(obj, object, this)
    {
        addAndMakeVisible(slider);

        slider.onDragStart = [this] {
            startEdition();
            float const val = slider.getCurrentValue();
            sendFloatValue(val);
        };

        slider.onValueChange = [this] {
            float const val = slider.getCurrentValue();
            sendFloatValue(val);
        };

        slider.onDragEnd = [this] {
            stopEdition();
        };

        objectParameters.addParamSize(&sizeProperty);
        objectParameters.addParamFloat("Minimum", cGeneral, &min, 0.0f);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 127.0f);
        objectParameters.addParamBool("Logarithmic", cGeneral, &isLogarithmic, { "Off", "On" }, 0);
        objectParameters.addParamBool("Steady", cGeneral, &steadyOnClick, { "Jump on click", "Steady on click" }, 1);
        iemHelper.addIemParameters(objectParameters);
    }

    void onConstrainerCreate() override
    {
        constexpr auto minLongSide = 8;
        constexpr auto minShortSide = 8;
        if (isVertical) {
            constrainer->setMinimumSize(minShortSide, minLongSide);
        } else {
            constrainer->setMinimumSize(minLongSide, minShortSide);
        }
    }

    void update() override
    {
        auto const steady = getSteadyOnClick();
        steadyOnClick = steady;
        slider.setSliderSnapsToMousePosition(!steady);

        if (auto obj = ptr.get<t_slider>()) {
            isVertical = obj->x_orientation;
            auto const min = obj->x_min;
            auto const max = obj->x_max;
            slider.setRangeFlipped(!approximatelyEqual(min, max) && min > max);
            sizeProperty = VarArray { var(obj->x_gui.x_w), var(obj->x_gui.x_h) };
        }

        min = getMinimum();
        max = getMaximum();

        updateRange();

        auto const currentValue = getValue();
        value = currentValue;
        slider.setCurrentValue(currentValue);
        slider.setOrientation(isVertical);

        isLogarithmic = isLogScale();

        iemHelper.update();

        backgroundColour = nvgColour(Colour::fromString(iemHelper.secondaryColour.toString()));
        slider.setTrackColour(Colour::fromString(iemHelper.primaryColour.toString()));
        repaint();
    }

    bool hideInlet() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlet() override
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
        }
        return iemHelper.getPdBounds().expanded(2, 0).withTrimmedLeft(-1);
    }

    void setPdBounds(Rectangle<int> const b) override
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
        auto const max = getMaximum();
        auto const min = getMinimum();
        if (isLogScale()) {
            if (slider.isRangeFlipped())
                slider.setNormalisableRange(makeLogarithmicRange<double>(max, min));
            else
                slider.setNormalisableRange(makeLogarithmicRange<double>(min, max));
        } else {
            if (slider.isRangeFlipped())
                slider.updateRange(max, min);
            else
                slider.updateRange(min, max);
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            value = atoms[0].getFloat();
            slider.setCurrentValue(value);
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
                bool const steady = atoms[0].getFloat();
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
        case hash("color"): {
            iemHelper.receiveObjectMessage(symbol, atoms);
            backgroundColour = nvgColour(Colour::fromString(iemHelper.secondaryColour.toString()));
            slider.setTrackColour(Colour::fromString(iemHelper.primaryColour.toString()));
            object->repaint();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        bool const selected = object->isSelected() && !cnv->isGraph;
        auto const outlineColour = nvgColour(selected ? PlugDataColours::objectSelectedOutlineColour : PlugDataColours::objectOutlineColour);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, outlineColour, Corners::objectCornerRadius);

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

            setParameterExcludingListener(sizeProperty, VarArray { var(iem->x_w), var(iem->x_h) });
        }
    }

    float getValue() const
    {
        if (auto slider = ptr.get<t_slider>()) {
            t_float fval;

            if (slider->x_lin0_log1)
                fval = slider->x_min * exp(slider->x_k * static_cast<double>(slider->x_val) * 0.01);
            else
                fval = static_cast<double>(slider->x_val) * 0.01 * slider->x_k + slider->x_min;
            if (fval < 1.0e-10 && fval > -1.0e-10)
                fval = 0.0;

            return std::isfinite(fval) ? fval : 0.0f;
        }

        return 0.0f;
    }

    float getMinimum() const
    {
        if (auto slider = ptr.get<t_slider>()) {
            return slider->x_min;
        }

        return 0.0f;
    }

    float getMaximum() const
    {
        if (auto slider = ptr.get<t_slider>()) {
            return slider->x_max;
        }

        return 127.0f;
    }

    void setMinimum(float const minValue)
    {
        float max = 127.0f;
        if (auto slider = ptr.get<t_slider>()) {
            slider->x_min = minValue;
            max = slider->x_max;
        }
        slider.setRangeFlipped(!approximatelyEqual(minValue, max) && minValue > max);
    }

    void setMaximum(float const maxValue)
    {
        float min = 0.0f;
        if (auto slider = ptr.get<t_slider>()) {
            slider->x_max = maxValue;
            min = slider->x_min;
        }
        slider.setRangeFlipped(approximatelyEqual(min, maxValue) ? false : min > maxValue);
    }

    void setSteadyOnClick(bool const steady) const
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
        float const width = object->getWidth();
        float const height = object->getHeight();
        if (isVertical)
            object->setSize(width, height);
        else
            object->setSize(height, width);
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const& arr = *sizeProperty.getValue().getArray();
            auto const* constrainer = getConstrainer();
            auto const width = std::max(static_cast<int>(arr[0]), constrainer->getMinimumWidth());
            auto const height = std::max(static_cast<int>(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, VarArray { var(width), var(height) });

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
            bool const steady = ::getValue<bool>(steadyOnClick);
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

    void setLogScale(bool const log)
    {
        auto* sym = pd->generateSymbol(log ? "log" : "lin");
        if (auto obj = ptr.get<t_pd>()) {
            pd_typedmess(obj.get(), sym, 0, nullptr);
        }

        update();
    }

    template<typename FloatType>
    static NormalisableRange<FloatType> makeLogarithmicRange(FloatType min, FloatType max)
    {
        min = std::max<FloatType>(min, max / 100000.0f);

        return NormalisableRange<FloatType>(
            min, max,
            [](FloatType min, FloatType max, FloatType linVal) {
                return std::pow(10.0f, std::log10(max / min) * linVal + std::log10(min));
            },
            [](FloatType min, FloatType max, FloatType logVal) {
                return std::log10(logVal / min) / std::log10(max / min);
            });
    }
};
