/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class SymbolAtomObject final : public ObjectBase
    , public KeyListener {

    bool isDown = false;
    bool isLocked = false;

    AtomHelper atomHelper;

    String lastMessage;

    Label input;

public:
    SymbolAtomObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
    {
        addAndMakeVisible(input);

        input.addMouseListener(this, false);

        input.onTextChange = [this]() {
            startEdition();
            setSymbol(input.getText().toStdString());
            stopEdition();

            auto width = input.getFont().getStringWidth(input.getText()) + 36;
            if (width < object->getWidth()) {
                object->setSize(width, object->getHeight());
            }
        };

        input.onEditorShow = [this]() {
            auto* editor = input.getCurrentTextEditor();
            editor->setBorder({ 1, 1, 0, 0 });
            editor->addKeyListener(this);
        };

        input.setMinimumHorizontalScale(0.9f);

        object->addMouseListener(this, false);
    }

    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
    }

    void updateBounds() override
    {
        atomHelper.updateBounds();
    }

    void applyBounds() override
    {
        atomHelper.applyBounds();
    }

    bool checkBounds(Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft) override
    {
        atomHelper.checkBounds(oldBounds, newBounds, resizingOnLeft);
        updateBounds();
        return true;
    }

    ObjectParameters getParameters() override
    {
        return atomHelper.getParameters();
    }

    void setSymbol(String const& value)
    {
        cnv->pd->enqueueDirectMessages(ptr, value.toStdString());
    }

    String getSymbol()
    {
        cnv->pd->setThis();
        return String::fromUTF8(atom_getsymbol(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)))->s_name);
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

    void paint(Graphics& g) override
    {
        getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
        getLookAndFeel().setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        getLookAndFeel().setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        getLookAndFeel().setColour(TextEditor::backgroundColourId, object->findColour(PlugDataColour::guiObjectBackgroundColourId));

        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::outlineColourId));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));
        triangle = triangle.createPathWithRoundedCorners(4.0f);
        g.fillPath(triangle);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), PlugDataLook::objectCornerRadius, 1.0f);
    }

    void updateLabel() override
    {
        atomHelper.updateLabel(label);
    }

    void valueChanged(Value& v) override
    {
        atomHelper.valueChanged(v);
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

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (objectMessageMapped[symbol]) {
            case objectMessage::msg_symbol: {
                input.setText(atoms[0].getSymbol(), dontSendNotification);
                break;
            }
            case objectMessage::msg_send: {
                if (atoms.size() >= 1)
                    setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].getSymbol());
                break;
            }
            case objectMessage::msg_receive: {
                if (atoms.size() >= 1) {
                    setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].getSymbol());
                }
                break;
            }
            default: break;
        }
    }
};
