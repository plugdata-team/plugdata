/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class ListObject final : public ObjectBase {

    AtomHelper atomHelper;
    DraggableListNumber listLabel;

public:
    ListObject(void* obj, Object* parent)
        : ObjectBase(obj, parent)
        , atomHelper(obj, parent, this)
    {
        listLabel.setBounds(2, 0, getWidth() - 2, getHeight() - 1);
        listLabel.setMinimumHorizontalScale(1.f);
        listLabel.setJustificationType(Justification::centredLeft);
        listLabel.setBorderSize(BorderSize<int>(2, 6, 2, 2));

        addAndMakeVisible(listLabel);

        listLabel.onEditorHide = [this]() {
            startEdition();
            updateFromGui();
            stopEdition();
        };

        listLabel.onEditorShow = [this]() {
            auto* editor = listLabel.getCurrentTextEditor();
            if (editor != nullptr) {
                editor->setBorder({ 1, 2, 0, 0 });
            }
        };

        listLabel.dragStart = [this]() {
            startEdition();
        };

        listLabel.valueChanged = [this](float) { updateFromGui(); };

        listLabel.dragEnd = [this]() {
            stopEdition();
        };

        listLabel.addMouseListener(this, false);

        listLabel.setText("0 0", dontSendNotification);
        updateFromGui();

        updateValue();
    }

    void valueChanged(Value& v) override
    {
        atomHelper.valueChanged(v);
    }

    void updateFromGui()
    {
        auto array = StringArray();
        array.addTokens(listLabel.getText(), true);
        std::vector<pd::Atom> list;
        list.reserve(array.size());
        for (auto const& elem : array) {
            if (elem.getCharPointer().isDigit()) {
                list.push_back({ elem.getFloatValue() });
            } else {
                list.push_back({ elem });
            }
        }
        if (list != getList()) {
            setList(list);
        }
    }

    ~ListObject()
    {
    }

    void resized() override
    {
        listLabel.setFont(Font(getHeight() - 6));
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

    bool checkBounds(Rectangle<int> oldBounds, Rectangle<int> newBounds, bool resizingOnLeft) override
    {
        atomHelper.checkBounds(oldBounds, newBounds, resizingOnLeft);
        object->updateBounds();
        return true;
    }

    ObjectParameters getParameters() override
    {
        return atomHelper.getParameters();
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
        g.setColour(object->findColour(PlugDataColour::outlineColourId));
        Path triangle;
        triangle.addTriangle(Point<float>(getWidth() - 8, 0), Point<float>(getWidth(), 0), Point<float>(getWidth(), 8));
        triangle = triangle.createPathWithRoundedCorners(4.0f);
        g.fillPath(triangle);

        bool selected = cnv->isSelected(object) && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
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

        g.setColour(object->findColour(PlugDataColour::outlineColourId));

        Path bottomTriangle;
        bottomTriangle.addTriangle(Point<float>(getWidth() - 8, getHeight()), Point<float>(getWidth(), getHeight()), Point<float>(getWidth(), getHeight() - 8));
        bottomTriangle = bottomTriangle.createPathWithRoundedCorners(4.0f);
        g.fillPath(bottomTriangle);
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
        cnv->pd->setThis();

        int ac = binbuf_getnatom(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);
        t_atom* av = binbuf_getvec(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);

        return pd::Atom::fromAtoms(ac, av);
    }

    void setList(std::vector<pd::Atom> const value)
    {
        cnv->pd->enqueueDirectMessages(ptr, std::move(value));
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (static_cast<bool>(object->locked.getValue()) && !e.mouseWasDraggedSinceMouseDown()) {

            listLabel.showEditor();
        }
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("symbol"):
        case hash("list"):
        case hash("set"): {
            updateValue();
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
