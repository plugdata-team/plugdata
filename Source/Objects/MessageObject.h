
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


struct MessageObject : public GUIObject
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

            auto width = input.getFont().getStringWidth(input.getText()) + 36;
            if (width < box->getWidth())
            {
                box->setSize(width, box->getHeight());
                checkBoxBounds();
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
                auto width = input.getFont().getStringWidth(editor->getText()) + 25;

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

    void updateBounds() override {
        int x = 0, y = 0, w = 0, h = 0;

        // If it's a text object, we need to handle the resizable width, which pd saves in amount of text characters
        auto* textObj = static_cast<t_text*>(ptr);

        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);
        
        Rectangle<int> bounds = {x, y, textObj->te_width, h};
        
        box->setBounds(bounds.expanded(Box::margin));
    }
    
    void checkBoxBounds() override
    {
       
        int numLines = getNumLines(getText(), box->getWidth() - Box::doubleMargin - 5);
        int newHeight = (numLines * 19) + Box::doubleMargin + 2;
        
        // parent component check prevents infinite recursion bug
        // TODO: fix this in a better way!
        if(getParentComponent() && box->getHeight() != newHeight) {
            box->setSize(box->getWidth(), newHeight);
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

    void resized() override
    {
        input.setBounds(getLocalBounds());
    }

    void update() override
    {
        input.setText(String(getSymbol()), sendNotification);
    }

    void paintOverChildren(Graphics& g) override
    {
        GUIObject::paintOverChildren(g);

        auto b = getLocalBounds();

        Path flagPath;
        flagPath.addQuadrilateral(b.getRight(), b.getY(), b.getRight() - 4, b.getY() + 4, b.getRight() - 4, b.getBottom() - 4, b.getRight(), b.getBottom());

        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        g.fillPath(flagPath);
        
        if(isDown) {
            g.drawRoundedRectangle(getLocalBounds().toFloat(), 2.0f, 4.0f);
        }

    }

    void updateValue() override
    {
        if (!edited)
        {
            String v = getSymbol();

            if (lastMessage != v && !v.startsWith("click"))
            {
                numLines = 1;
                longestLine = 7;

                int currentLineLength = 0;
                for (auto c : v)
                {
                    if (c == '\n')
                    {
                        numLines++;
                        longestLine = std::max(longestLine, currentLineLength);
                        currentLineLength = 0;
                    }
                    else
                    {
                        currentLineLength++;
                    }
                }
                if (numLines == 1) longestLine = std::max(longestLine, currentLineLength);

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
    
    /*
    bool usesCharWidth() override
    {
        return true;
    } */

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

    void setSymbol(std::string const& value) noexcept
    {
        cnv->pd->enqueueFunction(
            [this, value]() mutable
            {
                auto* cstr = value.c_str();
                auto* messobj = static_cast<t_message*>(ptr);
                binbuf_clear(messobj->m_text.te_binbuf);
                binbuf_text(messobj->m_text.te_binbuf, value.c_str(), value.size());
                glist_retext(messobj->m_glist, &messobj->m_text);
            });
    }

    
    Label input;

    int numLines = 1;
    int longestLine = 7;
};
