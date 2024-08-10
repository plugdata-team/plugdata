/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginEditor.h"
#include "PluginProcessor.h" // TODO: We shouldn't need this!
#include "Objects/ObjectBase.h"
#include "Heavy/CompatibleObjects.h"
#include "Utility/NanoVGGraphicsContext.h"
#include "Components/BouncingViewport.h"

extern "C" {
int is_gem_object(char const* sym);
}

// Component that sits on top of a TextEditor and will draw auto-complete suggestions over it
class AutoCompleteComponent
    : public Component
    , public NVGComponent
    , public ComponentListener {
    String suggestion;
    Canvas* cnv;
    Component::SafePointer<TextEditor> editor;
    std::unique_ptr<NanoVGGraphicsContext> nvgCtx;

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

        editor->setText(editor->getText() + suggestion.upToFirstOccurrenceOf(" ", false, false), sendNotification);
        editor->moveCaretToEnd();
        suggestion = "";
        repaint();
    }

    void enableAutocomplete(bool enabled)
    {
        shouldAutocomplete = enabled;
    }

    bool isAutocompleting()
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

        if (suggestionText.startsWith(editorText)) {
            auto textUpToSpace = editorText.upToFirstOccurrenceOf(" ", false, false);
            suggestion = suggestionText.fromFirstOccurrenceOf(textUpToSpace, false, true);
            setVisible(suggestionText.isNotEmpty() && textUpToSpace != suggestionText);
        } else if (editorText.isNotEmpty()) {
            stashedText = editorText;
            editor->setText("", dontSendNotification);
            suggestion = suggestionText;
        }

        suggestion = suggestion.upToFirstOccurrenceOf(" ", false, false);
        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        NVGScopedState scopedState(nvg);
        nvgTranslate(nvg, getX(), getY());
        if (!nvgCtx || nvgCtx->getContext() != nvg)
            nvgCtx = std::make_unique<NanoVGGraphicsContext>(nvg);
        Graphics g(*nvgCtx);
        {
            paintEntireComponent(g, true);
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

        auto colour = findColour(PlugDataColour::canvasTextColourId).withAlpha(0.5f);
        Fonts::drawText(g, suggestion, completionBounds.translated(-1.25f, -1.25f), colour);
    }
};
// Suggestions component that shows up when objects are edited
class SuggestionComponent : public Component
    , public KeyListener
    , public ComponentListener {

    class Suggestion : public TextButton {

        enum ObjectType {
            Data = 0,
            Signal = 1,
            Gem = 2
        };

        ObjectType type;

        String objectDescription;

    public:
        Suggestion(SuggestionComponent* parentComponent)
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

        void setText(String const& name, String const& description, bool icon)
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
            auto scrollbarIndent = parent->port->canScrollVertically() ? 6 : 0;

            auto backgroundColour = findColour(getToggleState() ? PlugDataColour::popupMenuActiveBackgroundColourId : PlugDataColour::popupMenuBackgroundColourId);

            auto buttonArea = getLocalBounds().withTrimmedRight((parent->canBeTransparent() ? 42 : 2) + scrollbarIndent).toFloat().reduced(4, 1);

            g.setColour(backgroundColour);
            g.fillRoundedRectangle(buttonArea, Corners::defaultCornerRadius);

            auto colour = findColour(PlugDataColour::popupMenuTextColourId);
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

            if (drawIcon) {
                Colour iconColour;
                String iconText;
                if (type == Data) {
                    iconColour = findColour(PlugDataColour::dataColourId);
                    iconText = "pd";
                } else if (type == Signal) {
                    iconColour = findColour(PlugDataColour::signalColourId);
                    iconText = "~";
                } else if (type == Gem) {
                    iconColour = findColour(PlugDataColour::gemColourId);
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
        , currentObject(nullptr)
        , windowMargin(canBeTransparent() ? 22 : 0)
    {
        // Set up the button list that contains our suggestions
        buttonholder = std::make_unique<Component>();

        for (int i = 0; i < 20; i++) {
            Suggestion* but = buttons.add(new Suggestion(this));
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

        // resizer.setAllowHostManagedResize(false);
        addAndMakeVisible(resizer);

        setInterceptsMouseClicks(true, true);
        setAlwaysOnTop(true);
        setWantsKeyboardFocus(false);
    }

    ~SuggestionComponent() override
    {
        buttons.clear();
    }

    void renderAutocompletion(NVGcontext* nvg)
    {
        if (autoCompleteComponent)
            autoCompleteComponent->render(nvg);
    }

    void createCalloutBox(Object* object, TextEditor* editor)
    {
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
                }
                else {
                    move(0, i);
                }

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
        auto buttonText = but->getButtonText();
        
        if (autoCompleteComponent && buttonText.startsWith(openedEditor->getText())) {
            autoCompleteComponent->setSuggestion(buttonText);
            autoCompleteComponent->enableAutocomplete(true);
            currentObject->updateBounds();
            resized();
        }
        else
        {
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
    void updateSuggestions(String const& currentText)
    {
        if (!currentObject || lastText == currentText) {
            return;
        }
        
        if(currentidx >= 0 && buttons[currentidx]->getButtonText() == currentText)
        {
            if(autoCompleteComponent) autoCompleteComponent->setSuggestion("");
            return;
        }
        
        lastText = currentText;
        
        auto& library = currentObject->cnv->pd->objectLibrary;

        class ObjectSorter {
        public:
            ObjectSorter(String searchQuery)
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
                if (a == (query + "~") && b != query && b != (query + "~")) {
                    return -1;
                }

                if (b == (query + "~") && a != query && a != (query + "~")) {
                    return 1;
                }

                // Check if suggestion is equal to query with "." appended
                if (a.startsWith(query + ".") && b != query && b != (query + "~") && !b.startsWith(query + ".")) {
                    return -1;
                }

                if (b.startsWith(query + ".") && a != query && a != (query + "~") && !a.startsWith(query + ".")) {
                    return 1;
                }
                
                if(a.startsWith(query) && !b.startsWith(query))
                {
                    return -1;
                }
                
                if(b.startsWith(query) && !a.startsWith(query))
                {
                    return 1;
                }
                
                return 0;
            }
            String const query;
        };

        auto sortSuggestions = [](String query, StringArray suggestions) -> StringArray {
            if (query.length() == 0)
                return suggestions;

            auto sorter = ObjectSorter(query);
            suggestions.strings.sort(sorter, true);
            return suggestions;
        };

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

            if (nearbyMethods.isEmpty() || textlen == 0) {
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
            auto name = currentText.upToFirstOccurrenceOf(" ", false, false);
            auto objectInfo = library->getObjectInfo(name);
            if (objectInfo.isValid()) {
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
                    auto def = found.getChild(i).getProperty("default").toString();

                    if (def.isNotEmpty())
                        description += " (default: " + def + ")";

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
        }

        if (isPositiveAndBelow(currentidx, buttons.size())) {
            buttons[currentidx]->setToggleState(true, dontSendNotification);
        }

        auto filterObjects = [_this = SafePointer(this), &library](StringArray& toFilter) {
            if (!_this || !_this->currentObject)
                return;
            
            if(!SettingsFile::getInstance()->getLibrariesTree().getChildWithProperty("Name", "Gem").isValid())
            {
                StringArray noGemObjects;
                for (auto& object : toFilter) {
                    if(object.startsWith("Gem/") || !library->isGemObject(object)) // Don't suggest Gem objects without "Gem/" prefix unless gem library is loaded
                    {
                        noGemObjects.add(object);
                    }
                }
                
                toFilter = noGemObjects;
            }

            // When hvcc mode is enabled, show only hvcc compatible objects
            if (_this->currentObject->hvccMode.get()) {

                StringArray hvccObjectsFound;
                for (auto& object : toFilter) {
                    // We support arrays, but when you create [array] it is really [array define] which is unsupported
                    if (HeavyCompatibleObjects::getAllCompatibleObjects().contains(object) && object != "array") {
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
            currentidx = 0;
            autoCompleteComponent->enableAutocomplete(true);
        }
        
        if (openedEditor) {
            numOptions = static_cast<int>(found.size());

            // Apply object name and descriptions to buttons
            for (int i = 0; i < std::min<int>(buttons.size(), numOptions); i++) {
                auto& name = found[i];

                auto info = library->getObjectInfo(name);
                auto description = info.isValid() ? info.getProperty("description").toString() : "";
                buttons[i]->setText(name, description, true);

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
            StackShadow::renderDropShadow(g, localPath, Colour(0, 0, 0).withAlpha(0.6f), 13, { 0, 1 });
        }

        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId));
        g.fillRoundedRectangle(port->getBounds().reduced(1).toFloat(), Corners::defaultCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId).darker(0.1f));
        g.drawRoundedRectangle(port->getBounds().reduced(1).toFloat(), Corners::defaultCornerRadius, 1.0f);
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
            } else {
                // if there is no autocomplete action going on, we want to close upon error
                // By ignoring the keypress we'll trigger the return callback on text editor which will close it
                return false;
            }
        }
        if (state != ShowingObjects || !isVisible())
            return false;

        if (key == KeyPress::upKey || key == KeyPress::downKey) {
            move(key == KeyPress::downKey ? 1 : -1);
            return true;
        }
        return false;
    }

    Array<std::tuple<String, String, String>> findNearbyMethods(String const& toSearch)
    {
        Array<std::tuple<String, ValueTree, int>> objects;
        auto* cnv = currentObject->cnv;
        for (auto* obj : cnv->objects) {
            int distance = currentObject->getPosition().getDistanceFrom(obj->getPosition());
            if (!obj->getPointer() || obj == currentObject || distance > 300)
                continue;

            auto objectName = obj->getType();
            auto alreadyExists = std::find_if(objects.begin(), objects.end(), [objectName](auto const& toCompare) {
                return std::get<0>(toCompare) == objectName;
            }) != objects.end();

            if (alreadyExists)
                continue;

            auto info = cnv->pd->objectLibrary->getObjectInfo(objectName);
            if (info.isValid()) {
                auto methods = info.getChildWithName("methods");
                objects.add({ objectName, methods, distance });
            }
        }

        // Sort by distance
        std::sort(objects.begin(), objects.end(), [](auto const& a, auto const& b) {
            return std::get<2>(a) > std::get<2>(b);
        });

        // Look for object name matches
        Array<std::tuple<String, String, String>> nearbyMethods;
        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                if (objectName.contains(toSearch)) {
                    auto methodName = method.getProperty("type").toString();
                    auto description = method.getProperty("description").toString();
                    nearbyMethods.add({ objectName, methodName, description });
                }
            }
        }

        // Look for method name matches
        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                auto methodName = method.getProperty("type").toString();
                if (methodName.contains(toSearch)) {
                    auto description = method.getProperty("description").toString();
                    nearbyMethods.add({ objectName, methodName, description });
                }
            }
        }

        // Look for description matches
        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                auto description = method.getProperty("description").toString();
                if (description.contains(toSearch)) {
                    auto methodName = method.getProperty("type").toString();
                    nearbyMethods.add({ objectName, methodName, description });
                }
            }
        }

        // Remove duplicates
        for (int i = nearbyMethods.size() - 1; i >= 0; i--) {
            auto& [objectName1, method1, distance1] = nearbyMethods.getReference(i);

            for (int j = nearbyMethods.size() - 1; j >= 0; j--) {
                auto& [objectName2, method2, distance2] = nearbyMethods.getReference(j);
                if (objectName1 == objectName2 && method1 == method2 && i != j) {
                    nearbyMethods.remove(i);
                    break;
                }
            }
        }

        return nearbyMethods;
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
    String lastText;

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
        "else",
        "cyclone"
    };

    int windowMargin;
};
