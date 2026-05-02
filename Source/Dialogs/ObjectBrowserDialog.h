/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <utility>

#include "Components/BouncingViewport.h"
#include "Components/ObjectDragAndDrop.h"
#include "ObjectReferenceDialog.h"
#include "Canvas.h"
#include "Dialogs.h"

class CategoriesListBox final : public ListBox
    , public ListBoxModel {

    StringArray categories = { "All" };
    BouncingViewportAttachment bouncer;

public:
    CategoriesListBox()
        : bouncer(getViewport())
    {
        setOutlineThickness(0);
        setRowHeight(25);

        setModel(this);

        setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        setColour(ListBox::outlineColourId, Colours::transparentBlack);
    }

    int getNumRows() override
    {
        return categories.size();
    }

    void selectedRowsChanged(int const row) override
    {
        changeCallback(categories[row]);
    }

    void paintListBoxItem(int const rowNumber, Graphics& g, int const width, int const height, bool const rowIsSelected) override
    {
        if (categories[rowNumber] == "--------") {
            g.setColour(PlugDataColours::outlineColour);
            g.drawHorizontalLine(height / 2, 5, width - 10);
            return;
        }
        if (rowIsSelected) {
            g.setColour(PlugDataColours::panelActiveBackgroundColour);
            g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, Corners::defaultCornerRadius);
        }

        Fonts::drawText(g, categories[rowNumber], 12, 0, width - 9, height, PlugDataColours::panelTextColour, 15);
    }

    void initialise(StringArray newCategories)
    {
        categories = std::move(newCategories);
        updateContent();
        repaint();

        selectRow(0, true, true);
    }

    std::function<void(String const&)> changeCallback;
};

