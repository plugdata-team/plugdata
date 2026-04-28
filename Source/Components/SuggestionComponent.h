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
    bool clickable = true;
    String detailLookupName;
    String completionOverride;

    String const& getCompletionText() const
    {
        return completionOverride.isNotEmpty() ? completionOverride : displayName;
    }
};

struct SuggestionQueryResult {
    HeapArray<SuggestionEntry> entries;
    String topAutocompleteText;
    bool autocompleteSupported : 1 = false;
    bool detailLookupSupported : 1 = false;
    bool detailPanelFillsPopup : 1 = false;
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
            setInterceptsMouseClicks(entry.clickable, false);
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

            // Send/receive rows are stored as e.g. "send foo" so we can reuse
            // the displayName as completion text. The visible label drops the prefix.
            auto displayed = current.displayName;
            if (displayed.startsWith("s ") || displayed.startsWith("send ")
                || displayed.startsWith("r ") || displayed.startsWith("receive "))
                displayed = displayed.fromFirstOccurrenceOf(" ", false, false);

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

    class DetailPanel final : public Component {
    public:
        DetailPanel() = default;

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

        bool hasContent() const
        {
            return hasInfo
                && (info.description.isNotEmpty()
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

            int const x = 14;
            int const w = jmax(40, width - 28);
            int y = 12;

            auto const text = PlugDataColours::popupMenuTextColour;
            auto const muted = text.withAlpha(0.55f);

            // Origin / category label
            auto const origin = info.categories.empty() ? String("pure-data") : info.categories[0];
            if (g)
                Fonts::drawStyledText(*g, origin.toUpperCase(), x, y, w, 14, muted, Semibold, 11);
            y += 16;

            // Object name
            if (g)
                Fonts::drawStyledText(*g, currentName, x, y, w, 22, text, Semibold, 18);
            y += 24;

            // Description body, word-wrapped
            if (info.description.isNotEmpty()) {
                AttributedString s(info.description);
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
            layoutIoletSection(g, "INLET", info.inlets, y, x, w, text, muted);
            layoutIoletSection(g, "OUTLET", info.outlets, y, x, w, text, muted);

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
            attr.append(description, Fonts::getDefaultFont().withHeight(14.0f), text);

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

        static void layoutIoletSection(Graphics* g, StringRef title, HeapArray<pd::Library::ObjectReferenceTable::IoletReference> const& iolets, int& y, int x, int w, Colour text, Colour muted)
        {
            if (iolets.size() == 0)
                return;

            bool const numbered = iolets.size() > 1;

            for (size_t i = 0; i < iolets.size(); i++) {
                auto const& iolet = iolets[i];

                String const header = numbered ? String(title) + " " + String(i + 1)
                                               : String(title);
                if (g)
                    Fonts::drawStyledText(*g, header, x, y, w, 14, muted, Semibold, 13);
                y += 16;

                if (iolet.messages.size() > 0) {
                    for (auto const& msg : iolet.messages)
                        y += layoutDefinitionRow(g, msg.type, msg.description, y, x, w, text);
                } else if (iolet.tooltip.isNotEmpty()) {
                    AttributedString attr(iolet.tooltip);
                    attr.setColour(text);
                    attr.setFont(Fonts::getDefaultFont().withHeight(14.0f));
                    attr.setJustification(Justification::centredLeft);
                    attr.setLineSpacing(1.05f);

                    TextLayout layout;
                    layout.createLayout(attr, static_cast<float>(w));
                    int const layoutH = static_cast<int>(std::ceil(layout.getHeight()));
                    if (g)
                        layout.draw(*g, Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(layoutH)));
                    y += jmax(16, layoutH + 2);
                }

                if (i + 1 < iolets.size())
                    y += 8;
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

        detailPanel = std::make_unique<DetailPanel>();
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

        applyConstrainerForLayout();
        setSize(560 + getMargin() * 2, 240 + getMargin() * 2);

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
        currentResultSupportsDetail = false;
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

        if (cnv->viewport) {
            setVisible(cnv->viewport->getBounds().contains(
                cnv->viewport->getLocalArea(currentObject, currentObject->getLocalBounds())));
        }
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
        applyQueryResult(queryActive(currentText));
    }

    bool isShowingDetailPanel() const
    {
        return layoutMode == LayoutMode::ListWithDetail || layoutMode == LayoutMode::DetailOnly;
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
            e.clickable = true;
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

        auto methods = findNearbyMethods(text);
        for (auto const& [objectName, methodName, description] : methods) {
            SuggestionEntry e;
            e.displayName = methodName;
            e.description = "(" + objectName + ") " + description;
            e.icon = SuggestionEntry::IconType::None;
            e.clickable = false;

            auto const nameOnly = methodName.upToFirstOccurrenceOf(" ", false, false);
            if (nameOnly != methodName)
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
        // Same reasoning as queryMessageMethods: completing "send f" to
        // "send foo" is the whole point of this mode.
        result.autocompleteSupported = true;

        auto const prefix = text.upToFirstOccurrenceOf(" ", false, false);
        auto const searchSymbol = text.fromFirstOccurrenceOf(" ", false, false).upToFirstOccurrenceOf(" ", false, false);

        auto matches = const_cast<SuggestionComponent*>(this)
                           ->findSendReceive(currentObject->cnv->patch, searchSymbol, !isSend);

        for (auto const& sr : matches) {
            SuggestionEntry e;
            auto const symbol = isSend ? sr.receiveSymbol : sr.sendSymbol;
            e.displayName = prefix + " " + symbol;
            e.description = sr.name;
            e.icon = SuggestionEntry::IconType::None;
            e.clickable = false;
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
        currentResultSupportsAutocomplete = result.autocompleteSupported;
        currentResultSupportsDetail = result.detailLookupSupported;

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
            bool const editorHasText = openedEditor && openedEditor->getText().isNotEmpty();
            setVisible(editorHasText && detailPanel->hasContent());
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
        setVisible(editorHasText && numOptions > 0);

        if (autoCompleteComponent) {
            autoCompleteComponent->setEnabled(result.autocompleteSupported);
            if (result.autocompleteSupported && result.topAutocompleteText.isNotEmpty()) {
                autoCompleteComponent->setSuggestion(result.topAutocompleteText);
            } else {
                autoCompleteComponent->clear();
            }
        }

        if (result.autocompleteSupported && result.topAutocompleteText.isNotEmpty()) {
            currentSelection = 0;
            rows[0]->setToggleState(true, dontSendNotification);
        } else {
            currentSelection = -1;
        }

        applyLayoutMode(currentResultSupportsDetail ? LayoutMode::ListWithDetail : LayoutMode::ListOnly);
        updateDetailPanel();
    }

    // Layout switching
    void applyLayoutMode(LayoutMode newMode)
    {
        bool const modeChanged = (newMode != layoutMode);

        if (modeChanged) {
            layoutMode = newMode;
            applyConstrainerForLayout();

            // Pick a sensible size if the current one doesn't fit the new mode.
            auto const margins = getMargin() * 2;
            switch (newMode) {
            case LayoutMode::ListOnly:
                if (getWidth() > 360 + margins)
                    setSize(310 + margins, jmax(getHeight(), 150 + margins));
                break;
            case LayoutMode::ListWithDetail:
                if (getWidth() < 480 + margins)
                    setSize(560 + margins, jmax(getHeight(), 240 + margins));
                break;
            case LayoutMode::DetailOnly:
                setSize(340 + margins, jmax(getHeight(), 200 + margins));
                break;
            }
        }

        port->setVisible(newMode != LayoutMode::DetailOnly);
        detailViewport->setVisible(newMode != LayoutMode::ListOnly);

        // Description visibility on rows depends on the mode.
        for (auto* row : rows)
            row->repaint();
        resized();
    }

    void applyConstrainerForLayout()
    {
        switch (layoutMode) {
        case LayoutMode::ListOnly:
            constrainer.setSizeLimits(150, 130, 500, 400);
            break;
        case LayoutMode::ListWithDetail:
            constrainer.setSizeLimits(450, 220, 800, 500);
            break;
        case LayoutMode::DetailOnly:
            constrainer.setSizeLimits(260, 180, 600, 460);
            break;
        }
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

        if (inCompletedState && currentResultSupportsAutocomplete && fullText.isNotEmpty()) {
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

        if (port->getViewPositionY() > row->getY()) {
            port->setViewPosition(0, row->getY() - 6);
        } else if (port->getViewPositionY() + port->getMaximumVisibleHeight() < row->getY() + row->getHeight()) {
            port->setViewPosition(0, row->getY() - row->getHeight() * 4 + 6);
        }

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

    void updateDetailPanel()
    {
        if (layoutMode != LayoutMode::ListWithDetail)
            return;

        if (!currentResultSupportsDetail || currentSelection < 0 || currentSelection >= rows.size()) {
            detailPanel->clear();
        } else {
            auto const& entry = rows[currentSelection]->getEntry();
            auto const lookupName = entry.detailLookupName.isNotEmpty() ? entry.detailLookupName : entry.displayName;

            if (currentObject)
                detailPanel->setObject(lookupName, currentObject->cnv->pd->objectLibrary.get());
        }

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

        objects.sort([](auto const& a, auto const& b) {
            return std::get<2>(a) > std::get<2>(b);
        });

        SmallArray<std::tuple<String, String, String>> nearbyMethods;

        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                if (objectName.contains(toSearch)) {
                    nearbyMethods.add({ objectName, method.type, method.description });
                }
            }
        }

        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                if (method.type.contains(toSearch)) {
                    nearbyMethods.add({ objectName, method.type, method.description });
                }
            }
        }

        for (auto& [objectName, methods, distance] : objects) {
            for (auto method : methods) {
                if (method.description.contains(toSearch)) {
                    nearbyMethods.add({ objectName, method.type, method.description });
                }
            }
        }

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
    static constexpr int rowHeight = 30;        // a touch more breathing room than before

    std::unique_ptr<AutoCompleteComponent> autoCompleteComponent;
    std::unique_ptr<BouncingViewport> port;
    std::unique_ptr<Component> buttonHolder;
    std::unique_ptr<DetailPanel> detailPanel;
    std::unique_ptr<BouncingViewport> detailViewport;
    OwnedArray<Row> rows;
    ResizerLookAndFeel resizerLookAndFeel;
    ResizableCornerComponent resizer;
    ComponentBoundsConstrainer constrainer;

    LayoutMode layoutMode = LayoutMode::ListWithDetail;

    int numOptions = 0;
    int currentSelection = -1;
    bool currentResultSupportsAutocomplete = false;
    bool currentResultSupportsDetail = false;

    String lastQueriedText;
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
