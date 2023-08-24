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
    SymbolAtomObject(void* obj, Object* parent)
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
        };

        input.setMinimumHorizontalScale(0.9f);
        
        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);
        atomHelper.addAtomParameters(objectParameters);
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
        g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));
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
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);

        bool highlighed = hasKeyboardFocus(true) && getValue<bool>(object->locked);

        if (highlighed) {
            g.setColour(object->findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Corners::objectCornerRadius, 2.0f);
        }
    }

    bool hideInlets() override
    {
        return atomHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return atomHelper.hasSendSymbol();
    }

    void updateLabel() override
    {
        atomHelper.updateLabel(label);
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(sizeProperty)) {
            auto* constrainer = getConstrainer();
            auto width = std::max(::getValue<int>(sizeProperty), constrainer->getMinimumWidth());
            
            setParameterExcludingListener(sizeProperty, width);
            
            atomHelper.setWidthInChars(width);
            object->updateBounds();
        }
        else {
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

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("set"),
            hash("symbol"),
            hash("list"),
            hash("send"),
            hash("receive")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {

        case hash("set"):
        case hash("list"):
        case hash("symbol"): {
            input.setText(atoms[0].getSymbol(), dontSendNotification);
            break;
        }
        case hash("float"): {
            input.setText("float", dontSendNotification);
            break;
        }
        case hash("send"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(atomHelper.sendSymbol, atoms[0].getSymbol());
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1) {
                setParameterExcludingListener(atomHelper.receiveSymbol, atoms[0].getSymbol());
            }
            break;
        }
        default:
            break;
        }
    }
};
