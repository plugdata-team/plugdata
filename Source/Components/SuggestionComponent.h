/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
extern "C" {
#include <g_all_guis.h>
}

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginEditor.h"
#include "Objects/ObjectBase.h"
#include "Heavy/CompatibleObjects.h"
#include "Utility/NVGGraphicsContext.h"
#include "Components/BouncingViewport.h"
#include "CanvasViewport.h"

extern "C" {
int is_gem_object(char const* sym);
}

// Component that sits on top of a TextEditor and will draw auto-complete suggestions over it
class AutoCompleteComponent final
    : public Component
    , public NVGComponent
    , public ComponentListener {
    String suggestion;
    Canvas* cnv;
    Component::SafePointer<TextEditor> editor;

public:
    AutoCompleteComponent(TextEditor* e, Canvas* c)
        : NVGComponent(this)
        , cnv(c)
        , editor(e)
    {
        setAlwaysOnTop(true);

        editor->addComponentListener(this);
        cnv->addAndMakeVisible(this);

        setInterceptsMouseClicks(false, false);
    }

    ~AutoCompleteComponent() override
    {
        if (editor)
            editor->removeComponentListener(this);
    }

    String getSuggestion() const
    {

        if (!editor)
            return { };

        return editor->getText() + suggestion;
    }

    void autocomplete()
    {
        if (!editor)
            return;

        editor->setText(editor->getText() + suggestion, sendNotification);
        editor->moveCaretToEnd();
        suggestion = "";
        repaint();
    }

    void enableAutocomplete(bool const enabled)
    {
        shouldAutocomplete = enabled;
    }

    bool isAutocompleting() const
    {
        return shouldAutocomplete && suggestion.isNotEmpty();
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

        suggestion = suggestion.replace("  ", " ");

        if (suggestionText.startsWith(editorText)) {
            auto const textUpToSpace = editorText.upToFirstOccurrenceOf(" ", false, false);
            suggestion = suggestionText.fromFirstOccurrenceOf(textUpToSpace, false, true);
            setVisible(suggestionText.isNotEmpty() && textUpToSpace != suggestionText);
        } else if (editorText.isNotEmpty()) {
            stashedText = editorText;
            editor->setText("", dontSendNotification);
            suggestion = suggestionText;
        }
        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        NVGScopedState scopedState(nvg);
        nvgTranslate(nvg, getX(), getY());
        Graphics g(*cnv->editor->getNanoLLGC());
        paintEntireComponent(g, true);
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

        auto const editorText = editor->getText();
        auto const editorTextWidth = Fonts::getStringWidth(editorText, editor->getFont());
        auto const completionBounds = getLocalBounds().toFloat().withTrimmedLeft(editorTextWidth + 7.5f);

        auto const colour = PlugDataColours::canvasTextColour.withAlpha(0.5f);
        Fonts::drawText(g, suggestion, completionBounds.translated(-1.25f, 0), colour);
    }
};

