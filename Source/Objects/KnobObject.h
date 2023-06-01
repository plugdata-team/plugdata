/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>
#include "TclColours.h"

class Knob : public Slider {

    Colour fgColour;
    Colour arcColour;

    bool drawArc = true;

    int numberOfTicks = 0;

public:
    Knob()
        : Slider(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox)
    {
        setScrollWheelEnabled(false);
        setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
    }

    ~Knob() = default;

    void drawTicks(Graphics& g, Rectangle<float> knobBounds, float startAngle, float endAngle, float tickWidth)
    {
        auto centre = knobBounds.getCentre();
        auto radius = (knobBounds.getWidth() * 0.5f) * 1.05f;

        // Calculate the angle between each tick
        float angleIncrement = (endAngle - startAngle) / static_cast<float>(jmax(numberOfTicks - 1, 1));

        // Position each tick around the larger circle
        float tickRadius = tickWidth * 0.5f;
        for (int i = 0; i < numberOfTicks; ++i) {
            float angle = startAngle + i * angleIncrement - MathConstants<float>::pi * 0.5f;
            float x = centre.getX() + radius * std::cos(angle);
            float y = centre.getY() + radius * std::sin(angle);

            // Draw the tick at this position
            g.setColour(fgColour);
            g.fillEllipse(x - tickRadius, y - tickRadius, tickRadius * 2.0f, tickRadius * 2.0f);
        }
    }

