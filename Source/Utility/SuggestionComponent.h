/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Suggestions component that shows up when objects are edited
class SuggestionComponent : public Component
    , public KeyListener
    , public TextEditor::InputFilter {

    class Suggestion : public TextButton {
        int idx = 0;
        int type = -1;

        String objectDescription;

    public:
        Suggestion(SuggestionComponent* parentComponent, int i)
            : idx(i)
            , parent(parentComponent)
        {
            setText("", "", false);
            setWantsKeyboardFocus(false);
            setConnectedEdges(12);
            setClickingTogglesState(true);
            setRadioGroupId(1001);
            setColour(TextButton::buttonOnColourId, findColour(ScrollBar::thumbColourId));
        }

        void setText(String const& name, String const& description, bool icon)
        {
            objectDescription = description;
            setButtonText(name);
            type = name.contains("~") ? 1 : 0;

            // Argument suggestions don't have icons!
            drawIcon = icon;

            repaint();
        }

        // TODO: why is this necessary?
        void mouseDown(MouseEvent const& e) override
        {
            onClick();
        }

        void paint(Graphics& g) override
        {
            auto colour = PlugDataColour::popupMenuBackgroundColourId;

            auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());

            auto scrollbarIndent = parent->port->canScrollVertically() ? 5 : 0;

            auto backgroundColour = findColour(getToggleState() ? PlugDataColour::popupMenuActiveBackgroundColourId : colour);

            auto buttonArea = getLocalBounds().reduced(6, 2).withTrimmedRight(scrollbarIndent).toFloat();

            g.setColour(backgroundColour);
            g.fillRoundedRectangle(buttonArea, Constants::defaultCornerRadius);

            auto font = lnf->semiBoldFont.withHeight(12.0f);
            g.setFont(font);

            g.setColour(getToggleState() ? findColour(PlugDataColour::popupMenuActiveTextColourId) : findColour(PlugDataColour::popupMenuTextColourId));

            auto yIndent = jmin(4, proportionOfHeight(0.3f));
            auto fontHeight = roundToInt(font.getHeight() * 0.6f);
            auto leftIndent = drawIcon ? 34 : 11;
            auto rightIndent = 11;
            auto textWidth = getWidth() - leftIndent - rightIndent;

            if (textWidth > 0)
                g.drawFittedText(getButtonText(), leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, Justification::left, 2);

            font = lnf->defaultFont.withHeight(12);
            g.setFont(font);

            if (objectDescription.isNotEmpty()) {
                auto textLength = font.getStringWidth(getButtonText());

                g.setColour(getToggleState() ? findColour(PlugDataColour::popupMenuActiveTextColourId) : findColour(PlugDataColour::popupMenuTextColourId));

                leftIndent += textLength;
                auto textWidth = getWidth() - leftIndent - rightIndent;

                // Draw seperator (which is an en dash)
                g.drawText(String::fromUTF8("  \xe2\x80\x93  ") + objectDescription, Rectangle<int>(leftIndent, yIndent, textWidth, getHeight() - yIndent * 2), Justification::left);
            }

            if (type == -1)
                return;

            if (drawIcon) {
                auto dataColour = findColour(PlugDataColour::dataColourId);
                auto signalColour = findColour(PlugDataColour::signalColourId);
                g.setColour(type ? signalColour : dataColour);
                auto iconbound = getLocalBounds().reduced(4);
                iconbound.setWidth(getHeight() - 8);
                iconbound.translate(6, 0);
                g.fillRoundedRectangle(iconbound.toFloat(), Constants::smallCornerRadius);

                g.setColour(Colours::white);
                g.setFont(font.withHeight(type ? 12 : 10));
                g.drawFittedText(type ? "~" : "pd", iconbound.reduced(1), Justification::centred, 1);
            }
        }

        SuggestionComponent* parent;
        bool drawIcon = true;
    };

