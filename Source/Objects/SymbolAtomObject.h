/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once

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
    Colour selCol;
    NVGcolor outlineColour;

public:
    SymbolAtomObject(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
    {
        addAndMakeVisible(input);

        input.setText(getSymbol(), dontSendNotification);

        input.addMouseListener(this, false);

        input.onTextChange = [this] {
            startEdition();
            setSymbol(input.getText(true).toStdString());
            stopEdition();
        };

        input.onEditorShow = [this] {
            auto* editor = input.getCurrentTextEditor();
            editor->setBorder({ 0, 1, 3, 0 });

            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            editor->addKeyListener(this);
            editor->onTextChange = [this] {
                // To resize while typing
                if (atomHelper.getWidthInChars() == 0) {
                    object->updateBounds();
                }
            };
            repaint();
        };

        input.setMinimumHorizontalScale(0.9f);

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty, var(), true, 0);
        atomHelper.addAtomParameters(objectParameters);
        lookAndFeelChanged();
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

    void lock(bool const locked) override
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
            cnv->pd->sendDirectMessage(gatom.get(), SmallString(value));
        }
    }

    String getSymbol() const
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
        if (isLocked && isShowing()) {
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
        selCol = cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectSelectedOutlineColourId);
        selectedOutlineColour = convertColour(selCol);
        outlineColour = convertColour(cnv->editor->getLookAndFeel().findColour(PlugDataColour::objectOutlineColourId));

        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        auto const b = getLocalBounds().toFloat();
        auto const sb = b.reduced(0.5f); // reduce size of background to stop AA edges from showing through

        // Draw background
        nvgDrawObjectWithFlag(nvg, sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(),
            cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol, cnv->guiObjectBackgroundCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagTop, static_cast<PlugDataLook&>(cnv->getLookAndFeel()).getUseFlagOutline());

        imageRenderer.renderJUCEComponent(nvg, input, getImageScale());

        bool const highlighted = hasKeyboardFocus(true) && getValue<bool>(object->locked);
        auto const flagCol = highlighted ? cnv->selectedOutlineCol : cnv->guiObjectInternalOutlineCol;
        auto const outlineCol = object->isSelected() || hasKeyboardFocus(true) ? cnv->selectedOutlineCol : cnv->objectOutlineCol;

        // Fill the internal of the shape with transparent colour, draw outline & flag with shader
        nvgDrawObjectWithFlag(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight(),
            nvgRGBA(0, 0, 0, 0), outlineCol, flagCol,
            Corners::objectCornerRadius, ObjectFlagType::FlagTop, static_cast<PlugDataLook&>(cnv->getLookAndFeel()).getUseFlagOutline());
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

    void propertyChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto const* constrainer = getConstrainer();
            auto const width = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());

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
                if (editor->getHighlightedRegion().getLength()) {
                    editor->setCaretPosition(editor->getHighlightedRegion().getEnd());
                    return true;
                }
            }
        } else if (key.getKeyCode() == KeyPress::returnKey) {
            setSymbol(input.getText(true).toStdString());
            cnv->grabKeyboardFocus();
            return true;
        }

        return false;
    }

    void receiveObjectMessage(hash32 const symbol, SmallArray<pd::Atom> const& atoms) override
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
            if (atoms.size() >= 1)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].toString());
            object->updateIolets();
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1) {
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].toString());
            }
            object->updateIolets();
            break;
        }
        default:
            break;
        }
    }
};
