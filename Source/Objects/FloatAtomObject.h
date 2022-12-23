/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/DraggableNumber.h"

struct FloatAtomObject final : public AtomObject {

    DraggableNumber input;

    FloatAtomObject(void* obj, Object* parent)
        : AtomObject(obj, parent)
        , input(false)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            editor->setBorder({ 0, 1, 3, 0 });

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            setValueOriginal(input.getText().getFloatValue());
            stopEdition();
        };

        addAndMakeVisible(input);

        input.setText(input.formatNumber(getValueOriginal()), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        addMouseListener(this, true);

        input.dragStart = [this]() { startEdition(); };

        input.valueChanged = [this](float value) { setValueOriginal(value); };

        input.dragEnd = [this]() { stopEdition(); };
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

    void paintOverChildren(Graphics& g) override
    {
        AtomObject::paintOverChildren(g);

        bool highlighed = hasKeyboardFocus(true) && static_cast<bool>(object->locked.getValue());

        if (highlighed) {
            g.setColour(object->findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Constants::objectCornerRadius, 2.0f);
        }
    }

    void resized() override
    {
        AtomObject::resized();

        input.setBounds(getLocalBounds());
        input.setFont(getHeight() - 6);
    }

    void update() override
    {
        input.setText(input.formatNumber(getValueOriginal()), dontSendNotification);
    }

    void lock(bool isLocked) override
    {
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    ObjectParameters defineParameters() override
    {
        return { { "Minimum", tFloat, cGeneral, &min, {} }, { "Maximum", tFloat, cGeneral, &max, {} } };
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
            updateValue();
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
            updateValue();
        } else {
            AtomObject::valueChanged(value);
        }
    }

    float getValue() override
    {
        return atom_getfloat(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)));
    }

    float getMinimum()
    {
        const auto* gatom = static_cast<t_fake_gatom const*>(ptr);
        return gatom->a_draglo;
    }

    float getMaximum()
    {
        const auto* gatom = static_cast<t_fake_gatom const*>(ptr);
        return gatom->a_draghi;
    }

    void setMinimum(float value)
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        input.setMinimum(value);
        gatom->a_draglo = value;
    }
    void setMaximum(float value)
    {
        auto* gatom = static_cast<t_fake_gatom*>(ptr);
        input.setMaximum(value);
        gatom->a_draghi = value;
    }
};
