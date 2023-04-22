/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>

class Knob : public Slider {

    Colour fgColour;
    Colour lnColour;

    bool drawArc;

    int numberOfTicks = 0;

public:
    Knob()
        : Slider(Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox)
    {
        setScrollWheelEnabled(false);
        setVelocityModeParameters(1.0f, 1, 0.0f, false, ModifierKeys::shiftModifier);
    }

    ~Knob() { }

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
        auto bounds = getBounds().toFloat().reduced(getWidth() * 0.13f);

        auto const lineThickness = std::max(bounds.getWidth() * 0.07f, 1.5f);
        auto const arcThickness = lineThickness * 3.0f / bounds.getWidth();

        auto sliderPosProportional = (getValue() - 0.01f) / (1 - 2 * 0.01f);

        auto startAngle = getRotaryParameters().startAngleRadians * -1 + MathConstants<float>::pi;
        auto endAngle = getRotaryParameters().endAngleRadians * -1 + MathConstants<float>::pi;
        float angle = startAngle + sliderPosProportional * (endAngle - startAngle);

        startAngle = std::clamp(startAngle, endAngle - MathConstants<float>::twoPi, endAngle + MathConstants<float>::twoPi);

        // draw range arc
        g.setColour(lnColour);
        auto arcBounds = bounds.reduced(lineThickness);
        auto arcRadius = arcBounds.getWidth() * 0.5;
        auto arcWidth = (arcRadius - lineThickness) / arcRadius;
        Path rangeArc;
        rangeArc.addPieSegment(arcBounds, startAngle, endAngle, arcWidth);
        g.fillPath(rangeArc);

        // draw arc
        if (drawArc) {
            Path arc;
            arc.addPieSegment(arcBounds, startAngle, angle, arcWidth);
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

    struct t_fake_knob {
        t_object x_obj;
        t_glist* x_glist;
        int x_size;
        t_float x_pos; // 0-1 normalized position
        t_float x_exp;
        int x_expmode;
        t_float x_init;
        int x_start_angle;
        int x_end_angle;
        int x_range;
        int x_offset;
        int x_ticks;
        double x_min;
        double x_max;
        int x_sel;
        int x_shift;
        t_float x_fval;
        t_symbol* x_fg;
        t_symbol* x_bg;
        t_symbol* x_snd;
        t_symbol* x_rcv;
        int x_circular;
        int x_arc;
        int x_zoom;
        int x_discrete;
        char x_tag_obj[128];
        char x_tag_circle[128];
        char x_tag_arc[128];
        char x_tag_center[128];
        char x_tag_wiper[128];
        char x_tag_wpr_c[128];
        char x_tag_ticks[128];
        char x_tag_outline[128];
        char x_tag_in[128];
        char x_tag_out[128];
        char x_tag_sel[128];
    };

    Knob knob;

    Value min = Value(0.0f);
    Value max = Value(0.0f);

    Value initialValue, circular, ticks, angularRange, angularOffset, discrete, showArc, exponential;
    Value primaryColour, secondaryColour, sendSymbol, receiveSymbol;

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
    }

    void updateDoubleClickValue()
    {
        auto val = jmap<float>(::getValue<int>(initialValue), getMinimum(), getMaximum(), 0.0f, 1.0f);
        knob.setDoubleClickReturnValue(true, val);
    }

    void setCircular(Slider::SliderStyle style)
    {
        static_cast<t_fake_knob*>(ptr)->x_circular = style == Slider::SliderStyle::RotaryHorizontalVerticalDrag;
    }

    bool isCircular()
    {
        return static_cast<t_fake_knob*>(ptr)->x_circular;
    }

    void lookAndFeelChanged() override
    {
        knob.setOutlineColour(object->findColour(PlugDataColour::outlineColourId));
    }

