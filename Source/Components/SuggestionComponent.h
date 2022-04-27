
// Suggestions component that shows up when objects are edited
class SuggestionComponent : public Component, public KeyListener, public TextEditor::InputFilter
{
    class Suggestion : public TextButton
    {
        int idx = 0;
        int type = -1;

        Array<String> letters = {"pd", "~"};

        String objectDescription;

       public:
        Suggestion(int i) : idx(i)
        {
            setText("", "");
            setWantsKeyboardFocus(true);
            setConnectedEdges(12);
            setClickingTogglesState(true);
            setRadioGroupId(1001);
            setColour(TextButton::buttonOnColourId, findColour(ScrollBar::thumbColourId));
        }

        void setText(const String& name, const String& description)
        {
            objectDescription = description;
            setButtonText(name);
            type = name.contains("~") ? 1 : 0;

            repaint();
        }

        void paint(Graphics& g) override
        {
             auto colour = idx & 1 ? PlugDataColour::toolbarColourId :  PlugDataColour::canvasColourId;
            
            getLookAndFeel().drawButtonBackground(g, *this, findColour(getToggleState() ? PlugDataColour::highlightColourId : colour), isMouseOver(), isMouseButtonDown());

            
            getLookAndFeel().drawButtonText(g, *this, isMouseOver(), false);

            if (objectDescription.isNotEmpty())
            {
                auto font = getLookAndFeel().getTextButtonFont(*this, getHeight());
                auto textLength = font.getStringWidth(getButtonText()) + 22;

                g.setColour(findColour(PlugDataColour::canvasOutlineColourId));

                auto yIndent = jmin(4, proportionOfHeight(0.3f));
                auto cornerSize = jmin(getHeight(), getWidth()) / 2;
                auto fontHeight = roundToInt(font.getHeight() * 0.5f);
                auto leftIndent = textLength + 10;
                auto rightIndent = jmin(fontHeight, 2 + cornerSize / 2);
                auto textWidth = getWidth() - leftIndent - rightIndent;

                g.drawText("- " + objectDescription, Rectangle<int>(leftIndent, yIndent, textWidth, getHeight() - yIndent * 2), Justification::left);
            }

            if (type == -1) return;

           
            g.setColour((type ? Colours::yellow : findColour(ScrollBar::thumbColourId)).withAlpha(float(0.8)));
            Rectangle<int> iconbound = getLocalBounds().reduced(4);
            iconbound.setWidth(getHeight() - 8);
            iconbound.translate(3, 0);
            g.fillRect(iconbound);

            g.setColour(Colours::white);
            g.drawFittedText(letters[type], iconbound.reduced(1), Justification::centred, 1);
        }
    };

   public:
    bool selecting = false;

    SuggestionComponent() : resizer(this, &constrainer), currentBox(nullptr)
    {
        // Set up the button list that contains our suggestions
        buttonholder = std::make_unique<Component>();

        for (int i = 0; i < 20; i++)
        {
            Suggestion* but = buttons.add(new Suggestion(i));
            buttonholder->addAndMakeVisible(buttons[i]);

            but->setClickingTogglesState(true);
            but->setRadioGroupId(110);
            but->setName("suggestions:button");

            // Colour pattern
            but->setColour(TextButton::buttonColourId, colours[i % 2]);
        }

        // Set up viewport
        port = std::make_unique<Viewport>();
        port->setScrollBarsShown(true, false);
        port->setViewedComponent(buttonholder.get(), false);
        port->setInterceptsMouseClicks(true, true);
        port->setViewportIgnoreDragFlag(true);
        addAndMakeVisible(port.get());

        constrainer.setSizeLimits(150, 120, 500, 400);
        setSize(250, 115);

        addAndMakeVisible(resizer);

        setInterceptsMouseClicks(true, true);
        setAlwaysOnTop(true);
    }

    ~SuggestionComponent() override
    {
        buttons.clear();
    }

    void createCalloutBox(Box* box, TextEditor* editor)
    {
        currentBox = box;
        openedEditor = editor;

        editor->setInputFilter(this, false);
        editor->addKeyListener(this);

        // Should run after the input filter
        editor->onTextChange = [this, editor, box]()
        {
            if (isCompleting && !editor->getText().containsChar(' '))
            {
                editor->setHighlightedRegion({highlightStart, highlightEnd});
            }
            auto width = editor->getTextWidth() + 10;

            if (width > box->getWidth())
            {
                box->setSize(width, box->getHeight());
            }
        };

        for (int i = 0; i < buttons.size(); i++)
        {
            auto* but = buttons[i];
            but->setAlwaysOnTop(true);

            but->onClick = [this, i, editor]() mutable
            {
                move(0, i);
                if (!editor->isVisible()) editor->setVisible(true);
                editor->grabKeyboardFocus();
            };
        }

        // buttons[0]->setToggleState(true, sendNotification);
        // setVisible(editor->getText().isNotEmpty());

        addToDesktop(ComponentPeer::StyleFlags::windowIsTemporary | ComponentPeer::StyleFlags::windowIgnoresKeyPresses);
        repaint();
    }

