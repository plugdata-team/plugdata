/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>

class ReversibleKnob : public Slider{
    
    bool isInverted;
    Colour bgColour;
    Colour fgColour;
    Colour lnColour;
    
    bool drawArc;

    int numberOfTicks = 0;
public:
    ReversibleKnob() : Slider(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox)
    {
        setScrollWheelEnabled(false);
        setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
    }

    ~ReversibleKnob() {}

    void drawTicks(Graphics& g, Rectangle<float> knobBounds, float startAngle, float endAngle)
    {
        auto centre = knobBounds.getCentre();
        auto radius = (knobBounds.getWidth() * 0.5f) * 1.05f;

        // Calculate the angle between each tick
        float angleIncrement = (endAngle - startAngle) / static_cast<float>(jmax(numberOfTicks - 1, 1));

        // Position each tick around the larger circle
        float tickRadius = radius * 0.05f;
        for (int i = 0; i < numberOfTicks; ++i)
        {
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
        auto bounds = getBounds().toFloat().reduced(getWidth() * 0.13f);
        
        auto const lineThickness = std::max(bounds.getWidth() * 0.07f, 1.5f);
        auto const arcThickness = lineThickness * 3.0f / bounds.getWidth();

        auto sliderPosProportional = (getValue() - getRange().getStart()) / getRange().getLength();
        sliderPosProportional = std::isfinite(sliderPosProportional) ? sliderPosProportional : 0.0f;

        auto startAngle = getRotaryParameters().startAngleRadians * -1 + MathConstants<float>::pi;
        auto endAngle = getRotaryParameters().endAngleRadians * -1 + MathConstants<float>::pi;
        float angle = startAngle  + sliderPosProportional * (endAngle - startAngle);

        startAngle = jmin(startAngle, endAngle + MathConstants<float>::twoPi);
        startAngle = jmax(startAngle, endAngle - MathConstants<float>::twoPi);
        
        // draw range arc
        g.setColour(lnColour);
        auto arcBounds = bounds.reduced(lineThickness);
        auto arcRadius = arcBounds.getWidth() * 0.5;
        auto arcWidth = (arcRadius - lineThickness) / arcRadius;
        Path rangeArc;
        rangeArc.addPieSegment(arcBounds, startAngle, endAngle, arcWidth);
        g.fillPath(rangeArc);

        // draw arc
        if(drawArc) {
            Path arc;
            arc.addPieSegment(arcBounds, startAngle, angle, arcWidth);
            g.setColour(fgColour);
            g.fillPath(arc);
        }

        // draw wiper
        Path wiperPath;
        auto wiperRadius = bounds.getWidth() * 0.5;
        auto line = Line<float>::fromStartAndAngle (bounds.getCentre(), wiperRadius, angle);
        wiperPath.startNewSubPath(line.getStart());
        wiperPath.lineTo(line.getPointAlongLine(wiperRadius - lineThickness * 1.5));
        g.setColour(fgColour);
        g.strokePath(wiperPath, PathStrokeType(lineThickness, PathStrokeType::JointStyle::curved, PathStrokeType::EndCapStyle::rounded));

        drawTicks(g, bounds, startAngle, endAngle);
    }

    void setRangeFlipped(bool invert)
    {
        isInverted = invert;
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
        if(!std::isfinite(value)) return 0.0f;
            
        if (isInverted)
            return 1.0f - (Slider::valueToProportionOfLength(value));
        else
            return Slider::valueToProportionOfLength(value);
    };

    void setBgColour(Colour newBgColour)
    {
        bgColour = newBgColour;
        repaint();
    }

    void setFgColour(Colour newFgColour)
    {
        fgColour = newFgColour;
        repaint();
    }
    
    void setOutlineColour(Colour newOutlineColour)
    {
        lnColour = newOutlineColour;
        repaint();
    }

    void setNumberOfTicks(int ticks)
    {
        numberOfTicks = ticks;
        repaint();
    }
};


class KnobObject : public ObjectBase {
    
