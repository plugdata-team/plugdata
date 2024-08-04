/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "TclColours.h"

extern "C" {
void knob_get_snd(void* x);
void knob_get_rcv(void* x);
}

class Knob : public Slider
    , public NVGComponent {

    Colour fgColour;
    Colour arcColour;

    bool drawArc = true;
    int numberOfTicks = 0;
    float arcStart = 63.5f;

public:
    Knob()
        : Slider(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox)
        , NVGComponent(this)
    {
        setScrollWheelEnabled(false);
        setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
    }

    ~Knob() = default;

    void drawTicks(NVGcontext* nvg, Rectangle<float> knobBounds, float startAngle, float endAngle, float tickWidth)
    {
        auto centre = knobBounds.getCentre();
        auto radius = (knobBounds.getWidth() * 0.5f) * 1.05f;

        // Calculate the angle between each tick
        float angleIncrement = (endAngle - startAngle) / static_cast<float>(jmax(numberOfTicks - 1, 1));

        // Position each tick around the larger circle
        float tickRadius = tickWidth * 0.5f;
        for (int i = 0; i < numberOfTicks; ++i) {
            float angle = startAngle + i * angleIncrement;
            float x = centre.getX() + radius * std::cos(angle);
            float y = centre.getY() + radius * std::sin(angle);

            // Draw the tick at this position
            nvgBeginPath(nvg);
            nvgCircle(nvg, x, y, tickRadius);
            nvgFillColor(nvg, convertColour(fgColour));
            nvgFill(nvg);
        }
    }

    void showArc(bool show)
    {
        drawArc = show;
        repaint();
    }

    void setArcStart(float newArcStart)
    {
        arcStart = newArcStart;
    }

    void render(NVGcontext* nvg) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(getWidth() * 0.14f);

        auto const lineThickness = std::max(bounds.getWidth() * 0.09f, 1.5f);

        auto sliderPosProportional = getValue();

        auto startAngle = getRotaryParameters().startAngleRadians - (MathConstants<float>::pi * 0.5f);
        auto endAngle = getRotaryParameters().endAngleRadians - (MathConstants<float>::pi * 0.5f);

        auto angle = jmap<float>(sliderPosProportional, startAngle, endAngle);
        auto centre = jmap<double>(arcStart, startAngle, endAngle);

        startAngle = std::clamp(startAngle, endAngle - MathConstants<float>::twoPi, endAngle + MathConstants<float>::twoPi);

        if (drawArc) {
            auto arcBounds = bounds.reduced(lineThickness);
            auto arcRadius = arcBounds.getWidth() * 0.5;
            auto arcWidth = (arcRadius - lineThickness) / arcRadius;

            nvgBeginPath(nvg);
            nvgArc(nvg, bounds.getCentreX(), bounds.getCentreY(), arcRadius, startAngle, endAngle, NVG_HOLE);
            nvgStrokeWidth(nvg, arcWidth * lineThickness);
            nvgStrokeColor(nvg, nvgRGBAf(arcColour.getFloatRed(), arcColour.getFloatGreen(), arcColour.getFloatBlue(), arcColour.getFloatAlpha()));
            nvgStroke(nvg);

            nvgBeginPath(nvg);
            if (centre < angle) {
                nvgArc(nvg, bounds.getCentreX(), bounds.getCentreY(), arcRadius, centre, angle, NVG_HOLE);
            } else {
                nvgArc(nvg, bounds.getCentreX(), bounds.getCentreY(), arcRadius, angle, centre, NVG_HOLE);
            }
            nvgStrokeColor(nvg, nvgRGBAf(fgColour.getFloatRed(), fgColour.getFloatGreen(), fgColour.getFloatBlue(), fgColour.getFloatAlpha()));
            nvgStrokeWidth(nvg, arcWidth * lineThickness);
            nvgStroke(nvg);
        }

        float wiperX = bounds.getCentreX() + (bounds.getWidth() * 0.4f) * std::cos(angle);
        float wiperY = bounds.getCentreY() + (bounds.getWidth() * 0.4f) * std::sin(angle);

        // draw wiper
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, bounds.getCentreX(), bounds.getCentreY()); // Adjust parameters as needed
        nvgLineTo(nvg, wiperX, wiperY);                           // Adjust parameters as needed
        nvgStrokeWidth(nvg, lineThickness);
        nvgStrokeColor(nvg, nvgRGBAf(fgColour.getFloatRed(), fgColour.getFloatGreen(), fgColour.getFloatBlue(), fgColour.getFloatAlpha()));
        nvgLineCap(nvg, NVG_ROUND);
        nvgStroke(nvg);

        drawTicks(nvg, bounds, startAngle, endAngle, lineThickness);
    }

    void setFgColour(Colour newFgColour)
    {
        fgColour = newFgColour;
        repaint();
    }

    void setArcColour(Colour newOutlineColour)
    {
        arcColour = newOutlineColour;
        repaint();
    }

    void setNumberOfTicks(int ticks)
    {
        numberOfTicks = ticks;
        repaint();
    }
};

