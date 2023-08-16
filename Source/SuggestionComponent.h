/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "PluginEditor.h"
#include "PluginProcessor.h" // TODO: We shouldn't need this!

// Component that sits on top of a TextEditor and will draw auto-complete suggestions over it
class AutoCompleteComponent
    : public Component
    , public ComponentListener {
    String suggestion;
    Canvas* cnv;
    Component::SafePointer<TextEditor> editor;

public:
    AutoCompleteComponent(TextEditor* e, Canvas* c)
        : editor(e)
        , cnv(c)
    {
        setAlwaysOnTop(true);

        editor->addComponentListener(this);
        cnv->addAndMakeVisible(this);

        setInterceptsMouseClicks(false, false);
    }

    ~AutoCompleteComponent() override
    {
        if(editor) editor->removeComponentListener(this);
    }

    String getSuggestion()
    {

        if (!editor)
            return {};

        return editor->getText() + suggestion;
    }

    void autocomplete()
    {
        if (!editor)
            return;

        editor->setText(editor->getText() + suggestion, sendNotification);
        suggestion = "";
        repaint();
    }

    void enableAutocomplete(bool enabled)
    {
        shouldAutocomplete = enabled;
    }

    void setSuggestion(String const& suggestionText)
    {
        if (!editor)
            return;

        auto editorText = editor->getText();

        if (editorText.startsWith(suggestionText)) {
            suggestion = "";
            repaint();
            return;
        }

        if (editorText.isEmpty()) {
            editor->setText(stashedText, dontSendNotification);
            editorText = stashedText;
        }

        if (suggestionText.startsWith(editorText)) {
            auto textUpToSpace = editorText.upToFirstOccurrenceOf(" ", false, false);
            suggestion = suggestionText.fromFirstOccurrenceOf(textUpToSpace, false, true);
            setVisible(suggestionText.isNotEmpty() && textUpToSpace != suggestionText);
            repaint();
        } else if (editorText.isNotEmpty()) {
            stashedText = editorText;
            editor->setText("", dontSendNotification);
            suggestion = suggestionText;
            repaint();
        }
    }

private:
    bool shouldAutocomplete = true;
    String stashedText;

    void componentMovedOrResized(Component& component, bool moved, bool resized) override
    {
        if (!editor)
            return;
        setBounds(cnv->getLocalArea(editor, editor->getLocalBounds()));
    }

    void componentBeingDeleted(Component& component) override
    {
        if (!editor)
            return;
        editor->removeComponentListener(this);
    }

    void paint(Graphics& g) override
    {
        if (!editor || !shouldAutocomplete)
            return;

        auto editorText = editor->getText();
        auto editorTextWidth = editor->getFont().getStringWidthFloat(editorText);
        auto completionBounds = getLocalBounds().toFloat().withTrimmedLeft(editorTextWidth + 7.5f);

        auto colour = findColour(PlugDataColour::canvasTextColourId).withAlpha(0.65f);
        Fonts::drawText(g, suggestion, completionBounds, colour);
    }
};
// Suggestions component that shows up when objects are edited
class SuggestionComponent : public Component
    , public KeyListener
    , public TextEditor::Listener
    , public ComponentListener
{

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
            setRadioGroupId(hash("suggestion_component"));
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
            auto scrollbarIndent = parent->port->canScrollVertically() ? 6 : 0;

            auto backgroundColour = findColour(getToggleState() ? PlugDataColour::popupMenuActiveBackgroundColourId : PlugDataColour::popupMenuBackgroundColourId);

            auto buttonArea = getLocalBounds().withTrimmedRight((parent->canBeTransparent() ? 42 : 2) + scrollbarIndent).toFloat().reduced(4, 1);

            g.setColour(backgroundColour);
            PlugDataLook::fillSmoothedRectangle(g, buttonArea,  Corners::defaultCornerRadius);

            auto colour = getToggleState() ? findColour(PlugDataColour::popupMenuActiveTextColourId) : findColour(PlugDataColour::popupMenuTextColourId);

            auto yIndent = jmin(4, proportionOfHeight(0.3f));
            auto leftIndent = drawIcon ? 32 : 11;
            auto rightIndent = 14;
            auto textWidth = getWidth() - leftIndent - rightIndent;

            if (textWidth > 0)
                Fonts::drawStyledText(g, getButtonText(), leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, colour, Semibold, 13);

            if (objectDescription.isNotEmpty()) {
                auto textLength = Fonts::getSemiBoldFont().withHeight(13).getStringWidth(getButtonText());

                leftIndent += textLength;
                auto textWidth = getWidth() - leftIndent - rightIndent;

                // Draw seperator (which is an en dash)
                Fonts::drawText(g, String::fromUTF8("  \xe2\x80\x93  ") + objectDescription, Rectangle<int>(leftIndent, yIndent, textWidth, getHeight() - yIndent * 2), colour, 13);
            }

            if (type == -1)
                return;

            if (drawIcon) {
                auto dataColour = findColour(PlugDataColour::dataColourId);
                auto signalColour = findColour(PlugDataColour::signalColourId);
                g.setColour(type ? signalColour : dataColour);
                auto iconbound = getLocalBounds().reduced(4);
                iconbound.setWidth(getHeight() - 8);
                iconbound.translate(4, 0);
                PlugDataLook::fillSmoothedRectangle(g, iconbound.toFloat(), Corners::defaultCornerRadius);

                Fonts::drawFittedText(g, type ? "~" : "pd", iconbound.reduced(1), Colours::white, 1, 1.0f, type ? 12 : 10, Justification::centred);
            }
        }

        SuggestionComponent* parent;
        bool drawIcon = true;
    };