class ObjectsListBox final : public ListBox
    , public ListBoxModel {

    class ObjectListBoxItem final : public Component {
    public:
        ObjectListBoxItem(ListBox* parent, PluginEditor* editor, SmallString const& name, SmallString const& description, int rowNumber, bool const isSelected, std::function<void(bool shouldFade)> dismissDialog)
            : row(rowNumber)
            , objectName(name)
            , objectDescription(description)
            , rowIsSelected(isSelected)
            , objectsListBox(parent)
            , editor(editor)
            , dismissMenu(std::move(dismissDialog))
        {
        }

        void paint(Graphics& g) override
        {
            if (rowIsSelected || mouseHover) {
                auto colour = PlugDataColours::panelActiveBackgroundColour;
                if (mouseHover && !rowIsSelected)
                    colour = colour.withAlpha(0.5f);

                g.setColour(colour);
                g.fillRoundedRectangle(getLocalBounds().reduced(4, 2).toFloat(), Corners::defaultCornerRadius);
            }

            auto const colour = PlugDataColours::panelTextColour;

            auto textBounds = Rectangle<int>(0, 0, getWidth(), getHeight()).reduced(18, 6);

            Fonts::drawStyledText(g, objectName.toString(), textBounds.removeFromTop(textBounds.proportionOfHeight(0.5f)), colour, Bold, 14);

            Fonts::drawText(g, objectDescription.toString(), textBounds, colour, 14);
        }

        bool hitTest(int const x, int const y) override
        {
            auto const bounds = getLocalBounds().reduced(4, 2);
            return bounds.contains(x, y);
        }

        void mouseEnter(MouseEvent const& e) override
        {
            mouseHover = true;
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            mouseHover = false;
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            objectsListBox->selectRow(row, true, true);
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (e.mouseWasDraggedSinceMouseDown() && !e.source.isTouch())
                dismissMenu(false);
        }

        void mouseDrag(MouseEvent const& e) override
        {
            if (e.source.isTouch())
                return;

            if (e.getDistanceFromDragStart() > 5) {
                ObjectDragAndDrop::attachToMouse(editor, ObjectThemeManager::get()->getCompleteFormat(objectName.toString()));
                dismissMenu(true);
            }
        }

        void refresh(SmallString const& name, SmallString const& description, int const rowNumber, bool const isSelected)
        {
            objectName = name;
            objectDescription = description;
            row = rowNumber;
            rowIsSelected = isSelected;
            repaint();
        }

    private:
        int row;
        SmallString objectName;
        SmallString objectDescription;
        bool rowIsSelected = false;
        ListBox* objectsListBox;
        PluginEditor* editor;

        bool mouseHover = false;

        std::function<void(bool shouldFade)> dismissMenu;
    };

    BouncingViewportAttachment bouncer;
    std::function<void(bool shouldFade)> dismiss;

public:
    explicit ObjectsListBox(PluginEditor* editor, std::function<void(bool shouldFade)> const& dismissMenu)
        : bouncer(getViewport())
        , dismiss(dismissMenu)
        , editor(editor)
    {
        setOutlineThickness(0);
        setRowHeight(45);

        setModel(this);

        setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        setColour(ListBox::outlineColourId, Colours::transparentBlack);
    }

    int getNumRows() override
    {
        return objects.size();
    }

    void selectedRowsChanged(int const row) override
    {
        if (row < 0 || row >= objects.size())
            return;

        changeCallback(objects[row].first.toString());
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
    }

    Component* refreshComponentForRow(int const rowNumber, bool const isRowSelected, Component* existingComponentToUpdate) override
    {
        if (rowNumber < 0 || rowNumber >= objects.size()) {
            if (existingComponentToUpdate)
                delete existingComponentToUpdate;
            return nullptr;
        }

        if (existingComponentToUpdate == nullptr) {
            auto const& [name, description] = objects[rowNumber];
            return new ObjectListBoxItem(this, editor, name, description, rowNumber, isRowSelected, dismiss);
        }
        auto* itemComponent = dynamic_cast<ObjectListBoxItem*>(existingComponentToUpdate);
        if (itemComponent != nullptr) {
            auto const& [name, description] = objects[rowNumber];
            itemComponent->refresh(name, description, rowNumber, isRowSelected);
        }
        return itemComponent;
    }

    static void removeAliasedDuplicates(StringArray& objectsToShow)
    {
        StringArray gemObjects;
        for (auto& object : objectsToShow) {
            if (object.startsWith("Gem")) {
                gemObjects.add(object.fromLastOccurrenceOf("/", false, false));
            }
        }
        for (auto& gemObject : gemObjects) {
            objectsToShow.removeString(gemObject);
        }
    }

    void showObjects(StringArray objectsToShow)
    {
        auto& library = *editor->pd->objectLibrary;

        objects.clear();
        removeAliasedDuplicates(objectsToShow);
        for (auto const& object : objectsToShow) {
            auto const& info = library.getObjectInfo(object);
            if (info.title.isNotEmpty()) {
                objects.add({ info.title, info.description });
            }
        }

        updateContent();
        repaint();

        selectRow(0, true, true);
    }

    PluginEditor* editor;
    HeapArray<std::pair<SmallString, SmallString>> objects;
    std::function<void(String const&)> changeCallback;
};

class ObjectViewerDragArea final : public Component {
public:
    ObjectViewerDragArea(PluginEditor* editor, std::function<void(bool shouldFade)> const& dismissMenu)
        : editor(editor)
        , dismissMenu(dismissMenu)
    {
        setBufferedToImage(true);
    }

    ~ObjectViewerDragArea() override { }

    void setObjectName(String const& name) { objectName = name; }

    void mouseDrag(MouseEvent const& e) override
    {
        ObjectDragAndDrop::attachToMouse(editor, ObjectThemeManager::get()->getCompleteFormat(objectName));
        dismissMenu(true);
    }

    bool hitTest(int const x, int const y) override
    {
        return getLocalBounds().contains(x, y);
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHovering = true;
        repaint();
    }
    void mouseExit(MouseEvent const& e) override
    {
        isHovering = false;
        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown())
            dismissMenu(false);
    }

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);

        if (isHovering) {
            g.setColour(PlugDataColours::panelActiveBackgroundColour);
        } else {
            g.setColour(PlugDataColours::panelBackgroundColour.brighter(0.025f));
        }
        g.fillRoundedRectangle(bounds, 6.0f);

        g.setColour(PlugDataColours::outlineColour);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        // Dot grid
        g.setColour(PlugDataColours::panelTextColour.withAlpha(isHovering ? 0.07f : 0.10f));
        constexpr int spacing = 16;
        for (int y = spacing; y < getHeight() - 4; y += spacing) {
            for (int x = spacing; x < getWidth() - 4; x += spacing) {
                g.fillEllipse(static_cast<float>(x) - 1.0f,
                    static_cast<float>(y) - 1.0f, 2.0f, 2.0f);
            }
        }
    }

