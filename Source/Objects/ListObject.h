/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ListObject final : public ObjectBase {

    AtomHelper atomHelper;
    DraggableListNumber listLabel;

    Value min = SynchronousValue(0.0f);
    Value max = SynchronousValue(0.0f);
    Value sizeProperty = SynchronousValue();

public:
    ListObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
    {
        listLabel.setBounds(2, 0, getWidth() - 2, getHeight() - 1);
        listLabel.setMinimumHorizontalScale(1.f);
        listLabel.setJustificationType(Justification::centredLeft);
        // listLabel.setBorderSize(BorderSize<int>(2, 6, 2, 2));

        addAndMakeVisible(listLabel);

        listLabel.onEditorHide = [this]() {
            startEdition();
            updateFromGui();
            stopEdition();
        };

        listLabel.onEditorShow = [this]() {
            auto* editor = listLabel.getCurrentTextEditor();
            editor->setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
            if (editor != nullptr) {
                editor->setBorder({ 0, 1, 3, 0 });
            }
        };

        listLabel.dragStart = [this]() {
            startEdition();
        };

        listLabel.onValueChange = [this](float) { updateFromGui(); };

        listLabel.dragEnd = [this]() {
            stopEdition();
        };

        listLabel.addMouseListener(this, false);

        listLabel.setText("0 0", dontSendNotification);
        updateFromGui();

        objectParameters.addParamInt("Width (chars)", cDimensions, &sizeProperty);
        objectParameters.addParamFloat("Minimum", cGeneral, &min);
        objectParameters.addParamFloat("Maximum", cGeneral, &max);
        atomHelper.addAtomParameters(objectParameters);
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
        }
        else if (value.refersToSameSourceAs(min)) {
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

    void updateFromGui()
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
            }
            else {
                list.emplace_back(elem);
            }
        }
        if (list != getList()) {
            setList(list);
        }
    }

    ~ListObject() override
    {
    }

    void resized() override
    {
        listLabel.setFont(listLabel.getFont().withHeight(getHeight() - 6));
        listLabel.setBounds(getLocalBounds());
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

    void updateLabel() override
    {
        atomHelper.updateLabel(label);
    }

    bool hideInlets() override
    {
        return atomHelper.hasReceiveSymbol();
    }

    bool hideOutlets() override
    {
        return atomHelper.hasSendSymbol();
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::guiObjectInternalOutlineColour));
        Path triangles;
        triangles.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));
        triangles.addTriangle(Point<float>(getWidth() - 8, getHeight()), Point<float>(getWidth(), getHeight()), Point<float>(getWidth(), getHeight() - 8));

        auto reducedBounds = getLocalBounds().toFloat().reduced(0.5f);

        Path roundEdgeClipping;
        roundEdgeClipping.addRoundedRectangle(reducedBounds, Corners::objectCornerRadius);

        g.saveState();
        g.reduceClipRegion(roundEdgeClipping);
        g.fillPath(triangles);
        g.restoreState();

        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(reducedBounds, Corners::objectCornerRadius, 1.0f);

        bool highlighed = hasKeyboardFocus(true) && getValue<bool>(object->locked);

        if (highlighed) {
            g.setColour(object->findColour(PlugDataColour::objectSelectedOutlineColourId));
            g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), Corners::objectCornerRadius, 2.0f);
        }
    }

    void lookAndFeelChanged() override
    {
        listLabel.setColour(Label::textWhenEditingColourId, object->findColour(PlugDataColour::canvasTextColourId));
        listLabel.setColour(Label::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        listLabel.setColour(TextEditor::textColourId, object->findColour(PlugDataColour::canvasTextColourId));
        repaint();
    }

    void paint(Graphics& g) override
    {
        g.setColour(object->findColour(PlugDataColour::guiObjectBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius);
    }

    // If we already know the atoms, this will allow a lock-free update
    void updateValue(std::vector<pd::Atom> array)
    {
        if (!listLabel.isBeingEdited()) {
            String message;
            for (auto const& atom : array) {
                if (message.isNotEmpty()) {
                    message += " ";
                }
                if (atom.isFloat()) {
                    message += String(atom.getFloat());
                } else if (atom.isSymbol()) {
                    message += String(atom.getSymbol());
                }
            }
            listLabel.setText(message, NotificationType::dontSendNotification);
        }
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
                if (atom.isFloat()) {
                    message += String(atom.getFloat());
                } else if (atom.isSymbol()) {
                    message += String(atom.getSymbol());
                }
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

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("float"),
            hash("symbol"),
            hash("list"),
            hash("set"),
            hash("send"),
            hash("receive")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("symbol"):
        case hash("list"):
        case hash("set"): {
            updateValue(atoms);
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