public:
    SuggestionComponent()
        : resizer(this, &constrainer)
        , currentObject(nullptr)
        , windowMargin(canBeTransparent() ? 22 : 0)
    {
        // Set up the button list that contains our suggestions
        buttonholder = std::make_unique<Component>();

        for (int i = 0; i < 20; i++) {
            Suggestion* but = buttons.add(new Suggestion(this, i));
            buttonholder->addAndMakeVisible(buttons[i]);

            but->setClickingTogglesState(true);
            but->setColour(TextButton::buttonColourId, findColour(PlugDataColour::dialogBackgroundColourId));
        }

        // Hack for maintaining keyboard focus when resizing
        resizer.addMouseListener(this, true);

        // Set up viewport
        port = std::make_unique<BouncingViewport>();
        port->setScrollBarsShown(true, false);
        port->setViewedComponent(buttonholder.get(), false);
        port->setInterceptsMouseClicks(true, true);
        port->setViewportIgnoreDragFlag(true);
        addAndMakeVisible(port.get());

        constrainer.setSizeLimits(150, 120, 500, 400);
        setSize(310 + (2 * windowMargin), 140 + (2 * windowMargin));

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
        currentObject = object;
        openedEditor = editor;

        setTransform(object->cnv->editor->getTransform());

        editor->addComponentListener(this);
        editor->addListener(this);
        editor->addKeyListener(this);

        autoCompleteComponent = std::make_unique<AutoCompleteComponent>(editor, object->cnv);

        for (int i = 0; i < buttons.size(); i++) {
            auto* but = buttons[i];
            but->setAlwaysOnTop(true);

            but->onClick = [this, i, but, editor]() mutable {
                // If the button is already selected, perform autocomplete
                if (but->getToggleState() && autoCompleteComponent) {
                    autoCompleteComponent->autocomplete();
                    return;
                }

                move(0, i);
                if (!editor->isVisible())
                    editor->setVisible(true);
                editor->grabKeyboardFocus();
            };
        }

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses);

        updateBounds();

        setVisible(false);
        toFront(false);

        repaint();
    }

    void updateBounds()
    {
        if (!currentObject)
            return;

        auto* cnv = currentObject->cnv;

        setTransform(cnv->editor->getTransform());

        auto scale = std::sqrt(std::abs(getTransform().getDeterminant()));

        auto objectPos = currentObject->getScreenBounds().reduced(Object::margin).getBottomLeft() / scale;

        setTopLeftPosition(objectPos.translated(-windowMargin, -windowMargin + 5));

        // If box is not contained in canvas bounds, hide suggestions
        if (cnv->viewport) {
            setVisible(cnv->viewport->getViewArea().contains(cnv->viewport->getLocalArea(currentObject, currentObject->getBounds())));
        }
    }

    void removeCalloutBox()
    {
        setVisible(false);

        if (isOnDesktop()) {
            removeFromDesktop();
        }

        autoCompleteComponent.reset(nullptr);
        if (openedEditor) {
            openedEditor->removeListener(this);
            openedEditor->removeComponentListener(this);
            openedEditor->removeKeyListener(this);
        }

        openedEditor = nullptr;
        currentObject = nullptr;
    }
    
    void componentBeingDeleted (Component &component) override
    {
        removeCalloutBox();
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

        if (autoCompleteComponent) {
            String newText = buttons[currentidx]->getButtonText();
            autoCompleteComponent->setSuggestion(newText);
            autoCompleteComponent->enableAutocomplete(true);
            currentObject->updateBounds();
        }

        // Auto-scroll item into viewport bounds
        if (port->getViewPositionY() > but->getY()) {
            port->setViewPosition(0, but->getY() - 6);
        } else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < but->getY() + but->getHeight()) {
            port->setViewPosition(0, but->getY() - (but->getHeight() * 4) + 6);
        }

        repaint();
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(windowMargin);

        int yScroll = port->getViewPositionY();
        port->setBounds(b);
        buttonholder->setBounds(b.getX() + 6, b.getY(), b.getWidth(), std::min(numOptions, 20) * 25 + 8);

        for (int i = 0; i < buttons.size(); i++)
            buttons[i]->setBounds(3, (i * 25) + 7, getWidth() - 6, 24);

        int const resizerSize = 12;

        resizer.setBounds(b.getRight() - (resizerSize + 1), b.getBottom() - (resizerSize + 1), resizerSize, resizerSize);

        port->setViewPosition(0, yScroll);
        repaint();
    }

    String getText() const
    {
        if (autoCompleteComponent) {
            return autoCompleteComponent->getSuggestion();
        }

        return {};
    }

