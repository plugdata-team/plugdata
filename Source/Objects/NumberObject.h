/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Components/DraggableNumber.h"
#include "ObjectBase.h"
#include "IEMHelper.h"

class NumberObject final : public ObjectBase {

    Value sizeProperty = SynchronousValue();

    DraggableNumber input;
    IEMHelper iemHelper;

    float preFocusValue;

    Value min = SynchronousValue(-std::numeric_limits<float>::infinity());
    Value max = SynchronousValue(std::numeric_limits<float>::infinity());
    Value logHeight = SynchronousValue();
    Value logMode = SynchronousValue();

    float value = 0.0f;

public:
    NumberObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , input(false)
        , iemHelper(ptr, object, this)

    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->setBorder({ 0, 8, 4, 1 });
            editor->setInputRestrictions(0, "e.-0123456789");
        };

        input.setFont(Fonts::getTabularNumbersFont().withHeight(15.5f));

        input.onEditorHide = [this]() {
            stopEdition();
        };

        input.setBorderSize({ 1, 12, 2, 2 });

        addAndMakeVisible(input);

        addMouseListener(this, true);

        input.dragStart = [this]() {
            startEdition();
        };

        input.onValueChange = [this](float newValue) {
            sendFloatValue(newValue);
        };

        input.dragEnd = [this]() {
            stopEdition();
        };

        objectParameters.addParamSize(&sizeProperty);

        objectParameters.addParamFloat("Minimum", cGeneral, &min, -9.999999933815813e36);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 9.999999933815813e36);
        objectParameters.addParamBool("Logarithmic mode", cGeneral, &logMode, { "Off", "On" }, var(false));
        objectParameters.addParamInt("Logarithmic height", cGeneral, &logHeight, var(256));

        iemHelper.addIemParameters(objectParameters);

        input.setResetValue(0.0f);
    }

    void update() override
    {
        if (input.isShowing())
            return;
                
        value = getValue();
        input.setText(input.formatNumber(value), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        input.setMinimum(::getValue<float>(min));
        input.setMaximum(::getValue<float>(max));

        if (auto nbx = ptr.get<t_my_numbox>()) {
            sizeProperty = Array<var> { var(nbx->x_gui.x_w), var(nbx->x_gui.x_h) };
            logMode = nbx->x_lin0_log1;
            logHeight = nbx->x_log_height;
        }

        iemHelper.update();
    }

    bool inletIsSymbol() override
    {
        return iemHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return iemHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        iemHelper.updateLabel(labels);
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

    void setPdBounds(Rectangle<int> b) override
    {
        if (auto nbx = ptr.get<t_my_numbox>()) {
            auto* patch = cnv->patch.getPointer().get();
            if (!patch)
                return;

            nbx->x_gui.x_w = b.getWidth() - 1;
            nbx->x_gui.x_h = b.getHeight() - 1;

            auto fontsize = nbx->x_gui.x_fontsize * 31;
            nbx->x_numwidth = (((nbx->x_gui.x_w - 4.0 - (nbx->x_gui.x_h / 2.0)) * 36.0) / fontsize) + 0.5;

            pd::Interface::moveObject(patch, nbx.cast<t_gobj>(), b.getX(), b.getY());
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto iem = ptr.get<t_iemgui>()) {
            setParameterExcludingListener(sizeProperty, Array<var> { var(iem->x_w), var(iem->x_h) });
        }
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        input.setFont(input.getFont().withHeight(getHeight() - 6));
    }

    void focusGained(FocusChangeType cause) override
    {
        preFocusValue = value;
        repaint();
    }
    
    
    bool keyPressed(KeyPress const& key) override
    {
        if(key.getKeyCode() == KeyPress::returnKey)
        {
            auto inputValue = input.getText().getFloatValue();
            preFocusValue = value;
            sendFloatValue(inputValue);
            cnv->grabKeyboardFocus();
            return true;
        }
        
        return false;
    }

    void focusLost(FocusChangeType cause) override
    {
        auto inputValue = input.getText().getFloatValue();
        if (!approximatelyEqual(inputValue, preFocusValue)) {
            sendFloatValue(inputValue);
        }
        repaint();
    }

    void focusOfChildComponentChanged(FocusChangeType cause) override
    {
        repaint();
    }

    void lock(bool isLocked) override
    {
        input.setResetEnabled(::getValue<bool>(cnv->locked));
        setInterceptsMouseClicks(isLocked, isLocked);
        repaint();
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            if (numAtoms > 0 && atoms[0].isFloat()) {
                value = std::clamp(atoms[0].getFloat(), ::getValue<float>(min), ::getValue<float>(max));
                input.setText(input.formatNumber(value), dontSendNotification);
            }
            break;
        }
        case hash("range"): {
            if (numAtoms >= 2 && atoms[0].isFloat() && atoms[1].isFloat()) {
                min = getMinimum();
                max = getMaximum();
            }
            break;
        }
        case hash("log"): {
            setParameterExcludingListener(logMode, true);
            input.setDragMode(DraggableNumber::Logarithmic);
            break;
        }
        case hash("lin"): {
            setParameterExcludingListener(logMode, false);
            input.setDragMode(DraggableNumber::Regular);
            break;
        }
        case hash("log_height"): {
            auto height = static_cast<int>(atoms[0].getFloat());
            setParameterExcludingListener(logHeight, height);
            input.setLogarithmicHeight(height);
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms, numAtoms);
            break;
        }
        }
    }

    void valueChanged(Value& value) override
    {

        if (value.refersToSameSourceAs(sizeProperty)) {
            auto& arr = *sizeProperty.getValue().getArray();
            auto* constrainer = getConstrainer();
            auto width = std::max(int(arr[0]), constrainer->getMinimumWidth());
            auto height = std::max(int(arr[1]), constrainer->getMinimumHeight());

            setParameterExcludingListener(sizeProperty, Array<var> { var(width), var(height) });

            if (auto nbx = ptr.get<t_my_numbox>()) {
                nbx->x_gui.x_w = width;
                nbx->x_gui.x_h = height;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
            setMinimum(::getValue<float>(min));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(::getValue<float>(max));
        } else if (value.refersToSameSourceAs(logHeight)) {
            auto height = ::getValue<int>(logHeight);
            if (auto nbx = ptr.get<t_my_numbox>()) {
                nbx->x_log_height = height;
            }

            input.setLogarithmicHeight(height);
        } else if (value.refersToSameSourceAs(logMode)) {
            auto logarithmicDrag = ::getValue<bool>(logMode);
            if (auto nbx = ptr.get<t_my_numbox>()) {
                nbx->x_lin0_log1 = logarithmicDrag;
            }
            input.setDragMode(logarithmicDrag ? DraggableNumber::Logarithmic : DraggableNumber::Regular);
        } else {
            iemHelper.valueChanged(value);
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();

        bool selected = object->isSelected() && !cnv->isGraph;
        auto backgroundColour = convertColour(iemHelper.getBackgroundColour());
        auto selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, selected ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        int const indent = 9;
        Rectangle<float> iconBounds = { static_cast<float>(b.getX() + 4), static_cast<float>(b.getY() + 4), static_cast<float>(indent - 4), static_cast<float>(b.getHeight() - 8) };

        auto centreY = iconBounds.getCentreY();
        auto leftX = iconBounds.getX();
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, leftX, centreY + 5.0f);
        nvgLineTo(nvg, iconBounds.getRight(), centreY);
        nvgLineTo(nvg, leftX, centreY - 5.0f);
        nvgClosePath(nvg);

        auto normalColour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour);
        auto highlightColour = cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId);
        bool highlighed = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);

        nvgFillColor(nvg, convertColour(highlighed ? highlightColour : normalColour));
        nvgFill(nvg);

        input.render(nvg);
    }

    float getValue()
    {
        if(auto numbox = ptr.get<t_my_numbox>())
        {
            return numbox->x_val;
        }
        return 0.0f;
    }

    float getMinimum()
    {
        if(auto numbox = ptr.get<t_my_numbox>())
        {
            return numbox->x_min;
        }
        return -std::numeric_limits<float>::infinity();
    }

    float getMaximum()
    {
        if(auto numbox = ptr.get<t_my_numbox>())
        {
            return numbox->x_max;
        }
        return std::numeric_limits<float>::infinity();
    }

    void setMinimum(float value)
    {
        input.setMinimum(value);
        if(auto numbox = ptr.get<t_my_numbox>()) {
            numbox->x_min = value;
        }
    }

    void setMaximum(float value)
    {
        input.setMaximum(value);
        if(auto numbox = ptr.get<t_my_numbox>()) {
            numbox->x_max = value;
        }
        
    }
};