// Suggestions component that shows up when objects are edited
class SuggestionComponent final : public Component
    , public KeyListener
    , public ComponentListener {

    class Suggestion final : public TextButton {

        enum ObjectType {
            Data = 0,
            Signal = 1,
            Gem = 2
        };

        ObjectType type;

        String objectDescription;

    public:
        explicit Suggestion(SuggestionComponent* parentComponent)
            : parent(parentComponent)
        {
            setText("", "", false);
            setWantsKeyboardFocus(false);
            setConnectedEdges(12);
            setClickingTogglesState(true);
            setTriggeredOnMouseDown(true);
            setRadioGroupId(hash("suggestion_component"));
            setColour(TextButton::buttonOnColourId, findColour(ScrollBar::thumbColourId));
        }

        void setText(String const& name, String const& description, bool const icon)
        {
            objectDescription = description;
            setButtonText(name);
            type = name.contains("~") ? Signal : Data;

            if (!type && is_gem_object(name.toRawUTF8())) {
                type = Gem;
            }

            // Argument suggestions don't have icons!
            drawIcon = icon;

            repaint();
        }

        void paint(Graphics& g) override
        {
            auto const scrollbarIndent = parent->port->canScrollVertically() ? 6 : 0;

            auto const backgroundColour = getToggleState() ? PlugDataColours::popupMenuActiveBackgroundColour : PlugDataColours::popupMenuBackgroundColour;

            auto const buttonArea = getLocalBounds().withTrimmedRight(2 + scrollbarIndent).toFloat().reduced(4, 1);

            g.setColour(backgroundColour);
            g.fillRoundedRectangle(buttonArea, Corners::defaultCornerRadius);

            auto const colour = PlugDataColours::popupMenuTextColour;
            auto const yIndent = jmin(4, proportionOfHeight(0.3f));
            auto leftIndent = drawIcon ? 32 : 11;
            constexpr auto rightIndent = 14;
            auto const textWidth = getWidth() - leftIndent - rightIndent;

            auto buttonText = getButtonText();
            if(buttonText.startsWith("s ") || buttonText.startsWith("send ") || buttonText.startsWith("r ") || buttonText.startsWith("receive "))
                buttonText = buttonText.fromFirstOccurrenceOf(" ", false, false);

            if (textWidth > 0)
                Fonts::drawStyledText(g, buttonText, leftIndent, yIndent, textWidth, getHeight() - yIndent * 2, colour, Semibold, 13);

            if (objectDescription.isNotEmpty()) {
                auto const textLength = Fonts::getStringWidth(buttonText, Fonts::getSemiBoldFont().withHeight(13));

                leftIndent += textLength;
                auto const textWidth = getWidth() - leftIndent - rightIndent;

                // Draw seperator (which is an en dash)
                Fonts::drawText(g, String::fromUTF8("  \xe2\x80\x93  ") + objectDescription, Rectangle<int>(leftIndent, yIndent, textWidth, getHeight() - yIndent * 2), colour, 13);
            }

            if (drawIcon) {
                Colour iconColour;
                String iconText;
                if (type == Data) {
                    iconColour = PlugDataColours::dataColour;
                    iconText = "pd";
                } else if (type == Signal) {
                    iconColour = PlugDataColours::signalColour;
                    iconText = "~";
                } else if (type == Gem) {
                    iconColour = PlugDataColours::gemColour;
                    iconText = "g";
                }
                g.setColour(iconColour);

                auto iconbound = getLocalBounds().reduced(4);
                iconbound.setWidth(getHeight() - 8);
                iconbound.translate(4, 0);
                g.fillRoundedRectangle(iconbound.toFloat(), Corners::defaultCornerRadius);

                Fonts::drawFittedText(g, iconText, iconbound.reduced(1), Colours::white, 1, 1.0f, type ? 12 : 10, Justification::centred);
            }
        }

        SuggestionComponent* parent;
        bool drawIcon = true;
    };

public:
    SuggestionComponent()
        : resizer(this, &constrainer)
    {
        resizer.setLookAndFeel(&resizerLookAndFeel);

        // Set up the button list that contains our suggestions
        buttonholder = std::make_unique<Component>();

        for (int i = 0; i < 20; i++) {
            Suggestion* but = buttons.add(new Suggestion(this));
            buttonholder->addAndMakeVisible(buttons[i]);

            but->setClickingTogglesState(true);
            but->setColour(TextButton::buttonColourId, PlugDataColours::dialogBackgroundColour);
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
        setSize(310 + getMargin() * 2, 140 + getMargin() * 2);

        resizer.setAllowHostManagedResize(false);
        addAndMakeVisible(resizer);

        setInterceptsMouseClicks(true, true);
        setAlwaysOnTop(true);
        setWantsKeyboardFocus(false);
    }

    ~SuggestionComponent() override
    {
        resizer.setLookAndFeel(nullptr);
        buttons.clear();
    }

    int getMargin()
    {
        return canBeTransparent() ? 8 : 0;
    }

    Rectangle<int> getWindowBounds()
    {
        return getLocalBounds().reduced(getMargin());
    }

    void renderAutocompletion(NVGcontext* nvg)
    {
        if (autoCompleteComponent)
            autoCompleteComponent->render(nvg);
    }

    void createCalloutBox(Object* object, TextEditor* editor)
    {
        sendReceiveDatabase = {};
        currentObject = object;
        openedEditor = editor;

        setTransform(object->editor->getTransform());

        editor->addComponentListener(this);
        editor->addKeyListener(this);

        autoCompleteComponent = std::make_unique<AutoCompleteComponent>(editor, object->cnv);

        for (int i = 0; i < buttons.size(); i++) {
            auto* but = buttons[i];
            but->setAlwaysOnTop(true);
            but->onClick = [this, i, editor]() mutable {
                // If the button is already selected, perform autocomplete
                if (i == currentidx && autoCompleteComponent) {
                    autoCompleteComponent->autocomplete();
                } else {
                    move(0, i);
                }

                if (!editor->isVisible())
                    editor->setVisible(true);
                editor->grabKeyboardFocus();
            };
        }

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses, OSUtils::getDesktopParentPeer(object->editor));

        updateBounds();

        setVisible(false);
        toFront(false);

        repaint();
    }

    void updateBounds()
    {
        if (!currentObject)
            return;

        auto const* cnv = currentObject->cnv;

        setTransform(cnv->editor->getTransform());

        auto const scale = std::sqrt(std::abs(getTransform().getDeterminant()));

        auto const objectPos = currentObject->getScreenBounds().reduced(Object::margin).getBottomLeft() / scale;

        setTopLeftPosition(objectPos.translated(-getMargin(), 5 - getMargin()));

        // If box is not contained in canvas bounds, hide suggestions
        if (cnv->viewport) {
            setVisible(cnv->viewport->getBounds().contains(cnv->viewport->getLocalArea(currentObject, currentObject->getLocalBounds())));
        }
    }

    void removeCalloutBox()
    {
        currentidx = 0;
        setVisible(false);

        if (isOnDesktop()) {
            removeFromDesktop();
        }

        autoCompleteComponent.reset(nullptr);
        if (openedEditor) {
            openedEditor->removeComponentListener(this);
            openedEditor->removeKeyListener(this);
        }

        openedEditor = nullptr;
        currentObject = nullptr;
    }

    void componentBeingDeleted(Component& component) override
    {
        removeCalloutBox();
    }

    void move(int const offset, int const setto = -1)
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
        int const numButtons = std::min(20, numOptions);
        currentidx = (currentidx + numButtons) % numButtons;

        auto* but = buttons[currentidx];

        but->setToggleState(true, dontSendNotification);
        auto const buttonText = but->getButtonText();

        if (openedEditor && autoCompleteComponent && buttonText.startsWith(openedEditor->getText())) {
            autoCompleteComponent->setSuggestion(buttonText);
            autoCompleteComponent->enableAutocomplete(true);
            currentObject->updateBounds();
            resized();
        } else {
            openedEditor->setText(buttonText, dontSendNotification);
            openedEditor->moveCaretToEnd(false);
            autoCompleteComponent->setSuggestion("");
            autoCompleteComponent->enableAutocomplete(false);
            currentObject->updateBounds();
            resized();
        }

        // Auto-scroll item into viewport bounds
        if (port->getViewPositionY() > but->getY()) {
            port->setViewPosition(0, but->getY() - 6);
        } else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < but->getY() + but->getHeight()) {
            port->setViewPosition(0, but->getY() - but->getHeight() * 4 + 6);
        }

        repaint();
    }

    void resized() override
    {
        auto b = getWindowBounds();

        int const yScroll = port->getViewPositionY();
        port->setBounds(b.reduced(2, 0));
        buttonholder->setBounds(b.getX() + 6, b.getY(), b.getWidth(), std::min(numOptions, 20) * 25 + 8);

        for (int i = 0; i < buttons.size(); i++)
            buttons[i]->setBounds(3, i * 25 + 7, getWidth() - 6, 24);

        resizer.setBounds(b.removeFromBottom(12).removeFromRight(12).translated(-2, 0));

        port->setViewPosition(0, yScroll);
        repaint();
    }

    String getText() const
    {
        if (autoCompleteComponent) {
            return autoCompleteComponent->getSuggestion();
        }

        return { };
    }
    void updateSuggestions(String const& currentText)
    {
        if (!currentObject || lastText == currentText) {
            return;
        }

        if (currentidx >= 0 && buttons[currentidx]->getButtonText() == currentText) {
            if (autoCompleteComponent)
                autoCompleteComponent->setSuggestion("");
            return;
        }

        lastText = currentText;

        auto& library = currentObject->cnv->pd->objectLibrary;

        class ObjectSorter {
        public:
            explicit ObjectSorter(String const& searchQuery)
                : query(searchQuery)
            {
            }

            int compareElements(String const& a, String const& b) const
            {
                // Check if suggestion exacly matches query
                if (a == query && b != query) {
                    return -1;
                }

                if (b == query && a != query) {
                    return 1;
                }

                // Check if suggestion is equal to query with "~" appended
                if (a == query + "~" && b != query && b != query + "~") {
                    return -1;
                }

                if (b == query + "~" && a != query && a != query + "~") {
                    return 1;
                }

                // Check if suggestion is equal to query with "." appended
                if (a.startsWith(query + ".") && b != query && b != query + "~" && !b.startsWith(query + ".")) {
                    return -1;
                }

                if (b.startsWith(query + ".") && a != query && a != query + "~" && !a.startsWith(query + ".")) {
                    return 1;
                }

                if (a.startsWith(query) && !b.startsWith(query)) {
                    return -1;
                }

                if (b.startsWith(query) && !a.startsWith(query)) {
                    return 1;
                }

                return 0;
            }
            String const query;
        };

        auto sortSuggestions = [](String const& query, StringArray suggestions) -> StringArray {
            if (query.length() == 0)
                return suggestions;

            auto sorter = ObjectSorter(query);
            suggestions.strings.sort(sorter, true);
            return suggestions;
        };

        // Autocomplete messages with nearby methods
        if (currentObject->gui && currentObject->getType(false) == "msg") {
            auto nearbyMethods = findNearbyMethods(currentText);

            numOptions = std::min<int>(buttons.size(), nearbyMethods.size());
            for (int i = 0; i < numOptions; i++) {
                auto [objectName, methodName, description] = nearbyMethods[i];

                buttons[i]->setText(methodName, "(" + objectName + ") " + description, false);
                buttons[i]->setInterceptsMouseClicks(false, false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            for (int i = numOptions; i < buttons.size(); i++) {
                buttons[i]->setText("", "", false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            setVisible(numOptions);

            // Get length of user-typed text
            int textlen = openedEditor->getText().length();

            if (nearbyMethods.empty() || textlen == 0) {
                state = Hidden;
                if (autoCompleteComponent)
                    autoCompleteComponent->enableAutocomplete(false);
                setVisible(false);
                return;
            }

            state = ShowingObjects;
            currentidx = -1;

            if (autoCompleteComponent) {
                autoCompleteComponent->setSuggestion("");
            }

            resized();
            return;
        }

        // Autocomplete send/receive with nearby send/receives
        auto objectName = currentText.upToFirstOccurrenceOf(" ", false, false);
        if (objectName == "send" || objectName == "s" ||  objectName == "receive" || objectName == "r") {
            bool isSend = objectName == "send" || objectName == "s";
            auto searchSymbol = currentText.fromFirstOccurrenceOf(" ", false, false).upToFirstOccurrenceOf(" ", false, false);
            auto allSendReceives = findSendReceive(currentObject->cnv->patch, searchSymbol, !isSend);
            numOptions = std::min<int>(buttons.size(), allSendReceives.size());
            for (int i = 0; i < numOptions; i++) {
                auto symbol = isSend ? allSendReceives[i].receiveSymbol : allSendReceives[i].sendSymbol;
                buttons[i]->setText(objectName + " " + symbol, allSendReceives[i].name, false);
                buttons[i]->setInterceptsMouseClicks(false, false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            for (int i = numOptions; i < buttons.size(); i++) {
                buttons[i]->setText("", "", false);
                buttons[i]->setToggleState(false, dontSendNotification);
            }

            setVisible(numOptions);

            // Get length of user-typed text
            int textlen = openedEditor->getText().length();

            if (allSendReceives.empty() || textlen == 0) {
                state = Hidden;
                if (autoCompleteComponent)
                    autoCompleteComponent->enableAutocomplete(false);
                setVisible(false);
                return;
            }

            state = ShowingObjects;
            currentidx = -1;

            if (autoCompleteComponent) {
                autoCompleteComponent->setSuggestion("");
            }

            resized();
            return;
        }

        // If there's a space, open arguments panel
        if (currentText.contains(" ")) {
            state = ShowingArguments;
            auto name = currentText.upToFirstOccurrenceOf(" ", false, false).fromLastOccurrenceOf("/", false, false);
            auto objectInfo = library->getObjectInfo(name);

            auto found = objectInfo.arguments;

            for (auto const& flag : objectInfo.flags) {
                auto name = flag.type;
                if (!name.startsWith("-"))
                    name = "-" + name;
                found.add({ name, flag.description });
            }

            numOptions = std::min<int>(buttons.size(), found.size());
            for (int i = 0; i < numOptions; i++) {
                auto const& type = found[i].type;
                auto const& description = found[i].description;
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
            }

            return;
        }

        if (isPositiveAndBelow(currentidx, buttons.size())) {
            buttons[currentidx]->setToggleState(true, dontSendNotification);
        }

        auto filterObjects = [_this = SafePointer(this), &library](StringArray& toFilter) {
            if (!_this || !_this->currentObject)
                return;

            if (!SettingsFile::getInstance()->getProperty<VarArray>("libraries").contains("Gem")) {
                StringArray noGemObjects;
                for (auto& object : toFilter) {
                    if (object.startsWith("Gem/") || !library->isGemObject(object)) // Don't suggest Gem objects without "Gem/" prefix unless gem library is loaded
                    {
                        noGemObjects.add(object);
                    }
                }

                toFilter = noGemObjects;
            }

            // When hvcc mode is enabled, show only hvcc compatible objects
            if (getValue<bool>(_this->currentObject->hvccMode)) {
                StringArray hvccObjectsFound;
                for (auto& object : toFilter) {
                    // We support arrays, but when you create [array] it is really [array define] which is unsupported
                    if (HeavyCompatibleObjects::isCompatible(object) && object != "array") {
                        hvccObjectsFound.add(object);
                    }
                }
                toFilter = hvccObjectsFound;
            }

            // Remove unhelpful objects
            for (int i = toFilter.size() - 1; i >= 0; i--) {
                if (_this->excludeList.contains(toFilter[i])) {
                    toFilter.remove(i);
                }
            }
        };
        auto patchDir = currentObject->cnv->patch.getPatchFile().getParentDirectory();
        if (!patchDir.isDirectory() || patchDir == File::getSpecialLocation(File::tempDirectory))
            patchDir = File();

        // Update suggestions
        auto found = library->autocomplete(currentText, patchDir);

        filterObjects(found);

        if (found.isEmpty() || !found[0].startsWith(currentText)) {
            autoCompleteComponent->enableAutocomplete(false);
            deselectAll();
            currentidx = -1;
        } else {
            found = sortSuggestions(currentText, found);
            if (currentText.isEmpty() || currentidx == -1 || !found[currentidx].startsWith(currentText)) {
                currentidx = 0;
                autoCompleteComponent->setSuggestion(found[0]);
            }
            buttons[currentidx]->setToggleState(true, dontSendNotification);
            autoCompleteComponent->enableAutocomplete(true);
        }

        if (openedEditor) {
            numOptions = found.size();

            // Apply object name and descriptions to buttons
            for (int i = 0; i < std::min<int>(buttons.size(), numOptions); i++) {
                auto& name = found[i];

                auto const& info = library->getObjectInfo(name);
                buttons[i]->setText(name, info.description, true);
                buttons[i]->setInterceptsMouseClicks(true, false);
            }

            for (int i = numOptions; i < buttons.size(); i++)
                buttons[i]->setText("", "", false);

            resized();

            // Get length of user-typed text
            int textlen = openedEditor->getText().length();

            if (found.isEmpty() || textlen == 0) {
                state = Hidden;
                if (autoCompleteComponent)
                    autoCompleteComponent->enableAutocomplete(false);
                setVisible(false);
                return;
            }

            // Limit it to minimum of the number of buttons and the number of suggestions
            int numButtons = std::min(20, numOptions);

            setVisible(true);

            state = ShowingObjects;

            if (currentidx < 0)
                return;

            currentidx = (currentidx + numButtons) % numButtons;

            // Retrieve best suggestion
            auto const& fullName = found[currentidx];

            if (fullName.length() > textlen && autoCompleteComponent) {
                autoCompleteComponent->setSuggestion(fullName);
            } else if (autoCompleteComponent) {
                autoCompleteComponent->setSuggestion("");
            }
        }
    }

private:
    void mouseDown(MouseEvent const& e) override
    {
        if (openedEditor)
            openedEditor->grabKeyboardFocus();
    }

    bool hitTest(int const x, int const y) override
    {
        return getWindowBounds().contains(x, y);
    }

    static bool canBeTransparent()
    {
        return ProjectInfo::canUseSemiTransparentWindows();
    }

    void paint(Graphics& g) override
    {
        if (!canBeTransparent()) {
            g.fillAll(PlugDataColours::canvasBackgroundColour);
        } else {
            StackShadow::drawShadowForRect(g, getLocalBounds().reduced(12), 12, Corners::defaultCornerRadius, 0.44f);
        }

        g.setColour(PlugDataColours::popupMenuBackgroundColour);
        g.fillRoundedRectangle(port->getBounds().toFloat(), Corners::defaultCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(PlugDataColours::outlineColour.darker(0.1f));
        g.drawRoundedRectangle(port->getBounds().toFloat(), Corners::defaultCornerRadius, 1.0f);
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        if (!currentObject) {
            return false;
        }
        if (openedEditor->getHighlightedRegion().isEmpty() && key == KeyPress::rightKey && autoCompleteComponent && autoCompleteComponent->isAutocompleting() && openedEditor->getCaretPosition() == openedEditor->getText().length()) {
            autoCompleteComponent->autocomplete();
            currentidx = 0;
            if (buttons.size())
                buttons[0]->setToggleState(true, dontSendNotification);
            return true;
        }
        if (key == KeyPress::tabKey && autoCompleteComponent->isAutocompleting() && openedEditor->getText() != autoCompleteComponent->getSuggestion() && numOptions != 0) {
            autoCompleteComponent->autocomplete();
            currentidx = 0;
            if (buttons.size())
                buttons[0]->setToggleState(true, dontSendNotification);
            return true;
        }
        if (key == KeyPress::returnKey) {
            if (autoCompleteComponent->isAutocompleting() && openedEditor->getText() != autoCompleteComponent->getSuggestion() && numOptions != 0) {
                autoCompleteComponent->autocomplete();
                currentidx = 0;
                if (buttons.size())
                    buttons[0]->setToggleState(true, dontSendNotification);
                return true;
            }
            // if there is no autocomplete action going on, we want to close upon error
            // By ignoring the keypress we'll trigger the return callback on text editor which will close it
            return false;
        }
        if (state != ShowingObjects || !isVisible())
            return false;

        if (key == KeyPress::upKey || key == KeyPress::downKey) {
            move(key == KeyPress::downKey ? 1 : -1);
            return true;
        }
        return false;
    }

    SmallArray<std::tuple<String, String, String>> findNearbyMethods(String const& toSearch) const
    {
        SmallArray<std::tuple<String, HeapArray<pd::Library::ObjectReferenceTable::ReferenceItem>, int>> objects;
        auto* cnv = currentObject->cnv;
        for (auto* obj : cnv->objects) {
            int distance = currentObject->getPosition().getDistanceFrom(obj->getPosition());
            if (!obj->getPointer() || obj == currentObject || distance > 300)
                continue;

            auto objectName = obj->getType();
            auto alreadyExists = std::ranges::find_if(objects, [objectName](auto const& toCompare) {
                return std::get<0>(toCompare) == objectName;
            }) != objects.end();

            if (alreadyExists)
                continue;

            auto const& info = cnv->pd->objectLibrary->getObjectInfo(objectName);
            objects.add({ objectName, info.methods, distance });
        }

        // Sort by distance
        objects.sort([](auto const& a, auto const& b) {
            return std::get<2>(a) > std::get<2>(b);
        });

        // Look for object name matches
        SmallArray<std::tuple<String, String, String>> nearbyMethods;
        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                if (objectName.contains(toSearch)) {
                    auto methodName = method.type;
                    auto description = method.description;
                    nearbyMethods.add({ objectName, methodName, description });
                }
            }
        }

        // Look for method name matches
        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                auto methodName = method.type;
                if (methodName.contains(toSearch)) {
                    auto description = method.description;
                    nearbyMethods.add({ objectName, methodName, description });
                }
            }
        }

        // Look for description matches
        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                auto description = method.description;
                if (description.contains(toSearch)) {
                    auto methodName = method.type;
                    nearbyMethods.add({ objectName, methodName, description });
                }
            }
        }

        // Remove duplicates
        for (int i = nearbyMethods.size() - 1; i >= 0; i--) {
            auto& [objectName1, method1, distance1] = nearbyMethods[i];

            for (int j = nearbyMethods.size() - 1; j >= 0; j--) {
                auto& [objectName2, method2, distance2] = nearbyMethods[j];
                if (objectName1 == objectName2 && method1 == method2 && i != j) {
                    nearbyMethods.remove_at(i);
                    break;
                }
            }
        }

        return nearbyMethods;
    }

    struct SendReceiveEntry
    {
        String name;
        String prefix;
        String sendSymbol;
        String receiveSymbol;
        String dollSym;
    };

    SmallArray<SendReceiveEntry> getAllSendReceives(pd::Patch& patch, int& total, String prefix = "", int depth = 0)
    {
        SmallArray<SendReceiveEntry> result;

        String currentDollsym;
        if(auto p = patch.getPointer()) {
            auto* realised = canvas_realizedollar(p.get(), currentObject->cnv->pd->generateSymbol("$0"));
            currentDollsym = String::fromUTF8(realised->s_name);
        }

        SmallArray<std::tuple<pd::WeakReference, String, String>> allObjects;
        for(auto& objectPtr : patch.getObjects())
        {
            String type, name;
            if (auto object = objectPtr.get<t_pd>()) {
                if (!pd::Interface::checkObject(object.get()))
                    continue;
                name = pd::Interface::getObjectText(object.cast<t_text>());
                type = String::fromUTF8(pd::Interface::getObjectClassName(object.get()));
            }
            allObjects.emplace_back(objectPtr, name, type);
        }

        for(auto& [objectPtr, name, type] : allObjects) {
            if(total > 15) break;

            SendReceiveEntry entry;
            auto nameWithoutArgs = name.upToFirstOccurrenceOf(" ", false, false);
            switch (hash(type)) {
                case hash("bng"):
                case hash("hsl"):
                case hash("vsl"):
                case hash("slider"):
                case hash("tgl"):
                case hash("nbx"):
                case hash("vradio"):
                case hash("hradio"):
                case hash("vu"):
                case hash("cnv"): {
                    if (auto iemgui = objectPtr.get<t_iemgui>()) {
                        auto srl_is_valid = [](t_symbol const* s)
                        {
                            return s != nullptr && s != gensym("");
                        };

                        t_symbol* srlsym[3];
                        iemgui_all_sym2dollararg(iemgui.get(), srlsym);
                        if (srl_is_valid(srlsym[0])) {
                            entry.sendSymbol = String::fromUTF8(iemgui->x_snd_unexpanded->s_name);
                        }
                        if (srl_is_valid(srlsym[1])) {
                            entry.receiveSymbol = String::fromUTF8(iemgui->x_rcv_unexpanded->s_name);
                        }
                    }
                    entry.name = nameWithoutArgs;
                    break;
                }
                case hash("keyboard"): {
                    if (auto keyboardObject = objectPtr.get<t_fake_keyboard>()) {
                        entry.sendSymbol = String(keyboardObject->x_send->s_name);
                        entry.receiveSymbol = String(keyboardObject->x_receive->s_name);
                    }
                    entry.name = nameWithoutArgs;
                    break;
                }
                case hash("pic"): {
                    if (auto picObject = objectPtr.get<t_fake_pic>()) {
                        entry.sendSymbol = String(picObject->x_send->s_name);
                        entry.receiveSymbol = String(picObject->x_receive->s_name);
                    }
                    entry.name = nameWithoutArgs;
                    break;
                }
                case hash("scope~"): {
                    if (auto scopeObject = objectPtr.get<t_fake_scope>()) {
                        entry.receiveSymbol = String(scopeObject->x_receive->s_name);
                    }
                    entry.name = nameWithoutArgs;
                    break;
                }
                case hash("function"): {
                    if (auto keyboardObject = objectPtr.get<t_fake_function>()) {
                        entry.sendSymbol = String(keyboardObject->x_send->s_name);
                        entry.receiveSymbol = String(keyboardObject->x_receive->s_name);
                    }
                    entry.name = nameWithoutArgs;
                    break;
                }
                case hash("note"): {
                    if (auto noteObject = objectPtr.get<t_fake_note>()) {
                        entry.receiveSymbol = String(noteObject->x_receive->s_name);
                    }
                    entry.name = nameWithoutArgs;
                    break;
                }
                case hash("knob"): {
                    if (auto knobObj = objectPtr.get<t_fake_knob>()) {
                        entry.sendSymbol = String(knobObj->x_snd->s_name);
                        entry.receiveSymbol = String(knobObj->x_rcv->s_name);
                    }
                    entry.name = nameWithoutArgs;
                    break;
                }
                case hash("gatom"): {
                    auto gatomObject = objectPtr.get<t_fake_gatom>();
                    String gatomName;
                    switch (gatomObject->a_flavor) {
                        case A_FLOAT:
                            gatomName = "floatbox";
                            break;
                        case A_SYMBOL:
                            gatomName = "symbolbox";
                            break;
                        case A_NULL:
                            gatomName = "listbox";
                            break;
                        default:
                            break;
                    }
                    entry.receiveSymbol = String(gatomObject->a_symfrom->s_name);
                    entry.sendSymbol = String(gatomObject->a_symto->s_name);
                    entry.name = gatomName;
                    break;
                }
                default: {
                    auto getFirstArgumentFromFullName = [](String const& fullName) -> String {
                        return fullName.fromFirstOccurrenceOf(" ", false, true).upToFirstOccurrenceOf(" ", false, true);
                    };

                    switch (hash(nameWithoutArgs)) {
                        case hash("s"):
                        case hash("s~"):
                        case hash("send"):
                        case hash("send~"):
                        case hash("throw~"): {
                            entry.sendSymbol = getFirstArgumentFromFullName(name);
                            entry.name = nameWithoutArgs;
                            break;
                        }
                        case hash("r"):
                        case hash("r~"):
                        case hash("receive"):
                        case hash("receive~"):
                        case hash("catch~"): {
                            entry.receiveSymbol = getFirstArgumentFromFullName(name);
                            entry.name = nameWithoutArgs;
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                }
            }
            if(entry.name.isNotEmpty()) {
                entry.prefix = prefix;
                entry.dollSym = currentDollsym;
                result.insert(result.begin(), entry);
                total++;
            }
        }

        for(auto& [objectPtr, name, type] : allObjects) {
            if(total > 15) break;
            if (type == "canvas" || type == "graph") {
                auto subpatch = pd::Patch(objectPtr, currentObject->cnv->pd, false);
                if(depth < 3) {
                    result.add_array(getAllSendReceives(subpatch, total, prefix + name + " -> ", ++depth));
                }
            }
        }
        return result;
    }


    SmallArray<SendReceiveEntry> findSendReceive(pd::Patch& patch, String searchSymbol, bool wantSend)
    {
        if(sendReceiveDatabase.empty()) // database gets cleared when we close the editor, so at least it doesn't update on every keystroke
        {
            int total = 0;
            sendReceiveDatabase = getAllSendReceives(patch, total);
        }

        String currentDollsym;
        if(auto p = patch.getPointer()) {
            auto* realised = canvas_realizedollar(p.get(), currentObject->cnv->pd->generateSymbol(searchSymbol));
            currentDollsym = String::fromUTF8(realised->s_name);
        }
        searchSymbol = searchSymbol.replace("$0", currentDollsym);

        SmallArray<SendReceiveEntry> filteredSendReceives;
        for(auto entry : sendReceiveDatabase)
        {
            auto targetSymbol = wantSend ? entry.sendSymbol : entry.receiveSymbol;
            if(entry.name.isNotEmpty() && targetSymbol.isNotEmpty())
            {
                String expandedSymbol = entry.dollSym != currentDollsym ? targetSymbol.replace("$0", entry.dollSym) : targetSymbol;
                if(expandedSymbol.contains(searchSymbol)) {
                    if(entry.prefix.isNotEmpty() && targetSymbol.contains("$")) {
                        if(wantSend)
                            entry.sendSymbol = expandedSymbol;
                        else
                            entry.receiveSymbol = expandedSymbol;
                    }
                    entry.name = entry.prefix + entry.name;
                    filteredSendReceives.add(entry);
                }
            }
        }
        return filteredSendReceives;
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

    class ResizerLookAndFeel : public LookAndFeel_V2 {
        void drawCornerResizer(Graphics& g, int const w, int const h, bool const isMouseOver, bool isMouseDragging) override
        {
            float const cornerSize = Corners::defaultCornerRadius;

            g.saveState();

            Path clip;
            clip.addRoundedRectangle(-cornerSize, -cornerSize, w + cornerSize, h + cornerSize, cornerSize);
            g.reduceClipRegion(clip);

            Path triangle;
            triangle.addTriangle(Point<float>(0, h), Point<float>(w, h), Point<float>(w, 0));

            g.setColour(PlugDataColours::objectSelectedOutlineColour.withAlpha(isMouseOver ? 1.0f : 0.6f));
            g.fillPath(triangle);

            g.restoreState();
        }
    };

    ResizerLookAndFeel resizerLookAndFeel;
    ResizableCornerComponent resizer;
    ComponentBoundsConstrainer constrainer;

    SugesstionState state = Hidden;

    SafePointer<TextEditor> openedEditor = nullptr;
    SafePointer<Object> currentObject = nullptr;
    String lastText;

    SmallArray<SendReceiveEntry> sendReceiveDatabase;

    StringArray excludeList = {
        "number~", // appears before numbox~ alphabetically, but is worse in every way
        "allpass_unit",
        "echo_unit",
        "multi.vsl.unit",
        "float2sig.unit",
        "imp.mc-unit",
        "multi.vsl.unit",
        "multi.vsl.clone.ex",
        "score-ex1",
        "voice",
        "args-example",
        "dollsym-example",
        "fontsize-example",
        "oscbank.unit",
        "oscbank2.unit",
        "osc.mc-unit",
        "All_objects",
        "All_about_else",
        "All_about_cyclone",
        "README.deken",
        "about.MERDA",
        "else-meta",
        "resonbank.unit",
        "resonbank2.unit",
        "stepnoise.mc-unit",
        "rampnoise.mc-unit",
        "onebang_proxy",
        "grain.sampler.grain",
        "freeze.osc.clone~",
        "presets.send.clone",
        "grain.synth.grain",
        "presets-abs",
        "grain.live.grain",
        "synth.voice.template",
        "vocoder.band_clone",
        "f2s~-help",
        "rev1-final",
        "rev1-stage",
        "bpclone",
        "libpd_receive",
        "out.mc.hip~",
        "pvoc~",
        "gran~",
        "convpartition",
        "gatehold.unit",
        "gaterelease.unit",
        "else",
        "cyclone",
        "circle-gui"
    };
};