    void update() override
    {
        auto currentValue = getValue();
        value = currentValue;
        knob.setValue(currentValue, dontSendNotification);

        auto* knb = static_cast<t_fake_knob*>(ptr);
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
        min = getMinimum();
        max = getMaximum();
        updateRange();
        updateDoubleClickValue();

        sendSymbol = getSendSymbol();
        receiveSymbol = getReceiveSymbol();

        knob.setFgColour(getForegroundColour());

        updateRotaryParameters();

        updateDoubleClickValue();
        knob.setOutlineColour(object->findColour(PlugDataColour::outlineColourId));
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
        pd->lockAudioThread();
        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        pd->unlockAudioThread();

        return Rectangle<int>(x, y, w + 1, h + 1);
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        auto* knb = static_cast<t_fake_knob*>(ptr);
        knb->x_obj.te_xpix = b.getX();
        knb->x_obj.te_ypix = b.getY();

        knb->x_size = b.getWidth() - 1;
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
            hash("exp"),
            hash("discrete"),
            hash("arc"),
            hash("angle"),
            hash("ticks"),
            hash("send"),
            hash("receive"),
            hash("color")
        };
    }

    void receiveObjectMessage(hash32 const& symbol, std::vector<pd::Atom>& atoms) override
    {
        auto setColour = [this](Value& targetValue, pd::Atom& atom) {
            if (atom.isSymbol()) {
                auto colour = "#FF" + atom.getSymbol().fromFirstOccurrenceOf("#", false, false);
                setParameterExcludingListener(targetValue, colour);
            }
        };

        switch (symbol) {
        case hash("float"):
        case hash("set"): {
            knob.setValue(getValue(), dontSendNotification);
            break;
        }
        case hash("range"): {
            if (atoms.size() >= 2) {
                setParameterExcludingListener(min, atoms[0].getFloat());
                setParameterExcludingListener(max, atoms[1].getFloat());
                updateRange();
                updateDoubleClickValue();
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
            exponential = atoms[0].getFloat();
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
        case hash("color"): {
            if (atoms.size() > 0)
                setColour(secondaryColour, atoms[0]);
            if (atoms.size() > 1)
                setColour(primaryColour, atoms[1]);
            repaint();
            break;
        }
        }

        // Update the colours of the actual slider
        if (symbol == hash("color")) {
            knob.setFgColour(Colour::fromString(primaryColour.toString()));
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(Colour::fromString(secondaryColour.toString()));
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

    bool hasSendSymbol()
    {
        return !getReceiveSymbol().isEmpty();
    }

    bool hasReceiveSymbol()
    {
        return !getReceiveSymbol().isEmpty();
    }

    String getSendSymbol()
    {
        pd->setThis();

        auto* knb = static_cast<t_fake_knob*>(ptr);

        if (!knb->x_snd || !knb->x_snd->s_name)
            return "";

        auto sym = String::fromUTF8(knb->x_snd->s_name);
        if (sym != "empty") {
            return sym;
        }

        return "";
    }

    String getReceiveSymbol()
    {
        pd->setThis();

        auto* knb = static_cast<t_fake_knob*>(ptr);

        if (!knb->x_rcv || !knb->x_rcv->s_name)
            return "";

        auto sym = String::fromUTF8(knb->x_rcv->s_name);
        if (sym != "empty") {
            return sym;
        }

        return "";
    }

    void setSendSymbol(String const& symbol) const
    {
        pd->enqueueDirectMessages(ptr, "send", { pd::Atom(symbol) });
    }

    void setReceiveSymbol(String const& symbol) const
    {
        pd->enqueueDirectMessages(ptr, "receive", { pd::Atom(symbol) });
    }

    Colour getBackgroundColour() const
    {
        auto* bg = static_cast<t_fake_knob*>(ptr)->x_bg;
        return Colour::fromString(String::fromUTF8(bg->s_name).replace("#", "ff"));
    }

    Colour getForegroundColour() const
    {
        auto* fg = static_cast<t_fake_knob*>(ptr)->x_fg;
        return Colour::fromString(String::fromUTF8(fg->s_name).replace("#", "ff"));
    }

    ObjectParameters getParameters() override
    {
        return {
            { "Minimum", tFloat, cGeneral, &min, {} },
            { "Maximum", tFloat, cGeneral, &max, {} },

            { "Initial value", tFloat, cGeneral, &initialValue, {} },
            { "Circular drag", tBool, cGeneral, &circular, { "No", "Yes" } },
            { "Ticks", tInt, cGeneral, &ticks, {} },
            { "Discrete", tBool, cGeneral, &discrete, { "No", "Yes" } },

            { "Angular range", tInt, cGeneral, &angularRange, {} },
            { "Angular offset", tInt, cGeneral, &angularOffset, {} },

            { "Exp", tFloat, cGeneral, &exponential, {} },

            { "Foreground", tColour, cAppearance, &primaryColour, {} },
            { "Background", tColour, cAppearance, &secondaryColour, {} },
            { "Show arc", tBool, cAppearance, &showArc, { "No", "Yes" } },
            { "Receive symbol", tString, cGeneral, &receiveSymbol, {} },
            { "Send symbol", tString, cGeneral, &sendSymbol, {} },
        };
    }

    float getValue()
    {
        pd->lockAudioThread();
        auto* knb = static_cast<t_fake_knob*>(ptr);
        auto pos = knb->x_pos;
        pd->unlockAudioThread();

        return pos;
    }

    float getMinimum()
    {
        return static_cast<t_fake_knob*>(ptr)->x_min;
    }

    float getMaximum()
    {
        return static_cast<t_fake_knob*>(ptr)->x_max;
    }

    void setMinimum(float value)
    {
        static_cast<t_fake_knob*>(ptr)->x_min = value;
    }

    void setMaximum(float value)
    {
        static_cast<t_fake_knob*>(ptr)->x_max = value;
    }

    void updateRotaryParameters()
    {
        auto* knb = static_cast<t_fake_knob*>(ptr);

        // For some reason, we need to compensate slightly to make 180 be exactly straight
        float startRad = degreesToRadians<float>(180.0f - knb->x_start_angle);
        float endRad = degreesToRadians<float>(180.0f - knb->x_end_angle);
        knob.setRotaryParameters({ startRad, endRad, true });
        knob.setNumberOfTicks(knb->x_ticks);
        knob.repaint();
    }

    void valueChanged(Value& value) override
    {
        auto* knb = static_cast<t_fake_knob*>(ptr);

        if (value.refersToSameSourceAs(min)) {
            setMinimum(::getValue<float>(min));
            updateRange();
            updateDoubleClickValue();
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(::getValue<float>(max));
            updateRange();
            updateDoubleClickValue();
        } else if (value.refersToSameSourceAs(initialValue)) {
            updateDoubleClickValue();
            knb->x_init = ::getValue<int>(initialValue);
        } else if (value.refersToSameSourceAs(circular)) {
            auto mode = ::getValue<int>(circular);
            knob.setSliderStyle(mode ? Slider::Rotary : Slider::RotaryHorizontalVerticalDrag);
            knb->x_circular = mode;
        } else if (value.refersToSameSourceAs(ticks)) {
            ticks = jmax(::getValue<int>(ticks), 0);
            knb->x_ticks = ::getValue<int>(ticks);
            updateRotaryParameters();
            updateRange();
        } else if (value.refersToSameSourceAs(angularRange)) {
            auto start = ::getValue<int>(angularRange);
            auto end = ::getValue<int>(angularOffset);
            pd->enqueueDirectMessages(knb, "angle", { pd::Atom(start), pd::Atom(end) });
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(showArc)) {
            bool arc = ::getValue<bool>(showArc);
            knob.showArc(arc);
            knb->x_arc = arc;
        } else if (value.refersToSameSourceAs(angularOffset)) {
            auto start = ::getValue<int>(angularRange);
            auto end = ::getValue<int>(angularOffset);
            pd->enqueueDirectMessages(knb, "angle", { pd::Atom(start), pd::Atom(end) });
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(discrete)) {
            knb->x_discrete = ::getValue<bool>(discrete);
            updateRange();
        } else if (value.refersToSameSourceAs(exponential)) {
            knb->x_exp = ::getValue<bool>(exponential);
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto colour = "#" + primaryColour.toString().substring(2);
            knb->x_fg = pd->generateSymbol(colour);
            knob.setFgColour(Colour::fromString(primaryColour.toString()));
            repaint();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto colour = "#" + secondaryColour.toString().substring(2);
            knb->x_bg = pd->generateSymbol(colour);
            repaint();
        }
    }

    void setValue(float v)
    {
        auto* knb = static_cast<t_fake_knob*>(ptr);

        knb->x_pos = v;

        t_float fval;
        t_float pos = (knb->x_pos - 0.01f) / (1 - 2 * 0.01f);
        if (pos < 0.0)
            pos = 0.0;
        else if (pos > 1.0)
            pos = 1.0;
        if (knb->x_discrete) {
            t_float ticks = (knb->x_ticks < 2 ? 2 : (float)knb->x_ticks) - 1;
            pos = rint(pos * ticks) / ticks;
        }
        if (knb->x_exp == 1) { // log
            if ((knb->x_min <= 0 && knb->x_max >= 0) || (knb->x_min >= 0 && knb->x_max <= 0)) {
                pd_error(knb, "[knob]: range cannot contain '0' in log mode");
                fval = knb->x_min;
            } else
                fval = exp(pos * log(knb->x_max / knb->x_min)) * knb->x_min;
        } else {
            if (knb->x_exp != 0) {
                if (knb->x_exp > 0)
                    pos = pow(pos, knb->x_exp);
                else
                    pos = 1 - pow(1 - pos, -knb->x_exp);
            }
            fval = pos * (knb->x_max - knb->x_min) + knb->x_min;
        }
        if ((fval < 1.0e-10) && (fval > -1.0e-10))
            fval = 0.0;

        sendFloatValue(fval);
    }
};
