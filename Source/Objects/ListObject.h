/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

class ListObject final : public ObjectBase {

    AtomHelper atomHelper;
    DraggableListNumber listLabel;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);
    Value sizeProperty = SynchronousValue();

    bool editorActive = false;

public:
    ListObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
    {
        listLabel.setBounds(2, 0, getWidth() - 2, getHeight() - 1);
        listLabel.setMinimumHorizontalScale(1.f);
        listLabel.setJustificationType(Justification::centredLeft);
        // listLabel.setBorderSize(BorderSize<int>(2, 6, 2, 2));

        addAndMakeVisible(listLabel);

        listLabel.onEditorHide = [this] {
            stopEdition();
            editorActive = false;
        };

        listLabel.onTextChange = [this] {
            // To resize while typing
            if (atomHelper.getWidthInChars() == 0) {
                object->updateBounds();
            }
        };

        listLabel.onReturnKey = [this](double) {
            updateFromGui(true);
        };

        listLabel.onEditorShow = [this] {
            startEdition();
            auto* editor = listLabel.getCurrentTextEditor();
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->setBorder({ 0, 1, 3, 0 });
            editorActive = true;
        };

        listLabel.dragStart = [this] {
            startEdition();
            editorActive = true;
        };

        listLabel.onValueChange = [this](float) { updateFromGui(); };

        listLabel.dragEnd = [this] {
            stopEdition();
            editorActive = false;
        };

        listLabel.addMouseListener(this, false);

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);
        objectParameters.addParamFloat("Minimum", cGeneral, &min);
        objectParameters.addParamFloat("Maximum", cGeneral, &max);
        atomHelper.addAtomParameters(objectParameters);
        lookAndFeelChanged();
    }

    void update() override
    {
        sizeProperty = atomHelper.getWidthInChars();

        min = atomHelper.getMinimum();
        max = atomHelper.getMaximum();
        updateValue();

        atomHelper.update();
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());
        setParameterExcludingListener(sizeProperty, atomHelper.getWidthInChars());
    }

    void propertyChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const width = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());

            setParameterExcludingListener(sizeProperty, width);

            atomHelper.setWidthInChars(width);
            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
            auto const v = getValue<float>(min);
            listLabel.setMinimum(v);
            atomHelper.setMinimum(v);
        } else if (value.refersToSameSourceAs(max)) {
            auto const v = getValue<float>(max);
            listLabel.setMaximum(v);
            atomHelper.setMaximum(v);
        } else {
            atomHelper.valueChanged(value);
        }
    }

    void updateFromGui(bool const force = false)
    {
        auto const text = listLabel.getText();
        if (force || text != getListText()) {
            SmallArray<pd::Atom> const list = pd::Atom::atomsFromString(text);
            setList(list);
        }
    }

    void resized() override
    {
        listLabel.setFont(listLabel.getFont().withHeight(getHeight() - 6));
        listLabel.setBounds(getLocalBounds());
    }

    Rectangle<int> getPdBounds() override
    {
        return atomHelper.getPdBounds(listLabel.getFont().getStringWidth(listLabel.getText()));
    }

    void setPdBounds(Rectangle<int> b) override
    {
        atomHelper.setPdBounds(b);
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return atomHelper.createConstrainer(object);
    }

    void updateLabel() override
    {
        atomHelper.updateLabel(labels);
    }

    bool inletIsSymbol() override
    {
        return atomHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return atomHelper.hasSendSymbol();
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        auto const sb = b.reduced(0.5f);

        // Draw background
        nvgDrawObjectWithFlag(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(),
            cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagTopBottom, static_cast<PlugDataLook&>(cnv->getLookAndFeel()).getUseFlagOutline());

        listLabel.render(nvg);

        // Draw outline & flag
        bool const highlighted = editorActive && getValue<bool>(object->locked);
        auto const flagCol = highlighted ? cnv->selectedOutlineCol : cnv->guiObjectInternalOutlineCol;
        auto const outlineCol = object->isSelected() || editorActive ? cnv->selectedOutlineCol : cnv->objectOutlineCol;

        // Fill the internal of the shape with transparent colour, draw outline & flag with shader
        nvgDrawObjectWithFlag(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(),
            nvgRGBA(0, 0, 0, 0), outlineCol, flagCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagTopBottom, static_cast<PlugDataLook&>(cnv->getLookAndFeel()).getUseFlagOutline());
    }

    void lookAndFeelChanged() override
    {
        listLabel.setColour(Label::textWhenEditingColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        listLabel.setColour(Label::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        listLabel.setColour(TextEditor::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));

        repaint();
    }

    bool keyPressed(KeyPress const& key) override
    {
        if (key.getKeyCode() == KeyPress::returnKey) {
            updateFromGui(true);
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

    void updateValue()
    {
        if (!listLabel.isBeingEdited()) {
            auto const listText = getListText();
            listLabel.setText(listText, NotificationType::dontSendNotification);
        }
    }

    String getListText() const
    {
        if (auto gatom = ptr.get<t_fake_gatom>()) {
            char* text;
            int size;
            binbuf_gettext(gatom->a_text.te_binbuf, &text, &size);

            auto result = String::fromUTF8(text, size);
            freebytes(text, size);

            return result;
        }

        return {};
    }

    void setList(SmallArray<pd::Atom> const& value)
    {
        if (auto gatom = ptr.get<t_fake_gatom>())
            cnv->pd->sendDirectMessage(gatom.get(), SmallArray<pd::Atom>(value.begin(), value.end()));
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(object->locked) && !e.mouseWasDraggedSinceMouseDown() && isShowing()) {

            listLabel.showEditor();
        }
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
    {
        switch (symbol) {
        case hash("float"):
        case hash("symbol"):
        case hash("list"):
        case hash("set"): {
            updateValue();
            break;
        }
        case hash("send"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        default:
            break;
        }
    }
};
