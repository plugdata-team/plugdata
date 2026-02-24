/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include "Components/DraggableNumber.h"
#include "ObjectBase.h"
#include "IEMHelper.h"

class NumberObject final : public ObjectBase {
    DraggableNumber input;
    IEMHelper iemHelper;

    Value widthProperty = SynchronousValue();
    Value heightProperty = SynchronousValue();
    Value min = SynchronousValue(-std::numeric_limits<float>::infinity());
    Value max = SynchronousValue(std::numeric_limits<float>::infinity());
    Value logHeight = SynchronousValue();
    Value logMode = SynchronousValue();

    float preFocusValue;
    float value = 0.0f;

    NVGcolor backgroundCol;
    NVGcolor foregroundCol;
    NVGcolor flagCol;

public:
    NumberObject(pd::WeakReference ptr, Object* object)
        : ObjectBase(ptr, object)
        , input(false)
        , iemHelper(ptr, object, this)

    {
        iemHelper.iemColourChangedCallback = [this] {
            // We use this callback to be informed when the IEM colour has changed.
            // As getBackgroundColour() will lock audio thread!
            backgroundCol = nvgColour(iemHelper.getBackgroundColour());

            foregroundCol = nvgColour(iemHelper.getForegroundColour());
            flagCol = nvgColour(iemHelper.getForegroundColour());

            input.setColour(Label::textColourId, iemHelper.getForegroundColour());
            input.setColour(Label::textWhenEditingColourId, iemHelper.getBackgroundColour().contrasting());
        };

        input.onEditorShow = [this] {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->setBorder({ 0, 8, 4, 1 });
        };

        input.onInteraction = [this](bool const isFocused) {
            if (isFocused)
                input.setColour(Label::textColourId, iemHelper.getBackgroundColour().contrasting());
            else
                input.setColour(Label::textColourId, iemHelper.getForegroundColour());
        };

        input.setEditableOnClick(false, false, true);
        input.onEditorHide = [this] {
            stopEdition();
        };

        input.setBorderSize({ 1, 12, 1, 0 });

        addAndMakeVisible(input);

        addMouseListener(this, true);

        input.dragStart = [this] {
            startEdition();
        };

        input.onValueChange = [this](float const newValue) {
            sendFloatValue(newValue);
        };

        input.onReturnKey = [this](double const newValue) {
            sendFloatValue(newValue);
        };

        input.dragEnd = [this] {
            stopEdition();
        };

        objectParameters.addParamInt("Width (chars)", cDimensions, &widthProperty, var(), true, 1);
        objectParameters.addParamInt("Height", cDimensions, &heightProperty, var(), true, 8);
        objectParameters.addParamInt("Text/Label Height", cDimensions, &iemHelper.labelHeight, 10, true, 1);
        objectParameters.addParamFloat("Minimum", cGeneral, &min, -9.999999933815813e36);
        objectParameters.addParamFloat("Maximum", cGeneral, &max, 9.999999933815813e36);
        objectParameters.addParamBool("Logarithmic mode", cGeneral, &logMode, { "Off", "On" }, var(false));
        objectParameters.addParamInt("Logarithmic height", cGeneral, &logHeight, var(256), true, 1);
        objectParameters.addParamColourFG(&iemHelper.primaryColour);
        objectParameters.addParamColourBG(&iemHelper.secondaryColour);
        objectParameters.addParamReceiveSymbol(&iemHelper.receiveSymbol);
        objectParameters.addParamSendSymbol(&iemHelper.sendSymbol);
        objectParameters.addParamString("Text", cLabel, &iemHelper.labelText, "");
        objectParameters.addParamColourLabel(&iemHelper.labelColour);
        objectParameters.addParamRange("Position", cLabel, &iemHelper.labelPosition, { 0, -8 });
        objectParameters.addParamBool("Initialise", cGeneral, &iemHelper.initialise, { "No", "Yes" }, 0);

        input.setResetValue(0.0f);
    }

    void update() override
    {
        if (auto nbx = ptr.get<t_my_numbox>()) {
            widthProperty = var(nbx->x_numwidth);
            heightProperty = var(nbx->x_gui.x_h);
            logMode = nbx->x_lin0_log1;
            logHeight = nbx->x_log_height;
            min = nbx->x_min;
            max = nbx->x_max;
            value = nbx->x_val;
        }
        
        input.setValue(value, dontSendNotification, false);
        input.setMinimum(getValue<float>(min));
        input.setMaximum(getValue<float>(max));
        input.setLogarithmicHeight(getValue<float>(logHeight));
        input.setDragMode(getValue<bool>(logMode) ? DraggableNumber::Logarithmic : DraggableNumber::Regular);

        iemHelper.update();
        
        auto const fontHeight = getValue<int>(iemHelper.labelHeight);
        input.setFont(Fonts::getTabularNumbersFont().withHeight(fontHeight + 3.0f));
        updateLabel();
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
        iemHelper.updateLabel(labels);
    }

