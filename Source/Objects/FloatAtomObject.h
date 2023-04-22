/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Utility/DraggableNumber.h"

class FloatAtomObject final : public ObjectBase {

    AtomHelper atomHelper;
    DraggableNumber input;

    Value min = Value(0.0f);
    Value max = Value(0.0f);

    float value = 0.0f;

public:
    FloatAtomObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
        , input(false)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();

            startEdition();

            editor->setBorder({ 0, 1, 3, 0 });
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);

            if (editor != nullptr) {
                editor->setInputRestrictions(0, ".-0123456789");
            }
        };

        input.onEditorHide = [this]() {
            sendFloatValue(input.getText().getFloatValue());
            stopEdition();
        };

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

        min = atomHelper.getMinimum();
        max = atomHelper.getMaximum();

        input.setMinimum(::getValue<float>(min));
        input.setMaximum(::getValue<float>(max));

        input.setText(input.formatNumber(value), dontSendNotification);

        atomHelper.update();
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

    bool hideInlets() override
    {
        return atomHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return atomHelper.hasSendSymbol();
    }

    void lookAndFeelChanged() override
    {
        input.setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
        input.setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        input.setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        repaint();
    }

    void paint(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::outlineColourId));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));
        triangle = triangle.createPathWithRoundedCorners(4.0f);
        g.fillPath(triangle);

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);

        bool highlighed = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);

        if (highlighed) {
            g.setColour(object->findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Corners::objectCornerRadius, 2.0f);
        }
    }

    void updateLabel() override
    {
        atomHelper.updateLabel(label);
    }

    Rectangle<int> getPdBounds() override
    {
        return atomHelper.getPdBounds();
    }

    void setPdBounds(Rectangle<int> b) override
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
        input.setFont(getHeight() - 6);
    }

    void lock(bool isLocked) override
    {
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    ObjectParameters getParameters() override
    {
        ObjectParameters allParameters = { { "Minimum", tFloat, cGeneral, &min, {} }, { "Maximum", tFloat, cGeneral, &max, {} } };

        auto atomParameters = atomHelper.getParameters();
        allParameters.insert(allParameters.end(), atomParameters.begin(), atomParameters.end());

        return allParameters;
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(min)) {
            auto v = ::getValue<float>(min);
            input.setMinimum(v);
            atomHelper.setMinimum(v);
        } else if (value.refersToSameSourceAs(max)) {
            auto v = ::getValue<float>(max);
            input.setMaximum(v);
            atomHelper.setMaximum(v);
        } else {
            atomHelper.valueChanged(value);
        }
    }

    float getValue()
    {
        return atom_getfloat(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)));
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("set"),
            hash("float"),
            hash("send"),
            hash("receive"),
            hash("list"),
        };
    }

    void receiveObjectMessage(hash32 const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (symbol) {

        case hash("set"):
        case hash("float"):
        case hash("list"):
        {
            if(!atoms[0].isFloat()) break;
            
            auto min = atomHelper.getMinimum();
            auto max = atomHelper.getMaximum();

            if (min != 0 || max != 0) {
                value = std::clamp(atoms[0].getFloat(), min, max);
            } else {
                value = atoms[0].getFloat();
            }
            input.setText(input.formatNumber(value), dontSendNotification);
            break;
        }
        case hash("send"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].getSymbol());
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].getSymbol());
            break;
        }
        default:
            break;
        }
    }
};
