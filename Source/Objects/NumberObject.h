/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/DraggableNumber.h"

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
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            startEdition();

            editor->setBorder({ 0, 11, 3, 0 });

            if (editor != nullptr) {
                editor->setInputRestrictions(0, "e.-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            sendFloatValue(input.getText().getFloatValue());
            stopEdition();
        };

        input.setBorderSize({ 1, 15, 1, 1 });

        addAndMakeVisible(input);

        addMouseListener(this, true);

        input.dragStart = [this]() {
            startEdition();
        };

        input.valueChanged = [this](float newValue) {
            sendFloatValue(newValue);
        };

        input.dragEnd = [this]() {
            stopEdition();
        };
    }

    void update() override
    {
        value = getValue();
        input.setText(input.formatNumber(value), dontSendNotification);

        min = getMinimum();
        max = getMaximum();

        input.setMinimum(::getValue<float>(min));
        input.setMaximum(::getValue<float>(max));

        iemHelper.update();
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
        auto bounds = Rectangle<int>(x, y, w + 1, h + 1);

        pd->unlockAudioThread();

        return bounds;
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* nbx = static_cast<t_my_numbox*>(ptr);

        nbx->x_gui.x_w = b.getWidth() - 1;
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

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("float"),
            hash("set"),
            hash("list"),
            IEMGUI_MESSAGES
        };
    }

    void receiveObjectMessage(hash32 const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            if(atoms[0].isFloat()) {
                value = std::clamp(atoms[0].getFloat(), ::getValue<float>(min), ::getValue<float>(max));
                input.setText(input.formatNumber(value), dontSendNotification);
            }
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
            setMinimum(::getValue<float>(min));
        } else if (value.refersToSameSourceAs(max)) {
            setMaximum(::getValue<float>(max));
        } else {
            iemHelper.valueChanged(value);
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
        bool highlighed = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);

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