    Rectangle<int> getPdBounds() override
    {
        if (auto nbx = ptr.get<t_my_numbox>()) {
            auto* patch = cnv->patch.getRawPointer();

            int x = 0, y = 0, w = 0, h = 0;
            pd::Interface::getObjectBounds(patch, nbx.cast<t_gobj>(), &x, &y, &w, &h);

            return { x, y, calcFontWidth(std::max(nbx->x_numwidth, 1)) + 1, h + 1 };
        }

        return {};
    }

    int getFontWidth() const
    {
        if (auto nbx = ptr.get<t_my_numbox>()) {
            return nbx->x_gui.x_fontsize;
        }
        return 10;
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        if (auto nbx = ptr.get<t_my_numbox>()) {
            auto* patchPtr = cnv->patch.getRawPointer();

            pd::Interface::moveObject(patchPtr, nbx.cast<t_gobj>(), b.getX(), b.getY());

            nbx->x_numwidth = calcNumWidth(b.getWidth() - 1);
            nbx->x_gui.x_w = b.getWidth() - 1;
            nbx->x_gui.x_h = b.getHeight() - 1;
        }
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());

        if (auto nbx = ptr.get<t_my_numbox>()) {
            setParameterExcludingListener(widthProperty, var(nbx->x_numwidth));
            setParameterExcludingListener(heightProperty, var(nbx->x_gui.x_h));
        }
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
    }

    void focusGained(FocusChangeType cause) override
    {
        preFocusValue = value;
        repaint();
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key.getKeyCode() == KeyPress::returnKey) {
            auto const inputValue = input.getText().getFloatValue();
            preFocusValue = value;
            sendFloatValue(inputValue);
            return true;
        }

        return false;
    }

    void focusLost(FocusChangeType cause) override
    {
        auto const inputValue = input.getText().getFloatValue();
        if (!approximatelyEqual(inputValue, preFocusValue)) {
            sendFloatValue(inputValue);
        }
        repaint();
    }

    void focusOfChildComponentChanged(FocusChangeType cause) override
    {
        repaint();
    }

    void lock(bool const isLocked) override
    {
        input.setResetEnabled(getValue<bool>(cnv->locked));
        setInterceptsMouseClicks(isLocked, isLocked);
        repaint();
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            if (atoms.size() > 0 && atoms[0].isFloat()) {
                value = atoms[0].getFloat();
                input.setValue(value, dontSendNotification, false);
            }
            break;
        }
        case hash("range"): {
            if (atoms.size() >= 2) {
                if (auto nbx = ptr.get<t_my_numbox>()) {
                    min = nbx->x_min;
                    max = nbx->x_max;
                }
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
            auto const height = static_cast<int>(atoms[0].getFloat());
            setParameterExcludingListener(logHeight, height);
            input.setLogarithmicHeight(height);
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }

    int calcFontWidth(int const numWidth) const
    {
        if (auto nbx = ptr.get<t_my_numbox>()) {
            int f = 31;

            if (nbx->x_gui.x_fsf.x_font_style == 1)
                f = 27;
            else if (nbx->x_gui.x_fsf.x_font_style == 2)
                f = 25;

            int w = nbx->x_gui.x_fontsize * f * numWidth;
            w /= 36;
            return w + nbx->x_gui.x_h / 2 + 4;
        }
        return 14;
    }

    int calcNumWidth(int const width) const
    {
        if (auto nbx = ptr.get<t_my_numbox>()) {
            int f = 31;
            if (nbx->x_gui.x_fsf.x_font_style == 1)
                f = 27;
            else if (nbx->x_gui.x_fsf.x_font_style == 2)
                f = 25;

            return -(18.0f * (8.0f + nbx->x_gui.x_h - 2 * width)) / (nbx->x_gui.x_fontsize * f) + 1;
        }
        return 1;
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(widthProperty)) {
            auto const numWidth = std::max(getValue<int>(widthProperty), 1);
            auto const width = calcFontWidth(numWidth) + 1;

            setParameterExcludingListener(widthProperty, var(numWidth));

            if (auto nbx = ptr.get<t_my_numbox>()) {
                nbx->x_numwidth = numWidth;
                nbx->x_gui.x_w = width;
            }

            object->updateBounds();
        } else if (value.refersToSameSourceAs(heightProperty)) {
            auto const height = std::max(getValue<int>(heightProperty), constrainer->getMinimumHeight());
            setParameterExcludingListener(heightProperty, var(height));
            if (auto nbx = ptr.get<t_my_numbox>()) {
                nbx->x_gui.x_h = height;
            }
            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
            input.setMinimum(getValue<float>(min));
            if (auto numbox = ptr.get<t_my_numbox>()) {
                numbox->x_min = getValue<float>(min);
            }
        } else if (value.refersToSameSourceAs(max)) {
            input.setMaximum(getValue<float>(max));
            if (auto numbox = ptr.get<t_my_numbox>()) {
                numbox->x_max = getValue<float>(max);
            }
        } else if (value.refersToSameSourceAs(logHeight)) {
            auto const height = getValue<int>(logHeight);
            if (auto nbx = ptr.get<t_my_numbox>()) {
                nbx->x_log_height = height;
            }

            input.setLogarithmicHeight(height);
        } else if (value.refersToSameSourceAs(logMode)) {
            auto const logarithmicDrag = getValue<bool>(logMode);
            if (auto nbx = ptr.get<t_my_numbox>()) {
                nbx->x_lin0_log1 = logarithmicDrag;
            }
            input.setDragMode(logarithmicDrag ? DraggableNumber::Logarithmic : DraggableNumber::Regular);
        } else if (value.refersToSameSourceAs(iemHelper.labelHeight)) {
            auto const fontHeight = getValue<int>(iemHelper.labelHeight);
            iemHelper.setFontHeight(fontHeight);
            updateLabel();

            input.setFont(Fonts::getTabularNumbersFont().withHeight(fontHeight + 3.0f));
            object->updateBounds();
        } else {
            iemHelper.valueChanged(value);
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();

        bool const selected = object->isSelected() && !cnv->isGraph;

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundCol, selected ? cnv->selectedOutlineCol : cnv->objectOutlineCol, Corners::objectCornerRadius);

        constexpr float indent = 9.0f;
        Rectangle<float> const iconBounds = { (b.getX() + 4.0f), (b.getY() + 4.0f), (indent - 4.0f), (b.getHeight() - 8.0f) };

        auto const centreY = iconBounds.getCentreY();
        auto const leftX = iconBounds.getX();
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, leftX, centreY + 5.0f);
        nvgLineTo(nvg, iconBounds.getRight(), centreY);
        nvgLineTo(nvg, leftX, centreY - 5.0f);
        nvgClosePath(nvg);

        bool const highlighted = hasKeyboardFocus(true) && getValue<bool>(object->locked);
        auto const flagCol = highlighted ? cnv->selectedOutlineCol : cnv->guiObjectInternalOutlineCol;

        nvgFillColor(nvg, flagCol);
        nvgFill(nvg);

        input.render(nvg, cnv->editor->getNanoLLGC());
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        class NumboxBoundsConstrainer : public ComponentBoundsConstrainer {
        public:
            Object* object;
            NumberObject* numbox;

            NumboxBoundsConstrainer(Object* parent, NumberObject* nbx)
                : object(parent)
                , numbox(nbx)
            {
            }

            void checkBounds(Rectangle<int>& bounds,
                Rectangle<int> const& old,
                Rectangle<int> const& limits,
                bool isStretchingTop,
                bool const isStretchingLeft,
                bool isStretchingBottom,
                bool isStretchingRight) override
            {
                auto const oldBounds = old.reduced(Object::margin);
                auto const newBounds = bounds.reduced(Object::margin);

                // Calculate the width in text characters for both
                auto const newCharWidth = numbox->calcNumWidth(newBounds.getWidth() - 1);

                // Set new width
                if (auto nbx = numbox->ptr.get<t_my_numbox>()) {
                    nbx->x_numwidth = newCharWidth;
                    nbx->x_gui.x_h = std::max(newBounds.getHeight(), 8);
                }

                bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;

                // If we're resizing the left edge, move the object left
                if (isStretchingLeft) {
                    auto const x = oldBounds.getRight() - (bounds.getWidth() - Object::doubleMargin);
                    auto const y = oldBounds.getY(); // don't allow y resize

                    if (auto nbx = numbox->ptr.get<t_my_numbox>()) {
                        auto* patch = object->cnv->patch.getRawPointer();
                        pd::Interface::moveObject(patch, nbx.cast<t_gobj>(), x - object->cnv->canvasOrigin.x, y - object->cnv->canvasOrigin.y);
                    }
                    bounds = object->gui->getPdBounds().expanded(Object::margin) + object->cnv->canvasOrigin;
                }
            }
        };

        return std::make_unique<NumboxBoundsConstrainer>(object, this);
    }
};