    void showArc(bool show)
    {
        drawArc = show;
        repaint();
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(getWidth() * 0.13f);

        auto const lineThickness = std::max(bounds.getWidth() * 0.07f, 1.5f);

        auto sliderPosProportional = getValue();

        auto startAngle = getRotaryParameters().startAngleRadians;
        auto endAngle = getRotaryParameters().endAngleRadians;

        auto angle = jmap<float>(sliderPosProportional, startAngle, endAngle);

        startAngle = std::clamp(startAngle, endAngle - MathConstants<float>::twoPi, endAngle + MathConstants<float>::twoPi);

        if (drawArc) {
            // draw range arc
            g.setColour(arcColour);
            auto arcBounds = bounds.reduced(lineThickness);
            auto arcRadius = arcBounds.getWidth() * 0.5;
            auto arcWidth = (arcRadius - lineThickness) / arcRadius;
            Path rangeArc;
            rangeArc.addPieSegment(arcBounds, startAngle, endAngle, arcWidth);
            g.fillPath(rangeArc);

            // draw arc
            auto centre = jmap<double>(getDoubleClickReturnValue(), startAngle, endAngle);

            Path arc;
            arc.addPieSegment(arcBounds, centre, angle, arcWidth);
            g.setColour(fgColour);
            g.fillPath(arc);
        }

        // draw wiper
        Path wiperPath;
        auto wiperRadius = bounds.getWidth() * 0.5;
        auto line = Line<float>::fromStartAndAngle(bounds.getCentre(), wiperRadius, angle);
        wiperPath.startNewSubPath(line.getStart());
        wiperPath.lineTo(line.getPointAlongLine(wiperRadius - lineThickness * 1.5));
        g.setColour(fgColour);
        g.strokePath(wiperPath, PathStrokeType(lineThickness, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));

        drawTicks(g, bounds, startAngle, endAngle, lineThickness);
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

class KnobObject : public ObjectBase {

    Knob knob;

    Value min = Value(0.0f);
    Value max = Value(0.0f);

    Value initialValue, circular, ticks, angularRange, angularOffset, discrete, outline, showArc, exponential;
    Value primaryColour, secondaryColour, arcColour, sendSymbol, receiveSymbol;

    float value = 0.0f;

public:
    KnobObject(void* obj, Object* object)
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

        objectParameters.addParamFloat("Minimum", cGeneral, &min, 0.0f);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 127.0f);
        objectParameters.addParamFloat("Initial value", cGeneral, &initialValue, 0.0f);
        objectParameters.addParamBool("Circular drag", cGeneral, &circular, { "No", "Yes" }, 0);
        objectParameters.addParamInt("Ticks", cGeneral, &ticks, 0);
        objectParameters.addParamBool("Discrete", cGeneral, &discrete, { "No", "Yes" }, 0);
        objectParameters.addParamInt("Angular range", cGeneral, &angularRange, 270);
        objectParameters.addParamInt("Angular offset", cGeneral, &angularOffset, 0);
        objectParameters.addParamFloat("Exp", cGeneral, &exponential, 0.0f);

        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);

        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);

        objectParameters.addParamColour("Arc color", cAppearance, &arcColour, PlugDataColour::guiObjectInternalOutlineColour);
        objectParameters.addParamBool("Fill background", cAppearance, &outline, { "No", "Yes" }, 1);
        objectParameters.addParamBool("Show arc", cAppearance, &showArc, { "No", "Yes" }, 1);
    }

    void updateDoubleClickValue()
    {
        auto val = jmap<float>(::getValue<float>(initialValue), getMinimum(), getMaximum(), 0.0f, 1.0f);
        knob.setDoubleClickReturnValue(true, std::clamp(val, 0.0f, 1.0f));
        knob.repaint();
    }

    void setCircular(Slider::SliderStyle style)
    {
        if(auto knob = ptr.get<t_fake_knob>())
        {
            knob->x_circular = style == Slider::SliderStyle::RotaryHorizontalVerticalDrag;
        }
    }

    bool isCircular()
    {
        if(auto knob = ptr.get<t_fake_knob>())
        {
            return knob->x_circular;
        }
        
        return false;
    }

    void update() override
    {
        auto currentValue = getValue();
        value = currentValue;
        knob.setValue(currentValue, dontSendNotification);

        if(auto knb = ptr.get<t_fake_knob>()) {
            initialValue = knb->x_init;
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

    bool hideInlets() override
    {
        return hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return hasSendSymbol();
    }

    Rectangle<int> getPdBounds() override
    {
        if(auto gobj = ptr.get<t_gobj>())
        {
            auto* patch = cnv->patch.getPointer().get();
            if(!patch) return {};
            
            int x = 0, y = 0, w = 0, h = 0;
            libpd_get_object_bounds(patch, gobj.get(), &x, &y, &w, &h);
            
            return { x, y, w + 1, h + 1 };
        }
        
        return {};
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if(auto knb = ptr.get<t_fake_knob>()) {
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

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("float"),
            hash("set"),
            hash("range"),
            hash("circular"),
            hash("discrete"),
            hash("arc"),
            hash("angle"),
            hash("offset"),
            hash("ticks"),
            hash("send"),
            hash("receive"),
            hash("fgcolor"),
            hash("bgcolor"),
            hash("arccolor"),
            hash("init"),
            hash("outline"),
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("set"): {
            knob.setValue(getValue(), dontSendNotification);
            break;
        }
        case hash("range"): {
            if (atoms.size() >= 2) {
                auto newMin = atoms[0].getFloat();
                auto newMax = atoms[1].getFloat();
                // we have to use our min/max as by the time we get the "range" message, it has already changed knb->x_min & knb->x_max!
                auto oldMin = ::getValue<float>(min);
                auto oldMax = ::getValue<float>(max);
                setParameterExcludingListener(min, newMin);
                setParameterExcludingListener(max, newMax);
                updateRange();
                updateDoubleClickValue();

                updateKnobPosFromMinMax(oldMin, oldMax, newMin, newMax);
            }
            break;
        }
        case hash("angle"): {
            if (atoms.size()) {
                auto range = std::clamp<int>(atoms[0].getFloat(), 0, 360);
                setParameterExcludingListener(angularRange, range);
                updateRotaryParameters();
            }
            break;
        }
        case hash("offset"): {
            if (atoms.size()) {
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
            if (atoms.size() >= 1)
                setParameterExcludingListener(sendSymbol, atoms[0].getSymbol());
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].getSymbol());
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
            if(auto knb = ptr.get<t_fake_knob>()) {
                initialValue = knb->x_init;
                knob.setValue(getValue(), dontSendNotification);
            }
            break;
        }
        case hash("outline"): {
            if (atoms.size() > 0 && atoms[0].isFloat())
                outline = atoms[0].getFloat();
            break;
        }
        }
    }

    void paint(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        if (::getValue<bool>(outline)) {
            g.setColour(Colour::fromString(secondaryColour.toString()));
            g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);

            g.setColour(outlineColour);
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
        } else {

            auto bounds = getLocalBounds().toFloat().reduced(getWidth() * 0.13f);
            auto const lineThickness = std::max(bounds.getWidth() * 0.07f, 1.5f);
            bounds = bounds.reduced(lineThickness - 0.5f);

            g.setColour(Colour::fromString(secondaryColour.toString()));
            g.fillEllipse(bounds);

            g.setColour(outlineColour);
            g.drawEllipse(bounds, 1.0f);
        }
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
        pd->setThis();

        if(auto knb = ptr.get<t_fake_knob>()) {
            
            if (!knb->x_snd || !knb->x_snd->s_name)
                return "";
            
            auto sym = String::fromUTF8(knb->x_snd->s_name);
            if (sym != "empty") {
                return sym;
            }
        }

        return "";
    }

    String getReceiveSymbol()
    {
        pd->setThis();

        if(auto knb = ptr.get<t_fake_knob>()) {
            if (!knb->x_rcv || !knb->x_rcv->s_name)
                return "";
            
            auto sym = String::fromUTF8(knb->x_rcv->s_name);
            if (sym != "empty") {
                return sym;
            }
        }

        return "";
    }

    void setSendSymbol(String const& symbol) const
    {
        if(auto knob = ptr.get<void>())
        {
            pd->sendDirectMessage(knob.get(), "send", { pd::Atom(symbol) });
        }
        
    }

    void setReceiveSymbol(String const& symbol) const
    {
        if(auto knob = ptr.get<void>())
        {
            pd->sendDirectMessage(knob.get(), "receive", { pd::Atom(symbol) });
        }
    }

    Colour getBackgroundColour() const
    {
        if(auto knob = ptr.get<t_fake_knob>())
        {
            auto bg = String::fromUTF8(knob->x_bg->s_name);
            return convertTclColour(bg);
        }
        
        return Colour();
    }

    Colour getForegroundColour() const
    {
        if(auto knob = ptr.get<t_fake_knob>())
        {
            auto fg = String::fromUTF8(knob->x_fg->s_name);
            return convertTclColour(fg);
        }
        
        return Colour();
    }

    Colour getArcColour() const
    {
        if(auto knob = ptr.get<t_fake_knob>())
        {
            auto mg = String::fromUTF8(ptr.get<t_fake_knob>()->x_mg->s_name);
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
        if(auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_pos;
        }
        
        return 0.0f;
    }

    float getMinimum()
    {
        if(auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_min;
        }
        
        return 0.0f;
    }

    float getMaximum()
    {
        if(auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_max;
        }
        
        return 127.0f;
    }

    void setMinimum(float value)
    {
        if(auto knb = ptr.get<t_fake_knob>()) {
            knb->x_min = value;
        }
    }

    void setMaximum(float value)
    {
        if(auto knb = ptr.get<t_fake_knob>()) {
            knb->x_max = value;
        }
    }

    void updateRotaryParameters()
    {
        float startRad, endRad;
        int numTicks;
        if(auto knb = ptr.get<t_fake_knob>())
        {
            startRad = degreesToRadians<float>(knb->x_start_angle) + MathConstants<float>::twoPi;
            endRad = degreesToRadians<float>(knb->x_end_angle) + MathConstants<float>::twoPi;
            numTicks = knb->x_ticks;
        }
        else {
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
        
        if(auto knb = ptr.get<t_fake_knob>())
        {
            exp = knb->x_exp;
        }
        else {
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
       

        if (value.refersToSameSourceAs(min)) {
            float oldMinVal, oldMaxVal, newMinVal;
            if(auto knb = ptr.get<t_fake_knob>()) {
                oldMinVal = static_cast<float>(knb->x_min);
                oldMaxVal = static_cast<float>(knb->x_max);
                newMinVal = ::getValue<float>(min);
            }
            else {
                return;
            }

            // set new min value and update knob
            setMinimum(newMinVal);
            updateRange();
            updateDoubleClickValue();
            updateKnobPosFromMin(oldMinVal, oldMaxVal, newMinVal);
        } else if (value.refersToSameSourceAs(max)) {
            float oldMinVal, oldMaxVal, newMaxVal;
            if(auto knb = ptr.get<t_fake_knob>()) {
                oldMinVal = static_cast<float>(knb->x_min);
                oldMaxVal = static_cast<float>(knb->x_max);
                newMaxVal = ::getValue<float>(max);
            }
            else {
                return;
            }

            // set new min value and update knob
            setMaximum(newMaxVal);
            updateRange();
            updateDoubleClickValue();

            updateKnobPosFromMax(oldMinVal, oldMaxVal, newMaxVal);
        } else if (value.refersToSameSourceAs(initialValue)) {
            updateDoubleClickValue();
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_init = ::getValue<int>(initialValue);
        } else if (value.refersToSameSourceAs(circular)) {
            auto mode = ::getValue<int>(circular);
            knob.setSliderStyle(mode ? Slider::Rotary : Slider::RotaryHorizontalVerticalDrag);
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_circular = mode;
        } else if (value.refersToSameSourceAs(ticks)) {
            ticks = jmax(::getValue<int>(ticks), 0);
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_ticks = ::getValue<int>(ticks);
            updateRotaryParameters();
            updateRange();
        } else if (value.refersToSameSourceAs(angularRange)) {
            auto range = limitValueRange(angularRange, 0, 360);
            if(auto knb = ptr.get<t_fake_knob>())  {
                pd->sendDirectMessage(knb.get(), "angle", { pd::Atom(range) });
            }
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(angularOffset)) {
            auto offset = limitValueRange(angularOffset, -180, 180);
            if(auto knb = ptr.get<t_fake_knob>()) {
                pd->sendDirectMessage(knb.get(), "offset", { pd::Atom(offset) });
            }
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(showArc)) {
            bool arc = ::getValue<bool>(showArc);
            knob.showArc(arc);
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_arc = arc;
        } else if (value.refersToSameSourceAs(discrete)) {
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_discrete = ::getValue<bool>(discrete);
            updateRange();
        } else if (value.refersToSameSourceAs(outline)) {
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_outline = ::getValue<bool>(outline);
            repaint();
        } else if (value.refersToSameSourceAs(exponential)) {
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_exp = ::getValue<float>(exponential);
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto colour = "#" + primaryColour.toString().substring(2);
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_fg = pd->generateSymbol(colour);
            knob.setFgColour(Colour::fromString(primaryColour.toString()));
            repaint();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto colour = "#" + secondaryColour.toString().substring(2);
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_bg = pd->generateSymbol(colour);
            repaint();
        } else if (value.refersToSameSourceAs(arcColour)) {
            auto colour = "#" + arcColour.toString().substring(2);
            if(auto knb = ptr.get<t_fake_knob>()) knb->x_mg = pd->generateSymbol(colour);
            knob.setArcColour(Colour::fromString(arcColour.toString()));
            repaint();
        }
    }

    void setValue(float pos)
    {
        float exp, min, max;
        int numTicks;
        bool discrete;
        if(auto knb = ptr.get<t_fake_knob>())
        {
            knb->x_pos = pos;
            exp = knb->x_exp;
            numTicks = knb->x_ticks;
            discrete = knb->x_discrete;
            min = knb->x_min;
            max = knb->x_max;
        }
        else {
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
