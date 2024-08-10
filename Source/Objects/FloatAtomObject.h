/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

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
    Colour selCol;
    NVGcolor outlineColour;

public:
    FloatAtomObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
        , input(false)
    {
        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();

            startEdition();

            editor->setBorder({ 0, -2, 3, 0 });
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->setInputRestrictions(0, ".-0123456789");
        };

        input.onEditorHide = [this]() {
            stopEdition();
        };

        addAndMakeVisible(input);

        addMouseListener(this, true);

        input.dragStart = [this]() {
            startEdition();
        };

        input.onTextChange = [this]() {
            // To resize while typing
            if (atomHelper.getWidthInChars() == 0) {
                object->updateBounds();
            }
        };

        input.onValueChange = [this](float newValue) {
            sendFloatValue(newValue);

            if (atomHelper.getWidthInChars() == 0) {
                object->updateBounds();
            }
        };

        input.dragEnd = [this]() {
            stopEdition();
        };

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);
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

        input.setText(input.formatNumber(value), dontSendNotification);

        atomHelper.update();
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());
        setParameterExcludingListener(sizeProperty, atomHelper.getWidthInChars());
    }
    
    bool keyPressed(KeyPress const& key) override
    {
        if(key.getKeyCode() == KeyPress::returnKey)
        {
            auto inputValue = input.getText().getFloatValue();
            sendFloatValue(inputValue);
            cnv->grabKeyboardFocus();
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

    bool inletIsSymbol() override
    {
        return atomHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return atomHelper.hasSendSymbol();
    }

    void lookAndFeelChanged() override
    {
        input.setColour(Label::textWhenEditingColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        input.setColour(Label::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        input.setColour(TextEditor::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));

        backgroundColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        selCol = cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId);
        selectedOutlineColour = convertColour(selCol);
        outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        repaint();
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));

        auto reducedBounds = getLocalBounds().toFloat().reduced(0.5f);

        Path roundEdgeClipping;
        roundEdgeClipping.addRoundedRectangle(reducedBounds, Corners::objectCornerRadius);

        g.saveState();
        g.reduceClipRegion(roundEdgeClipping);
        g.fillPath(triangle);
        g.restoreState();

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = cnv->editor->getLookAndFeel().findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);

        bool highlighed = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);

        if (highlighed) {
            g.setColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Corners::objectCornerRadius, 2.0f);
        }
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();
        auto sb = b.reduced(0.5f);

        nvgDrawRoundedRect(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(), backgroundColour, backgroundColour, Corners::objectCornerRadius);

        input.render(nvg);

        // draw flag
        bool active = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);
        atomHelper.drawTriangleFlag(nvg, active);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBAf(0, 0, 0, 0), (active || object->isSelected()) ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);
    }

    void updateLabel() override
    {
        atomHelper.updateLabel(labels);
    }

    Rectangle<int> getPdBounds() override
    {
        return atomHelper.getPdBounds(input.getFont().getStringWidth(input.formatNumber(input.getText(true).getDoubleValue())));
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
        input.setFont(input.getFont().withHeight(getHeight() - 6));
    }

    void lock(bool isLocked) override
    {
        setInterceptsMouseClicks(isLocked, isLocked);
        input.setResetEnabled(::getValue<bool>(cnv->locked));
    }

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto width = ::getValue<int>(sizeProperty);

            setParameterExcludingListener(sizeProperty, width);

            atomHelper.setWidthInChars(width);
            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
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
        if (auto gatom = ptr.get<t_fake_gatom>()) {
            return atom_getfloat(fake_gatom_getatom(gatom.get()));
        }

        return 0.0f;
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {

        case hash("set"):
        case hash("float"):
        case hash("list"): {
            if (numAtoms < 1 || !atoms[0].isFloat())
                break;

            auto min = atomHelper.getMinimum();
            auto max = atomHelper.getMaximum();

            if (!approximatelyEqual(min, 0.0f) || !approximatelyEqual(max, 0.0f)) {
                value = std::clamp(atoms[0].getFloat(), min, max);
            } else {
                value = atoms[0].getFloat();
            }
            input.setText(input.formatNumber(value), dontSendNotification);
            break;
        }
        case hash("send"): {
            if (numAtoms <= 0)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].toString());
            break;
        }
        case hash("receive"): {
            if (numAtoms <= 0)
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].toString());
            break;
        }
        default:
            break;
        }
    }
};