private:
    void mouseDown(MouseEvent const& e) override
    {
        if (openedEditor)
            openedEditor->grabKeyboardFocus();
    }

    bool hitTest(int x, int y) override
    {
        return getLocalBounds().reduced(windowMargin).contains(x, y);
    }

    static bool canBeTransparent()
    {
        return ProjectInfo::canUseSemiTransparentWindows();
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(windowMargin);

        if (!canBeTransparent()) {
            g.fillAll(findColour(PlugDataColour::canvasBackgroundColourId));
        } else {
            Path localPath;
            localPath.addRoundedRectangle(b.toFloat().reduced(6.0f), Corners::defaultCornerRadius);
            StackShadow::renderDropShadow(g, localPath, Colour(0, 0, 0).withAlpha(0.6f), 12, { 0, 2 });
        }

        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        PlugDataLook::fillSmoothedRectangle(g, port->getBounds().reduced(1).toFloat(), Corners::defaultCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId).darker(0.1f));
        PlugDataLook::drawSmoothedRectangle(g, PathStrokeType(1.0f), port->getBounds().reduced(1).toFloat(), Corners::defaultCornerRadius);
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        if (!currentObject) {
            return false;
        }

        if (key == KeyPress::rightKey && !openedEditor->getHighlightedRegion().isEmpty()) {
            openedEditor->setCaretPosition(openedEditor->getHighlightedRegion().getEnd());
            return true;
        }
        if (key == KeyPress::rightKey && autoCompleteComponent && openedEditor->getCaretPosition() == openedEditor->getText().length()) {
            autoCompleteComponent->autocomplete();
            return true;
        }
        if (key == KeyPress::leftKey && !openedEditor->getHighlightedRegion().isEmpty()) {
            openedEditor->setCaretPosition(openedEditor->getHighlightedRegion().getStart());
            return true;
        }
        if (key == KeyPress::returnKey && autoCompleteComponent->getSuggestion() == openedEditor->getText()) {
            // if the caret is already at the end, we want to close upon enter key
            // By ignoring the keypress we'll trigger the return callback on text editor which will close it
            return false;
        }
        if ((key == KeyPress::returnKey || key == KeyPress::tabKey) && autoCompleteComponent) {
            autoCompleteComponent->autocomplete();
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

    void textEditorTextChanged(TextEditor& e) override
    {
        if (!currentObject)
            return;

        String currentText = e.getText();
        resized();

        auto& library = currentObject->cnv->pd->objectLibrary;
        
        
        class ObjectSorter
        {
        public:
            ObjectSorter(String searchQuery) : query(searchQuery)
            {
            }
            
            int compareElements (const String& a, const String& b)
            {
                auto check = [this](const String& a, const String& b) -> bool {
                    // Check if suggestion exacly matches query
                    if (a == query) {
                        return -1;
                    }
                    
                    // Check if suggestion is equal to query with "~" appended
                    if (a == (query + "~") && b != query && b != (query + "~")) {
                        return -1;
                    }
                    
                    // Check if suggestion is equal to query with "." appended
                    if (a.startsWith(query + ".") && b != query && b != (query + "~") && !b.startsWith(query + ".")) {
                        return 1;
                    }
                    
                    if (a.length() < b.length()) {
                        return 1;
                    }
                    
                    return -1;
                };
                
                if (check(a, b))
                    return true;
                if (check(b, a))
                    return false;
                
                return a.compareNatural(b) >= 0;
            }
            const String query;
        };

        auto sortSuggestions = [](String query, StringArray suggestions) -> StringArray {
            if (query.length() == 0)
                return suggestions;
            
            auto sorter = ObjectSorter(query);
            suggestions.strings.sort(sorter);
            return suggestions;
        };

        // If there's a space, open arguments panel
        if (currentText.contains(" ")) {
            state = ShowingArguments;
            auto name = currentText.upToFirstOccurrenceOf(" ", false, false);
            auto objectInfo = library->getObjectInfo(name);
            auto found = objectInfo.getChildWithName("arguments").createCopy();
            for (auto flag : objectInfo.getChildWithName("flags")) {
                auto flagCopy = flag.createCopy();
                auto name = flagCopy.getProperty("name").toString().trim();
                if (!name.startsWith("-"))
                    name = "-" + name;
                flagCopy.setProperty("type", name, nullptr);
                found.appendChild(flagCopy, nullptr);
            }

            numOptions = std::min<int>(buttons.size(), found.getNumChildren());
            for (int i = 0; i < numOptions; i++) {
                auto type = found.getChild(i).getProperty("type").toString();
                auto description = found.getChild(i).getProperty("description").toString();

                buttons[i]->setText(type, description, false);
                buttons[i]->setInterceptsMouseClicks(false, false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            for (int i = numOptions; i < buttons.size(); i++) {
                buttons[i]->setText("", "", false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            setVisible(numOptions);

            if (autoCompleteComponent) {
                autoCompleteComponent->enableAutocomplete(false);
                currentObject->updateBounds();
            }

            resized();

            return;
        }

        if (isPositiveAndBelow(currentidx, buttons.size())) {
            buttons[currentidx]->setToggleState(true, dontSendNotification);
        }

        auto filterNonHvccObjectsIfNeeded = [_this = SafePointer(this)](StringArray& toFilter) {
            if (!_this || !_this->currentObject)
                return;

            if (getValue<bool>(_this->currentObject->cnv->editor->hvccMode)) {

                StringArray hvccObjectsFound;
                for (auto& object : toFilter) {
                    if (Object::hvccObjects.contains(object)) {
                        hvccObjectsFound.add(object);
                    }
                }

                toFilter = hvccObjectsFound;
            }
        };
        auto patchDir = currentObject->cnv->patch.getPatchFile().getParentDirectory();
        if(!patchDir.isDirectory() || patchDir == File::getSpecialLocation(File::tempDirectory)) patchDir = File();

        // Update suggestions
        auto found = library->autocomplete(currentText, patchDir);

        // When hvcc mode is enabled, show only hvcc compatible objects
        filterNonHvccObjectsIfNeeded(found);

        if (found.isEmpty()) {
            autoCompleteComponent->enableAutocomplete(false);
            deselectAll();
            currentidx = -1;
        } else {
            found = sortSuggestions(currentText, found);
            currentidx = 0;
            autoCompleteComponent->enableAutocomplete(true);
        }

        auto applySuggestionsToButtons = [this, &library](StringArray& suggestions, String originalQuery) {
            numOptions = static_cast<int>(suggestions.size());

            // Apply object name and descriptions to buttons
            for (int i = 0; i < std::min<int>(buttons.size(), numOptions); i++) {
                auto& name = suggestions[i];

                auto description = library->getObjectInfo(name).getProperty("description").toString();
                buttons[i]->setText(name, description, true);

                buttons[i]->setInterceptsMouseClicks(true, false);
            }

            for (int i = numOptions; i < buttons.size(); i++)
                buttons[i]->setText("", "", false);

            resized();

            // Get length of user-typed text
            int textlen = openedEditor->getText().length();

            if (suggestions.isEmpty() || textlen == 0) {
                state = Hidden;
                if (autoCompleteComponent)
                    autoCompleteComponent->enableAutocomplete(false);
                currentObject->updateBounds();
                setVisible(false);
                return;
            }

            // Limit it to minimum of the number of buttons and the number of suggestions
            int numButtons = std::min(20, numOptions);

            // duplicate call to updateBounds :( do we need this?t
            currentObject->updateBounds();

            setVisible(true);

            state = ShowingObjects;

            if (currentidx < 0)
                return;

            currentidx = (currentidx + numButtons) % numButtons;

            // Retrieve best suggestion
            auto const& fullName = suggestions[currentidx];

            if (fullName.length() > textlen && autoCompleteComponent) {
                autoCompleteComponent->setSuggestion(fullName);
            } else if (autoCompleteComponent) {
                autoCompleteComponent->setSuggestion("");
            }
        };

        if (openedEditor) {
            applySuggestionsToButtons(found, currentText);
        }

        library->getExtraSuggestions(found.size(), currentText, [this, filterNonHvccObjectsIfNeeded, applySuggestionsToButtons, found, currentText](StringArray s) mutable {
            filterNonHvccObjectsIfNeeded(s);

            // This means the extra suggestions have returned too late to still be relevant
            if (!openedEditor || currentText != openedEditor->getText())
                return;

            found.addArray(s);
            found.removeDuplicates(false);

            applySuggestionsToButtons(found, currentText);
        });
    }

    void deselectAll()
    {
        for (auto* button : buttons) {
            button->setToggleState(false, dontSendNotification);
        }
    }

    enum SugesstionState {
        Hidden,
        ShowingObjects,
        ShowingArguments
    };

    int numOptions = 0;
    int currentidx = 0;

    std::unique_ptr<AutoCompleteComponent> autoCompleteComponent;
    std::unique_ptr<BouncingViewport> port;
    std::unique_ptr<Component> buttonholder;
    OwnedArray<Suggestion> buttons;

    ResizableCornerComponent resizer;
    ComponentBoundsConstrainer constrainer;

    SugesstionState state = Hidden;

    SafePointer<TextEditor> openedEditor = nullptr;
    SafePointer<Object> currentObject = nullptr;

    int windowMargin;
};
