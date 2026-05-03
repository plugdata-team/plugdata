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

struct SuggestionEntry {
    enum class IconType { None,
        Data,
        Signal,
        Gem };

    String displayName;
    String description;
    IconType icon = IconType::None;
    String displayOverride;
    String detailLookupName;
    String completionOverride;

    String const& getDisplayText() const
    {
        return displayOverride.isNotEmpty() ? displayOverride : displayName;
    }

    String const& getCompletionText() const
    {
        return completionOverride.isNotEmpty() ? completionOverride : displayName;
    }
};

struct SuggestionQueryResult {
    HeapArray<SuggestionEntry> entries;
    String topAutocompleteText;
    bool autocompleteSupported : 1 = false;
    bool autocompleteRequiresNavigation : 1 = false;
    bool detailLookupSupported : 1 = false;
    bool detailPanelFillsPopup : 1 = false;
    bool shrinkHeightToFitEntries : 1 = false;
    String detailLookupTarget;
};

//  AutoCompleteComponent: An overlay drawn on top of the open TextEditor that paints "ghost text" for
//  the current top suggestion.
class AutoCompleteComponent final
    : public Component
    , public NVGComponent
    , public ComponentListener {
public:
    AutoCompleteComponent(TextEditor* e, Canvas* c)
        : NVGComponent(this)
        , cnv(c)
        , editor(e)
    {
        setAlwaysOnTop(true);
        setInterceptsMouseClicks(false, false);

        if (editor)
            editor->addComponentListener(this);
        cnv->addAndMakeVisible(this);
    }

    ~AutoCompleteComponent() override
    {
        if (editor)
            editor->removeComponentListener(this);
    }

    void setSuggestion(String const& fullSuggestion)
    {
        if (!editor)
            return;

        if (!enabled || fullSuggestion.isEmpty()) {
            ghostText.clear();
            repaint();
            return;
        }

        auto const editorText = editor->getText();
        if (editorText.isEmpty() || fullSuggestion.startsWith(editorText)) {
            ghostText = fullSuggestion.substring(editorText.length());
        } else {
            ghostText.clear();
        }
        repaint();
    }

    void clear()
    {
        ghostText.clear();
        repaint();
    }

    void setEnabled(bool isEnabled)
    {
        enabled = isEnabled;
        if (!enabled)
            ghostText.clear();
        repaint();
    }

    bool hasGhostText() const
    {
        return enabled && ghostText.isNotEmpty();
    }

    String getCompletedText() const
    {
        if (!editor)
            return { };
        return editor->getText() + ghostText;
    }

    // Commit the ghost text into the editor.
    void accept()
    {
        if (!editor || ghostText.isEmpty())
            return;

        editor->setText(editor->getText() + ghostText, sendNotification);
        editor->moveCaretToEnd();
        ghostText.clear();
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
    void componentMovedOrResized(Component&, bool, bool) override
    {
        if (!editor)
            return;
        setBounds(cnv->getLocalArea(editor, editor->getLocalBounds()));
    }

    void componentBeingDeleted(Component&) override
    {
        if (editor)
            editor->removeComponentListener(this);
    }

    void paint(Graphics& g) override
    {
        if (!editor || !enabled || ghostText.isEmpty())
            return;

        auto const editorText = editor->getText();
        auto const xOffset = Fonts::getStringWidth(editorText, editor->getFont()) + 7.5f;
        auto const colour = PlugDataColours::canvasTextColour.withAlpha(0.5f);

        Fonts::drawText(g, ghostText,
            getLocalBounds().toFloat().withTrimmedLeft(xOffset).translated(-1.25f, 0),
            colour);
    }

    Canvas* cnv;
    Component::SafePointer<TextEditor> editor;
    String ghostText;
    bool enabled = true;
};

class SuggestionComponent final
    : public Component
    , public KeyListener
    , public ComponentListener {

    enum class LayoutMode { ListOnly,
        ListWithDetail,
        DetailOnly };

    class Row final : public TextButton {
    public:
        explicit Row(SuggestionComponent* parentComponent)
            : owner(parentComponent)
        {
            setButtonText("");
            setWantsKeyboardFocus(false);
            setConnectedEdges(12);
            setClickingTogglesState(true);
            setTriggeredOnMouseDown(true);
            setRadioGroupId(hash("suggestion_component"));
            setColour(TextButton::buttonOnColourId, findColour(ScrollBar::thumbColourId));
        }

        void setEntry(SuggestionEntry const& entry)
        {
            current = entry;
            setButtonText(entry.displayName);
            setInterceptsMouseClicks(true, false);
            repaint();
        }

        void clear()
        {
            current = { };
            setButtonText("");
            setInterceptsMouseClicks(false, false);
            repaint();
        }

        SuggestionEntry const& getEntry() const { return current; }
        bool hasEntry() const { return current.displayName.isNotEmpty(); }

    private:
        void paint(Graphics& g) override
        {
            auto const scrollbarIndent = owner->port->canScrollVertically() ? 6 : 0;
            auto const buttonArea = getLocalBounds().withTrimmedRight(2 + scrollbarIndent).toFloat().reduced(4, 2);

            // When toggled, draw the highlight; otherwise stay transparent so
            // the popup's unified rounded rectangle shows through.
            if (getToggleState()) {
                g.setColour(PlugDataColours::popupMenuActiveBackgroundColour);
                g.fillRoundedRectangle(buttonArea, Corners::defaultCornerRadius);
            }

            auto const textColour = PlugDataColours::popupMenuTextColour;
            auto const yIndent = jmin(4, proportionOfHeight(0.3f));
            auto leftIndent = current.icon != SuggestionEntry::IconType::None ? 36 : 11;
            constexpr auto rightIndent = 14;
            auto textWidth = getWidth() - leftIndent - rightIndent;

            auto const& displayed = current.getDisplayText();

            if (textWidth > 0) {
                Fonts::drawStyledText(g, displayed, leftIndent, yIndent, textWidth,
                    getHeight() - yIndent * 2, textColour, Semibold, 14);
            }

            if (current.description.isNotEmpty()) {
                auto const nameWidth = Fonts::getStringWidth(displayed, Fonts::getSemiBoldFont().withHeight(14));
                leftIndent += nameWidth;
                textWidth = getWidth() - leftIndent - rightIndent;

                Fonts::drawText(g, String::fromUTF8("  \xe2\x80\x93  ") + current.description,
                    Rectangle<int>(leftIndent, yIndent, textWidth, getHeight() - yIndent * 2),
                    textColour, 14);
            }

            if (current.icon != SuggestionEntry::IconType::None)
                drawIcon(g);
        }

        void drawIcon(Graphics& g)
        {
            Colour iconColour;
            String iconText;
            int textSize = 12;

            switch (current.icon) {
            case SuggestionEntry::IconType::Data:
                iconColour = PlugDataColours::dataColour;
                iconText = "pd";
                textSize = 10;
                break;
            case SuggestionEntry::IconType::Signal:
                iconColour = PlugDataColours::signalColour;
                iconText = "~";
                break;
            case SuggestionEntry::IconType::Gem:
                iconColour = PlugDataColours::gemColour;
                iconText = "g";
                break;
            default:
                return;
            }

            auto rect = getLocalBounds().reduced(5);
            rect.setWidth(getHeight() - 8);
            rect.translate(3, 0);

            g.setColour(iconColour);
            g.fillRoundedRectangle(rect.toFloat(), Corners::defaultCornerRadius);
            Fonts::drawFittedText(g, iconText, rect.reduced(1), Colours::white, 1, 1.0f, textSize, Justification::centred);
        }

        SuggestionComponent* owner;
        SuggestionEntry current;
    };

    class ObjectDetailPanel final : public Component {
    public:
        ObjectDetailPanel() = default;

        void setObject(String const& objectName, pd::Library* library)
        {
            currentName = objectName;
            if (objectName.isEmpty() || library == nullptr) {
                hasInfo = false;
            } else {
                info = library->getObjectInfo(objectName);
                hasInfo = true;
            }
            repaint();
        }

        void clear()
        {
            currentName.clear();
            hasInfo = false;
            repaint();
        }

        String const& getDescription() const
        {
            return info.body.isNotEmpty() ? info.body : info.description;
        }

        bool hasContent() const
        {
            return hasInfo
                && (getDescription().isNotEmpty()
                    || info.arguments.size() > 0
                    || info.inlets.size() > 0
                    || info.outlets.size() > 0
                    || info.flags.size() > 0);
        }

        int getContentHeight(int width) const
        {
            return computeLayout(nullptr, width);
        }

    private:
        void paint(Graphics& g) override
        {
            computeLayout(&g, getWidth());
        }

        // When g is non-null the content drawn; when null we just measure and return total height.
        int computeLayout(Graphics* g, int width) const
        {
            if (!hasInfo || currentName.isEmpty())
                return 0;

            int const x = 12;
            int const w = jmax(40, width - 16);
            int y = 12;

            auto const text = PlugDataColours::popupMenuTextColour;
            auto const muted = text.withAlpha(0.55f);

            // Origin / category label
            auto const origin = info.categories.empty() ? String("") : info.categories[0];
            if (g)
                Fonts::drawStyledText(*g, origin.toUpperCase(), x, y, w, 14, muted, Semibold, 12);
            y += 16;

            // Object name
            if (g)
                Fonts::drawStyledText(*g, currentName, x, y, w, 22, text, Semibold, 18);
            y += 24;

            // Description body, word-wrapped
            if (info.description.isNotEmpty()) {
                AttributedString s(getDescription());
                s.setColour(text);
                s.setFont(Fonts::getDefaultFont().withHeight(14.0f));
                s.setJustification(Justification::centredLeft);
                s.setLineSpacing(1.05f);

                TextLayout layout;
                layout.createLayout(s, static_cast<float>(w));
                int const layoutH = static_cast<int>(std::ceil(layout.getHeight()));

                if (g)
                    layout.draw(*g, Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(layoutH)));
                y += layoutH + 14;
            }

            layoutReferenceSection(g, "ARGUMENTS", info.arguments, y, x, w, text, muted);
            layoutReferenceSection(g, "FLAGS", info.flags, y, x, w, text, muted);
            layoutIoletSection(g, "INLETS", info.inlets, y, x, w, text, muted);
            layoutIoletSection(g, "OUTLETS", info.outlets, y, x, w, text, muted);
            layoutReferenceSection(g, "METHODS", info.methods, y, x, w, text, muted);

            return y + 12; // bottom padding
        }

        static int layoutDefinitionRow(Graphics* g,
            String const& type, String const& description,
            int y, int x, int w, Colour text)
        {
            int const descWidth = jmax(40, w);

            AttributedString attr;
            attr.setJustification(Justification::centredLeft);
            attr.setLineSpacing(1.05f);

            attr.append(type + ": ", Fonts::getSemiBoldFont().withHeight(13.0f), text);
            attr.append(description, Fonts::getDefaultFont().withHeight(13.0f), text);

            TextLayout layout;
            layout.createLayout(attr, static_cast<float>(descWidth));

            int const layoutH = static_cast<int>(std::ceil(layout.getHeight()));
            int const rowHeight = jmax(16, layoutH + 2);

            if (g) {
                layout.draw(*g, Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(descWidth), static_cast<float>(layoutH)));
            }

            return rowHeight;
        }

        static void layoutReferenceSection(Graphics* g, StringRef title, HeapArray<pd::Library::ObjectReferenceTable::ReferenceItem> const& items, int& y, int x, int w, Colour text, Colour muted)
        {
            if (items.size() == 0)
                return;

            if (g)
                Fonts::drawStyledText(*g, title, x, y, w, 14, muted, Semibold, 13);
            y += 16;

            for (auto const& item : items) {
                y += layoutDefinitionRow(g, item.type, item.description, y, x, w, text);
            }
            y += 10;
        }

        static void layoutIoletSection(Graphics* g, StringRef title,
            HeapArray<pd::Library::ObjectReferenceTable::IoletReference> const& iolets,
            int& y, int x, int w, Colour text, Colour muted)
        {
            if (iolets.size() == 0)
                return;

            if (g)
                Fonts::drawStyledText(*g, title, x, y, w, 14, muted, Semibold, 13);
            y += 16;

            int const subX = x + 12;
            int const subW = jmax(40, w - 12);

            for (size_t i = 0; i < iolets.size(); i++) {
                auto const& iolet = iolets[i];

                if (g)
                    Fonts::drawStyledText(*g, String(i + 1) + ".", x, y, subW, 14, text, Semibold, 13);

                if (iolet.messages.size() > 0) {
                    for (auto const& msg : iolet.messages)
                        y += layoutDefinitionRow(g, msg.type, msg.description, y, subX, subW, text);
                } else if (iolet.tooltip.isNotEmpty()) {
                    AttributedString attr(iolet.tooltip);
                    attr.setColour(text);
                    attr.setFont(Fonts::getDefaultFont().withHeight(14.0f));
                    attr.setJustification(Justification::centredLeft);
                    attr.setLineSpacing(1.05f);

                    TextLayout layout;
                    layout.createLayout(attr, static_cast<float>(subW));
                    int const layoutH = static_cast<int>(std::ceil(layout.getHeight()));
                    if (g)
                        layout.draw(*g, Rectangle<float>(static_cast<float>(subX), static_cast<float>(y), static_cast<float>(subW), static_cast<float>(layoutH)));
                    y += jmax(16, layoutH + 2);
                }

                if (i + 1 < iolets.size())
                    y += 6;
            }
            y += 10;
        }

        String currentName;
        pd::Library::ObjectReferenceTable info;
        bool hasInfo = false;
    };

    class ResizerLookAndFeel : public LookAndFeel_V2 {
        void drawCornerResizer(Graphics& g, int const w, int const h, bool const isMouseOver, bool /*isMouseDragging*/) override
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

public:
    SuggestionComponent()
        : resizer(this, &constrainer)
    {
        resizer.setLookAndFeel(&resizerLookAndFeel);
        resizer.setAllowHostManagedResize(false);
        resizer.addMouseListener(this, true);

        detailPanel = std::make_unique<ObjectDetailPanel>();
        detailViewport = std::make_unique<BouncingViewport>();
        detailViewport->setScrollBarsShown(true, false);
        detailViewport->setViewedComponent(detailPanel.get(), false);
        detailViewport->setInterceptsMouseClicks(true, true);
        detailViewport->setViewportIgnoreDragFlag(true);
        detailViewport->setWantsKeyboardFocus(false);
        addChildComponent(*detailViewport);

        buttonHolder = std::make_unique<Component>();
        for (int i = 0; i < numRowsAllocated; i++) {
            auto* row = new Row(this);
            rows.add(row);
            buttonHolder->addAndMakeVisible(row);
            row->setColour(TextButton::buttonColourId, PlugDataColours::dialogBackgroundColour);
        }

        // Viewport
        port = std::make_unique<BouncingViewport>();
        port->setScrollBarsShown(true, false);
        port->setViewedComponent(buttonHolder.get(), false);
        port->setInterceptsMouseClicks(true, true);
        port->setViewportIgnoreDragFlag(true);
        port->setWantsKeyboardFocus(false);
        addAndMakeVisible(port.get());

        addAndMakeVisible(resizer);

        constrainer.setSizeLimits(150, 130, 800, 500);

        loadSavedSize();
        setSize(savedListSize.x, savedListSize.y);

        setInterceptsMouseClicks(true, true);
        setAlwaysOnTop(true);
        setWantsKeyboardFocus(false);

        for (int i = 0; i < rows.size(); i++) {
            int const idx = i;
            rows[i]->onClick = [this, idx] { onRowClicked(idx); };
        }
    }

    ~SuggestionComponent() override
    {
        resizer.setLookAndFeel(nullptr);
        rows.clear();
    }

    void createCalloutBox(Object* object, TextEditor* editor)
    {
        sendReceiveDatabase = { };
        currentObject = object;
        openedEditor = editor;

        setTransform(object->editor->getTransform());

        editor->addComponentListener(this);
        editor->addKeyListener(this);

        autoCompleteComponent = std::make_unique<AutoCompleteComponent>(editor, object->cnv);

        addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses,
            OSUtils::getDesktopParentPeer(object->editor));

        updateBounds();

        setVisible(false);
        toFront(false);

        repaint();
    }

    void removeCalloutBox()
    {
        currentSelection = -1;
        numOptions = 0;
        setVisible(false);

        if (isOnDesktop())
            removeFromDesktop();

        autoCompleteComponent.reset();

        if (openedEditor) {
            openedEditor->removeComponentListener(this);
            openedEditor->removeKeyListener(this);
        }

        openedEditor = nullptr;
        currentObject = nullptr;

        for (auto* row : rows)
            row->clear();
        detailPanel->clear();
        sendReceiveDatabase = { };
        lastQueriedText = "<unset>";

        currentResultSupportsAutocomplete = false;
        currentResultAutocompleteRequiresNavigation = false;
        currentResultSupportsDetail = false;
        autocompleteActivatedByNavigation = false;
        currentResultShouldShrinkHeightToFitEntries = false;
        autocompleteNavigationBaseText.clear();
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

        updatePopupVisibility();
    }

    String getText() const
    {
        return autoCompleteComponent ? autoCompleteComponent->getCompletedText() : String();
    }

    void renderAutocompletion(NVGcontext* nvg)
    {
        if (autoCompleteComponent)
            autoCompleteComponent->render(nvg);
    }

    void updateSuggestions(String const& currentText)
    {
        if (!currentObject || lastQueriedText == currentText)
            return;

        if (currentSelection >= 0 && currentSelection < rows.size()
            && rows[currentSelection]->getEntry().displayName == currentText) {
            if (autoCompleteComponent)
                autoCompleteComponent->clear();
            lastQueriedText = currentText;
            return;
        }

        lastQueriedText = currentText;
        autocompleteActivatedByNavigation = false;
        autocompleteNavigationBaseText = currentText;
        applyQueryResult(queryActive(currentText));
    }

    bool isShowingDetailPanel() const
    {
        return layoutMode == LayoutMode::ListWithDetail || layoutMode == LayoutMode::DetailOnly;
    }

    bool shouldKeepEditorOpen(TextEditor* editor) const
    {
        if (Component::getCurrentlyFocusedComponent() == editor || hasKeyboardFocus(true))
            return true;

        // On Linux, mouse-down on this temporary desktop window can make the
        // edited TextEditor lose native focus before JUCE assigns keyboard
        // focus to the popup/resizer. Treat an active click inside the popup as
        // part of the editor interaction so resizing does not commit the edit.
        if (isVisible() && ModifierKeys::currentModifiers.isAnyMouseButtonDown()) {
            auto const mousePos = Desktop::getInstance().getMainMouseSource().getScreenPosition();
            return getScreenBounds().toFloat().contains(mousePos);
        }

        return false;
    }

