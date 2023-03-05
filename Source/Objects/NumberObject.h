/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../Utility/DraggableNumber.h"

class NumberObject final : public ObjectBase {

    DraggableNumber input;
    IEMHelper iemHelper;

    float preFocusValue;

    Value min = Value(0.0f);
    Value max = Value(0.0f);

    float value = 0.0f;

public:
    NumberObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
        , iemHelper(ptr, object, this)
        , input(false)

    {
        value = getValue();

        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            editor->setBorder({ 0, 11, 3, 0 });

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            sendFloatValue(input.getText().getFloatValue());
            stopEdition();
        };

        value = getValue();

        input.setBorderSize({ 1, 15, 1, 1 });

        addAndMakeVisible(input);

        input.setText(input.formatNumber(value), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        addMouseListener(this, true);

        input.dragStart = [this]() {
            startEdition();
        };

        input.valueChanged = [this](float newValue) {
            sendFloatValue(newValue);
        };

        input.setMinimum(static_cast<float>(min.getValue()));
        input.setMaximum(static_cast<float>(max.getValue()));

        input.dragEnd = [this]() {
            stopEdition();
        };
    }

    void initialiseParameters() override
    {
        iemHelper.initialiseParameters();
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
        pd->lockAudioThread();

        int x = 0, y = 0, w = 0, h = 0;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        auto bounds = Rectangle<int>(x, y, w, h + 1);

        pd->unlockAudioThread();

        return bounds;
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* nbx = static_cast<t_my_numbox*>(ptr);

        nbx->x_gui.x_w = b.getWidth();
        nbx->x_gui.x_h = b.getHeight() - 1;

        nbx->x_numwidth = (b.getWidth() / 9) - 1;
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        input.setFont(getHeight() - 6);
    }

    void focusGained(FocusChangeType cause) override
    {
        preFocusValue = value;
        repaint();
    }

    void focusLost(FocusChangeType cause) override
    {
        auto inputValue = input.getText().getFloatValue();
        if (inputValue != preFocusValue) {
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
        setInterceptsMouseClicks(isLocked, isLocked);
        repaint();
    }

    ObjectParameters getParameters() override
    {

        ObjectParameters allParameters = { { "Minimum", tFloat, cGeneral, &min, {} }, { "Maximum", tFloat, cGeneral, &max, {} } };

        auto iemParameters = iemHelper.getParameters();
        allParameters.insert(allParameters.end(), iemParameters.begin(), iemParameters.end());

        return allParameters;
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("set"): {
            value = std::clamp(atoms[0].getFloat(), static_cast<float>(min.getValue()), static_cast<float>(max.getValue()));
            input.setText(input.formatNumber(value), dontSendNotification);
            break;
        }
        default: {
            iemHelper.receiveObjectMessage(symbol, atoms);
            break;
        }
        }
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            setMinimum(static_cast<float>(min.getValue()));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(static_cast<float>(max.getValue()));
        } else {
            iemHelper.valueChanged(value);
        }
    }

    void paint(Graphics& g) override
    {
        g.setColour(iemHelper.getBackgroundColour());
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        int const indent = 9;

        Rectangle<int> const iconBounds = getLocalBounds().withWidth(indent - 4).withHeight(getHeight() - 8).translated(4, 4);

        Path triangle;
        //    a
        //    |\
        //    | \
        //    |  b
        //    | /
        //    |/
        //    c

        auto centre_y = iconBounds.getCentreY();
        auto left_x = iconBounds.getTopLeft().getX();
        Point<float> point_a(left_x, centre_y + 5.0);
        Point<float> point_b(iconBounds.getRight(), centre_y);
        Point<float> point_c(left_x, centre_y - 5.0);
        triangle.addTriangle(point_a, point_b, point_c);

        auto normalColour = object->findColour(PlugDataColour::objectOutlineColourId);
        auto highlightColour = object->findColour(PlugDataColour::objectSelectedOutlineColourId);
        bool highlighed = hasKeyboardFocus(true) && static_cast<bool>(object->locked.getValue());

        g.setColour(highlighed ? highlightColour : normalColour);
        g.fillPath(triangle);
    }

    float getValue()
    {
        return (static_cast<t_my_numbox*>(ptr))->x_val;
    }

    float getMinimum()
    {
        return (static_cast<t_my_numbox*>(ptr))->x_min;
    }

    float getMaximum()
    {
        return (static_cast<t_my_numbox*>(ptr))->x_max;
    }

    void setMinimum(float value)
    {
        input.setMinimum(value);
        static_cast<t_my_numbox*>(ptr)->x_min = value;
    }

    void setMaximum(float value)
    {
        input.setMaximum(value);
        static_cast<t_my_numbox*>(ptr)->x_max = value;
    }
};