public:
    bool selecting = false;

    SuggestionComponent()
        : resizer(this, &constrainer)
        , currentBox(nullptr)
        , dropShadower(DropShadow(Colour(0, 0, 0).withAlpha(0.25f), 8, { 0, 3 }))
    {
        // Set up the button list that contains our suggestions
        buttonholder = std::make_unique<Component>();
        
        dropShadower.setOwner(this);

        for (int i = 0; i < 20; i++) {
            Suggestion* but = buttons.add(new Suggestion(this, i));
            buttonholder->addAndMakeVisible(buttons[i]);

            but->setClickingTogglesState(true);
            but->setRadioGroupId(110);
            but->setColour(TextButton::buttonColourId, findColour(PlugDataColour::dialogBackgroundColourId));
        }

        // Hack for maintaining keyboard focus when resizing
        resizer.addMouseListener(this, true);

        // Set up viewport
        port = std::make_unique<Viewport>();
        port->setScrollBarsShown(true, false);
        port->setViewedComponent(buttonholder.get(), false);
        port->setInterceptsMouseClicks(true, true);
        port->setViewportIgnoreDragFlag(true);
        addAndMakeVisible(port.get());

        constrainer.setSizeLimits(150, 120, 500, 400);
        setSize(300, 140);

        addAndMakeVisible(resizer);

        setInterceptsMouseClicks(true, true);
        setAlwaysOnTop(true);
        setWantsKeyboardFocus(false);
    }

    ~SuggestionComponent() override
    {
        buttons.clear();
    }

    void createCalloutBox(Object* object, TextEditor* editor)
    {
        currentBox = object;
        openedEditor = editor;

        setTransform(object->cnv->editor->getTransform());

        editor->setInputFilter(this, false);
        editor->addKeyListener(this);

        // Should run after the input filter
        editor->onTextChange = [this, editor, object]() {
            if (state == ShowingObjects && !editor->getText().containsChar(' ')) {
                editor->setHighlightedRegion({ highlightStart, highlightEnd });
            }
        };

        for (int i = 0; i < buttons.size(); i++) {
            auto* but = buttons[i];
            but->setAlwaysOnTop(true);

            but->onClick = [this, i, editor]() mutable {
                move(0, i);
                if (!editor->isVisible())
                    editor->setVisible(true);
                editor->grabKeyboardFocus();
            };
        }

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses);

        auto scale = std::sqrt(std::abs(getTransform().getDeterminant()));
        auto objectPos = (object->getScreenPosition() / scale).translated(5, 35);

        setTopLeftPosition(objectPos);

        setVisible(true);
        toFront(false);

        repaint();
    }

    void removeCalloutBox()
    {
        setVisible(false);
        if (isOnDesktop()) {
            removeFromDesktop();
        }

        if (openedEditor) {
            openedEditor->setInputFilter(nullptr, false);
        }

        openedEditor = nullptr;
        currentBox = nullptr;
    }

    void move(int offset, int setto = -1)
    {
        if (!openedEditor)
            return;

        // Calculate new selected index
        if (setto == -1)
            currentidx += offset;
        else
            currentidx = setto;

        if (numOptions == 0)
            return;

        // Limit it to minimum of the number of buttons and the number of suggestions
        int numButtons = std::min(20, numOptions);
        currentidx = (currentidx + numButtons) % numButtons;

        auto* but = buttons[currentidx];

        but->setToggleState(true, dontSendNotification);

        if (openedEditor) {
            String newText = buttons[currentidx]->getButtonText();
            openedEditor->setText(newText, dontSendNotification);
            highlightEnd = newText.length();
            openedEditor->setHighlightedRegion({ highlightStart, highlightEnd });
        }

        // Auto-scroll item into viewport bounds
        if (port->getViewPositionY() > but->getY()) {
            port->setViewPosition(0, but->getY() - 6);
        } else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < but->getY() + but->getHeight()) {
            port->setViewPosition(0, but->getY() - (but->getHeight() * 4) + 6);
        }

        repaint();
    }

    TextEditor* openedEditor = nullptr;
    SafePointer<Object> currentBox;

    void resized() override
    {
        int yScroll = port->getViewPositionY();
        port->setBounds(getLocalBounds());
        buttonholder->setBounds(6, 0, getWidth(), std::min(numOptions, 20) * 25 + 8);

        for (int i = 0; i < buttons.size(); i++)
            buttons[i]->setBounds(2, (i * 25) + 4, getWidth() - 4, 24);

        int const resizerSize = 12;

        resizer.setBounds(getWidth() - (resizerSize + 1), getHeight() - (resizerSize + 1), resizerSize, resizerSize);

        port->setViewPosition(0, yScroll);
        repaint();
    }