private:
    PluginEditor* editor;
    std::function<void(bool shouldFade)> dismissMenu;
    bool isHovering = false;
    String objectName;
};

class ObjectViewer final : public Component {
public:
    ObjectViewer(PluginEditor* editor, ObjectReferenceDialog& objectReference, std::function<void(bool shouldFade)> dismissMenu)
        : objectDragArea(editor, std::move(dismissMenu))
        , library(*editor->pd->objectLibrary)
        , reference(objectReference)
    {
        setInterceptsMouseClicks(false, true);

        addChildComponent(openHelp);
        addChildComponent(openReference);
        addChildComponent(objectDragArea);

        openReference.onClick = [this] {
            reference.showObject(objectName);
        };

        openHelp.onClick = [this, editor, dismissMenu] {
            auto const file = pd::Library::findHelpfile(objectName);
            editor->getTabComponent().openHelpPatch(URL(file));
            dismissMenu(false);
        };

        SmallArray<TextButton*> buttons = { &openHelp, &openReference };
        for (auto* button : buttons) {
            button->setColour(TextButton::buttonColourId, PlugDataColours::panelBackgroundColour);
            button->setColour(TextButton::buttonOnColourId, PlugDataColours::panelActiveBackgroundColour);
        }

        setBufferedToImage(true);
    }

    void resized() override
    {
        // Buttons stay anchored to the bottom (existing layout)
        auto buttonBounds = getLocalBounds().removeFromBottom(60).reduced(20, 0);
        openReference.setBounds(buttonBounds.removeFromTop(25));
        buttonBounds.removeFromTop(5);
        openHelp.setBounds(buttonBounds.removeFromTop(25));

        // Drag area = the viz card. Its position depends on how tall the pill
        // row turned out, so we compute that here too.
        auto layout = computeLayout();
        objectDragArea.setBounds(layout.vizBounds);
    }

