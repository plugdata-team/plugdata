/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "TclColours.h"
#include "Components/PropertiesPanel.h"

extern "C" {
void knob_get_snd(void* x);
void knob_get_rcv(void* x);
t_float knob_getfval(void* x);
}

class Knob final : public Component
    , public NVGComponent {

    Colour fgColour;
    Colour arcColour;

    bool drawArc : 1 = true;
    bool shiftIsDown : 1 = false;
    bool jumpOnClick : 1 = false;
    bool isInverted : 1 = false;
    bool isCircular : 1 = false;
    bool readOnly : 1 = false;
    int numberOfTicks = 0;
    float arcStart = 63.5f;

    float value = 0.0f; // Default knob value
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float mouseDragSensitivity = 200.f;
    float originalValue = 0.0f;
    float arcBegin = 3.927, arcEnd = 8.639;
    float doubleClickValue = 0.0f;
    float interval = 0.0f;

public:
    std::function<void()> onDragStart, onDragEnd;
    std::function<void()> onValueChange;

    Knob()
        : NVGComponent(this)
    {
    }

    ~Knob() override = default;

    void drawTicks(NVGcontext* nvg, Rectangle<float> const knobBounds, float const startAngle, float const endAngle, float const tickWidth) const
    {
        auto const centre = knobBounds.getCentre();
        auto const radius = knobBounds.getWidth() * 0.5f * 1.05f;

        // Calculate the angle between each tick
        float const angleIncrement = (endAngle - startAngle) / static_cast<float>(jmax(numberOfTicks - 1, 1));

        // Position each tick around the larger circle
        float const tickRadius = tickWidth * 0.33f;
        for (int i = 0; i < numberOfTicks; ++i) {
            float const angle = startAngle + i * angleIncrement;
            float const x = centre.getX() + radius * std::cos(angle);
            float const y = centre.getY() + radius * std::sin(angle);

            // Draw the tick at this position
            nvgBeginPath(nvg);
            nvgCircle(nvg, x, y, tickRadius);
            nvgFillColor(nvg, convertColour(fgColour));
            nvgFill(nvg);
        }
    }

    void mouseDown(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown() || readOnly)
            return;

        constexpr auto normalSensitivity = 250;
        constexpr auto highSensitivity = normalSensitivity * 10;

        if (ModifierKeys::getCurrentModifiersRealtime().isShiftDown()) {
            mouseDragSensitivity = highSensitivity;
            shiftIsDown = true;
        } else {
            mouseDragSensitivity = normalSensitivity;
        }

        originalValue = getValue();
        if (jumpOnClick) {
            mouseDrag(e);
        }

        onDragStart();
        onValueChange(); // else/knob always outputs on mouseDown
    }

    void mouseDrag(MouseEvent const& e) override
    {
        if (!e.mods.isLeftButtonDown() || readOnly)
            return;

        float const delta = e.getDistanceFromDragStartY() - e.getDistanceFromDragStartX();
        bool const jumpMouseDownEvent = jumpOnClick && !e.mouseWasDraggedSinceMouseDown();
        bool valueChanged = false;

        if (isCircular || jumpMouseDownEvent) {
            float const dx = e.position.x - getLocalBounds().getCentreX();
            float const dy = e.position.y - getLocalBounds().getCentreY();
            float angle = std::atan2(dx, -dy);
            while (angle < 0.0 || angle < arcBegin)
                angle += MathConstants<double>::twoPi;

            if (isCircular) {
                auto smallestAngleBetween = [](double const a1, double const a2) {
                    return jmin(std::abs(a1 - a2),
                        std::abs(a1 + MathConstants<double>::twoPi - a2),
                        std::abs(a2 + MathConstants<double>::twoPi - a1));
                };

                if (angle > arcEnd) {
                    if (smallestAngleBetween(angle, arcBegin)
                        <= smallestAngleBetween(angle, arcEnd))
                        angle = arcBegin;
                    else
                        angle = arcEnd;
                }
            }

            float const rangeSize = maxValue - minValue;
            float const normalizedAngle = (angle - arcBegin) / (arcEnd - arcBegin);
            float newValue = minValue + normalizedAngle * rangeSize;

            newValue = std::ceil(newValue / interval) * interval;
            if (jumpMouseDownEvent)
                originalValue = newValue;
            valueChanged = !approximatelyEqual(newValue, value);
            setValue(newValue);
        } else {
            float newValue = originalValue - delta / mouseDragSensitivity;
            newValue = std::ceil(newValue / interval) * interval;
            newValue = std::clamp(newValue, minValue, maxValue);
            valueChanged = !approximatelyEqual(newValue, value);
            setValue(newValue);
        }

        if (valueChanged)
            onValueChange();
    }
    
    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        bool valueChanged = false;
        float newValue = originalValue - wheel.deltaY;
        newValue = std::ceil(newValue / interval) * interval;
        newValue = std::clamp(newValue, minValue, maxValue);
        valueChanged = !approximatelyEqual(newValue, value);
        setValue(newValue);
        
        if (valueChanged) {
            originalValue = newValue;
            onValueChange();
        }
    }


    void mouseUp(MouseEvent const& e) override
    {
        mouseDragSensitivity = 250;
        shiftIsDown = false;
        onDragEnd();
    }

    void showArc(bool const show)
    {
        drawArc = show;
        repaint();
    }

    void setArcStart(float const newArcStart)
    {
        arcStart = newArcStart;
    }

    void setRangeFlipped(bool const invert)
    {
        isInverted = invert;
    }

    bool isRangeFlipped() const
    {
        return isInverted;
    }

    void render(NVGcontext* nvg) override
    {
        auto const bounds = getLocalBounds().toFloat().reduced(getWidth() * 0.14f);

        auto const lineThickness = std::max(bounds.getWidth() * 0.09f, 1.5f);

        auto const sliderPosProportional = getValue();

        auto startAngle = arcBegin - MathConstants<float>::pi * 0.5f;
        auto const endAngle = arcEnd - MathConstants<float>::pi * 0.5f;

        auto const angle = jmap<float>(sliderPosProportional, startAngle, endAngle);
        auto const centre = jmap<double>(arcStart, startAngle, endAngle);

        startAngle = std::clamp<float>(startAngle, endAngle - MathConstants<float>::twoPi, endAngle + MathConstants<float>::twoPi);

        if (drawArc) {
            auto const arcBounds = bounds.reduced(lineThickness);
            auto const arcRadius = arcBounds.getWidth() * 0.5;
            auto const arcWidth = (arcRadius - lineThickness) / arcRadius;

            nvgBeginPath(nvg);
            nvgArc(nvg, bounds.getCentreX(), bounds.getCentreY(), arcRadius, startAngle, endAngle, NVG_HOLE);
            nvgStrokeWidth(nvg, arcWidth * lineThickness);
            nvgStrokeColor(nvg, convertColour(arcColour));
            nvgStroke(nvg);

            nvgBeginPath(nvg);
            if (centre < angle) {
                nvgArc(nvg, bounds.getCentreX(), bounds.getCentreY(), arcRadius, centre, angle, NVG_HOLE);
            } else {
                nvgArc(nvg, bounds.getCentreX(), bounds.getCentreY(), arcRadius, angle, centre, NVG_HOLE);
            }
            nvgStrokeColor(nvg, convertColour(fgColour));
            nvgStrokeWidth(nvg, arcWidth * lineThickness);
            nvgStroke(nvg);
        }

        float const wiperX = bounds.getCentreX() + bounds.getWidth() * 0.4f * std::cos(angle);
        float const wiperY = bounds.getCentreY() + bounds.getWidth() * 0.4f * std::sin(angle);

        // draw wiper
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, bounds.getCentreX(), bounds.getCentreY()); // Adjust parameters as needed
        nvgLineTo(nvg, wiperX, wiperY);                           // Adjust parameters as needed
        nvgStrokeWidth(nvg, lineThickness);
        nvgStrokeColor(nvg, convertColour(fgColour));
        nvgLineCap(nvg, NVG_ROUND);
        nvgStroke(nvg);

        drawTicks(nvg, bounds, startAngle, endAngle, lineThickness);
    }

    void setFgColour(Colour const newFgColour)
    {
        fgColour = newFgColour;
        repaint();
    }

    void setArcColour(Colour const newOutlineColour)
    {
        arcColour = newOutlineColour;
        repaint();
    }

    void setNumberOfTicks(int const steps)
    {
        numberOfTicks = steps;
        repaint();
    }

    void doubleClicked()
    {
        setValue(std::clamp(doubleClickValue, minValue, maxValue));
    }

    float getValue() const { return value; }

    void setValue(float const newValue)
    {
        value = newValue;
        repaint();
    }

    void setRotaryParameters(float const start, float const end)
    {
        arcBegin = start;
        arcEnd = end;
    }

    void setJumpOnClick(bool const snap)
    {
        jumpOnClick = snap;
    }

    void setDoubleClickValue(float const newDoubleClickValue)
    {
        doubleClickValue = newDoubleClickValue;
    }

    void setInterval(float const newInterval)
    {
        interval = newInterval;
    }

    void setCircular(bool const newCircular)
    {
        isCircular = newCircular;
    }

    void setReadOnly(bool const newReadOnly)
    {
        readOnly = newReadOnly;
    }
};

