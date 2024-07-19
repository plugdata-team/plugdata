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

    NVGcolor backgroundColour;
    NVGcolor selectedOutlineColour;
    NVGcolor outlineColour;
    NVGcolor flagColour;

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
            repaint();
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

        backgroundColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectBackgroundColourId));
        selectedOutlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId));
        outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));
        flagColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::guiObjectInternalOutlineColour));

        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        auto b = getLocalBounds().toFloat();
        auto sb = b.reduced(0.5f); // reduce size of background to stop AA edges from showing through

        bool highlighted = hasKeyboardFocus(true) && ::getValue<bool>(object->locked);

        nvgSave(nvg);
        nvgIntersectRoundedScissor(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(), Corners::objectCornerRadius);

        // Background
        nvgBeginPath(nvg);
        nvgRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
        nvgFillColor(nvg, backgroundColour);
        nvgFill(nvg);

        imageRenderer.renderJUCEComponent(nvg, input, getImageScale());

        // draw flag
        nvgBeginPath(nvg);
        nvgFillColor(nvg, highlighted ? selectedOutlineColour : flagColour);
        nvgMoveTo(nvg, b.getRight() - 8, b.getY());
        nvgLineTo(nvg, b.getRight(), b.getY());
        nvgLineTo(nvg, b.getRight(), b.getY() + 8);
        nvgClosePath(nvg);
        nvgFill(nvg);

        nvgRestore(nvg);

        nvgDrawRoundedRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(), nvgRGBAf(0, 0, 0, 0), (object->isSelected() || highlighted) ? selectedOutlineColour : outlineColour, Corners::objectCornerRadius);
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