    void paintOverChildren(Graphics& g) override
    {
        // Left separator (preserved from the original)
        g.setColour(PlugDataColours::outlineColour);
        g.drawLine(5, 0, 5, getHeight());

        if (objectName.isEmpty())
            return;

        auto const layout = computeLayout();
        auto const colour = PlugDataColours::panelTextColour;
        auto const labelColour = colour.withAlpha(0.55f);

        for (auto const& pill : layout.pills) {
            if (pill.bg.getAlpha() > 0) {
                g.setColour(pill.bg);
                g.fillRoundedRectangle(pill.bounds.toFloat(), 4.0f);
            }
            g.setColour(pill.stroke);
            g.drawRoundedRectangle(pill.bounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
            Fonts::drawStyledText(g, pill.text, pill.bounds, pill.fg,
                FontStyle::Semibold, 10.5f, Justification::centred);
        }

        {
            g.setFont(Fonts::getSemiBoldFont().withHeight(18.f));
            g.setColour(colour);
            g.drawText(objectName, layout.titleBounds.translated(2, 0), Justification::centredLeft);
        }

        if (!unknownInletLayout && !unknownOutletLayout) {
            drawObject(g, layout.vizBounds);
        } else {
            auto qBounds = layout.vizBounds.withSizeKeepingCentre(40, 40);
            g.setColour(PlugDataColours::outlineColour);
            g.drawRoundedRectangle(qBounds.toFloat(), 6.0f, 2.0f);
            Fonts::drawText(g, "?", qBounds,
                colour.withAlpha(0.8f), 28, Justification::centred);
        }
        {
            auto headerBounds = Rectangle<int>(layout.contentX,
                layout.infoHeaderY,
                layout.contentW, 16);
            Fonts::drawStyledText(g, "INFO", headerBounds, labelColour, FontStyle::Semibold, 10.0f, Justification::centredLeft);
            g.setColour(PlugDataColours::outlineColour);
            g.drawHorizontalLine(layout.infoHeaderY + 16,
                static_cast<float>(layout.contentX),
                static_cast<float>(layout.contentX + layout.contentW));
        }

        StringArray const labels = { "Type", "Inlets", "Outlets" };
        StringArray const values = {
            objectName.contains("~") ? String("Signal") : String("Data"),
            unknownInletLayout ? String("Unknown") : String(static_cast<int>(inlets.size())),
            unknownOutletLayout ? String("Unknown") : String(static_cast<int>(outlets.size()))
        };

        int rowY = layout.infoRowsY;
        constexpr int rowH = 20;
        constexpr int labelW = 64;
        for (int i = 0; i < labels.size(); i++) {
            Rectangle<int> row(layout.contentX, rowY, layout.contentW, rowH);
            Fonts::drawText(g, labels[i], row.removeFromLeft(labelW), labelColour, 13, Justification::centredLeft);
            Fonts::drawText(g, values[i], row, colour, 13, Justification::centredLeft);
            rowY += rowH;
        }

        {
            auto headerBounds = Rectangle<int>(layout.contentX,
                layout.descHeaderY,
                layout.contentW, 16);
            Fonts::drawStyledText(g, "DESCRIPTION", headerBounds, labelColour, FontStyle::Semibold, 10.0f, Justification::centredLeft);
            g.setColour(PlugDataColours::outlineColour);
            g.drawHorizontalLine(layout.descHeaderY + 16, static_cast<float>(layout.contentX), static_cast<float>(layout.contentX + layout.contentW));
        }

        auto descBounds = Rectangle<int>(layout.contentX, layout.descBodyY, layout.contentW, layout.descBodyH);
        Fonts::drawFittedText(g, description, descBounds, colour.withAlpha(0.85f), 8, 0.9f, 13.5f, Justification::topLeft);
    }

    void drawObject(Graphics& g, Rectangle<int> const objectRect)
    {
        constexpr int ioletSize = 9;
        int const ioletWidth = (ioletSize + 4) * std::max<int>(static_cast<int>(inlets.size()), static_cast<int>(outlets.size()));
        int const textW = Fonts::getStringWidthInt(objectName, 15);
        int const objW = std::max(ioletWidth, textW) + 14;
        int const objH = 22;

        auto objRect = objectRect.withSizeKeepingCentre(objW, objH).toFloat();

        // Object box
        g.setColour(PlugDataColours::textObjectBackgroundColour);
        g.fillRoundedRectangle(objRect, Corners::objectCornerRadius);
        g.setColour(PlugDataColours::objectOutlineColour);
        g.drawRoundedRectangle(objRect, Corners::objectCornerRadius, 1.0f);

        Fonts::drawText(g, objectName, objRect.toNearestInt(), PlugDataColours::canvasTextColour, 15, Justification::centred);

        // Iolets
        auto themeTree = SettingsFile::getInstance()->getCurrentTheme();
        bool squareIolets = static_cast<bool>(themeTree->getProperty("square_iolets"));

        auto drawIolets = [&](SmallArray<bool> const& ports, bool isInletRow) {
            int const total = static_cast<int>(ports.size());
            if (total == 0)
                return;

            float const y = isInletRow ? (objRect.getY() - ioletSize * 0.5f)
                                       : (objRect.getBottom() - ioletSize * 0.5f);
            auto ioletStrip = objRect.reduced(8.0f, 0.0f);

            for (int i = 0; i < total; i++) {
                float x;
                if (total == 1) {
                    x = objW < 40 ? ioletStrip.getCentreX() - ioletSize * 0.5f
                                  : ioletStrip.getX();
                } else {
                    float ratio = (ioletStrip.getWidth() - ioletSize) / static_cast<float>(total - 1);
                    x = ioletStrip.getX() + ratio * i;
                }
                Rectangle<float> bb(x, y, static_cast<float>(ioletSize), static_cast<float>(ioletSize));
                Colour fill = ports[i] ? PlugDataColours::signalColour : PlugDataColours::dataColour;

                g.setColour(fill);
                if (squareIolets) {
                    g.fillRect(bb);
                    g.setColour(PlugDataColours::objectOutlineColour);
                    g.drawRect(bb, 1.0f);
                } else {
                    g.fillEllipse(bb);
                    g.setColour(PlugDataColours::objectOutlineColour);
                    g.drawEllipse(bb, 1.0f);
                }
            }
        };

        g.saveState();
        g.reduceClipRegion(objRect.getSmallestIntegerContainer());
        drawIolets(inlets, true);
        drawIolets(outlets, false);
        g.restoreState();
    }

    void showObject(String const& name)
    {
        auto const& objectInfo = library.getObjectInfo(name);
        bool const valid = name.isNotEmpty() && objectInfo.title.isNotEmpty();

        openHelp.setEnabled(pd::Library::findHelpfile(name).existsAsFile());
        openHelp.setVisible(valid);
        openReference.setVisible(valid);
        objectDragArea.setVisible(valid);

        inlets.clear();
        outlets.clear();
        categoryList.clear();

        if (!valid) {
            objectName = "";
            unknownInletLayout = false;
            unknownOutletLayout = false;
            repaint();
            return;
        }

        unknownInletLayout = false;
        unknownOutletLayout = false;

        for (auto& inlet : objectInfo.inlets) {
            if (inlet.repeating)
                unknownInletLayout = true;
            inlets.add(inlet.tooltip.upToFirstOccurrenceOf(":", false, false).contains("signal"));
        }
        for (auto& outlet : objectInfo.outlets) {
            if (outlet.repeating)
                unknownOutletLayout = true;
            outlets.add(outlet.tooltip.upToFirstOccurrenceOf(":", false, false).contains("signal"));
        }

        objectName = name;
        objectDragArea.setObjectName(name);

        for (auto const& category : objectInfo.categories)
            if (category.isNotEmpty())
                categoryList.add(category);

        origin = objectInfo.origin.isNotEmpty() ? objectInfo.origin : String("Unknown");
        description = objectInfo.description.isNotEmpty()
            ? objectInfo.description
            : String("No description available");

        resized(); // pill-row height may have changed → drag-area position
        repaint();
    }

private:
    static Colour getOriginColour(String const& origin)
    {
        if (origin == "ELSE")
            return Colour::fromRGB(245, 124, 38);
        if (origin == "cyclone")
            return Colour::fromRGB(12, 110, 232);
        if (origin == "vanilla")
            return Colour::fromRGB(36, 143, 95);
        if (origin == "Gem")
            return Colour::fromRGB(140, 51, 204);
        if (origin == "heavylib")
            return Colour::fromRGB(217, 38, 105);
        if (origin == "pdlua")
            return Colour::fromRGB(34, 130, 195);
        if (origin == "plugdata")
            return Colour::fromRGB(184, 145, 20);
        return PlugDataColours::panelTextColour.withAlpha(0.6f);
    }

    struct ComputedLayout {
        struct Pill {
            Rectangle<int> bounds;
            String text;
            Colour fg, bg, stroke;
        };
        SmallArray<Pill> pills;

        int contentX = 0, contentW = 0;
        Rectangle<int> titleBounds;
        Rectangle<int> vizBounds;
        int infoHeaderY = 0;
        int infoRowsY = 0;
        int descHeaderY = 0;
        int descBodyY = 0;
        int descBodyH = 0;
    };

    ComputedLayout computeLayout() const
    {
        constexpr int leftLineGap = 5; // matches the vertical separator
        constexpr int padX = 14;
        constexpr int padR = 12;
        constexpr int padTop = 12;
        constexpr int titleH = 26;
        constexpr int vizH = 84;
        constexpr int sectionGap = 14;
        constexpr int sectionHeaderH = 22;
        constexpr int infoRowH = 20;
        constexpr int buttonStripH = 60;

        ComputedLayout L;
        L.contentX = leftLineGap + padX;
        L.contentW = jmax(80, getWidth() - leftLineGap - padX - padR);

        // ----- Pill row (origin + categories), wrapping as needed -----
        constexpr int pillH = 20, pillHGap = 5, pillVGap = 5;
        int x = L.contentX;
        int y = padTop;
        int rowBottom = y + pillH;

        auto addPill = [&](String const& txt, Colour fg, Colour bg, Colour stroke) {
            auto font = Fonts::getSemiBoldFont().withHeight(10.5f);
            int textW = Fonts::getStringWidthInt(txt, font);
            int w = textW + 14;
            if (x > L.contentX && x + w > L.contentX + L.contentW) {
                x = L.contentX;
                y += pillH + pillVGap;
                rowBottom = y + pillH;
            }
            L.pills.add({ Rectangle<int>(x, y, w, pillH), txt, fg, bg, stroke });
            x += w + pillHGap;
        };

        if (origin.isNotEmpty()) {
            auto c = getOriginColour(origin);
            addPill(origin.toUpperCase(), c, c.withAlpha(0.10f), c.withAlpha(0.30f));
        }
        for (auto const& cat : categoryList) {
            addPill(cat.toUpperCase(),
                PlugDataColours::panelTextColour.withAlpha(0.65f),
                Colour(0, 0, 0).withAlpha(0.0f),
                PlugDataColours::outlineColour);
        }

        int cursor = (origin.isEmpty() && categoryList.isEmpty()) ? padTop : (rowBottom + 8);

        // ----- Title -----
        L.titleBounds = Rectangle<int>(L.contentX, cursor, L.contentW, titleH);
        cursor += titleH + 6;

        // ----- Viz card -----
        L.vizBounds = Rectangle<int>(L.contentX, cursor, L.contentW, vizH);
        cursor += vizH + sectionGap;

        // ----- Info section -----
        L.infoHeaderY = cursor;
        cursor += sectionHeaderH;
        L.infoRowsY = cursor;
        cursor += infoRowH * 3 + 8;

        // ----- Description section -----
        L.descHeaderY = cursor;
        cursor += sectionHeaderH;
        L.descBodyY = cursor;
        L.descBodyH = jmax(40, getHeight() - buttonStripH - 8 - L.descBodyY);

        return L;
    }

public:
    bool unknownInletLayout = false;
    bool unknownOutletLayout = false;

    String objectName;
    SmallArray<bool> inlets;
    SmallArray<bool> outlets;

    String origin;
    StringArray categoryList;
    String description;

    TextButton openHelp = TextButton("Show Help");
    TextButton openReference = TextButton("Show Reference");

    ObjectViewerDragArea objectDragArea;

    pd::Library& library;
    ObjectReferenceDialog& reference;
};

class ObjectSearchComponent final : public Component
    , public ListBoxModel
    , public ScrollBar::Listener
    , public KeyListener {

public:
    explicit ObjectSearchComponent(pd::Library& library)
        : library(library)
        , bouncer(listBox.getViewport())
    {
        listBox.setModel(this);
        listBox.setRowHeight(28);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();

        listBox.getViewport()->setScrollBarsShown(true, false, false, false);

        input.setTextToShowWhenEmpty("Type to search for objects", findColour(TextEditor::textColourId).withAlpha(0.5f));
        input.addKeyListener(this);
        input.onTextChange = [this] {
            updateResults(input.getText());
        };

        addAndMakeVisible(listBox);
        addAndMakeVisible(input);

        listBox.addMouseListener(this, true);

        input.setJustification(Justification::centredLeft);
        input.setBorder({ 0, 3, 5, 1 });
        input.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);

        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

        listBox.getViewport()->getVerticalScrollBar().addListener(this);

        setInterceptsMouseClicks(false, true);

        for (auto& object : library.getAllObjects()) {
            auto const& objectInfo = library.getObjectInfo(object);
            objectDescriptions[object] = objectInfo.description;
        }
    }

