
struct MessageComponent : public GUIComponent
{
    bool isDown = false;
    bool isLocked = false;
    bool shouldOpenEditor = false;

    std::string lastMessage;

    MessageComponent(const pd::Gui& pdGui, Box* parent, bool newObject) : GUIComponent(pdGui, parent, newObject)
    {
        addAndMakeVisible(input);

        input.setInterceptsMouseClicks(false, false);

        input.onTextChange = [this]()
        {
            startEdition();
            gui.setSymbol(input.getText().toStdString());
            stopEdition();

            auto width = input.getFont().getStringWidth(input.getText()) + 25;
            if (width < box->getWidth())
            {
                box->setSize(width, box->getHeight());
                checkBoxBounds();
            }
        };

        // message box behaviour
        if (!gui.isAtom())
        {
            // For the autoresize while typing feature
            input.onEditorShow = [this]()
            {
                auto* editor = input.getCurrentTextEditor();

                editor->onTextChange = [this, editor]()
                {
                    auto width = input.getFont().getStringWidth(editor->getText()) + 25;

                    if (width > box->getWidth())
                    {
                        box->setSize(width, box->getHeight());
                    }
                };
            };
        }

        initialise(newObject);

        if (gui.isAtom())
        {
            auto fontHeight = gui.getFontHeight();
            if (fontHeight == 0)
            {
                fontHeight = glist_getfont(box->cnv->patch.getPointer());
            }
            input.setFont(fontHeight);
        }

        box->addMouseListener(this, false);
    }

    void checkBoxBounds() override
    {
        // Apply size limits
        int w = jlimit(50, maxSize, box->getWidth());
        int h = jlimit(Box::height - 2, maxSize, box->getHeight());

        if (w != box->getWidth() || h != box->getHeight())
        {
            box->setSize(w, h);
        }
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
        input.setText(String(gui.getSymbol()), sendNotification);
    }

    void paintOverChildren(Graphics& g) override
    {
        GUIComponent::paintOverChildren(g);

        auto b = getLocalBounds();

        if (!gui.isAtom())
        {
            Path flagPath;
            flagPath.addQuadrilateral(b.getRight(), b.getY(), b.getRight() - 4, b.getY() + 4, b.getRight() - 4, b.getBottom() - 4, b.getRight(), b.getBottom());

            g.setColour(box->findColour(PlugDataColour::highlightColourId));
            g.fillPath(flagPath);
        }

        g.setColour(box->findColour(PlugDataColour::canvasOutlineColourId));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 2.0f, 1.5f);
    }

    void updateValue() override
    {
        if (!edited)
        {
            std::string const v = gui.getSymbol();

            if (lastMessage != v && !String(v).startsWith("click"))
            {
                numLines = 1;
                longestLine = 7;

                int currentLineLength = 0;
                for (auto& c : v)
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
        GUIComponent::mouseDown(e);
        if (!gui.isAtom() && isLocked)
        {
            isDown = true;
            repaint();

            startEdition();
            gui.click();
            stopEdition();
        }
        if (box->cnv->isSelected(box) && !box->selectionChanged)
        {
            shouldOpenEditor = true;
        }
    }

    void mouseUp(const MouseEvent& e) override
    {
        isDown = false;

        // Edit messages when unlocked, edit atoms when locked
        if ((!isLocked && shouldOpenEditor && !e.mouseWasDraggedSinceMouseDown() && !gui.isAtom()) || (isLocked && gui.isAtom()))
        {
            input.showEditor();
            shouldOpenEditor = false;
        }

        repaint();
    }

    void valueChanged(Value& v) override
    {
        if (gui.isAtom() && v.refersToSameSourceAs(labelHeight))
        {
            updateLabel();
            box->updateBounds(false);  // update box size based on new font

            auto fontHeight = gui.getFontHeight();
            if (fontHeight == 0)
            {
                fontHeight = glist_getfont(box->cnv->patch.getPointer());
            }
            input.setFont(fontHeight);
        }
        else
        {
            GUIComponent::valueChanged(v);
        }
    }

    int numLines = 1;
    int longestLine = 7;
};
