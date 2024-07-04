/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SymbolAtomObject final : public ObjectBase
    , public KeyListener {

    bool isDown = false;
    bool isLocked = false;

    Value sizeProperty = SynchronousValue();
    AtomHelper atomHelper;

    String lastMessage;

    Label input;

public:
    SymbolAtomObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
    {
        addAndMakeVisible(input);

        input.setText(getSymbol(), dontSendNotification);

        input.addMouseListener(this, false);

        input.onTextChange = [this]() {
            startEdition();
            setSymbol(input.getText().toStdString());
            stopEdition();
        };

        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            editor->setBorder({ 0, 1, 3, 0 });

            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->addKeyListener(this);
            editor->onTextChange = [this]() {
                // To resize while typing
                if (atomHelper.getWidthInChars() == 0) {
                    object->updateBounds();
                }
            };
        };

        input.setMinimumHorizontalScale(0.9f);

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);
        atomHelper.addAtomParameters(objectParameters);
        lookAndFeelChanged();
    }

    void update() override
    {
        sizeProperty = atomHelper.getWidthInChars();
        atomHelper.update();
    }

    void updateSizeProperty() override
    {
        setPdBounds(object->getObjectBounds());
        setParameterExcludingListener(sizeProperty, atomHelper.getWidthInChars());
    }

    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    void resized() override
    {
        input.setFont(Font(getHeight() - 6));
        input.setBounds(getLocalBounds());
    }

    Rectangle<int> getPdBounds() override
    {
        return atomHelper.getPdBounds(input.getFont().getStringWidth(input.getText(true)));
    }

    void setPdBounds(Rectangle<int> b) override
    {
        atomHelper.setPdBounds(b);
    }

    std::unique_ptr<ComponentBoundsConstrainer> createConstrainer() override
    {
        return atomHelper.createConstrainer(object);
    }

    void setSymbol(String const& value)
    {
        if (auto gatom = ptr.get<t_fake_gatom>()) {
            cnv->pd->sendDirectMessage(gatom.get(), value.toStdString());
        }
    }

    String getSymbol()
    {
        if (auto gatom = ptr.get<t_fake_gatom>()) {
            return String::fromUTF8(atom_getsymbol(fake_gatom_getatom(gatom.get()))->s_name);
        }

        return {};
    }

    void mouseUp(MouseEvent const& e) override
    {
        isDown = false;

        // Edit messages when unlocked, edit atoms when locked
        if (isLocked) {
            input.showEditor();
        }

        repaint();
    }

    void lookAndFeelChanged() override
    {
        input.setColour(Label::textWhenEditingColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        input.setColour(Label::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        input.setColour(TextEditor::textColourId, cnv->editor->getLookAndFeel().findColour(PlugDataColour::canvasTextColourId));
        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat().reduced(0.5f);
        auto backgroundColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        auto selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        auto outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));
        bool highlighed = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), backgroundColour, (object->isSelected() || highlighed) ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);

        nvgSave(nvg);
        nvgIntersectRoundedScissor(nvg, b.getX() + 0.25f, b.getY() + 0.25f, b.getWidth() - 0.5f, b.getHeight() - 0.5f, Corners::objectCornerRadius);

        nvgBeginPath(nvg);
        nvgFillColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour)));
        nvgMoveTo(nvg, b.getRight() - 8, b.getY());
        nvgLineTo(nvg, b.getRight(), b.getY());
        nvgLineTo(nvg, b.getRight(), b.getY() + 8);
        nvgClosePath(nvg);
        nvgFill(nvg);

        nvgRestore(nvg);

        imageRenderer.renderJUCEComponent(nvg, input, getImageScale());

        if (object->isSelected()) // If object is selected, draw outline over top too, so the flag doesn't poke into the selected outline
        {
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
            nvgStrokeColor(nvg, selectedOutlineColour);
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);
        }

        if (highlighed) {
            nvgStrokeColor(nvg, convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId)));
            nvgStrokeWidth(nvg, 2.0f);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius);
            nvgStroke(nvg);
        }
    }

    bool inletIsSymbol() override
    {
        return atomHelper.hasReceiveSymbol();
    }

    bool outletIsSymbol() override
    {
        return atomHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        atomHelper.updateLabel(labels);
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto width = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());

            setParameterExcludingListener(sizeProperty, width);

            atomHelper.setWidthInChars(width);
            object->updateBounds();
        } else {
            atomHelper.valueChanged(v);
        }
    }

    bool keyPressed(KeyPress const& key, Component* originalComponent) override
    {
        if (key == KeyPress::rightKey) {
            if (auto* editor = input.getCurrentTextEditor()) {
                editor->setCaretPosition(editor->getHighlightedRegion().getEnd());
                return true;
            }
        }
        return false;
    }

    void receiveObjectMessage(hash32 symbol, pd::Atom const atoms[8], int numAtoms) override
    {
        switch (symbol) {

        case hash("set"):
        case hash("list"):
        case hash("symbol"): {
            input.setText(atoms[0].toString(), dontSendNotification);
            break;
        }
        case hash("float"): {
            input.setText("float", dontSendNotification);
            break;
        }
        case hash("send"): {
            if (numAtoms >= 1)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].toString());
            break;
        }
        case hash("receive"): {
            if (numAtoms >= 1) {
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].toString());
            }
            break;
        }
        default:
            break;
        }
    }
};