    // Divert up/down key events from text editor to the listbox
    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        if (key.isKeyCode(KeyPress::upKey) || key.isKeyCode(KeyPress::downKey)) {
            listBox.keyPressed(key);
            return true;
        }

        return false;
    }

    void selectedRowsChanged(int const row) override
    {
        if (isPositiveAndBelow(row, searchResult.size())) {
            changeCallback(searchResult[row]);
        }
    }

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().withTrimmedTop(42).removeFromLeft(getWidth() - 260).toFloat(), Corners::windowCornerRadius);
    }

    void startSearching()
    {
        setVisible(true);
        input.grabKeyboardFocus();
    }

    void stopSearching()
    {
        input.clear();
        clearSearchResults();
        setVisible(false);
    }

    void paintListBoxItem(int const rowNumber, Graphics& g, int const w, int const h, bool const rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(PlugDataColours::panelActiveBackgroundColour);
            g.fillRoundedRectangle(4, 2, w - 8, h - 4, Corners::defaultCornerRadius);
        }

        g.setColour(findColour(ComboBox::textColourId));
        String const item = searchResult[rowNumber];

        auto const colour = PlugDataColours::popupMenuTextColour;
        auto const yIndent = jmin<float>(4, h * 0.3f);
        auto leftIndent = 34;
        constexpr auto rightIndent = 11;
        auto const textWidth = w - leftIndent - rightIndent;

        if (textWidth > 0)
            Fonts::drawStyledText(g, item, leftIndent, yIndent, textWidth, h - yIndent * 2, colour, Semibold, 12, Justification::left);

        auto const objectDescription = objectDescriptions[item];

        if (objectDescription.isNotEmpty()) {
            auto const font = Fonts::getDefaultFont().withHeight(12.f);
            auto const textLength = Fonts::getStringWidth(item, font);

            g.setColour(colour);

            leftIndent += textLength;
            auto const textWidth = getWidth() - leftIndent - rightIndent;

            g.setFont(font);

            // Draw seperator (which is an en dash)
            g.drawText(String::fromUTF8("  \xe2\x80\x93  ") + objectDescription, Rectangle<int>(leftIndent, yIndent, textWidth, h - yIndent * 2), Justification::left);
        }

        auto const dataColour = PlugDataColours::dataColour;
        auto const signalColour = PlugDataColours::signalColour;
        auto const type = item.endsWith("~");
        g.setColour(type ? signalColour : dataColour);

        auto iconbound = g.getClipBounds().reduced(6);
        iconbound.setWidth(iconbound.getHeight());
        iconbound.translate(6, 0);
        g.fillRoundedRectangle(iconbound.toFloat(), Corners::defaultCornerRadius);

        Fonts::drawFittedText(g, type ? "~" : "pd", iconbound.reduced(1), Colours::white, 1, 1.0f, type ? 12 : 10, Justification::centred);
    }

    int getNumRows() override
    {
        return searchResult.size();
    }

    Component* refreshComponentForRow(int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;
        return nullptr;
    }

    void clearSearchResults()
    {
        searchResult.clear();
        listBox.updateContent();
    }

    void updateResults(String const& query)
    {
        clearSearchResults();

        if (query.isEmpty())
            return;

        searchResult = library.searchObjectDocumentation(query);

        listBox.updateContent();
        listBox.repaint();

        if (listBox.getSelectedRow() == -1)
            listBox.selectRow(0, true, true);

        selectedRowsChanged(listBox.getSelectedRow());
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto const inputBounds = tableBounds.removeFromTop(40).reduced(42, 5);

        tableBounds.removeFromTop(4);

        input.setBounds(inputBounds);

        listBox.setBounds(tableBounds.removeFromLeft(getWidth() - 260));
    }

    std::function<void(String const&)> changeCallback;

