
struct SymbolAtomObject final : public AtomObject
{
    bool isDown = false;
    bool isLocked = false;

    String lastMessage;

    SymbolAtomObject(void* obj, Box* parent) : AtomObject(obj, parent)
    {
        addAndMakeVisible(input);

        input.setInterceptsMouseClicks(false, false);

        input.onTextChange = [this]()
        {
            startEdition();
            setSymbol(input.getText().toStdString());
            stopEdition();

            auto width = input.getFont().getStringWidth(input.getText()) + 36;
            if (width < box->getWidth())
            {
                box->setSize(width, box->getHeight());
                checkBounds();
            }
        };

        input.setMinimumHorizontalScale(0.9f);

        box->addMouseListener(this, false);
    }


    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    void resized() override
    {
        AtomObject::resized();

        input.setBounds(getLocalBounds());
    }

    void update() override
    {
        input.setText(String(getSymbol()), sendNotification);
    }

    void setSymbol(String const& value) 
    {
        cnv->pd->enqueueDirectMessages(ptr, value.toStdString());
    }

    String getSymbol()
    {
        cnv->pd->setThis();
        return atom_getsymbol(fake_gatom_getatom(static_cast<t_fake_gatom*>(ptr)))->s_name;
    }

    void updateValue() override
    {
        if (!edited)
        {
            String v = getSymbol();

            if (lastMessage != v && !v.startsWith("click"))
            {
                lastMessage = v;
                update();
            }
        }
    }

    void mouseUp(const MouseEvent& e) override
    {
        isDown = false;

        // Edit messages when unlocked, edit atoms when locked
        if (isLocked)
        {
            input.showEditor();
        }

        repaint();
    }

    void valueChanged(Value& v) override
    {
        if (v.refersToSameSourceAs(labelHeight))
        {
            updateLabel();
            if (getParentComponent())
            {
                box->updateBounds();  // update box size based on new font
            }
        }
        else
        {
            AtomObject::valueChanged(v);
        }
    }

    Label input;
};