class KnobObject final : public ObjectBase {

    Knob knob;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);

    Value initialValue = SynchronousValue();
    Value circular = SynchronousValue();
    Value ticks = SynchronousValue();
    Value angularRange = SynchronousValue();
    Value angularOffset = SynchronousValue();
    Value discrete = SynchronousValue();
    Value outline = SynchronousValue();
    Value showArc = SynchronousValue();
    Value exponential = SynchronousValue();
    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value arcColour = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value arcStart = SynchronousValue();

    Value sizeProperty = SynchronousValue();

    bool locked;
    float value = 0.0f;

public:
    KnobObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
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

        locked = ::getValue<bool>(object->locked);

        objectParameters.addParamSize(&sizeProperty, true);
        objectParameters.addParamFloat("Minimum", cGeneral, &min, 0.0f);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 127.0f);
        objectParameters.addParamFloat("Initial value", cGeneral, &initialValue, 0.0f);
        objectParameters.addParamBool("Circular drag", cGeneral, &circular, { "No", "Yes" }, 0);
        objectParameters.addParamInt("Ticks", cGeneral, &ticks, 0);
        objectParameters.addParamBool("Discrete", cGeneral, &discrete, { "No", "Yes" }, 0);
        objectParameters.addParamInt("Angular range", cGeneral, &angularRange, 270);
        objectParameters.addParamInt("Angular offset", cGeneral, &angularOffset, 0);
        objectParameters.addParamFloat("Arc start", cGeneral, &arcStart, 0.0f);
        objectParameters.addParamFloat("Exp", cGeneral, &exponential, 0.0f);

        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);

        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);

        objectParameters.addParamColour("Arc color", cAppearance, &arcColour, PlugDataColour::guiObjectInternalOutlineColour);
        objectParameters.addParamBool("Fill background", cAppearance, &outline, { "No", "Yes" }, 1);
        objectParameters.addParamBool("Show arc", cAppearance, &showArc, { "No", "Yes" }, 1);
    }
    
    bool canReceiveMouseEvent(int x, int y) override
    {
        if (outline.getValue() || !locked)
            return true;

        // If knob is circular limit hit test to circle, and expand more if there are ticks around the knob
        auto hitPoint = getLocalPoint(object, Point<float>(x, y));
        auto centre = getLocalBounds().toFloat().getCentre();
        auto knobRadius = getWidth() * 0.33f;
        auto knobRadiusWithTicks = knobRadius + (getWidth() * 0.06f);
        if (centre.getDistanceFrom(hitPoint) < (ticks.getValue() ? knobRadiusWithTicks : knobRadius)) {
            return true;
        }

        return false;
    }

    bool isTransparent() override
    {
        return !::getValue<bool>(outline);
    }

    void updateDoubleClickValue()
    {
        auto val = jmap<float>(::getValue<float>(initialValue), getMinimum(), getMaximum(), 0.0f, 1.0f);
        knob.setDoubleClickReturnValue(true, std::clamp(val, 0.0f, 1.0f));
        knob.setArcStart(jmap<float>(::getValue<float>(arcStart), getMinimum(), getMaximum(), 0.0f, 1.0f));
        knob.repaint();
    }

    void update() override
    {
        auto currentValue = getValue();
        value = currentValue;
        knob.setValue(currentValue, dontSendNotification);

        if (auto knb = ptr.get<t_fake_knob>()) {
            initialValue = knb->x_load;
            ticks = knb->x_ticks;
            angularRange = knb->x_range;
            angularOffset = knb->x_offset;
            discrete = knb->x_discrete;
            circular = knb->x_circular;
            showArc = knb->x_arc;
            exponential = knb->x_exp;
            primaryColour = getForegroundColour().toString();
            secondaryColour = getBackgroundColour().toString();
            arcColour = getArcColour().toString();
            outline = knb->x_outline;
            sizeProperty = knb->x_size;
            arcStart = knb->x_start;
        }

        min = getMinimum();
        max = getMaximum();
        updateRange();
        updateDoubleClickValue();

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();

        knob.setFgColour(getForegroundColour());
        knob.setArcColour(getArcColour());
        updateRotaryParameters();

        updateDoubleClickValue();
        knob.setSliderStyle(::getValue<bool>(circular) ? Slider::Rotary : Slider::RotaryHorizontalVerticalDrag);
        knob.showArc(::getValue<bool>(showArc));
    }

    bool inletIsSymbol() override
    {
        return hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return hasSendSymbol();
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto knob = ptr.get<t_fake_knob>()) {
            setParameterExcludingListener(sizeProperty, var(knob->x_size));
        }
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto gobj = ptr.get<t_gobj>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return {};

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, gobj.get(), &x, &y, &w, &h);

            return { x, y, w + 1, h + 1 };
        }

        return {};
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            knb->x_obj.te_xpix = b.getX();
            knb->x_obj.te_ypix = b.getY();
            knb->x_size = b.getWidth() - 1;
        }
    }
    void updateRange()
    {
        auto numTicks = std::max(::getValue<int>(ticks) - 1, 1);
        auto increment = ::getValue<bool>(discrete) ? 1.0 / numTicks : std::numeric_limits<double>::epsilon();

        knob.setRange(0.0, 1.0, increment);
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            knob.setValue(getValue(), dontSendNotification);
            break;
        }
        case hash("range"): {
            if (numAtoms >= 2) {
                auto newMin = atoms[0].getFloat();
                auto newMax = atoms[1].getFloat();
                // we have to use our min/max as by the time we get the "range" message, it has already changed knb->x_min & knb->x_max!
                auto oldMin = ::getValue<float>(min);
                auto oldMax = ::getValue<float>(max);

                setParameterExcludingListener(min, std::min(newMin, newMax - 0.0001f));
                setParameterExcludingListener(max, std::max(newMax, newMin + 0.0001f));

                updateRange();
                updateDoubleClickValue();

                updateKnobPosFromMinMax(oldMin, oldMax, newMin, newMax);
            }
            break;
        }
        case hash("angle"): {
            if (numAtoms) {
                auto range = std::clamp<int>(atoms[0].getFloat(), 0, 360);
                setParameterExcludingListener(angularRange, range);
                updateRotaryParameters();
            }
            break;
        }
        case hash("offset"): {
            if (numAtoms) {
                auto offset = std::clamp<int>(atoms[0].getFloat(), -180, 180);
                setParameterExcludingListener(angularOffset, offset);
                updateRotaryParameters();
            }
            break;
        }
        case hash("arc"): {
            setParameterExcludingListener(showArc, atoms[0].getFloat());
            knob.showArc(atoms[0].getFloat());
            break;
        }
        case hash("start"): {
            setParameterExcludingListener(arcStart, atoms[0].getFloat());
            updateDoubleClickValue();
            break;
        }
        case hash("discrete"): {
            setParameterExcludingListener(discrete, atoms[0].getFloat());
            updateRange();
            break;
        }
        case hash("circular"): {
            setParameterExcludingListener(circular, atoms[0].getFloat());
            knob.setSliderStyle(atoms[0].getFloat() ? Slider::Rotary : Slider::RotaryHorizontalVerticalDrag);
            break;
        }
        case hash("ticks"): {
            setParameterExcludingListener(ticks, atoms[0].getFloat());
            updateRotaryParameters();
            updateRange();
            break;
        }
        case hash("send"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(sendSymbol, atoms[0].toString());
            break;
        }
        case hash("receive"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            break;
        }
        case hash("fgcolor"): {
            primaryColour = getForegroundColour().toString();
            break;
        }
        case hash("bgcolor"): {
            secondaryColour = getBackgroundColour().toString();
            break;
        }
        case hash("arccolor"): {
            arcColour = getArcColour().toString();
            break;
        }
        case hash("init"): {
            if (auto knb = ptr.get<t_fake_knob>()) {
                initialValue = knb->x_load;
                knob.setValue(getValue(), dontSendNotification);
            }
            break;
        }
        case hash("outline"): {
            if (numAtoms > 0 && atoms[0].isFloat())
                outline = atoms[0].getFloat();
            break;
        }
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();
        auto bgColour = Colour::fromString(secondaryColour.toString());

        if (::getValue<bool>(outline)) {
            bool selected = object->isSelected() && !cnv->isGraph;
            auto outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

            nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), convertColour(bgColour), convertColour(outlineColour), Corners::objectCornerRadius);
        } else {
            auto circleBounds = getLocalBounds().toFloat().reduced(getWidth() * 0.13f);
            auto const lineThickness = std::max(circleBounds.getWidth() * 0.07f, 1.5f);
            circleBounds = circleBounds.reduced(lineThickness - 0.5f);

            nvgFillColor(nvg, convertColour(bgColour));
            nvgBeginPath(nvg);
            nvgCircle(nvg, circleBounds.getCentreX(), circleBounds.getCentreY(), circleBounds.getWidth() / 2.0f);
            nvgFill(nvg);

            nvgStrokeColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(objectOutlineColourId)));
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);
        }

        knob.render(nvg);
    }

    void resized() override
    {
        knob.setBounds(getLocalBounds());
    }

    bool hasSendSymbol()
    {
        return !getSendSymbol().isEmpty();
    }

    bool hasReceiveSymbol()
    {
        return !getReceiveSymbol().isEmpty();
    }

    String getSendSymbol()
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            knob_get_snd(knb.get()); // get unexpanded send symbol from binbuf

            if (!knb->x_snd_raw || !knb->x_snd_raw->s_name)
                return "";

            auto sym = String::fromUTF8(knb->x_snd_raw->s_name);
            if (sym != "empty") {
                return sym;
            }
        }

        return "";
    }

    String getReceiveSymbol()
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            knob_get_rcv(knb.get()); // get unexpanded receive symbol from binbuf

            if (!knb->x_rcv_raw || !knb->x_rcv_raw->s_name)
                return "";

            auto sym = String::fromUTF8(knb->x_rcv_raw->s_name);
            if (sym != "empty") {
                return sym;
            }
        }

        return "";
    }

    void setSendSymbol(String const& symbol) const
    {
        if (auto knob = ptr.get<void>()) {
            pd->sendDirectMessage(knob.get(), "send", { pd::Atom(pd->generateSymbol(symbol)) });
        }
    }

    void setReceiveSymbol(String const& symbol) const
    {
        if (auto knob = ptr.get<void>()) {
            pd->sendDirectMessage(knob.get(), "receive", { pd::Atom(pd->generateSymbol(symbol)) });
        }
    }

    Colour getBackgroundColour() const
    {
        if (auto knob = ptr.get<t_fake_knob>()) {
            auto bg = String::fromUTF8(knob->x_bg->s_name);
            return convertTclColour(bg);
        }

        return Colour();
    }

    Colour getForegroundColour() const
    {
        if (auto knob = ptr.get<t_fake_knob>()) {
            auto fg = String::fromUTF8(knob->x_fg->s_name);
            return convertTclColour(fg);
        }

        return Colour();
    }

    Colour getArcColour() const
    {
        if (auto knob = ptr.get<t_fake_knob>()) {
            auto mg = String::fromUTF8(knob->x_mg->s_name);
            return convertTclColour(mg);
        }

        return Colour();
    }

    Colour convertTclColour(String const& colourStr) const
    {
        if (tclColours.count(colourStr)) {
            return tclColours[colourStr];
        }

        return Colour::fromString(colourStr.replace("#", "ff"));
    }

    float getValue()
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_pos;
        }

        return 0.0f;
    }

    float getMinimum()
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_min;
        }

        return 0.0f;
    }

    float getMaximum()
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_max;
        }

        return 127.0f;
    }

    void setMinimum(float value)
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            knb->x_min = value;
        }
    }

    void setMaximum(float value)
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            knb->x_max = value;
        }
    }

    void updateRotaryParameters()
    {
        float startRad, endRad;
        int numTicks;
        if (auto knb = ptr.get<t_fake_knob>()) {
            startRad = degreesToRadians<float>(knb->x_start_angle) + MathConstants<float>::twoPi;
            endRad = degreesToRadians<float>(knb->x_end_angle) + MathConstants<float>::twoPi;
            numTicks = knb->x_ticks;
        } else {
            return;
        }

        knob.setRotaryParameters({ startRad, endRad, true });
        knob.setNumberOfTicks(numTicks);
        knob.repaint();
    }

    void updateKnobPosFromMin(float oldMin, float oldMax, float newMin)
    {
        updateKnobPosFromMinMax(oldMin, oldMax, newMin, oldMax);
    }

    void updateKnobPosFromMax(float oldMin, float oldMax, float newMax)
    {
        updateKnobPosFromMinMax(oldMin, oldMax, oldMin, newMax);
    }

    void updateKnobPosFromMinMax(float oldMin, float oldMax, float newMin, float newMax)
    {
        // map current value to new range
        float knobVal = knob.getValue();
        float exp;

        if (auto knb = ptr.get<t_fake_knob>()) {
            exp = knb->x_exp;
        } else {
            return;
        }

        // if exponential mode, map current position factor into exponential
        if (exp != 0) {
            if (exp > 0.0f)
                knobVal = pow(knobVal, exp);
            else
                knobVal = 1 - pow(1 - knobVal, -exp);
        }

        auto currentVal = jmap(knobVal, 0.0f, 1.0f, oldMin, oldMax);
        auto newValNormalised = jmap(currentVal, newMin, newMax, 0.0f, 1.0f);

        // if exponential mode, remove exponential mapping from position
        if (exp != 0) {
            if (exp > 0.0f)
                newValNormalised = pow(newValNormalised, 1 / exp);
            else
                newValNormalised = 1 - pow(1 - newValNormalised, -1 / exp);
        }

        knob.setValue(newValNormalised);
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto size = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            setParameterExcludingListener(sizeProperty, size);
            if (auto knob = ptr.get<t_fake_knob>()) {
                knob->x_size = size;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
            float oldMinVal, oldMaxVal, newMinVal;
            if (auto knb = ptr.get<t_fake_knob>()) {
                oldMinVal = static_cast<float>(knb->x_min);
                oldMaxVal = static_cast<float>(knb->x_max);
                newMinVal = limitValueMax(min, ::getValue<float>(max) - 0.0001f);
            } else {
                return;
            }

            // set new min value and update knob
            setMinimum(newMinVal);
            updateRange();
            updateDoubleClickValue();
            updateKnobPosFromMin(oldMinVal, oldMaxVal, newMinVal);
            if (::getValue<float>(arcStart) < newMinVal)
                arcStart = newMinVal;
        } else if (value.refersToSameSourceAs(max)) {
            float oldMinVal, oldMaxVal, newMaxVal;
            if (auto knb = ptr.get<t_fake_knob>()) {
                oldMinVal = static_cast<float>(knb->x_min);
                oldMaxVal = static_cast<float>(knb->x_max);
                newMaxVal = limitValueMin(max, ::getValue<float>(min) + 0.0001f);
            } else {
                return;
            }

            // set new min value and update knob
            setMaximum(newMaxVal);
            updateRange();
            updateDoubleClickValue();

            updateKnobPosFromMax(oldMinVal, oldMaxVal, newMaxVal);
            if (::getValue<float>(arcStart) > newMaxVal)
                arcStart = newMaxVal;
        } else if (value.refersToSameSourceAs(initialValue)) {
            updateDoubleClickValue();
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_load = ::getValue<float>(initialValue);
        } else if (value.refersToSameSourceAs(circular)) {
            auto mode = ::getValue<int>(circular);
            knob.setSliderStyle(mode ? Slider::Rotary : Slider::RotaryHorizontalVerticalDrag);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_circular = mode;
        } else if (value.refersToSameSourceAs(ticks)) {
            ticks = jmax(::getValue<int>(ticks), 0);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_ticks = ::getValue<int>(ticks);
            updateRotaryParameters();
            updateRange();
        } else if (value.refersToSameSourceAs(angularRange)) {
            auto range = limitValueRange(angularRange, 0, 360);
            if (auto knb = ptr.get<t_fake_knob>()) {
                pd->sendDirectMessage(knb.get(), "angle", { pd::Atom(range) });
            }
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(angularOffset)) {
            auto offset = limitValueRange(angularOffset, -180, 180);
            if (auto knb = ptr.get<t_fake_knob>()) {
                pd->sendDirectMessage(knb.get(), "offset", { pd::Atom(offset) });
            }
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(showArc)) {
            bool arc = ::getValue<bool>(showArc);
            knob.showArc(arc);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_arc = arc;
        } else if (value.refersToSameSourceAs(discrete)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_discrete = ::getValue<bool>(discrete);
            updateRange();
        } else if (value.refersToSameSourceAs(outline)) {
            if (auto knb = ptr.get<t_fake_knob>()) {
                knb->x_outline = ::getValue<bool>(outline);
            }
            repaint();
        } else if (value.refersToSameSourceAs(exponential)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_exp = ::getValue<float>(exponential);
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto colour = "#" + primaryColour.toString().substring(2);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_fg = pd->generateSymbol(colour);
            knob.setFgColour(Colour::fromString(primaryColour.toString()));
            repaint();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto colour = "#" + secondaryColour.toString().substring(2);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_bg = pd->generateSymbol(colour);
            repaint();
        } else if (value.refersToSameSourceAs(arcStart)) {
            auto arcStartLimited = limitValueRange(arcStart, ::getValue<float>(min), ::getValue<float>(max));
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_start = arcStartLimited;
            updateDoubleClickValue();
            repaint();
        } else if (value.refersToSameSourceAs(arcColour)) {
            auto colour = "#" + arcColour.toString().substring(2);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_mg = pd->generateSymbol(colour);
            knob.setArcColour(Colour::fromString(arcColour.toString()));
            repaint();
        }
    }

    void lock(bool isLocked) override
    {
        ObjectBase::lock(isLocked);
        locked = isLocked;
        repaint();
    }

    void setValue(float pos)
    {
        float exp, min, max;
        int numTicks;
        bool discrete;
        if (auto knb = ptr.get<t_fake_knob>()) {
            knb->x_pos = pos;
            exp = knb->x_exp;
            numTicks = knb->x_ticks;
            discrete = knb->x_discrete;
            min = knb->x_min;
            max = knb->x_max;
        } else {
            return;
        }

        t_float fval;
        if (pos < 0.0f)
            pos = 0.0f;
        else if (pos > 1.0f)
            pos = 1.0f;
        if (discrete) {
            t_float ticks = (numTicks < 2 ? 2 : (float)numTicks) - 1;
            pos = rint(pos * ticks) / ticks;
        }
        if (exp == 1) { // log
            if ((min <= 0.0f && max >= 0.0f) || (min >= 0.0f && max <= 0.0f)) {
                pd_error(NULL, "[knob]: range cannot contain '0' in log mode");
                fval = min;
            } else
                fval = expf(pos * log(max / min)) * min;
        } else {
            if (exp != 0) {
                if (exp > 0)
                    pos = pow(pos, exp);
                else
                    pos = 1 - pow(1 - pos, -exp);
            }
            fval = pos * (max - min) + min;
        }
        if ((fval < 1.0e-10) && (fval > -1.0e-10))
            fval = 0.0;

        sendFloatValue(fval);
    }
};