private:
    pd::Library& library;
    ListBox listBox;
    BouncingViewportAttachment bouncer;

    StringArray searchResult;
    SearchEditor input;

    UnorderedMap<String, String> objectDescriptions;
};

class ObjectBrowserDialog final : public Component {

public:
    ObjectBrowserDialog(Component* pluginEditor)
        : editor(dynamic_cast<PluginEditor*>(pluginEditor))
        , objectsList(editor, [this](bool const shouldFade) { dismiss(shouldFade); })
        , objectReference(editor, true)
        , objectViewer(editor, objectReference, [this](bool const shouldFade) { dismiss(shouldFade); })
        , objectSearch(*editor->pd->objectLibrary)
    {
        auto& library = *editor->pd->objectLibrary;

        for (auto& object : library.getAllObjects()) {
            auto const& info = library.getObjectInfo(object);
            for (auto const& category : info.categories) {
                if (category == "MERDA")
                    objectsByCategory["ELSE"].add(object);
                else
                    objectsByCategory[category].add(object);
            }
            objectsByCategory[info.origin].add(object);
        }

        searchButton.setClickingTogglesState(true);
        searchButton.onClick = [this] {
            if (searchButton.getToggleState()) {
                objectSearch.startSearching();
            } else {
                objectSearch.stopSearching();
            }
        };

        categoriesList.changeCallback = [this](String const& category) {
            objectsList.showObjects(objectsByCategory[category]);
        };

        objectsList.changeCallback = [this](String const& object) {
            objectViewer.showObject(object);
        };

        objectSearch.changeCallback = [this](String const& object) {
            objectViewer.showObject(object);
        };

        objectsList.showObjects(library.getAllObjects());

        addAndMakeVisible(categoriesList);
        addAndMakeVisible(objectsList);
        addAndMakeVisible(objectViewer);
        addChildComponent(objectSearch);
        addAndMakeVisible(searchButton);
        addChildComponent(objectReference);

        objectsByCategory["All"] = StringArray();

        StringArray categories;
        for (auto& [category, objects] : objectsByCategory) {
            // Sort alphabetically
            objects.sort(true);

            // Add objects from every category to "All"
            if (category != "All") {
                objectsByCategory["All"].addArray(objects);
            }
            if (!pd::Library::objectOrigins.contains(category))
                categories.add(category);
        }

        // Also include undocumented objects
        objectsByCategory["All"].addArray(library.getAllObjects());
        objectsByCategory["All"].removeDuplicates(true);

        // First sort alphabetically
        objectsByCategory["All"].sort(true);
        categories.sort(true);

        // Make sure "All" is the first category
        categories.move(categories.indexOf("All"), 0);

        categories.insert(1, "--------");
        categories.insert(2, "--------");
        for (int i = pd::Library::objectOrigins.size() - 1; i >= 0; i--) {
            categories.insert(2, pd::Library::objectOrigins[i]);
        }

        /*
#if JUCE_DEBUG
        int numEmpty = 0;
        for (auto& object : objectsByCategory["All"]) {
            auto objectDescription = library.getObjectInfo(object).getProperty("description").toString();
            if (objectDescription.isEmpty()) {
                std::cout << object << std::endl;
                numEmpty++;
            }
        }

        std::cout << "Num Left:" << numEmpty << std::endl;
        float percentage = 1.0f - (numEmpty / static_cast<float>(objectsByCategory["All"].size()));
        std::cout << "Percentage done:" << percentage << std::endl;
#endif */

        categoriesList.initialise(categories);
        updater.addAnimator(fadeAnimator);
        fadeAnimator.complete();
    }