class KnobObject final : public ObjectBase {

    Knob knob;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);

    Value initialValue = SynchronousValue();
    Value circular = SynchronousValue();
    Value showTicks = SynchronousValue();
    Value steps = SynchronousValue();
    Value angularRange = SynchronousValue();
    Value angularOffset = SynchronousValue();
    Value discrete = SynchronousValue();
    Value square = SynchronousValue();
    Value showArc = SynchronousValue();
    Value exponential = SynchronousValue();
    Value logMode = SynchronousValue();
    Value primaryColour = SynchronousValue();
    Value secondaryColour = SynchronousValue();
    Value arcColour = SynchronousValue();
    Value sendSymbol = SynchronousValue();
    Value receiveSymbol = SynchronousValue();
    Value arcStart = SynchronousValue();
    Value readOnly = SynchronousValue();
    Value jumpOnClick = SynchronousValue();
    Value parameterName = SynchronousValue();
    Value variableName = SynchronousValue();

    Value showNumber = SynchronousValue();
    Value numberSize = SynchronousValue();
    Value numberPosition = SynchronousValue();

    Value sizeProperty = SynchronousValue();
    Value transparent = SynchronousValue();

    NVGcolor bgCol;

    String typeBuffer;

    bool locked;
    float value = 0.0f;

