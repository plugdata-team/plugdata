/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ListObject final : public ObjectBase, public KeyListener{

    AtomHelper atomHelper;
    DraggableListNumber listLabel;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);
    Value sizeProperty = SynchronousValue();

    NVGcolor backgroundColour;
    NVGcolor selectedOutlineColour;
    Colour selectedOutlineCol;
    NVGcolor outlineColour;

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

        listLabel.onEditorHide = [this]() {
            stopEdition();
            editorActive = false;
        };

        listLabel.onTextChange = [this]() {
            // To resize while typing
            if (atomHelper.getWidthInChars() == 0) {
                object->updateBounds();
            }
        };

        listLabel.onEditorShow = [this]() {
            startEdition();
            auto* editor = listLabel.getCurrentTextEditor();
            editor->addKeyListener(this);
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->setBorder({ 0, 1, 3, 0 });
            editorActive = true;
        };

        listLabel.dragStart = [this]() {
            startEdition();
            editorActive = true;
        };

        listLabel.onValueChange = [this](float) { updateFromGui(); };

        listLabel.dragEnd = [this]() {
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

    void valueChanged(Value& value) override
    {
        if (value.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto width = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());

            setParameterExcludingListener(sizeProperty, width);

            atomHelper.setWidthInChars(width);
            object->updateBounds();
        } else if (value.refersToSameSourceAs(min)) {
            auto v = getValue<float>(min);
            listLabel.setMinimum(v);
            atomHelper.setMinimum(v);
        } else if (value.refersToSameSourceAs(max)) {
            auto v = getValue<float>(max);
            listLabel.setMaximum(v);
            atomHelper.setMaximum(v);
        } else {
            atomHelper.valueChanged(value);
        }
    }

    void updateFromGui(bool force = false)
    {
        auto array = StringArray();
        array.addTokens(listLabel.getText(), true);
        std::vector<pd::Atom> list;
        list.reserve(array.size());
        for (auto const& elem : array) {
            auto charptr = elem.getCharPointer();
            auto numptr = charptr;
            auto value = CharacterFunctions::readDoubleValue(numptr);

            if (numptr - charptr == elem.getNumBytesAsUTF8()) {
                list.emplace_back(value);
            } else {
                list.emplace_back(pd->generateSymbol(elem));
            }
        }
        if (force || list != getList()) {
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
        return atomHelper.getPdBounds(listLabel.getFont().getStringWidth(listLabel.getText(true)));
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
        auto b = getLocalBounds().toFloat();
        auto sb = b.reduced(0.5f);

        nvgDrawRoundedRect(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(), backgroundColour, backgroundColour, Corners::objectCornerRadius);

        imageRenderer.renderJUCEComponent(nvg, listLabel, getImageScale());

        // draw flag
        bool highlighted = editorActive && getValue<bool>(object->locked);
        atomHelper.drawTriangleFlag(nvg, highlighted, true);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBAf(0, 0, 0, 0), (object->isSelected() || highlighted) ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);
    }

    void lookAndFeelChanged() override
    {
        listLabel.setColour(Label::textWhenEditingColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        listLabel.setColour(Label::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        listLabel.setColour(TextEditor::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));

        backgroundColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        selectedOutlineCol = cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId);
        selectedOutlineColour = convertColour(selectedOutlineCol);
        outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        repaint();
    }
    
    bool keyPressed(KeyPress const& key, Component*) override
    {
        if(key.getKeyCode() == KeyPress::returnKey)
        {
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
            auto const array = getList();
            String message;
            for (auto const& atom : array) {
                if (message.isNotEmpty()) {
                    message += " ";
                }

                message += atom.toString();
            }
            listLabel.setText(message, NotificationType::dontSendNotification);
        }
    }

    std::vector<pd::Atom> getList() const
    {
        if (auto gatom = ptr.get<t_fake_gatom>()) {
            int ac = binbuf_getnatom(gatom->a_text.te_binbuf);
            t_atom* av = binbuf_getvec(gatom->a_text.te_binbuf);
            return pd::Atom::fromAtoms(ac, av);
        }

        return {};
    }

    void setList(std::vector<pd::Atom> value)
    {
        if (auto gatom = ptr.get<t_fake_gatom>())
            cnv->pd->sendDirectMessage(gatom.get(), std::move(value));
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (getValue<bool>(object->locked) && !e.mouseWasDraggedSinceMouseDown()) {

            listLabel.showEditor();
        }
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
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
            if (numAtoms >= 1)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].toString());
            break;
        }
        case hash("receive"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].toString());
            break;
        }
        default:
            break;
        }
    }
};