    struct t_fake_knb
    {
        t_iemgui        x_gui;
        float           x_pos;  // 0-1 normalized position
        float           x_exp;
        float           x_init;
        int             x_start_angle;
        int             x_end_angle;
        int             x_range;
        int             x_offset;
        int             x_ticks;
        double          x_min;
        double          x_max;
        t_float         x_fval;
        int             x_circular;
        int             x_arc;
        int             x_discrete;
    };

    ReversibleKnob knob;

    IEMHelper iemHelper;

    Value min = Value(0.0f);
    Value max = Value(0.0f);
    
    Value initialValue, circular, ticks, angularRange, angularOffset, discrete, showArc;

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
        
        auto* knb = static_cast<t_fake_knb*>(ptr);
        initialValue = knb->x_init;
        ticks = knb->x_ticks;
        angularRange = knb->x_range;
        angularOffset = knb->x_offset;
        discrete = knb->x_discrete;
        circular = knb->x_circular;
        showArc = knb->x_arc;
        
        onConstrainerCreate = [this]() {
            constrainer->setFixedAspectRatio(1.0f);
            constrainer->setMinimumSize(this->object->minimumSize, this->object->minimumSize);
        };
        
        updateRotaryParameters();
        
        knob.setDoubleClickReturnValue(true, static_cast<int>(initialValue.getValue()));
        knob.setOutlineColour(object->findColour(PlugDataColour::outlineColourId));
        knob.setSliderStyle(static_cast<bool>(circular.getValue()) ? Slider::Rotary : Slider::RotaryHorizontalVerticalDrag);
        knob.showArc(static_cast<bool>(showArc.getValue()));
    }
    
    void setCircular(Slider::SliderStyle style)
    {
        static_cast<t_fake_knb*>(ptr)->x_circular = style == Slider::SliderStyle::RotaryHorizontalVerticalDrag;
    }
    
    bool isCircular()
    {
        return static_cast<t_fake_knb*>(ptr)->x_circular;
    }
    
    void lookAndFeelChanged() override
    {
        knob.setOutlineColour(object->findColour(PlugDataColour::outlineColourId));
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

        iemHelper.update();

        knob.setFgColour(Colour::fromString(iemHelper.primaryColour.toString()));
    }

