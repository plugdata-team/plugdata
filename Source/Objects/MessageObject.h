
typedef struct _messresponder
{
    t_pd mr_pd;
    t_outlet* mr_outlet;
} t_messresponder;

typedef struct _message
{
    t_text m_text;
    t_messresponder m_messresponder;
    t_glist* m_glist;
    t_clock* m_clock;
} t_message;

struct MessageObject final : public GUIObject
{
    bool isDown = false;
    bool isLocked = false;
    bool shouldOpenEditor = false;

    String lastMessage;

    MessageObject(void* obj, Box* parent) : GUIObject(obj, parent)
    {
        addAndMakeVisible(input);

        input.setInterceptsMouseClicks(false, false);

        input.onTextChange = [this]()
        {
            startEdition();
            setSymbol(input.getText().toStdString());
            stopEdition();

            auto width = Font(15).getStringWidth(input.getText()) + 35;
            if (width < box->getWidth())
            {
                box->setSize(width, box->getHeight());
                checkBounds();
            }
        };

        // For the autoresize while typing feature
        input.onEditorShow = [this]()
        {
            auto* editor = input.getCurrentTextEditor();

            editor->setMultiLine(true, true);
            editor->setReturnKeyStartsNewLine(true);
            editor->onTextChange = [this, editor]()
            {
                auto width = input.getFont().getStringWidth(editor->getText()) + 35;

                if (width > box->getWidth())
                {
                    box->setSize(width, box->getHeight());
                }
            };
        };

        input.setMinimumHorizontalScale(0.9f);
        initialise();

        box->addMouseListener(this, false);
    }

    void updateBounds() override
    {
        int x = 0, y = 0, w = 0, h = 0;

        // If it's a text object, we need to handle the resizable width, which pd saves in amount of text characters
        auto* textObj = static_cast<t_text*>(ptr);

        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        int width = textObj->te_width * glist_fontwidth(cnv->patch.getPointer());

        if (textObj->te_width == 0)
        {
            width = Font(15).getStringWidth(getText()) + 19;
        }

        box->setObjectBounds({x, y, width, h});
    }

    void checkBounds() override
    {
        int numLines = getNumLines(getText(), box->getWidth() - Box::doubleMargin - 5);
        int newHeight = (numLines * 19) + Box::doubleMargin + 2;

        int newWidth = std::max(50, box->getWidth());
        if (getParentComponent() && (box->getHeight() != newHeight || newWidth != box->getWidth()))
        {
            box->setSize(newWidth, newHeight);
        }
    }

    void showEditor() override
    {
        input.showEditor();
    }

    void lock(bool locked) override
    {
        isLocked = locked;
        setInterceptsMouseClicks(isLocked, isLocked);
    }

    void applyBounds() override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), box->getX() + Box::margin, box->getY() + Box::margin);

        auto* textObj = static_cast<t_text*>(ptr);
        textObj->te_width = getWidth() / glist_fontwidth(cnv->patch.getPointer());
    }

    void resized() override
    {
        input.setBounds(getLocalBounds());
        if (auto* editor = input.getCurrentTextEditor())
        {
            editor->setBounds(getLocalBounds());
        }
    }

    void update() override
    {
        input.setText(String(getSymbol()), sendNotification);
    }

    void paintOverChildren(Graphics& g) override
    {
        GUIObject::paintOverChildren(g);

        auto b = getLocalBounds().reduced(1);

        Path flagPath;
        flagPath.addQuadrilateral(b.getRight(), b.getY(), b.getRight() - 4, b.getY() + 4, b.getRight() - 4, b.getBottom() - 4, b.getRight(), b.getBottom());

        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        g.fillPath(flagPath);

        if (isDown)
        {
            g.drawRoundedRectangle(b.reduced(1).toFloat(), 2.0f, 3.0f);
        }
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
                // repaint();
            }
        }
    }

    void mouseDown(const MouseEvent& e) override
    {
        GUIObject::mouseDown(e);
        if (isLocked)
        {
            isDown = true;
            repaint();

            startEdition();
            click();
            stopEdition();
        }
        if (cnv->isSelected(box) && !box->selectionChanged)
        {
            shouldOpenEditor = true;
        }
    }

    void click() noexcept
    {
        cnv->pd->enqueueDirectMessages(ptr, 0);
    }

    void mouseUp(const MouseEvent& e) override
    {
        isDown = false;

        // Edit messages when unlocked, edit atoms when locked
        if (!isLocked && shouldOpenEditor && !e.mouseWasDraggedSinceMouseDown())
        {
            input.showEditor();
            shouldOpenEditor = false;
        }

        repaint();
    }

    void valueChanged(Value& v) override
    {
        GUIObject::valueChanged(v);
    }


    String getSymbol() const noexcept
    {
        cnv->pd->setThis();

        char* text;
        int size;

        binbuf_gettext(static_cast<t_message*>(ptr)->m_text.te_binbuf, &text, &size);

        auto result = String(text, size);
        freebytes(text, size);

        return result;
    }

    void setSymbol(String const& value) noexcept
    {
        cnv->pd->enqueueFunction(
            [this, value]() mutable
            {
                auto* cstr = value.toRawUTF8();
                auto* messobj = static_cast<t_message*>(ptr);
                binbuf_clear(messobj->m_text.te_binbuf);
                binbuf_text(messobj->m_text.te_binbuf, cstr, value.getNumBytesAsUTF8());
                glist_retext(messobj->m_glist, &messobj->m_text);
            });
    }

    bool hideInGraph() override
    {
        return true;
    }

    Label input;
};