public:
    KnobObject(pd::WeakReference obj, Object* object)
        : ObjectBase(obj, object)
    {
        addAndMakeVisible(knob);

        knob.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);

        knob.onDragStart = [this] {
            startEdition();
            float const val = knob.getValue();
            setValue(val, false);
        };

        knob.onValueChange = [this] {
            float const val = knob.getValue();
            setValue(val, true);
        };

        knob.onDragEnd = [this] {
            stopEdition();
        };

        knob.addMouseListener(this, false);

        locked = ::getValue<bool>(object->locked);

        objectParameters.addParamSize(&sizeProperty, true);
        objectParameters.addParamFloat("Minimum", cGeneral, &min, 0.0f);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 127.0f);
        objectParameters.addParamFloat("Initial value", cGeneral, &initialValue, 0.0f);
        objectParameters.addParamInt("Angular range", cGeneral, &angularRange, 270, true, 0, 360);
        objectParameters.addParamInt("Angular offset", cGeneral, &angularOffset, 0, true, 0, 360);
        objectParameters.addParamFloat("Arc start", cGeneral, &arcStart, 0.0f);
        objectParameters.addParamCombo("Log mode", cGeneral, &logMode, { "Linear", "Logarithmic", "Exponential" }, 0);
        objectParameters.addParamFloat("Exp factor", cGeneral, &exponential, 0.0f);
        objectParameters.addParamBool("Discrete", cGeneral, &discrete, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Show ticks", cGeneral, &showTicks, { "No", "Yes" }, 0);
        objectParameters.addParamInt("Steps", cGeneral, &steps, 0, true, 0);
        objectParameters.addParamBool("Circular drag", cGeneral, &circular, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Read only", cGeneral, &readOnly, { "No", "Yes" }, 0);
        objectParameters.addParamBool("Jump on click", cGeneral, &jumpOnClick, { "No", "Yes" }, 0);

        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);
        objectParameters.addParamString("Variable", cGeneral, &variableName, "");
        objectParameters.addParamString("Parameter", cGeneral, &parameterName, "");

        objectParameters.addParamCombo("Show number", cLabel, &showNumber, { "Never", "Always", "When active", "When typing" }, 0);
        objectParameters.addParamInt("Size", cLabel, &numberSize, 3, true, 8);
        objectParameters.addParamRangeInt("Position", cLabel, &numberPosition, { 6, -15 });

        objectParameters.addParamColourFG(&primaryColour);
        objectParameters.addParamColourBG(&secondaryColour);

        objectParameters.addParamColour("Arc", cAppearance, &arcColour, PlugDataColour::guiObjectInternalOutlineColour);
        objectParameters.addParamBool("Square", cAppearance, &square, { "No", "Yes" }, 1);
        objectParameters.addParamBool("Show arc", cAppearance, &showArc, { "No", "Yes" }, 1);
        objectParameters.addParamBool("Transparent", cAppearance, &transparent, { "No", "Yes" }, 0);
    }

    void onConstrainerCreate() override
    {
        constrainer->setFixedAspectRatio(1.0f);
        constrainer->setMinimumSize(17, 17);
    }
    
    ResizeDirection getAllowedResizeDirections() const override
    {
        return DiagonalOnly;
    }

    bool canReceiveMouseEvent(int const x, int const y) override
    {
        if (square.getValue() || !locked)
            return true;

        // If knob is circular limit hit test to circle, and expand more if there are ticks around the knob
        auto const hitPoint = getLocalPoint(object, Point<float>(x, y));
        auto const centre = getLocalBounds().toFloat().getCentre();
        auto const knobRadius = getWidth() * 0.45f;
        auto const knobRadiusWithTicks = knobRadius + getWidth() * 0.06f;
        if (centre.getDistanceFrom(hitPoint) < (steps.getValue() ? knobRadiusWithTicks : knobRadius)) {
            return true;
        }

        return false;
    }

    bool isTransparent() override
    {
        return !::getValue<bool>(square);
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (!locked)
            return false;

        if (key.getKeyCode() == KeyPress::upKey || key.getKeyCode() == KeyPress::rightKey) {
            if (auto knob = ptr.get<t_fake_knob>()) {
                knob->x_clicked = 1;
                pd->sendDirectMessage(knob.cast<void>(), "list", { pd::Atom(1.0f), pd::Atom(gensym("Up")) });
                knob->x_clicked = 0;
            }
            return true;
        }
        if (key.getKeyCode() == KeyPress::downKey || key.getKeyCode() == KeyPress::leftKey) {
            if (auto knob = ptr.get<t_fake_knob>()) {
                knob->x_clicked = 1;
                pd->sendDirectMessage(knob.cast<void>(), "list", { pd::Atom(1.0f), pd::Atom(gensym("Down")) });
                knob->x_clicked = 0;
            }
            return true;
        }
        if (key.getKeyCode() == KeyPress::backspaceKey) {
            typeBuffer = typeBuffer.substring(0, typeBuffer.length() - 1);
            return true;
        }
        if (key.getKeyCode() == KeyPress::returnKey) {
            if (auto obj = ptr.get<t_fake_knob>()) {
                auto const value = typeBuffer.isEmpty() ? getValue() : typeBuffer.getFloatValue();
                pd->sendDirectMessage(obj.get(), value);
                typeBuffer = "";
            }
            return true;
        }
        auto const chr = key.getTextCharacter();
        if ((chr >= '0' && chr <= '9') || chr == '+' || chr == '-' || chr == '.') {
            typeBuffer += chr;
            updateLabel();
            return true;
        }

        return false;
    }

    void updateDoubleClickValue()
    {
        auto const min = std::min(getMinimum(), getMaximum());
        auto max = std::max(getMinimum(), getMaximum());
        if (min == max)
            max += 0.001;
        auto const val = jmap<float>(::getValue<float>(initialValue), min, max, 0.0f, 1.0f);
        knob.setDoubleClickValue(std::clamp(val, 0.0f, 1.0f));
        knob.setArcStart(jmap<float>(::getValue<float>(arcStart), min, max, 0.0f, 1.0f));
        knob.repaint();
    }

    void update() override
    {
        auto const currentValue = getValue();
        value = currentValue;
        knob.setValue(currentValue);

        if (auto knb = ptr.get<t_fake_knob>()) {
            initialValue = knb->x_load;
            steps = knb->x_steps;
            showTicks = knb->x_ticks;
            angularRange = knb->x_angle_range;
            angularOffset = knb->x_angle_offset;
            discrete = knb->x_discrete;
            circular = knb->x_circular;
            showArc = knb->x_arc;
            exponential = knb->x_exp;
            logMode = knb->x_expmode + 1;
            primaryColour = getForegroundColour().toString();
            secondaryColour = getBackgroundColour().toString();
            transparent = knb->x_transparent;
            arcColour = getArcColour().toString();
            square = knb->x_square;
            sizeProperty = knb->x_size;
            arcStart = knb->x_arcstart;
            numberSize = knb->n_size;
            readOnly = knb->x_readonly;
            jumpOnClick = knb->x_jump;

            showNumber = knb->x_number_mode + 1;
            numberPosition = VarArray(knb->x_xpos, knb->x_ypos);
            auto varName = knb->x_var_raw ? String::fromUTF8(knb->x_var_raw->s_name) : String("");
            if (varName == "empty")
                varName = "";
            variableName = varName;

            auto paramName = knb->x_param ? String::fromUTF8(knb->x_param->s_name) : String("");
            if (paramName == "empty")
                paramName = "";
            parameterName = paramName;
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
        knob.setCircular(::getValue<bool>(circular));
        knob.showArc(::getValue<bool>(showArc));
        knob.setJumpOnClick(::getValue<bool>(jumpOnClick));
        knob.setReadOnly(::getValue<bool>(readOnly));

        updateColours();
    }

    bool hideInlet() override
    {
        return hasReceiveSymbol();
    }

    bool hideOutlet() override
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
            auto* patch = cnv->patch.getRawPointer();

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
        auto const numTicks = std::max(::getValue<int>(steps) - 1, 1);
        auto const interval = ::getValue<bool>(discrete) ? 1.0 / numTicks : std::numeric_limits<double>::epsilon();
        if (::getValue<float>(min) == ::getValue<float>(max)) {
            max = ::getValue<float>(max) + 0.001f;
        }

        knob.setInterval(interval);
        knob.setRangeFlipped(!approximatelyEqual(min, max) && min > max);
        auto clampedValue = std::clamp(knob.getValue(), 0.0f, 1.0f);
        if (!std::isfinite(clampedValue))
            clampedValue = 0.0f;
        if (clampedValue != getValue()) {
            setValue(clampedValue, false);
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"):
        case hash("inc"):
        case hash("dec"):
        case hash("shift"):
        {
            knob.setValue(getValue());
            updateLabel();
            break;
        }
        case hash("range"): {
            if (atoms.size() >= 2) {
                updateRange();
                updateDoubleClickValue();
                knob.setValue(getValue());
                updateLabel();
            }
            break;
        }
        case hash("angle"): {
            if (atoms.size()) {
                auto const range = std::clamp<int>(atoms[0].getFloat(), 0, 360);
                setParameterExcludingListener(angularRange, range);
                updateRotaryParameters();
            }
            break;
        }
        case hash("offset"): {
            if (atoms.size()) {
                auto const offset = std::clamp<int>(atoms[0].getFloat(), -180, 180);
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
            knob.setCircular(atoms[0].getFloat());
            break;
        }
        case hash("send"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(sendSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].toString());
            object->updateIolets();
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
        case hash("colors"): {
            primaryColour = getForegroundColour().toString();
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
                knob.setValue(getValue());
            }
            break;
        }
        case hash("square"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                square = atoms[0].getFloat();
            }
            break;
        }
        case hash("readonly"): {
            if (atoms.size() > 0 && atoms[0].isFloat())
                readOnly = atoms[0].getFloat();
            break;
        }
        case hash("number"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                setParameterExcludingListener(showNumber, static_cast<int>(atoms[0].getFloat()) + 1);
                updateLabel();
            }
            break;
        }
        case hash("numbersize"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                setParameterExcludingListener(numberSize, static_cast<int>(atoms[0].getFloat()));
                updateLabel();
            }
            break;
        }
        case hash("numberpos"): {
            if (atoms.size() > 1 && atoms[0].isFloat() && atoms[1].isFloat()) {
                setParameterExcludingListener(numberPosition, VarArray { atoms[0].getFloat(), atoms[1].getFloat() });
                updateLabel();
            }
            break;
        }
        case hash("active"): {
            if (atoms.size() >= 1) {
                if (atoms[0].getFloat()) {
                    grabKeyboardFocus();
                } else {
                    cnv->grabKeyboardFocus();
                }
            }
            break;
        }
        case hash("jump"): {
            if (atoms.size() >= 1) {
                setParameterExcludingListener(jumpOnClick, atoms[0].getFloat());
                knob.setJumpOnClick(atoms[0].getFloat());
            }
            break;
        }
        case hash("arcstart"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                setParameterExcludingListener(arcStart, atoms[0].getFloat());
                knob.setArcStart(atoms[0].getFloat());
            }
            break;
        }
        case hash("var"): {
            if (atoms.size() > 0 && atoms[0].isSymbol()) {
                auto sym = atoms[0].toString();
                if (sym == "empty")
                    sym = "";
                setParameterExcludingListener(variableName, sym);
            }
            break;
        }
        case hash("param"): {
            if (atoms.size() > 0 && atoms[0].isSymbol()) {
                auto sym = atoms[0].toString();
                if (sym == "empty")
                    sym = "";
                setParameterExcludingListener(parameterName, sym);
            }
            break;
        }
        case hash("exp"): {
            if (atoms.size() >= 1) {
                setParameterExcludingListener(exponential, atoms[0].getFloat());
            }
            break;
        }
        case hash("log"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                setParameterExcludingListener(logMode, atoms[0].getFloat() + 1);
            }
            break;
        }
        case hash("ticks"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                setParameterExcludingListener(showTicks, static_cast<bool>(atoms[0].getFloat()));
                setParameterExcludingListener(steps, static_cast<int>(atoms[0].getFloat()));
                updateRotaryParameters();
                updateRange();
            }
            break;
        }
        case hash("transparent"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                transparent = atoms[0].getFloat();
                repaint();
            }
            break;
        }
        default:
            break;
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        
        auto const background = ::getValue<bool>(transparent) ? nvgRGBA(0, 0, 0, 0) : bgCol;
        if (::getValue<bool>(square)) {
            bool const selected = object->isSelected() && !cnv->isGraph;
            auto const outlineColour = selected ? cnv->selectedOutlineCol : cnv->objectOutlineCol;
            auto const lineThickness = std::max(b.getWidth() * 0.03f, 1.0f);

            nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), background, outlineColour, Corners::objectCornerRadius);

            if (!::getValue<bool>(showArc)) {
                nvgBeginPath(nvg);
                nvgStrokeWidth(nvg, lineThickness);
                nvgStrokeColor(nvg, convertColour(::getValue<Colour>(arcColour)));
                nvgCircle(nvg, b.getCentreX(), b.getCentreY(), b.getWidth() / 2.7f);
                nvgStroke(nvg);
            }

            knob.render(nvg);
        } else {
            auto circleBounds = getLocalBounds().toFloat().reduced(getWidth() * 0.13f);
            auto const lineThickness = std::max(circleBounds.getWidth() * 0.07f, 1.5f);
            circleBounds = circleBounds.reduced(lineThickness - 0.5f);

            NVGScopedState state(nvg);
            float constexpr scaleFactor = 1.3f;
            auto const originalCentre = circleBounds.getCentre();
            float const scaleOffsetX = originalCentre.x * (1.0f - scaleFactor);
            float const scaleOffsetY = originalCentre.y * (1.0f - scaleFactor);

            nvgTranslate(nvg, scaleOffsetX, scaleOffsetY);
            nvgScale(nvg, scaleFactor, scaleFactor);

            nvgFillColor(nvg, background);
            nvgBeginPath(nvg);
            nvgCircle(nvg, circleBounds.getCentreX(), circleBounds.getCentreY(), circleBounds.getWidth() / 2.0f);
            nvgFill(nvg);

            nvgStrokeColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(objectOutlineColourId)));
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);

            knob.render(nvg);
        }
    }

    void resized() override
    {
        knob.setBounds(getLocalBounds());
    }

    bool hasSendSymbol() const
    {
        return !getSendSymbol().isEmpty();
    }

    bool hasReceiveSymbol() const
    {
        return !getReceiveSymbol().isEmpty();
    }

    String getSendSymbol() const
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            knob_get_snd(knb.get()); // get unexpanded send symbol from binbuf

            if (!knb->x_snd_raw || !knb->x_snd_raw->s_name)
                return "";

            auto sym = String::fromUTF8(knb->x_snd_raw->s_name);
            if (sym != "empty") {
                return sym.replace("\\ ", " ");
            }
        }

        return "";
    }

    String getReceiveSymbol() const
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            knob_get_rcv(knb.get()); // get unexpanded receive symbol from binbuf

            if (!knb->x_rcv_raw || !knb->x_rcv_raw->s_name)
                return "";

            auto sym = String::fromUTF8(knb->x_rcv_raw->s_name);
            if (sym != "empty") {
                return sym.replace("\\ ", " ");
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

    void updateLabel() override
    {
        ObjectLabel* label = nullptr;
        if (labels.isEmpty()) {
            label = labels.add(new ObjectLabel());
            object->cnv->addChildComponent(label);
        } else {
            label = labels[0];
        }

        if (label) {
            auto const& arr = *numberPosition.getValue().getArray();
            auto const height = ::getValue<int>(numberSize);
            auto const font = Font(FontOptions(height));
            auto const labelText = String(getScaledValue(), 2);
            auto const width = Fonts::getStringWidth(labelText, font);
            auto const bounds = Rectangle<int>(object->getX() + 5 + static_cast<int>(arr[0]), object->getY() + 3 + static_cast<int>(arr[1]), width, height);
            label->setFont(font);
            label->setBounds(bounds);
            label->setText(typeBuffer.isEmpty() ? labelText : typeBuffer, dontSendNotification);

            auto showNumberType = ::getValue<int>(showNumber);
            if (showNumberType == 1) {
                label->setVisible(false);
            } else if (showNumberType == 2) {
                label->setVisible(true);
            } else if (showNumberType == 3) {
                label->setVisible(hasKeyboardFocus(true) && locked);
            } else if (showNumberType == 4) {
                label->setVisible(typeBuffer.isNotEmpty() && locked);
            }
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.mods.isCommandDown()) {
            if (auto knob = ptr.get<t_fake_knob>()) {
                auto const message = e.mods.isShiftDown() ? SmallString("forget") : SmallString("learn");
                pd->sendDirectMessage(knob.cast<void>(), message, {});
            }
        }
    }

    Colour getBackgroundColour() const
    {
        if (auto knob = ptr.get<t_fake_knob>()) {
            auto const bg = String::fromUTF8(knob->x_bg->s_name);
            return convertTclColour(bg);
        }

        return Colour();
    }

    Colour getForegroundColour() const
    {
        if (auto knob = ptr.get<t_fake_knob>()) {
            auto const fg = String::fromUTF8(knob->x_fg->s_name);
            return convertTclColour(fg);
        }

        return Colour();
    }

    Colour getArcColour() const
    {
        if (auto knob = ptr.get<t_fake_knob>()) {
            auto const mg = String::fromUTF8(knob->x_mg->s_name);
            return convertTclColour(mg);
        }

        return Colour();
    }

    static Colour convertTclColour(String const& colourStr)
    {
        if (tclColours.count(colourStr)) {
            return tclColours[colourStr];
        }

        return Colour::fromString(colourStr.replace("#", "ff"));
    }

    float getValue() const
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_pos;
        }

        return 0.0f;
    }

    float getScaledValue() const
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_fval;
        }

        return 0.0f;
    }

    float getMinimum() const
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_lower;
        }

        return 0.0f;
    }

    float getMaximum() const
    {
        if (auto knb = ptr.get<t_fake_knob>()) {
            return knb->x_upper;
        }

        return 127.0f;
    }

    void mouseDoubleClick(MouseEvent const& e) override
    {
        knob.doubleClicked();
        float const val = knob.getValue();
        setValue(val, true);
    }

    void updateRotaryParameters()
    {
        float startRad, endRad;
        int numTicks;
        if (auto knb = ptr.get<t_fake_knob>()) {
            startRad = degreesToRadians<float>(knb->x_arcstart_angle) + MathConstants<float>::twoPi;
            endRad = degreesToRadians<float>(knb->x_end_angle) + MathConstants<float>::twoPi;
            numTicks = knb->x_steps * knb->x_ticks;
        } else {
            return;
        }

        knob.setRotaryParameters(startRad, endRad);
        knob.setNumberOfTicks(numTicks);
        knob.repaint();
    }

    void updateColours()
    {
        bgCol = convertColour(Colour::fromString(secondaryColour.toString()));
        repaint();
    }

    float clipArcStart(float const newArcStart, float const min, float const max)
    {
        auto const clampedValue = min >= max ? min : std::clamp<float>(newArcStart, min, max);
        setParameterExcludingListener(arcStart, clampedValue);
        return clampedValue;
    }
    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const size = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            setParameterExcludingListener(sizeProperty, size);
            if (auto knob = ptr.get<t_fake_knob>())
                knob->x_size = size;

            object->updateBounds();
            knob.setValue(getValue());
            updateLabel();
        } else if (value.refersToSameSourceAs(min)) {
            // set new min value and update knob
            if (auto knb = ptr.get<t_fake_knob>())
                pd->sendDirectMessage(knb.get(), "range", { ::getValue<float>(min), static_cast<float>(knb->x_upper) });

            knob.setValue(getValue());
            updateRange();
            updateDoubleClickValue();
            updateLabel();
        } else if (value.refersToSameSourceAs(max)) {
            // set new min value and update knob
            if (auto knb = ptr.get<t_fake_knob>())
                pd->sendDirectMessage(knb.get(), "range", { static_cast<float>(knb->x_lower), ::getValue<float>(max) });

            knob.setValue(getValue());
            updateRange();
            updateDoubleClickValue();
            updateLabel();
        } else if (value.refersToSameSourceAs(initialValue)) {
            updateDoubleClickValue();
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_load = ::getValue<float>(initialValue);
        } else if (value.refersToSameSourceAs(circular)) {
            auto const mode = ::getValue<int>(circular);
            knob.setCircular(mode);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_circular = mode;
        } else if (value.refersToSameSourceAs(showTicks)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_ticks = ::getValue<int>(showTicks);
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(steps)) {
            steps = jmax(::getValue<int>(steps), 0);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_steps = ::getValue<int>(steps);
            updateRotaryParameters();
            updateRange();
        } else if (value.refersToSameSourceAs(angularRange)) {
            if (auto knb = ptr.get<t_fake_knob>())
                pd->sendDirectMessage(knb.get(), "angle", { pd::Atom(::getValue<int>(angularRange)) });
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(angularOffset)) {
            if (auto knb = ptr.get<t_fake_knob>())
                pd->sendDirectMessage(knb.get(), "offset", { pd::Atom(::getValue<int>(angularOffset)) });
            updateRotaryParameters();
        } else if (value.refersToSameSourceAs(showArc)) {
            bool const arc = ::getValue<bool>(showArc);
            knob.showArc(arc);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_arc = arc;
        } else if (value.refersToSameSourceAs(discrete)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_discrete = ::getValue<bool>(discrete);
            updateRange();
        } else if (value.refersToSameSourceAs(square)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_square = ::getValue<bool>(square);
            repaint();
        } else if (value.refersToSameSourceAs(exponential)) {
            if (auto knb = ptr.get<t_fake_knob>()) {
                if (knb->x_expmode == 2)
                    knb->x_exp = ::getValue<float>(exponential);
            }
        } else if (value.refersToSameSourceAs(logMode)) {
            if (auto knb = ptr.get<t_fake_knob>()) {
                knb->x_expmode = ::getValue<float>(logMode) - 1;
                knb->x_log = knb->x_expmode == 1;
                if (knb->x_expmode <= 1) {
                    knb->x_exp = 0;
                } else {
                    knb->x_exp = ::getValue<float>(exponential);
                }
            }
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            setSendSymbol(sendSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            setReceiveSymbol(receiveSymbol.toString());
            object->updateIolets();
        } else if (value.refersToSameSourceAs(primaryColour)) {
            auto const colour = "#" + primaryColour.toString().substring(2);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_fg = pd->generateSymbol(colour);
            knob.setFgColour(Colour::fromString(primaryColour.toString()));
            updateColours();
        } else if (value.refersToSameSourceAs(secondaryColour)) {
            auto const colour = "#" + secondaryColour.toString().substring(2);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_bg = pd->generateSymbol(colour);
            updateColours();
        } else if (value.refersToSameSourceAs(arcStart)) {
            auto const arcStartLimited = clipArcStart(::getValue<float>(arcStart), ::getValue<float>(min), ::getValue<float>(max));
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_arcstart = arcStartLimited;
            updateDoubleClickValue();
            repaint();
        } else if (value.refersToSameSourceAs(arcColour)) {
            auto const colour = "#" + arcColour.toString().substring(2);
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_mg = pd->generateSymbol(colour);
            knob.setArcColour(Colour::fromString(arcColour.toString()));
            repaint();
        } else if (value.refersToSameSourceAs(readOnly)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_readonly = ::getValue<bool>(readOnly);
            knob.setReadOnly(::getValue<bool>(readOnly));
        } else if (value.refersToSameSourceAs(jumpOnClick)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_jump = ::getValue<bool>(jumpOnClick);
            knob.setJumpOnClick(::getValue<bool>(jumpOnClick));
        } else if (value.refersToSameSourceAs(parameterName)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_param = pd->generateSymbol(parameterName.toString());
        } else if (value.refersToSameSourceAs(variableName)) {
            if (auto knb = ptr.get<t_fake_knob>()) {
                auto* s = pd->generateSymbol(variableName.toString());

                if (s == gensym(""))
                    s = gensym("empty");
                t_symbol* var = s == gensym("empty") ? gensym("") : canvas_realizedollar(knb->x_glist, s);
                if (var != knb->x_var) {
                    knb->x_var_set = 1;
                    knb->x_var_raw = s;
                    knb->x_var = var;
                }
            }
        } else if (value.refersToSameSourceAs(showNumber)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->x_number_mode = ::getValue<int>(showNumber) - 1;
            updateLabel();
        } else if (value.refersToSameSourceAs(numberSize)) {
            if (auto knb = ptr.get<t_fake_knob>())
                knb->n_size = ::getValue<int>(numberSize);
            updateLabel();
        } else if (value.refersToSameSourceAs(numberPosition)) {
            if (auto knb = ptr.get<t_fake_knob>()) {
                auto const& arr = *numberPosition.getValue().getArray();
                knb->x_xpos = static_cast<int>(arr[0]);
                knb->x_ypos = static_cast<int>(arr[1]);
            }
            updateLabel();
        }
        else if (value.refersToSameSourceAs(transparent)) {
            if (auto knb = ptr.get<t_fake_knob>()) {
                knb->x_transparent = ::getValue<bool>(transparent);
            }
            repaint();
        }
    }

    void focusGained(FocusChangeType cause) override
    {
        updateLabel();
    }

    void focusLost(FocusChangeType cause) override
    {
        updateLabel();
    }

    void lock(bool const isLocked) override
    {
        ObjectBase::lock(isLocked);
        locked = isLocked;
        repaint();
    }

    void setValue(float const pos, bool const sendNotification)
    {
        float fval = 0.0f;
        if (auto knb = ptr.get<t_fake_knob>()) {
            knb->x_pos = pos;
            fval = knob_getfval(knb.get());
            knb->x_fval = fval;
        }

        if (sendNotification) {
            sendFloatValue(fval);
        }
    }
};
