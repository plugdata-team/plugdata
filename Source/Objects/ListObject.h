
struct ListObject final : public AtomObject
{
    ListObject(void* obj, Box* parent) : AtomObject(obj, parent), dragger(label)
    {
        label.setBounds(2, 0, getWidth() - 2, getHeight() - 1);
        label.setMinimumHorizontalScale(1.f);
        label.setJustificationType(Justification::centredLeft);
        label.setBorderSize(BorderSize<int>(2, 6, 2, 2));
        label.setText(String(getValueOriginal()), dontSendNotification);
        label.setEditable(true, false);

        setInterceptsMouseClicks(true, false);
        addAndMakeVisible(label);

        label.onEditorHide = [this]()
        {
            startEdition();
            updateFromGui();
            stopEdition();
        };

        label.onEditorShow = [this]()
        {
            auto* editor = label.getCurrentTextEditor();
            if (editor != nullptr)
            {
                editor->setIndents(1, 2);
                editor->setBorder(BorderSize<int>(2, 6, 2, 2));
            }
        };

        dragger.dragStart = [this]() { startEdition(); };

        dragger.valueChanged = [this](float) { updateFromGui(); };

        dragger.dragEnd = [this]() { stopEdition(); };

        updateValue();

        initialise();
    }

    void updateFromGui()
    {
        auto array = StringArray();
        array.addTokens(label.getText(), true);
        std::vector<pd::Atom> list;
        list.reserve(array.size());
        for (auto const& elem : array)
        {
            if (elem.getCharPointer().isDigit())
            {
                list.push_back({elem.getFloatValue()});
            }
            else
            {
                list.push_back({elem.toStdString()});
            }
        }
        if (list != getList())
        {
            setList(list);
        }
    }

    void checkBounds() override
    {
        // Apply size limits
        int w = jlimit(100, maxSize, box->getWidth());
        int h = jlimit(Box::height - 2, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
    }

    ~ListObject()
    {
    }

    void resized() override
    {
        AtomObject::resized();

        label.setBounds(getLocalBounds());
    }

    void paint(Graphics& g) override
    {
        getLookAndFeel().setColour(Label::textWhenEditingColourId, box->findColour(Label::textWhenEditingColourId));
        getLookAndFeel().setColour(Label::textColourId, box->findColour(Label::textColourId));

        g.fillAll(box->findColour(PlugDataColour::canvasOutlineColourId));

        static auto const border = 1.0f;
        const auto h = static_cast<float>(getHeight());
        const auto w = static_cast<float>(getWidth());
        const auto o = h * 0.33f;
        Path p;
        p.startNewSubPath(0.5f, 0.5f);
        p.lineTo(0.5f, h - 0.5f);
        p.lineTo(w - o, h - 0.5f);
        p.lineTo(w - 0.5f, h - o);
        p.lineTo(w - 0.5f, o);
        p.lineTo(w - o, 0.5f);
        p.closeSubPath();

        g.setColour(box->findColour(PlugDataColour::toolbarColourId));
        g.fillPath(p);
    }

    void update() override
    {
        if (!edited && !label.isBeingEdited())
        {
            auto const array = getList();
            String message;
            for (auto const& atom : array)
            {
                if (message.isNotEmpty())
                {
                    message += " ";
                }
                if (atom.isFloat())
                {
                    message += String(atom.getFloat());
                }
                else if (atom.isSymbol())
                {
                    message += String(atom.getSymbol());
                }
            }
            label.setText(message, NotificationType::dontSendNotification);
        }
    }

    std::vector<pd::Atom> getList() const noexcept
    {
        std::vector<pd::Atom> array;
        cnv->pd->setThis();

        int ac = binbuf_getnatom(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);
        t_atom* av = binbuf_getvec(static_cast<t_fake_gatom*>(ptr)->a_text.te_binbuf);
        array.reserve(ac);
        for (int i = 0; i < ac; ++i)
        {
            if (av[i].a_type == A_FLOAT)
            {
                array.emplace_back(atom_getfloat(av + i));
            }
            else if (av[i].a_type == A_SYMBOL)
            {
                array.emplace_back(atom_getsymbol(av + i)->s_name);
            }
            else
            {
                array.emplace_back();
            }
        }
        return array;
    }

    void setList(std::vector<pd::Atom> const& value) noexcept
    {
        cnv->pd->enqueueDirectMessages(ptr, value);
    }

   private:
    Label label;
    DraggableListNumber dragger;
};
