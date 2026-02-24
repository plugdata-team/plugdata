/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

#include "Components/DraggableNumber.h"

class FloatAtomObject final : public ObjectBase {

    AtomHelper atomHelper;
    DraggableNumber input;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);
    Value sizeProperty = SynchronousValue();

    float value = 0.0f;

    NVGcolor backgroundColour;
    NVGcolor selectedOutlineColour;
    NVGcolor outlineColour;

public:
    FloatAtomObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
        , input(false)
    {
        input.onEditorShow = [this] {
            auto* editor = input.getCurrentTextEditor();

            startEdition();

            editor->setBorder({ 0, -2, 3, 0 });
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        };

        input.onEditorHide = [this] {
            stopEdition();
        };

        addAndMakeVisible(input);

        addMouseListener(this, true);

        input.setEditableOnClick(false, true, true);
        input.dragStart = [this] {
            startEdition();
        };

        input.onTextChange = [this] {
            // To resize while typing
            if (atomHelper.getWidthInChars() == 0) {
                object->updateBounds();
            }
        };

        input.onValueChange = [this](float const newValue) {
            sendFloatValue(newValue);

            if (atomHelper.getWidthInChars() == 0) {
                object->updateBounds();
            }
        };

        input.onReturnKey = [this](double const newValue) {
            sendFloatValue(newValue);
        };

        input.dragEnd = [this] {
            stopEdition();
        };

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty, 0, true, 0);
        objectParameters.addParamFloat("Minimum", cGeneral, &min);
        objectParameters.addParamFloat("Maximum", cGeneral, &max);
        atomHelper.addAtomParameters(objectParameters);

        input.setBorderSize(BorderSize<int>(1, 2, 1, 0));

        input.setResetValue(0.0f);
        input.setShowEllipsesIfTooLong(false);

        lookAndFeelChanged();
    }

    void update() override
    {
        if (input.isShowing())
            return;

        value = getValue();

        min = atomHelper.getMinimum();
        max = atomHelper.getMaximum();

        sizeProperty = atomHelper.getWidthInChars();

        input.setMinimum(::getValue<float>(min));
        input.setMaximum(::getValue<float>(max));

        input.setValue(value, dontSendNotification);

        atomHelper.update();
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());
        setParameterExcludingListener(sizeProperty, atomHelper.getWidthInChars());
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key.getKeyCode() == KeyPress::returnKey) {
            auto const inputValue = input.getText().getFloatValue();
            sendFloatValue(inputValue);
            return true;
        }

        return false;
    }

    void focusGained(FocusChangeType cause) override
    {
        repaint();
    }

    void focusLost(FocusChangeType cause) override
    {
        repaint();
    }

    void focusOfChildComponentChanged(FocusChangeType cause) override
    {
        repaint();
    }

    bool hideInlet() override
    {
        return atomHelper.hasReceiveSymbol();
    }

    bool hideOutlet() override
    {
        return atomHelper.hasSendSymbol();
    }

    void lookAndFeelChanged() override
    {
        input.setColour(Label::textWhenEditingColourId, PlugDataColours::canvasTextColour);
        input.setColour(Label::textColourId, PlugDataColours::canvasTextColour);

        backgroundColour = nvgColour(PlugDataColours::guiObjectBackgroundColour);
        selectedOutlineColour = nvgColour(PlugDataColours::objectSelectedOutlineColour);
        outlineColour = nvgColour(PlugDataColours::objectOutlineColour);

        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        auto const sb = b.reduced(0.5f);

        // Draw background
        nvgDrawObjectWithFlag(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(),
            cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagTop, PlugDataLook::getUseFlagOutline());

        input.render(nvg, cnv->editor->getNanoLLGC());

        // draw flag
        bool const highlighted = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);
        auto const flagCol = highlighted ? cnv->selectedOutlineCol : cnv->guiObjectInternalOutlineCol;
        auto const outlineCol = object->isSelected() || hasKeyboardFocus(true) ? cnv->selectedOutlineCol : cnv->objectOutlineCol;

        // Fill the internal of the shape with transparent colour, draw outline & flag with shader
        nvgDrawObjectWithFlag(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(),
            nvgRGBA(0, 0, 0, 0), outlineCol, flagCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagTop, PlugDataLook::getUseFlagOutline());
    }

    void updateLabel() override
    {
        atomHelper.updateLabel(labels);
    }

    Rectangle<int> getPdBounds() override
    {
        return atomHelper.getPdBounds(Fonts::getStringWidth(input.formatNumber(input.getText().getDoubleValue()), input.getFont()));
    }

    void setPdBounds(Rectangle<int> const b) override
    {
        atomHelper.setPdBounds(b);
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return atomHelper.createConstrainer(object);
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        input.setFont(input.getFont().withHeight(getHeight() - 6));
    }

    void lock(bool const isLocked) override
    {
        setInterceptsMouseClicks(isLocked, isLocked);
        input.setResetEnabled(::getValue<bool>(cnv->locked));
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const width = ::getValue<int>(sizeProperty);

            setParameterExcludingListener(sizeProperty, width);

            atomHelper.setWidthInChars(width);
            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
            auto const v = ::getValue<float>(min);
            input.setMinimum(v);
            atomHelper.setMinimum(v);
        } else if (value.refersToSameSourceAs(max)) {
            auto const v = ::getValue<float>(max);
            input.setMaximum(v);
            atomHelper.setMaximum(v);
        } else {
            atomHelper.valueChanged(value);
        }
    }

    float getValue() const
    {
        if (auto gatom = ptr.get<t_fake_gatom>()) {
            return atom_getfloat(fake_gatom_getatom(gatom.get()));
        }

        return 0.0f;
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {

        case hash("set"):
        case hash("float"):
        case hash("list"): {
            if (atoms.size() < 1 || !atoms[0].isFloat())
                break;
            value = atoms[0].getFloat();
            input.setValue(value, dontSendNotification, false);
            break;
        }
        case hash("send"): {
            if (atoms.size() <= 0)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("receive"): {
            if (atoms.size() <= 0)
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        default:
            break;
        }
    }
};