    bool hideInlets() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return iemHelper.hasSendSymbol();
    }

    Rectangle<int> getPdBounds() override
    {
        auto* iemgui = static_cast<t_iemgui*>(ptr);
        
        pd->lockAudioThread();
        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        pd->unlockAudioThread();
        
        return Rectangle<int>(x, y, w + 1, h + 1);
    }

    void setPdBounds(Rectangle<int> b) override
    {
        iemHelper.setPdBounds(b.withTrimmedRight(-1).withTrimmedBottom(-1));
    }

    void updateRange()
    {
        auto min = getMinimum();
        auto max = getMaximum();
        auto range = std::abs(max - min);
        auto numTicks = std::max(static_cast<int>(ticks.getValue()) - 1, 0);
        auto increment = static_cast<bool>(discrete.getValue()) ? range / numTicks : std::numeric_limits<float>::epsilon();
        
        if (knob.isRangeFlipped())
            knob.setRange(max, min, increment);
        else
            knob.setRange(min, max, increment);
    }

    std::vector<hash32> getAllMessages() override {
        return {
            hash("float"),
            hash("set"),
            hash("range"),
            hash("circular"),
            hash("exp"),
            hash("discrete"),
            hash("arc"),
            hash("angle"),
            hash("ticks"),
            IEMGUI_MESSAGES
        };
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
        case hash("range"): {
            if (atoms.size() >= 2) {
                knob.setRangeFlipped(atoms[0].getFloat() > atoms[1].getFloat());
                setParameterExcludingListener(min, atoms[0].getFloat());
                setParameterExcludingListener(max, atoms[1].getFloat());
                updateRange();
            }
            break;
        }
        case hash("angle"): {
            if (atoms.size() >= 2) {
                setParameterExcludingListener(angularRange, atoms[0].getFloat());
                setParameterExcludingListener(angularOffset, atoms[1].getFloat());
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
        case hash("exp"): {
            //exp = atoms[0].getFloat();
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }

        // Update the colours of the actual slider
        if (hash(symbol) == hash("color")) {
            knob.setBgColour(Colour::fromString(iemHelper.secondaryColour.toString()));
            knob.setFgColour(Colour::fromString(iemHelper.primaryColour.toString()));
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
        knob.setBounds(getLocalBounds());
    }


    ObjectParameters getParameters() override
    {
        return {
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },
            
            { "Initial value", tFloat, cGeneral, &initialValue, {} },
            { "Circular drag", tBool, cGeneral, &circular, {"No", "Yes"}},
            { "Ticks", tInt, cGeneral, &ticks, {} },
            { "Discrete", tBool, cGeneral, &discrete, {"No", "Yes"} },
            
            { "Angular range", tInt, cGeneral, &angularRange, {} },
            { "Angular offset", tInt, cGeneral, &angularOffset, {} },
            
            { "Foreground", tColour, cAppearance, &iemHelper.primaryColour, {} },
            { "Background", tColour, cAppearance, &iemHelper.secondaryColour, {} },
            { "Show arc", tBool, cAppearance, &showArc, {"No", "Yes"} },
            { "Receive symbol", tString, cGeneral, &iemHelper.receiveSymbol, {} },
            { "Send symbol", tString, cGeneral, &iemHelper.sendSymbol, {} },
        };
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

    void updateRotaryParameters()
    {
        auto* knb = static_cast<t_fake_knb*>(ptr);
        float startRad = degreesToRadians<float>(180 - knb->x_start_angle);
        float endRad = degreesToRadians<float>(180 - knb->x_end_angle);
        knob.setRotaryParameters({startRad, endRad, true});
        knob.setNumberOfTicks(knb->x_ticks);
        knob.repaint();
    }

    void valueChanged(Value& value) override
    {
        auto* knb = static_cast<t_fake_knb*>(ptr);
        
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
            updateRange();
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
            updateRange();
        }
        else if (value.refersToSameSourceAs(initialValue)) {
            knob.setDoubleClickReturnValue(true, static_cast<int>(initialValue.getValue()));
            knb->x_init = static_cast<float>(initialValue.getValue());
        }
        else if (value.refersToSameSourceAs(circular)) {
            auto mode = static_cast<int>(circular.getValue());
            knob.setSliderStyle(mode ? Slider::Rotary : Slider::RotaryHorizontalVerticalDrag);
            knb->x_circular = mode;
            
        }
        else if (value.refersToSameSourceAs(ticks)) {
            ticks = jmax(static_cast<int>(ticks.getValue()), 0);
            knb->x_ticks = static_cast<int>(ticks.getValue());
            updateRotaryParameters();
            updateRange();
        }
        else if (value.refersToSameSourceAs(angularRange)) {
            auto start = static_cast<int>(angularRange.getValue());
            auto end = static_cast<int>(angularOffset.getValue());
            pd->enqueueDirectMessages(knb, "angle", {pd::Atom(start), pd::Atom(end)});
            updateRotaryParameters();
        }
        else if (value.refersToSameSourceAs(showArc)) {
            bool arc = static_cast<bool>(showArc.getValue());
            knob.showArc(arc);
            knb->x_arc = arc;
        }
        else if (value.refersToSameSourceAs(angularOffset)) {
            auto start = static_cast<int>(angularRange.getValue());
            auto end = static_cast<int>(angularOffset.getValue());
            pd->enqueueDirectMessages(knb, "angle", {pd::Atom(start), pd::Atom(end)});
            updateRotaryParameters();
        }
        else if (value.refersToSameSourceAs(discrete)) {
            knb->x_discrete = static_cast<bool>(discrete.getValue());
            updateRange();
        }
        else {
            iemHelper.valueChanged(value);
            knob.setFgColour(Colour::fromString(iemHelper.primaryColour.toString()));
        }
    }

    void setValue(float v)
    {
        sendFloatValue(v);
    }
};