private:
    void mouseDown(MouseEvent const& e) override
    {
        if (openedEditor)
            openedEditor->grabKeyboardFocus();
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        g.fillRoundedRectangle(port->getBounds().reduced(1).toFloat(), Constants::defaultCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(bordercolor);
        g.drawRoundedRectangle(port->getBounds().reduced(1).toFloat(), Constants::defaultCornerRadius, 1.0f);
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        if (!currentBox) {
            return false;
        }

        if (key == KeyPress::rightKey && !openedEditor->getHighlightedRegion().isEmpty()) {
            openedEditor->setCaretPosition(openedEditor->getHighlightedRegion().getEnd());
            return true;
        }
        if (key == KeyPress::leftKey && !openedEditor->getHighlightedRegion().isEmpty()) {
            openedEditor->setCaretPosition(openedEditor->getHighlightedRegion().getStart());
            return true;
        }
        if (key == KeyPress::tabKey && !openedEditor->getHighlightedRegion().isEmpty()) {
            openedEditor->setCaretPosition(openedEditor->getHighlightedRegion().getEnd());
            // openedEditor->insertTextAtCaret(" "); // Will show argument suggestions
            return true;
        }

        if (state != ShowingObjects)
            return false;

        if (key == KeyPress::upKey || key == KeyPress::downKey) {
            move(key == KeyPress::downKey ? 1 : -1);
            return true;
        }
        return false;
    }

    String filterNewText(TextEditor& e, String const& newInput) override
    {
        if (!currentBox) {
            return newInput;
        }

        String mutableInput = newInput;

        // Find start of highlighted region
        // This is the start of the last auto-completion suggestion
        // This region will automatically be removed after this function because it's selected
        int start = e.getHighlightedRegion().getLength() > 0 ? e.getHighlightedRegion().getStart() : e.getText().length();

        // Reconstruct users typing
        String typedText = e.getText().substring(0, start) + mutableInput;
        highlightStart = typedText.length();

        constrainer.setSizeLimits(150, 100, 500, 400);
        resized();

        auto& library = currentBox->cnv->pd->objectLibrary;

        // If there's a space, open arguments panel
        if ((e.getText() + mutableInput).contains(" ")) {
            state = ShowingArguments;
            auto found = library.getArguments()[typedText.upToFirstOccurrenceOf(" ", false, false)];
            for (int i = 0; i < std::min<int>(buttons.size(), static_cast<int>(found.size())); i++) {
                auto& [type, description, init] = found[i];
                buttons[i]->setText(type, description, false);
                buttons[i]->setInterceptsMouseClicks(false, false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            numOptions = static_cast<int>(found.size());

            for (int i = numOptions; i < buttons.size(); i++) {
                buttons[i]->setText("", "", false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            setVisible(numOptions);
            currentidx = 0;

            resized();

            return mutableInput;
        }

        buttons[currentidx]->setToggleState(true, dontSendNotification);

        // Update suggestions
        auto found = library.autocomplete(typedText.toStdString());

        // When hvcc mode is enabled, show only hvcc compatible objects
        if (static_cast<bool>(currentBox->cnv->editor->hvccMode.getValue())) {
            std::vector<std::pair<String, bool>> hvccObjectsFound;
            for (auto& object : found) {
                if (Object::hvccObjects.contains(object.first)) {
                    hvccObjectsFound.push_back(object);
                }
            }

            found = hvccObjectsFound;
        }

        numOptions = static_cast<int>(found.size());

        for (int i = 0; i < std::min<int>(buttons.size(), numOptions); i++) {
            auto& [name, autocomplete] = found[i];

            auto descriptions = library.getObjectDescriptions();

            if (descriptions.find(name) != descriptions.end()) {
                buttons[i]->setText(name, descriptions[name], true);
            } else {
                buttons[i]->setText(name, "", true);
            }
            buttons[i]->setInterceptsMouseClicks(true, false);
        }

        for (int i = numOptions; i < buttons.size(); i++)
            buttons[i]->setText("", "", false);

        resized();

        // Get length of user-typed text
        int textlen = e.getText().substring(0, start).length();

        if (found.empty() || textlen == 0) {
            state = Hidden;
            setVisible(false);
            highlightEnd = 0;
            return mutableInput;
        }

        if (newInput.isEmpty() || e.getCaretPosition() != textlen) {
            highlightEnd = 0;
            return mutableInput;
        }

        // Limit it to minimum of the number of buttons and the number of suggestions
        int numButtons = std::min(20, numOptions);

        currentidx = (currentidx + numButtons) % numButtons;

        // Retrieve best suggestion
        auto const& fullName = found[currentidx].first;

        state = ShowingObjects;
        if (fullName.length() > textlen) {
            mutableInput = fullName.substring(textlen);
        }

        setVisible(true);
        highlightEnd = fullName.length();

        return mutableInput;
    }

    enum SugesstionState {
        Hidden,
        ShowingObjects,
        ShowingArguments
    };

    int numOptions = 0;
    int currentidx = 0;

    std::unique_ptr<Viewport> port;
    std::unique_ptr<Component> buttonholder;
    OwnedArray<Suggestion> buttons;

    ResizableCornerComponent resizer;
    ComponentBoundsConstrainer constrainer;
        
    StackDropShadower dropShadower;

    Colour bordercolor = Colour(142, 152, 155);

    int highlightStart = 0;
    int highlightEnd = 0;

    SugesstionState state = Hidden;
};
