
struct ListObject final : public AtomObject {
    ListObject(void* obj, Object* parent)
        : AtomObject(obj, parent)
    {
        listLabel.setBounds(2, 0, getWidth() - 2, getHeight() - 1);
        listLabel.setMinimumHorizontalScale(1.f);
        listLabel.setJustificationType(Justification::centredLeft);
        listLabel.setBorderSize(BorderSize<int>(2, 6, 2, 2));
        listLabel.setText(String(getValueOriginal()), dontSendNotification);

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

        listLabel.dragStart = [this]() { startEdition(); };

        listLabel.valueChanged = [this](float) { updateFromGui(); };

        listLabel.dragEnd = [this]() { stopEdition(); };

        listLabel.addMouseListener(this, false);

        listLabel.setText("0 0", dontSendNotification);
        updateFromGui();

        updateValue();
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
                list.push_back({ elem.toStdString() });
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
        AtomObject::resized();

        listLabel.setBounds(getLocalBounds());
    }

    void paint(Graphics& g) override
    {
        getLookAndFeel().setColour(Label::textWhenEditingColourId, object->findColour(Label::textWhenEditingColourId));
        getLookAndFeel().setColour(Label::textColourId, object->findColour(Label::textColourId));

        g.setColour(object->findColour(PlugDataColour::toolbarColourId));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 2.0f);

        g.setColour(object->findColour(PlugDataColour::canvasOutlineColourId));

        Path bottomTriangle;
        bottomTriangle.addTriangle(Point<float>(getWidth() - 8, getHeight()), Point<float>(getWidth(), getHeight()), Point<float>(getWidth(), getHeight() - 8));
        bottomTriangle = bottomTriangle.createPathWithRoundedCorners(4.0f);
        g.fillPath(bottomTriangle);
    }

    void updateValue() override
    {
        if (!edited && !listLabel.isBeingEdited()) {
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
        std::vector<pd::Atom> array;
        cnv->pd->setThis();

        int ac = binbuf_getnatom(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);
        t_atom* av = binbuf_getvec(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);
        array.reserve(ac);
        for (int i = 0; i < ac; ++i) {
            if (av[i].a_type == A_FLOAT) {
                array.emplace_back(atom_getfloat(av + i));
            } else if (av[i].a_type == A_SYMBOL) {
                array.emplace_back(atom_getsymbol(av + i)->s_name);
            } else {
                array.emplace_back();
            }
        }
        return array;
    }

    void setList(std::vector<pd::Atom> const& value)
    {
        cnv->pd->enqueueDirectMessages(ptr, value);
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (static_cast<bool>(object->locked.getValue()) && !e.mouseWasDraggedSinceMouseDown()) {

            listLabel.showEditor();
        }
    }

private:
    DraggableListNumber listLabel;
};
