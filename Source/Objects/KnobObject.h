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
    Colour arcColour;

    int arcWidth = 1;
    int numberOfTicks = 0;
public:
    ReversibleKnob() : Slider(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox)
    {
        setScrollWheelEnabled(false);
        setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
    }
    
    ~ReversibleKnob() {}

    void drawTicks(Graphics& g, Rectangle<int> knobBounds, float startAngle, float endAngle)
    {
        auto centre = knobBounds.getCentre();
        auto radius = (knobBounds.getWidth() * 0.5f) * 1.1f;

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
            g.setColour(Colours::grey);
            g.fillEllipse(x - tickRadius, y - tickRadius, tickRadius * 2.0f, tickRadius * 2.0f);
        }
    }

    void paint(Graphics& g) override 
    {
        auto bounds = getBounds().reduced(getWidth() * 0.15f);
        auto x = bounds.getX();
        auto y = bounds.getY();
        auto w = bounds.getWidth();
        auto h = bounds.getHeight();

        auto arcWidthProportional = static_cast<float>(arcWidth) / bounds.getWidth();//* 0.001f;

        auto sliderPosProportional = (getValue() - getRange().getStart()) / getRange().getLength();

        auto startAngle = getRotaryParameters().startAngleRadians * -1 + MathConstants<float>::pi;
        auto endAngle = getRotaryParameters().endAngleRadians * -1 + MathConstants<float>::pi;
        float angle = startAngle  + sliderPosProportional * (endAngle - startAngle);

        startAngle = jmin(startAngle, endAngle + MathConstants<float>::twoPi);
        startAngle = jmax(startAngle, endAngle - MathConstants<float>::twoPi);

        Path arc;

        arc.addPieSegment(x, y, w, h, startAngle, angle, 1.0f - arcWidthProportional);
        
        auto const lineThickness = std::max(w * 0.06f, 1.5f);
        
        auto line = Line<float>::fromStartAndAngle (bounds.getCentre().toFloat(), (w / 2.0f) - 1.5f, angle);
        
        g.setColour(lnColour);
        g.drawEllipse(bounds.toFloat().reduced(1.0f), lineThickness);
        
        g.setColour(fgColour);
        g.drawLine(line, lineThickness);
        
        g.setColour(arcColour);
        g.fillPath(arc);

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

    
    void setArcColour(Colour newArcColour)
    {
        arcColour = newArcColour;
        repaint();
    }
    
    void setArcWidth(int newArcWidth)
    {
        arcWidth = newArcWidth;
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
        int             x_arc_width;
        int             x_start_angle;
        int             x_end_angle;
        int             x_ticks;
        double          x_min;
        double          x_max;
        t_float         x_fval;
        int             x_acol;
        t_symbol       *x_move_mode; // "xy" or "angle"
        unsigned int    x_wiper_visible:1;
        unsigned int    x_arc_visible:1;
        unsigned int    x_center_visible:1;
    };

    ReversibleKnob knob;

    IEMHelper iemHelper;

    Value min = Value(0.0f);
    Value max = Value(0.0f);
    
    Value initialValue, moveMode, ticks, arcThickness, startAngle, endAngle, arcColour;

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
        
        auto mode = getModeMove();
        knob.setSliderStyle(mode);
        moveMode = mode - 3;
        
        initialValue = knb->x_init;
        ticks = knb->x_ticks;
        arcThickness = knb->x_arc_width;
        startAngle = knb->x_start_angle;
        endAngle = knb->x_end_angle;
        
        arcColour = Colour(static_cast<uint32>(convert_from_iem_color(knb->x_acol))).toString();
        
        onConstrainerCreate = [this]() {
            constrainer->setFixedAspectRatio(1.0f);
            constrainer->setMinimumSize(this->object->minimumSize, this->object->minimumSize);
        };
        
        updateRotaryParameters();
        
        knob.setOutlineColour(object->findColour(PlugDataColour::outlineColourId));
        knob.setArcColour(Colour::fromString(arcColour.toString()));
    }
    
    void setModeMove(Slider::SliderStyle style)
    {
        auto& mode = static_cast<t_fake_knb*>(ptr)->x_move_mode;
        if(style == Slider::SliderStyle::RotaryHorizontalDrag)
        {
            mode = gensym("x");
        }
        if(style == Slider::SliderStyle::RotaryVerticalDrag)
        {
            mode = gensym("y");
        }
        if(style == Slider::SliderStyle::RotaryHorizontalVerticalDrag)
        {
            mode = gensym("xy");
        }
        if(style == Slider::SliderStyle::RotaryHorizontalVerticalDrag)
        {
            mode = gensym("angle");
        }
    }
    
    Slider::SliderStyle getModeMove()
    {
        auto mode = static_cast<t_fake_knb*>(ptr)->x_move_mode;
        if(mode == gensym("x"))
        {
            return Slider::SliderStyle::RotaryHorizontalDrag;
        }
        if(mode == gensym("y"))
        {
            return Slider::SliderStyle::RotaryVerticalDrag;
        }
        if(mode == gensym("xy"))
        {
            return Slider::SliderStyle::RotaryHorizontalVerticalDrag;
        }
        if(mode == gensym("angle"))
        {
            return Slider::SliderStyle::Rotary;
        }
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

    void updateLabel() override
    {
        iemHelper.updateLabel(label);
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
        if (knob.isRangeFlipped())
            knob.setRange(getMaximum(), getMinimum(), std::numeric_limits<float>::epsilon());
        else
            knob.setRange(getMinimum(), getMaximum(), std::numeric_limits<float>::epsilon());
    }

    std::vector<hash32> getAllMessages() override {
        return {
            hash("float"),
            hash("set"),
            hash("range"),
            IEMGUI_MESSAGES
            // TODO: finish this
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
            { "Move mode", tCombo, cGeneral, &moveMode, {"Angle", "X", "Y", "X + Y"}},
            { "Ticks", tInt, cGeneral, &ticks, {} },
            { "Arc thickness", tFloat, cGeneral, &arcThickness, {} },
            { "Start angle", tInt, cGeneral, &startAngle, {} },
            { "End angle", tInt, cGeneral, &endAngle, {} },
            
            { "Foreground", tColour, cAppearance, &iemHelper.primaryColour, {} },
            { "Background", tColour, cAppearance, &iemHelper.secondaryColour, {} },
            { "Arc", tColour, cAppearance, &arcColour, {} },
            { "Receive Symbol", tString, cGeneral, &iemHelper.receiveSymbol, {} },
            { "Send Symbol", tString, cGeneral, &iemHelper.sendSymbol, {} },
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
        knob.setArcWidth(knb->x_arc_width);
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
            knb->x_init = static_cast<float>(initialValue.getValue());
        }
        else if (value.refersToSameSourceAs(moveMode)) {
            auto mode = static_cast<Slider::SliderStyle>(static_cast<int>(moveMode.getValue()) + 3);
            knob.setSliderStyle(mode);
            setModeMove(mode);
            
        }
        else if (value.refersToSameSourceAs(arcColour)) {
            auto colourStr = arcColour.toString();
            knob.setArcColour(Colour::fromString(colourStr));
            knb->x_acol = convert_to_iem_color(colourStr.toRawUTF8());
        }
        else if (value.refersToSameSourceAs(ticks)) {
            ticks = jmax(static_cast<int>(ticks.getValue()), 0);
            knb->x_ticks = static_cast<int>(ticks.getValue());
            updateRotaryParameters();
        }
        else if (value.refersToSameSourceAs(arcThickness)) {
            arcThickness = jmax(static_cast<int>(arcThickness.getValue()), 0);
            knb->x_arc_width = static_cast<int>(arcThickness.getValue());
            updateRotaryParameters();
        }
        else if (value.refersToSameSourceAs(startAngle)) {
            knb->x_start_angle = static_cast<int>(startAngle.getValue());
            updateRotaryParameters();
        }
        else if (value.refersToSameSourceAs(endAngle)) {
            knb->x_end_angle = static_cast<int>(endAngle.getValue());
            updateRotaryParameters();
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