    void dismiss(bool const shouldFade)
    {
        if (shouldFade && fadeAnimator.isComplete()) {
            fadeAnimator.start();
        } else {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this) {
                    _this->editor->openedDialog.reset(nullptr);
                }
            });
        }
    }

    void resized() override
    {
        objectSearch.setBounds(getLocalBounds());
        searchButton.setBounds(2, 1, 38, 38);

        auto b = getLocalBounds().withTrimmedTop(42).reduced(1);
        objectViewer.setBounds(b.removeFromRight(260));

        categoriesList.setBounds(b.removeFromLeft(170));
        objectsList.setBounds(b);

        objectReference.setBounds(getLocalBounds());
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        g.setColour(PlugDataColours::panelBackgroundColour);
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto const titlebarBounds = getLocalBounds().removeFromTop(40);

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(PlugDataColours::toolbarBackgroundColour);
        g.fillPath(p);

        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawHorizontalLine(40, 0.0f, getWidth());

        Fonts::drawStyledText(g, "Object Browser", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), PlugDataColours::panelTextColour, Semibold, 15, Justification::centred);
    }

private:
    PluginEditor* editor;
    CategoriesListBox categoriesList;
    ObjectsListBox objectsList;

    ObjectReferenceDialog objectReference;
    ObjectViewer objectViewer;
    ObjectSearchComponent objectSearch;

    MainToolbarButton searchButton = MainToolbarButton(Icons::Search);

    VBlankAnimatorUpdater updater { this };
    Animator fadeAnimator = ValueAnimatorBuilder { }
                                .withDurationMs(280)
                                .withEasing(Easings::createEaseInOut())
                                .withValueChangedCallback([this](float v) {
                                    getParentComponent()->setAlpha(1.0f - v);
                                })
                                .build();

    UnorderedMap<String, StringArray> objectsByCategory;
};