private:
    SuggestionQueryResult queryActive(String const& text) const
    {
        if (!currentObject)
            return { };

        // 1) [msg] objects: suggest methods used by neighbouring objects
        if (currentObject->gui && currentObject->getType(false) == "msg")
            return queryMessageMethods(text);

        // 2) send/receive prefixes: suggest matching nearby symbols
        auto const firstWord = text.upToFirstOccurrenceOf(" ", false, false);
        if (firstWord == "send" || firstWord == "s")
            return querySendReceive(text, /*isSend*/ true);
        if (firstWord == "receive" || firstWord == "r")
            return querySendReceive(text, /*isSend*/ false);

        // 3) After a space we're past the object name — show the detail panel
        //    for that object filling the whole popup.
        if (text.contains(" "))
            return queryArguments(text);

        // 4) Default: object-name autocomplete from the library
        return queryObjectNames(text);
    }

    // Mode 1: object names
    SuggestionQueryResult queryObjectNames(String const& text) const
    {
        SuggestionQueryResult result;
        result.autocompleteSupported = true;
        result.detailLookupSupported = true;

        auto& library = currentObject->cnv->pd->objectLibrary;

        auto patchDir = currentObject->cnv->patch.getPatchFile().getParentDirectory();
        if (!patchDir.isDirectory() || patchDir == File::getSpecialLocation(File::tempDirectory))
            patchDir = File();

        auto found = library->autocomplete(text, patchDir);
        filterObjectList(found);
        found = sortByQuery(text, found);

        for (auto const& name : found) {
            SuggestionEntry e;
            e.displayName = name;
            e.detailLookupName = name;
            e.description = library->getObjectInfo(name).description;
            e.icon = iconForName(name);
            result.entries.add(std::move(e));

            if (result.entries.size() >= numRowsAllocated)
                break;
        }

        if (!result.entries.empty() && found[0].startsWith(text))
            result.topAutocompleteText = found[0];

        return result;
    }

    // Mode 2: arguments (detail panel fills popup)
    SuggestionQueryResult queryArguments(String const& text) const
    {
        SuggestionQueryResult result;
        // The detail panel itself decides whether there is anything to show;
        // we mark the result as "panel only" and let applyQueryResult check.
        auto const name = text.upToFirstOccurrenceOf(" ", false, false)
                              .fromLastOccurrenceOf("/", false, false);

        if (name.isNotEmpty()) {
            result.detailPanelFillsPopup = true;
            result.detailLookupTarget = name;
        }

        return result;
    }

    // Mode 3: message methods
    SuggestionQueryResult queryMessageMethods(String const& text) const
    {
        SuggestionQueryResult result;
        result.autocompleteSupported = true;
        result.autocompleteRequiresNavigation = true;
        result.shrinkHeightToFitEntries = true;

        auto methods = findNearbyMessages(text);
        for (auto const& [objectName, messageName, description] : methods) {
            SuggestionEntry e;
            e.displayName = messageName;
            e.description = "(" + objectName + ") " + description;
            e.icon = SuggestionEntry::IconType::None;

            auto const nameOnly = messageName.upToFirstOccurrenceOf(" ", false, false);
            if (nameOnly != messageName)
                e.completionOverride = nameOnly;

            result.entries.add(std::move(e));

            if (result.entries.size() >= numRowsAllocated)
                break;
        }

        // Method order is determined by relevance category (object-name match,
        // method-name match, description match). The first row whose
        // completion target actually starts with the user's text — if any —
        // is the right autocomplete candidate.
        for (auto const& e : result.entries) {
            auto const& completionText = e.getCompletionText();
            if (completionText.startsWith(text)) {
                result.topAutocompleteText = completionText;
                break;
            }
        }

        return result;
    }

    // Mode 4: send/receive symbols
    SuggestionQueryResult querySendReceive(String const& text, bool isSend) const
    {
        SuggestionQueryResult result;
        result.autocompleteSupported = true;
        result.autocompleteRequiresNavigation = true;
        result.shrinkHeightToFitEntries = true;

        auto const prefix = text.upToFirstOccurrenceOf(" ", false, false);
        auto const searchSymbol = text.fromFirstOccurrenceOf(" ", false, false).upToFirstOccurrenceOf(" ", false, false);

        auto matches = const_cast<SuggestionComponent*>(this)
                           ->findSendReceive(currentObject->cnv->patch, searchSymbol, !isSend);

        for (auto const& sr : matches) {
            SuggestionEntry e;
            auto const symbol = isSend ? sr.receiveSymbol : sr.sendSymbol;
            e.displayName = prefix + " " + symbol;
            e.displayOverride = symbol;
            e.description = sr.name;
            e.icon = SuggestionEntry::IconType::None;
            result.entries.add(std::move(e));

            if (result.entries.size() >= numRowsAllocated)
                break;
        }

        for (auto const& e : result.entries) {
            auto const& completionText = e.getCompletionText();
            if (completionText.startsWith(text)) {
                result.topAutocompleteText = completionText;
                break;
            }
        }

        return result;
    }

    // Apply a query result to the UI
    void applyQueryResult(SuggestionQueryResult const& result)
    {
        currentResultAutocompleteRequiresNavigation = result.autocompleteRequiresNavigation;
        currentResultSupportsAutocomplete = result.autocompleteSupported
            && (!result.autocompleteRequiresNavigation || autocompleteActivatedByNavigation);
        currentResultSupportsDetail = result.detailLookupSupported;
        currentResultShouldShrinkHeightToFitEntries = result.shrinkHeightToFitEntries;

        // Argument mode: detail panel fills the popup
        if (result.detailPanelFillsPopup) {
            numOptions = 0;

            for (auto* row : rows) {
                row->clear();
                row->setToggleState(false, dontSendNotification);
            }

            detailPanel->setObject(result.detailLookupTarget,
                currentObject ? currentObject->cnv->pd->objectLibrary.get() : nullptr);

            if (autoCompleteComponent) {
                autoCompleteComponent->setEnabled(false);
                autoCompleteComponent->clear();
            }

            applyLayoutMode(LayoutMode::DetailOnly);

            // Only show the popup if we actually found something useful.
            updatePopupVisibility();
            currentSelection = -1;
            return;
        }

        numOptions = static_cast<int>(result.entries.size());

        for (int i = 0; i < rows.size(); i++) {
            if (i < numOptions) {
                rows[i]->setEntry(result.entries[i]);
                rows[i]->setToggleState(false, dontSendNotification);
            } else {
                rows[i]->clear();
                rows[i]->setToggleState(false, dontSendNotification);
            }
        }

        bool const editorHasText = openedEditor && openedEditor->getText().isNotEmpty();
        if (autoCompleteComponent) {
            autoCompleteComponent->setEnabled(currentResultSupportsAutocomplete && editorHasText);
            if (currentResultSupportsAutocomplete && editorHasText && result.topAutocompleteText.isNotEmpty()) {
                autoCompleteComponent->setSuggestion(result.topAutocompleteText);
            } else {
                autoCompleteComponent->clear();
            }
        }

        if (currentResultSupportsAutocomplete && editorHasText && result.topAutocompleteText.isNotEmpty() && numOptions > 0) {
            currentSelection = 0;
            rows[0]->setToggleState(true, dontSendNotification);
        } else {
            currentSelection = -1;
        }

        applyLayoutMode(currentResultSupportsDetail ? LayoutMode::ListWithDetail : LayoutMode::ListOnly);
        shrinkHeightToFitEntriesIfNeeded();
        updateDetailPanel();
        updatePopupVisibility();
    }

    void shrinkHeightToFitEntriesIfNeeded()
    {
        if (!currentResultShouldShrinkHeightToFitEntries || layoutMode == LayoutMode::DetailOnly || numOptions <= 0)
            return;

        auto const margins = getMargin() * 2;
        int const numVisibleRows = std::min(numOptions, numRowsAllocated);
        int const targetHeight = numVisibleRows * rowHeight + 12 + margins;
        if (targetHeight < getHeight())
            setSize(getWidth(), targetHeight);
    }

    void updatePopupVisibility()
    {
        bool const editorHasText = openedEditor && openedEditor->getText().isNotEmpty();
        bool const hasContent = layoutMode == LayoutMode::DetailOnly ? detailPanel->hasContent() : numOptions > 0;

        bool isInViewport = true;
        if (currentObject && currentObject->cnv->viewport) {
            auto* viewport = currentObject->cnv->viewport.get();
            isInViewport = viewport->getBounds().contains(
                viewport->getLocalArea(currentObject, currentObject->getLocalBounds()));
        }

        setVisible(editorHasText && hasContent && isInViewport);
    }

    // Layout switching
    void applyLayoutMode(LayoutMode requestedMode)
    {
        bool const wasSmallPanel = (layoutMode != LayoutMode::ListWithDetail);
        auto const margins = getMargin() * 2;

        if (requestedMode == LayoutMode::DetailOnly || requestedMode == LayoutMode::ListOnly) {
            layoutMode = requestedMode;
            setSize(340 + margins, 160 + margins);
        } else {
            int const targetWidth = wasSmallPanel ? savedListSize.x : getWidth();
            int const targetHeight = wasSmallPanel ? savedListSize.y : getHeight();

            layoutMode = decideListModeForWidth(targetWidth);
            if (wasSmallPanel)
                setSize(targetWidth, targetHeight);
        }

        port->setVisible(layoutMode != LayoutMode::DetailOnly);
        detailViewport->setVisible(layoutMode != LayoutMode::ListOnly);

        // Description visibility on rows depends on the mode.
        for (auto* row : rows)
            row->repaint();
        resized();
    }

    LayoutMode decideListModeForWidth(int width) const
    {
        if (!currentResultSupportsDetail)
            return LayoutMode::ListOnly;
        if (width < detailPanelMinWidth + getMargin() * 2)
            return LayoutMode::ListOnly;
        return LayoutMode::ListWithDetail;
    }

    //  Navigation
    void move(int offset, int setTo = -1)
    {
        if (!openedEditor || numOptions == 0)
            return;

        int const numRows = std::min(numRowsAllocated, numOptions);
        int const newSelection = (setTo == -1)
            ? (currentSelection + offset + numRows) % numRows
            : jlimit(0, numRows - 1, setTo);

        // If the editor's text exactly matches the currently selected entry, the
        // user has already accepted ("completed") this suggestion. In that case,
        // moving to another row should replace the entire editor text with the
        // new selection's completion rather than just updating the ghost text.
        bool const inCompletedState = currentSelection >= 0
            && currentSelection < rows.size()
            && rows[currentSelection]->hasEntry()
            && openedEditor->getText() == rows[currentSelection]->getEntry().getCompletionText();

        for (int i = 0; i < rows.size(); i++) {
            bool const shouldBeOn = (i == newSelection);
            if (rows[i]->getToggleState() != shouldBeOn)
                rows[i]->setToggleState(shouldBeOn, dontSendNotification);
        }

        currentSelection = newSelection;
        auto* row = rows[currentSelection];

        auto const& fullText = row->getEntry().getCompletionText();

        if (currentResultAutocompleteRequiresNavigation && fullText.isNotEmpty()) {
            autocompleteActivatedByNavigation = true;
            currentResultSupportsAutocomplete = true;
            lastQueriedText = fullText;
            openedEditor->setText(fullText, sendNotification);
            openedEditor->moveCaretToEnd();
            if (autoCompleteComponent) {
                autoCompleteComponent->setEnabled(true);
                autoCompleteComponent->clear();
            }
            currentObject->updateBounds();
        } else if (!currentResultAutocompleteRequiresNavigation && currentResultSupportsAutocomplete
            && autoCompleteComponent && fullText.isNotEmpty()) {
            auto const& baseText = autocompleteNavigationBaseText;
            if (baseText.isNotEmpty() && fullText.startsWith(baseText)) {
                if (openedEditor->getText() != baseText) {
                    lastQueriedText = baseText;
                    openedEditor->setText(baseText, sendNotification);
                    openedEditor->moveCaretToEnd();
                }

                autoCompleteComponent->setEnabled(true);
                autoCompleteComponent->setSuggestion(fullText);
            } else {
                lastQueriedText = fullText;
                openedEditor->setText(fullText, sendNotification);
                openedEditor->moveCaretToEnd();
                autoCompleteComponent->setEnabled(true);
                autoCompleteComponent->clear();
            }

            currentObject->updateBounds();
        } else if (inCompletedState && currentResultSupportsAutocomplete && fullText.isNotEmpty()) {
            lastQueriedText = fullText;
            openedEditor->setText(fullText, sendNotification);
            openedEditor->moveCaretToEnd();
            if (autoCompleteComponent) {
                autoCompleteComponent->setEnabled(true);
                autoCompleteComponent->clear();
            }
            currentObject->updateBounds();
        } else if (currentResultSupportsAutocomplete && autoCompleteComponent
            && fullText.startsWith(openedEditor->getText())) {
            autoCompleteComponent->setEnabled(true);
            autoCompleteComponent->setSuggestion(fullText);
            currentObject->updateBounds();
        } else if (autoCompleteComponent) {
            autoCompleteComponent->clear();
        }

        auto const viewTop = port->getViewPositionY();
        auto const viewBottom = viewTop + port->getMaximumVisibleHeight();

        auto const rowTop = row->getY();
        auto const rowBottom = rowTop + row->getHeight();
        constexpr int margin = 6;

        if (rowTop < viewTop)
            port->setViewPosition(0, rowTop - margin);
        else if (rowBottom > viewBottom)
            port->setViewPosition(0, rowBottom - port->getMaximumVisibleHeight() + margin);

        updateDetailPanel();
        repaint();
    }

    void onRowClicked(int idx)
    {
        if (!openedEditor)
            return;

        if (idx == currentSelection && autoCompleteComponent && autoCompleteComponent->hasGhostText()) {
            autoCompleteComponent->accept();
        } else {
            move(0, idx);
        }

        if (!openedEditor->isVisible())
            openedEditor->setVisible(true);
        openedEditor->grabKeyboardFocus();
    }

    void refreshDetailPanelContent()
    {
        if (!currentResultSupportsDetail || currentSelection < 0 || currentSelection >= rows.size()) {
            detailPanel->clear();
            return;
        }
        auto const& entry = rows[currentSelection]->getEntry();
        auto const lookupName = entry.detailLookupName.isNotEmpty() ? entry.detailLookupName : entry.displayName;
        if (currentObject)
            detailPanel->setObject(lookupName, currentObject->cnv->pd->objectLibrary.get());
    }

    void updateDetailPanel()
    {
        if (layoutMode != LayoutMode::ListWithDetail)
            return;

        refreshDetailPanelContent();

        if (detailViewport && detailViewport->isVisible())
            resized();
    }

    static SuggestionEntry::IconType iconForName(String const& name)
    {
        if (name.contains("~"))
            return SuggestionEntry::IconType::Signal;
        if (is_gem_object(name.toRawUTF8()))
            return SuggestionEntry::IconType::Gem;
        return SuggestionEntry::IconType::Data;
    }

    void filterObjectList(StringArray& list) const
    {
        if (!currentObject)
            return;
        auto& library = currentObject->cnv->pd->objectLibrary;

        if (!SettingsFile::getInstance()->getProperty<VarArray>("libraries").contains("Gem")) {
            StringArray filtered;
            for (auto& object : list) {
                if (object.startsWith("Gem/") || !library->isGemObject(object))
                    filtered.add(object);
            }
            list = filtered;
        }

        if (getValue<bool>(currentObject->hvccMode)) {
            StringArray filtered;
            for (auto& object : list) {
                if (HeavyCompatibleObjects::isCompatible(object) && object != "array")
                    filtered.add(object);
            }
            list = filtered;
        }

        for (int i = list.size() - 1; i >= 0; i--) {
            if (excludeList.contains(list[i]))
                list.remove(i);
        }
    }

    static StringArray sortByQuery(String const& query, StringArray suggestions)
    {
        if (query.isEmpty())
            return suggestions;

        struct Sorter {
            String query;
            int compareElements(String const& a, String const& b) const
            {
                if (a == query && b != query)
                    return -1;
                if (b == query && a != query)
                    return 1;

                if (a == query + "~" && b != query && b != query + "~")
                    return -1;
                if (b == query + "~" && a != query && a != query + "~")
                    return 1;

                if (a.startsWith(query + ".") && b != query && b != query + "~" && !b.startsWith(query + "."))
                    return -1;
                if (b.startsWith(query + ".") && a != query && a != query + "~" && !a.startsWith(query + "."))
                    return 1;

                if (a.startsWith(query) && !b.startsWith(query))
                    return -1;
                if (b.startsWith(query) && !a.startsWith(query))
                    return 1;

                return 0;
            }
        };

        Sorter sorter { query };
        suggestions.strings.sort(sorter, true);
        return suggestions;
    }

    int getMargin() const
    {
        return canBeTransparent() ? 8 : 0;
    }

    Rectangle<int> getWindowBounds() const
    {
        return getLocalBounds().reduced(getMargin());
    }

    static bool canBeTransparent()
    {
        return ProjectInfo::canUseSemiTransparentWindows();
    }

    void resized() override
    {
        auto const wholeBounds = getWindowBounds();

        // While in a list mode, the actual mode (ListOnly vs ListWithDetail)
        // is decided by the current width. This keeps the user-visible side
        // panel in sync with the resizer drag.
        if (layoutMode != LayoutMode::DetailOnly) {
            auto const refinedMode = decideListModeForWidth(getWidth());
            if (refinedMode != layoutMode) {
                layoutMode = refinedMode;
                port->setVisible(true);
                detailViewport->setVisible(layoutMode == LayoutMode::ListWithDetail);
                if (layoutMode == LayoutMode::ListWithDetail)
                    refreshDetailPanelContent();
            }
        }

        // Carve list / detail areas first.
        Rectangle<int> listBounds, detailBounds;
        switch (layoutMode) {
        case LayoutMode::ListOnly:
            listBounds = wholeBounds;
            break;
        case LayoutMode::ListWithDetail: {
            constexpr int gap = 1; // one-pixel separator drawn in paintOverChildren
            int const detailWidth = jlimit(200, 260, wholeBounds.getWidth() / 2);
            int const listWidth = wholeBounds.getWidth() - detailWidth - gap;
            listBounds = wholeBounds.withWidth(listWidth);
            detailBounds = wholeBounds.withTrimmedLeft(listWidth + gap);
            break;
        }
        case LayoutMode::DetailOnly:
            detailBounds = wholeBounds;
            break;
        }

        // Apply bounds
        if (layoutMode != LayoutMode::DetailOnly) {
            int const yScroll = port->getViewPositionY();
            port->setBounds(listBounds.reduced(2, 2));
            buttonHolder->setBounds(listBounds.getX() + 6, listBounds.getY(), listBounds.getWidth(), std::min(numOptions, numRowsAllocated) * rowHeight + 8);
            for (int i = 0; i < rows.size(); i++)
                rows[i]->setBounds(3, i * rowHeight + 4, listBounds.getWidth() - 6, rowHeight - 1);
            port->setViewPosition(0, yScroll);
        }

        if (layoutMode != LayoutMode::ListOnly) {
            detailViewport->setBounds(detailBounds);

            int innerWidth = jmax(40, detailViewport->getMaximumVisibleWidth());
            int contentHeight = jmax(1, detailPanel->getContentHeight(innerWidth));
            detailPanel->setSize(innerWidth, contentHeight);

            int const checkedWidth = jmax(40, detailViewport->getMaximumVisibleWidth());
            if (checkedWidth != innerWidth) {
                int const recomputedHeight = jmax(1, detailPanel->getContentHeight(checkedWidth));
                detailPanel->setSize(checkedWidth, recomputedHeight);
            }
        }

        resizer.setBounds(wholeBounds.getRight() - 14, wholeBounds.getBottom() - 14, 12, 12);

        repaint();
    }

    void paint(Graphics& g) override
    {
        if (!canBeTransparent()) {
            g.fillAll(PlugDataColours::canvasBackgroundColour);
        } else {
            StackShadow::drawShadowForRect(g, getLocalBounds().reduced(12), 12, Corners::defaultCornerRadius, 0.44f);
        }

        // Single rounded rectangle that covers list + header + detail panel.
        g.setColour(PlugDataColours::popupMenuBackgroundColour);
        g.fillRoundedRectangle(getWindowBounds().toFloat(), Corners::defaultCornerRadius);
    }

    void paintOverChildren(Graphics& g) override
    {
        auto const winBounds = getWindowBounds().toFloat();

        g.setColour(PlugDataColours::outlineColour.darker(0.1f));
        g.drawRoundedRectangle(winBounds, Corners::defaultCornerRadius, 1.0f);

        // Subtle separator between list and detail panel when both are visible.
        if (layoutMode == LayoutMode::ListWithDetail) {
            auto const sepX = static_cast<float>(detailViewport->getX()) - 0.5f;
            g.setColour(PlugDataColours::outlineColour.darker(0.1f).withAlpha(0.5f));
            g.drawLine(sepX, winBounds.getY() + 6.0f, sepX, winBounds.getBottom() - 6.0f, 1.0f);
        }
    }

    bool hitTest(int x, int y) override
    {
        return getWindowBounds().contains(x, y);
    }

    void mouseDown(MouseEvent const&) override
    {
        if (openedEditor)
            openedEditor->grabKeyboardFocus();
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.eventComponent == &resizer && layoutMode != LayoutMode::DetailOnly && currentResultSupportsDetail)
            saveCurrentSize();
    }

    void loadSavedSize()
    {
        auto const sizeProperty = SettingsFile::getInstance()->getProperty<VarArray>("suggestions_size");
        if (sizeProperty.size() == 2) {
            savedListSize = {
                static_cast<int>(sizeProperty[0]),
                static_cast<int>(sizeProperty[1])
            };
        } else {
            savedListSize = {
                560 + getMargin() * 2,
                240 + getMargin() * 2
            };
        }
    }

    void saveCurrentSize()
    {
        savedListSize = { getWidth(), getHeight() };
        SettingsFile::getInstance()->setProperty("suggestions_size",
            VarArray { var(getWidth()), var(getHeight()) });
    }

    void componentBeingDeleted(Component&) override
    {
        removeCalloutBox();
    }

    bool keyPressed(KeyPress const& key, Component* /*originatingComponent*/) override
    {
        if (!currentObject || !openedEditor)
            return false;

        bool const isAutocompleting = autoCompleteComponent && autoCompleteComponent->hasGhostText();
        bool const caretAtEnd = openedEditor->getCaretPosition() == openedEditor->getText().length();
        bool const noSelection = openedEditor->getHighlightedRegion().isEmpty();

        if (key == KeyPress::rightKey && noSelection && isAutocompleting && caretAtEnd) {
            acceptCurrentSuggestion();
            return true;
        }

        if (key == KeyPress::tabKey && isAutocompleting
            && openedEditor->getText() != autoCompleteComponent->getCompletedText()
            && numOptions != 0) {
            acceptCurrentSuggestion();
            return true;
        }

        if (key == KeyPress::returnKey) {
            if (isAutocompleting && openedEditor->getText() != autoCompleteComponent->getCompletedText() && numOptions != 0) {
                acceptCurrentSuggestion();
                return true;
            }
            return false;
        }

        // Up/down only when there is a list to navigate (not in DetailOnly).
        if (isVisible() && numOptions > 0 && (key == KeyPress::upKey || key == KeyPress::downKey)) {
            move(key == KeyPress::downKey ? 1 : -1);
            return true;
        }

        return false;
    }

    void acceptCurrentSuggestion()
    {
        if (autoCompleteComponent)
            autoCompleteComponent->accept();
        currentSelection = 0;
        if (!rows.isEmpty())
            rows[0]->setToggleState(true, dontSendNotification);
    }

    SmallArray<std::tuple<String, String, String>> findNearbyMessages(String const& toSearch) const
    {
        struct NearbyObject {
            Object* object = nullptr;
            String objectName;
            int distance = 0;
        };

        struct MessageCandidate {
            String objectName;
            String messageName;
            String description;
            int distance = 0;
            int relevance = 0;
            bool directAutocomplete = false;
        };

        SmallArray<NearbyObject> objects;
        auto* cnv = currentObject->cnv;

        auto isInsideViewport = [cnv](Object* object) {
            if (!cnv->viewport)
                return true;

            auto* viewport = cnv->viewport.get();
            return viewport->getBounds().intersects(
                viewport->getLocalArea(object, object->getLocalBounds()));
        };

        UnorderedSet<Object*> connectedObjects;
        for (auto* connection : currentObject->getConnections()) {
            if (connection && connection->outobj == currentObject && connection->inobj)
                connectedObjects.insert(connection->inobj.get());
        }

        for (auto* obj : cnv->objects) {
            int distance = currentObject->getPosition().getDistanceFrom(obj->getPosition());
            if (!obj->getPointer() || obj == currentObject || distance > 200 || !isInsideViewport(obj))
                continue;

            if (!connectedObjects.empty() && !connectedObjects.contains(obj))
                continue;

            auto objectName = obj->getType();
            auto alreadyExists = std::ranges::find_if(objects, [objectName](auto const& toCompare) {
                return toCompare.objectName == objectName;
            }) != objects.end();

            if (alreadyExists)
                continue;

            objects.add({ obj, objectName, distance });
        }

        objects.sort([](auto const& a, auto const& b) {
            return a.distance < b.distance;
        });

        SmallArray<MessageCandidate> candidates;

        auto addCandidate = [&candidates, &toSearch](String const& objectName,
                                pd::Library::ObjectReferenceTable::ReferenceItem const& item,
                                String const& descriptionPrefix, int const distance) {
            int relevance = -1;
            if (objectName.contains(toSearch))
                relevance = 0;
            else if (item.type.upToFirstOccurrenceOf(" ", false, false).contains(toSearch))
                relevance = 1;
            else if (item.description.contains(toSearch))
                relevance = 2;

            if (relevance < 0)
                return;

            auto const completionText = item.type.upToFirstOccurrenceOf(" ", false, false);
            bool const directAutocomplete = toSearch.isNotEmpty() && completionText.startsWith(toSearch);
            candidates.add({ objectName, item.type, descriptionPrefix + item.description, distance, relevance, directAutocomplete });
        };

        for (auto& object : objects) {
            auto const& info = cnv->pd->objectLibrary->getObjectInfo(object.objectName);

            for (auto const& method : info.methods)
                addCandidate(object.objectName, method, "", object.distance);

            for (int i = 0; i < info.inlets.size(); i++) {
                auto const descriptionPrefix = "inlet " + String(i + 1) + ": ";
                for (auto const& message : info.inlets[i].messages) {
                    if(message.type.contains("signal")) continue;

                    addCandidate(object.objectName, message, descriptionPrefix, object.distance);
                }
            }
        }

        candidates.sort([](auto const& a, auto const& b) {
            if (a.distance != b.distance)
                return a.distance < b.distance;
            if (a.directAutocomplete != b.directAutocomplete)
                return a.directAutocomplete;
            return a.relevance < b.relevance;
        });

        for (int i = candidates.size() - 1; i >= 0; i--) {
            for (int j = candidates.size() - 1; j >= 0; j--) {
                if (i != j
                    && candidates[i].objectName == candidates[j].objectName
                    && candidates[i].messageName == candidates[j].messageName
                    && candidates[i].description == candidates[j].description) {
                    candidates.remove_at(i);
                    break;
                }
            }
        }

        SmallArray<std::tuple<String, String, String>> nearbyMessages;
        for (auto const& candidate : candidates)
            nearbyMessages.add({ candidate.objectName, candidate.messageName, candidate.description });

        return nearbyMessages;
    }

    struct SendReceiveEntry {
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
        if (auto p = patch.getPointer()) {
            auto* realised = canvas_realizedollar(p.get(), currentObject->cnv->pd->generateSymbol("$0"));
            currentDollsym = String::fromUTF8(realised->s_name);
        }

        SmallArray<std::tuple<pd::WeakReference, String, String>> allObjects;
        for (auto& objectPtr : patch.getObjects()) {
            String type, name;
            if (auto object = objectPtr.get<t_pd>()) {
                if (!pd::Interface::checkObject(object.get()))
                    continue;
                name = pd::Interface::getObjectText(object.cast<t_text>());
                type = String::fromUTF8(pd::Interface::getObjectClassName(object.get()));
            }
            allObjects.emplace_back(objectPtr, name, type);
        }

        for (auto& [objectPtr, name, type] : allObjects) {
            if (total > 15)
                break;

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
                    auto srl_is_valid = [](t_symbol const* s) {
                        return s != nullptr && s != gensym("");
                    };
                    t_symbol* srlsym[3];
                    iemgui_all_sym2dollararg(iemgui.get(), srlsym);
                    if (srl_is_valid(srlsym[0]))
                        entry.sendSymbol = String::fromUTF8(iemgui->x_snd_unexpanded->s_name);
                    if (srl_is_valid(srlsym[1]))
                        entry.receiveSymbol = String::fromUTF8(iemgui->x_rcv_unexpanded->s_name);
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
                if (auto scopeObject = objectPtr.get<t_fake_scope>())
                    entry.receiveSymbol = String(scopeObject->x_receive->s_name);
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
                if (auto noteObject = objectPtr.get<t_fake_note>())
                    entry.receiveSymbol = String(noteObject->x_receive->s_name);
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
                case hash("throw~"):
                    entry.sendSymbol = getFirstArgumentFromFullName(name);
                    entry.name = nameWithoutArgs;
                    break;
                case hash("r"):
                case hash("r~"):
                case hash("receive"):
                case hash("receive~"):
                case hash("catch~"):
                    entry.receiveSymbol = getFirstArgumentFromFullName(name);
                    entry.name = nameWithoutArgs;
                    break;
                default:
                    break;
                }
                break;
            }
            }
            if (entry.name.isNotEmpty()) {
                entry.prefix = prefix;
                entry.dollSym = currentDollsym;
                result.insert(result.begin(), entry);
                total++;
            }
        }

        for (auto& [objectPtr, name, type] : allObjects) {
            if (total > 15)
                break;
            if (type == "canvas" || type == "graph") {
                auto subpatch = pd::Patch(objectPtr, currentObject->cnv->pd, false);
                if (depth < 3)
                    result.add_array(getAllSendReceives(subpatch, total, prefix + name + " -> ", ++depth));
            }
        }
        return result;
    }

    SmallArray<SendReceiveEntry> findSendReceive(pd::Patch& patch, String searchSymbol, bool wantSend)
    {
        if (sendReceiveDatabase.empty()) {
            int total = 0;
            sendReceiveDatabase = getAllSendReceives(patch, total);
        }

        String currentDollsym;
        if (auto p = patch.getPointer()) {
            auto* realised = canvas_realizedollar(p.get(), currentObject->cnv->pd->generateSymbol(searchSymbol));
            currentDollsym = String::fromUTF8(realised->s_name);
        }
        searchSymbol = searchSymbol.replace("$0", currentDollsym);

        SmallArray<SendReceiveEntry> filtered;
        for (auto entry : sendReceiveDatabase) {
            auto targetSymbol = wantSend ? entry.sendSymbol : entry.receiveSymbol;
            if (entry.name.isNotEmpty() && targetSymbol.isNotEmpty()) {
                String expandedSymbol = entry.dollSym != currentDollsym
                    ? targetSymbol.replace("$0", entry.dollSym)
                    : targetSymbol;
                if (expandedSymbol.contains(searchSymbol)) {
                    if (entry.prefix.isNotEmpty() && targetSymbol.contains("$")) {
                        if (wantSend)
                            entry.sendSymbol = expandedSymbol;
                        else
                            entry.receiveSymbol = expandedSymbol;
                    }
                    entry.name = entry.prefix + entry.name;
                    filtered.add(entry);
                }
            }
        }

        SmallArray<SendReceiveEntry> deduplicated;
        for (int i = filtered.size() - 1; i >= 0; i--) {
            auto symbol = wantSend ? filtered[i].sendSymbol : filtered[i].receiveSymbol;
            SendReceiveEntry* alreadySeen = nullptr;
            for (auto& s : deduplicated) {
                auto seenSymbol = wantSend ? s.sendSymbol : s.receiveSymbol;
                if (seenSymbol == symbol) {
                    alreadySeen = &s;
                    break;
                }
            }
            if (alreadySeen) {
                alreadySeen->name += ", " + filtered[i].name;
                filtered.remove_at(i);
            } else {
                deduplicated.add(filtered[i]);
            }
        }

        return deduplicated;
    }

    static constexpr int numRowsAllocated = 20;
    static constexpr int rowHeight = 30;
    static constexpr int detailPanelMinWidth = 410; // below this width, hide the detail side panel

    std::unique_ptr<AutoCompleteComponent> autoCompleteComponent;
    std::unique_ptr<BouncingViewport> port;
    std::unique_ptr<Component> buttonHolder;
    std::unique_ptr<ObjectDetailPanel> detailPanel;
    std::unique_ptr<BouncingViewport> detailViewport;
    OwnedArray<Row> rows;
    ResizerLookAndFeel resizerLookAndFeel;
    MouseRateReducedComponent<ResizableCornerComponent> resizer;
    ComponentBoundsConstrainer constrainer;

    LayoutMode layoutMode = LayoutMode::ListWithDetail;
    Point<int> savedListSize { 560, 240 }; // user-preferred list-mode size; persisted to SettingsFile

    int numOptions = 0;
    int currentSelection = -1;
    bool currentResultSupportsAutocomplete = false;
    bool currentResultAutocompleteRequiresNavigation = false;
    bool currentResultSupportsDetail = false;
    bool autocompleteActivatedByNavigation = false;
    bool currentResultShouldShrinkHeightToFitEntries = false;

    String lastQueriedText;
    String autocompleteNavigationBaseText;
    SafePointer<TextEditor> openedEditor = nullptr;
    SafePointer<Object> currentObject = nullptr;
    SmallArray<SendReceiveEntry> sendReceiveDatabase;

    StringArray excludeList = {
        "number~", // appears before numbox~ alphabetically, but is worse in every way
        "allpass_unit", "echo_unit", "multi.vsl.unit", "float2sig.unit",
        "imp.mc-unit", "multi.vsl.clone.ex", "score-ex1", "voice",
        "args-example", "dollsym-example", "fontsize-example",
        "oscbank.unit", "oscbank2.unit", "osc.mc-unit",
        "All_objects", "All_about_else", "All_about_cyclone",
        "README.deken", "about.MERDA", "else-meta",
        "resonbank.unit", "resonbank2.unit",
        "stepnoise.mc-unit", "rampnoise.mc-unit",
        "onebang_proxy",
        "grain.sampler.grain", "freeze.osc.clone~",
        "presets.send.clone", "grain.synth.grain", "presets-abs",
        "grain.live.grain", "synth.voice.template",
        "vocoder.band_clone", "f2s~-help",
        "rev1-final", "rev1-stage", "bpclone", "libpd_receive",
        "out.mc.hip~", "pvoc~", "gran~", "convpartition",
        "gatehold.unit", "gaterelease.unit",
        "else", "cyclone", "circle-gui"
    };
};