    void move(int offset, int setto = -1)
    {
        if (!openedEditor) return;

        // Calculate new selected index
        if (setto == -1)
            currentidx += offset;
        else
            currentidx = setto;

        if (numOptions == 0) return;

        // Limit it to minimum of the number of buttons and the number of suggestions
        int numButtons = std::min(20, numOptions);
        currentidx = (currentidx + numButtons) % numButtons;

        auto* but = buttons[currentidx];

        // If we use setto, the toggle state should already be set
        if (setto == -1) but->setToggleState(true, dontSendNotification);

        if (openedEditor)
        {
            String newText = buttons[currentidx]->getButtonText();
            openedEditor->setText(newText, dontSendNotification);
            highlightEnd = newText.length();
            openedEditor->setHighlightedRegion({highlightStart, highlightEnd});
        }

        // Auto-scroll item into viewport bounds
        if (port->getViewPositionY() > but->getY())
        {
            port->setViewPosition(0, but->getY());
        }
        else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < but->getY() + but->getHeight())
        {
            port->setViewPosition(0, but->getY() - (but->getHeight() * 4));
        }
    }

    TextEditor* openedEditor = nullptr;
    Box* currentBox;

    void resized() override
    {
        int yScroll = port->getViewPositionY();
        port->setBounds(getLocalBounds());
        buttonholder->setBounds(0, 0, getWidth(), std::min(numOptions, 20) * 22 + 2);

        for (int i = 0; i < buttons.size(); i++) buttons[i]->setBounds(2, (i * 22) + 2, getWidth() - 2, 23);

        const int resizerSize = 12;

        resizer.setBounds(getWidth() - (resizerSize + 1), getHeight() - (resizerSize + 1), resizerSize, resizerSize);

        port->setViewPosition(0, yScroll);
        repaint();
    }

   private:
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRect(port->getBounds());
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(bordercolor);
        g.drawRoundedRectangle(port->getBounds().reduced(1).toFloat(), 3.0f, 2.5f);
    }

    bool keyPressed(const KeyPress& key, Component* originatingComponent) override
    {
        if (key == KeyPress::upKey || key == KeyPress::downKey)
        {
            move(key == KeyPress::downKey ? 1 : -1);
            return true;
        }
        return false;
    }

    String filterNewText(TextEditor& e, const String& newInput) override
    {
        String mutableInput = newInput;

        // Find start of highlighted region
        // This is the start of the last auto-completion suggestion
        // This region will automatically be removed after this function because it's selected
        int start = e.getHighlightedRegion().getLength() > 0 ? e.getHighlightedRegion().getStart() : e.getText().length();

        // Reconstruct users typing
        String typedText = e.getText().substring(0, start) + mutableInput;
        highlightStart = typedText.length();

        auto& library = currentBox->cnv->pd->objectLibrary;
        // Update suggestions
        auto found = library.autocomplete(typedText.toStdString());

        for (int i = 0; i < std::min<int>(buttons.size(), found.size()); i++)
        {
            auto& [name, autocomplete] = found[i];

            if (library.objectDescriptions.find(name) != library.objectDescriptions.end())
            {
                buttons[i]->setText(name, library.objectDescriptions[name]);
            }
            else
            {
                buttons[i]->setText(name, "");
            }
        }

        for (int i = found.size(); i < buttons.size(); i++) buttons[i]->setText("", "");

        numOptions = found.size();

        setVisible(typedText.isNotEmpty() && numOptions);

        constrainer.setSizeLimits(150, 100, 500, 400);
        resized();

        // Get length of user-typed text
        int textlen = e.getText().substring(0, start).length();

        // Retrieve best suggestion
        if (currentidx >= found.size() || textlen == 0)
        {
            highlightEnd = 0;
            return mutableInput;
        }

        String fullName = found[currentidx].first;

        highlightEnd = fullName.length();

        if (!mutableInput.containsNonWhitespaceChars() || (e.getText() + mutableInput).contains(" ") || !found[currentidx].second)

        {
            isCompleting = false;
            return mutableInput;
        }

        isCompleting = true;
        mutableInput = fullName.substring(textlen);

        return mutableInput;
    }

    bool running = false;
    int numOptions = 0;
    int currentidx = 0;

    std::unique_ptr<Viewport> port;
    std::unique_ptr<Component> buttonholder;
    OwnedArray<Suggestion> buttons;

    ResizableCornerComponent resizer;
    ComponentBoundsConstrainer constrainer;

    Array<Colour> colours = {findColour(PlugDataColour::toolbarColourId), findColour(PlugDataColour::canvasColourId)};

    Colour bordercolor = Colour(142, 152, 155);

    int highlightStart = 0;
    int highlightEnd = 0;

    bool isCompleting = false;
};
