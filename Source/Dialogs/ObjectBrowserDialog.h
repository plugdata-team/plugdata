/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <utility>

#include "Components/BouncingViewport.h"
#include "Components/ObjectDragAndDrop.h"
#include "ObjectReferenceDialog.h"
#include "Canvas.h"
#include "Dialogs.h"

class CategoriesListBox : public ListBox
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

    void selectedRowsChanged(int row) override
    {
        changeCallback(categories[row]);
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (categories[rowNumber] == "--------") {
            g.setColour(findColour(PlugDataColour::outlineColourId));
            g.drawHorizontalLine(height / 2, 5, width - 10);
            return;
        }
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, Corners::defaultCornerRadius);
        }

        Fonts::drawText(g, categories[rowNumber], 12, 0, width - 9, height, findColour(PlugDataColour::panelTextColourId), 15);
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

class ObjectsListBox : public ListBox
    , public ListBoxModel {

    class ObjectListBoxItem : public ObjectDragAndDrop {
    public:
        ObjectListBoxItem(ListBox* parent, PluginEditor* editor, String const& name, String const& description, bool isSelected, std::function<void(bool shouldFade)> dismissDialog)
            : ObjectDragAndDrop(editor)
            , objectName(name)
            , objectDescription(description)
            , rowIsSelected(isSelected)
            , objectsListBox(parent)
            , dismissMenu(std::move(dismissDialog))
        {
        }

        void paint(juce::Graphics& g) override
        {
            if (rowIsSelected || mouseHover) {
                auto colour = findColour(PlugDataColour::panelActiveBackgroundColourId);
                if (mouseHover && !rowIsSelected)
                    colour = colour.withAlpha(0.5f);

                g.setColour(colour);
                g.fillRoundedRectangle(getLocalBounds().reduced(4, 2).toFloat(), Corners::defaultCornerRadius);
            }

            auto colour = findColour(PlugDataColour::panelTextColourId);

            auto textBounds = Rectangle<int>(0, 0, getWidth(), getHeight()).reduced(18, 6);

            Fonts::drawStyledText(g, objectName, textBounds.removeFromTop(textBounds.proportionOfHeight(0.5f)), colour, Bold, 14);

            Fonts::drawText(g, objectDescription, textBounds, colour, 14);
        }

        bool hitTest(int x, int y) override
        {
            auto bounds = getLocalBounds().reduced(4, 2);
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

            ObjectDragAndDrop::mouseDrag(e);
        }

        void dismiss(bool withAnimation) override
        {
            dismissMenu(withAnimation);
        }

        String getObjectString() override
        {
            return ObjectThemeManager::get()->getCompleteFormat(objectName);
        }

        String getPatchStringName() override
        {
            return objectName + String(" object");
        }

        void refresh(String name, String description, int rowNumber, bool isSelected)
        {
            objectName = name;
            objectDescription = description;
            row = rowNumber;
            rowIsSelected = isSelected;
            repaint();
        }

    private:
        int row = 0;
        String objectName;
        String objectDescription;
        bool rowIsSelected = false;
        ListBox* objectsListBox;

        bool mouseHover = false;

        std::function<void(bool shouldFade)> dismissMenu;
    };

    BouncingViewportAttachment bouncer;
    std::function<void(bool shouldFade)> dismiss;

public:
    explicit ObjectsListBox(PluginEditor* editor, pd::Library& library, std::function<void(bool shouldFade)> dismissMenu)
        : bouncer(getViewport())
        , dismiss(dismissMenu)
        , editor(editor)
    {
        setOutlineThickness(0);
        setRowHeight(45);

        setModel(this);

        setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        setColour(ListBox::outlineColourId, Colours::transparentBlack);

        for (auto const& object : library.getAllObjects()) {
            auto info = library.getObjectInfo(object);
            if (info.isValid() && info.hasProperty("name") && info.hasProperty("description")) {
                descriptions[info.getProperty("name").toString()] = info.getProperty("description").toString();
            }
        }
    }

    int getNumRows() override
    {
        return objects.size();
    }

    void selectedRowsChanged(int row) override
    {
        changeCallback(objects[row]);
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
    }

    Component* refreshComponentForRow(int rowNumber, bool isRowSelected, Component* existingComponentToUpdate) override
    {
        if (existingComponentToUpdate == nullptr) {
            auto name = objects[rowNumber];
            auto description = descriptions[name.fromLastOccurrenceOf("/", false, false)];
            return new ObjectListBoxItem(this, editor, name, description, isRowSelected, dismiss);
        } else {
            auto* itemComponent = dynamic_cast<ObjectListBoxItem*>(existingComponentToUpdate);
            if (itemComponent != nullptr) {
                auto name = objects[rowNumber];
                auto description = descriptions[name.fromLastOccurrenceOf("/", false, false)];
                itemComponent->refresh(name, description, rowNumber, isRowSelected);
            }
            return itemComponent;
        }
    }

    void removeAliasedDuplicates(StringArray& objectsToShow)
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
        removeAliasedDuplicates(objectsToShow);
        objects = std::move(objectsToShow);
        updateContent();
        repaint();

        selectRow(0, true, true);
    }

    PluginEditor* editor;
    std::unordered_map<String, String> descriptions;
    StringArray objects;
    std::function<void(String const&)> changeCallback;
};

class ObjectViewerDragArea : public ObjectDragAndDrop {
public:
    ObjectViewerDragArea(PluginEditor* editor, std::function<void(bool shouldFade)> dismissMenu)
        : ObjectDragAndDrop(editor)
        , dismissMenu(dismissMenu)
    {
        setBufferedToImage(true);
    }

    ~ObjectViewerDragArea() override { }

    void setObjectName(String name)
    {
        objectName = name;
    }

    String getObjectString() override
    {
        return ObjectThemeManager::get()->getCompleteFormat(objectName);
    }

    String getPatchStringName() override
    {
        return objectName + String(" object");
    }

    void dismiss(bool shouldFade) override
    {
        dismissMenu(shouldFade);
    }

    bool hitTest(int x, int y) override
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
        if (isHovering) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::defaultCornerRadius);
        }
    }

private:
    std::function<void(bool shouldFade)> dismissMenu;
    bool isHovering = false;
    String objectName;
};

class ObjectViewer : public Component {

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

        openReference.onClick = [this]() {
            reference.showObject(objectName);
        };

        openHelp.onClick = []() {
            // TODO: implement this!
        };

        openHelp.setVisible(false);

        Array<TextButton*> buttons = { &openHelp, &openReference };

        for (auto* button : buttons) {
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::panelBackgroundColourId));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::panelActiveBackgroundColourId));
        }

        // We only need to respond to explicit repaints anyway!
        setBufferedToImage(true);
    }

    void resized() override
    {
        auto buttonBounds = getLocalBounds().removeFromBottom(60).reduced(30, 0);
        openReference.setBounds(buttonBounds.removeFromTop(25));
        buttonBounds.removeFromTop(5);
        openHelp.setBounds(buttonBounds.removeFromTop(25));

        objectDragArea.setBounds(getLocalBounds().withTrimmedTop(48).withTrimmedBottom(120).withTrimmedLeft(12).withTrimmedRight(6));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(5, 0, 5, getHeight());

        if (objectName.isEmpty())
            return;

        auto infoBounds = getLocalBounds().withTrimmedBottom(100).reduced(20);
        auto objectDisplayBounds = infoBounds.removeFromTop(100).reduced(60);

        auto colour = findColour(PlugDataColour::panelTextColourId);
        Fonts::drawStyledText(g, objectName, getLocalBounds().removeFromTop(24).translated(0, 4), colour, Bold, 16.0f, Justification::centred);

        auto numInlets = unknownInletLayout ? "Unknown" : String(inlets.size());
        auto numOutlets = unknownOutletLayout ? "Unknown" : String(outlets.size());

        StringArray infoNames = { "Categories:", "Origin:", "Type:", "Num. Inlets:", "Num. Outlets:" };
        StringArray infoText = { categories, origin, objectName.contains("~") ? String("Signal") : String("Data"), numInlets, numOutlets };

        for (int i = 0; i < infoNames.size(); i++) {
            auto localBounds = infoBounds.removeFromTop(25);
            Fonts::drawText(g, infoNames[i], localBounds.removeFromLeft(90), colour, 15, Justification::topLeft);
            Fonts::drawText(g, infoText[i], localBounds, colour, 15, Justification::topLeft);
        }

        auto descriptionBounds = infoBounds.removeFromTop(25);
        Fonts::drawText(g, "Description: ", descriptionBounds.removeFromLeft(90), colour, 15, Justification::topLeft);

        Fonts::drawFittedText(g, description, descriptionBounds.withHeight(180), colour, 10, 0.9f, 15, Justification::topLeft);

        if (!unknownInletLayout && !unknownOutletLayout) {
            drawObject(g, objectDisplayBounds);
        } else {
            auto questionMarkBounds = objectDisplayBounds.withSizeKeepingCentre(48, 48);
            g.drawRoundedRectangle(questionMarkBounds.toFloat(), 6.0f, 3.0f);
            Fonts::drawText(g, "?", questionMarkBounds, colour, 40, Justification::centred);
        }
    }

    void drawObject(Graphics& g, Rectangle<int> objectRect)
    {
        int const ioletSize = 8;
        int const ioletWidth = (ioletSize + 4) * std::max(inlets.size(), outlets.size());
        int const textWidth = Fonts::getCurrentFont().getStringWidth(objectName);
        int const width = std::max(ioletWidth, textWidth) + 14;

        auto outlineBounds = objectRect.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRoundedRectangle(outlineBounds, Corners::objectCornerRadius, 1.0f);

        auto squareIolets = PlugDataLook::getUseSquareIolets();

        auto drawIolet = [this, squareIolets](Graphics& g, Rectangle<float> bounds, bool type) mutable {
            g.setColour(type ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));

            if (squareIolets) {
                g.fillRect(bounds);

                g.setColour(findColour(PlugDataColour::objectOutlineColourId));
                g.drawRect(bounds, 1.0f);
            } else {

                g.fillEllipse(bounds);

                g.setColour(findColour(PlugDataColour::objectOutlineColourId));
                g.drawEllipse(bounds, 1.0f);
            }
        };

        auto textBounds = outlineBounds.reduced(2.0f);
        Fonts::drawText(g, objectName, textBounds.toNearestInt(), findColour(PlugDataColour::panelTextColourId), 15, Justification::centred);

        auto ioletBounds = outlineBounds.reduced(8, 0);

        for (int i = 0; i < inlets.size(); i++) {
            auto inletBounds = Rectangle<int>();

            int const total = inlets.size();
            float const yPosition = ioletBounds.getY() + 1 - ioletSize / 2.0f;

            if (total == 1 && i == 0) {
                int xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();

                inletBounds = Rectangle<int>(xPosition, yPosition, ioletSize, ioletSize);
            } else if (total > 1) {
                float const ratio = (ioletBounds.getWidth() - ioletSize) / static_cast<float>(total - 1);

                inletBounds = Rectangle<int>(ioletBounds.getX() + ratio * i, yPosition, ioletSize, ioletSize);
            }

            drawIolet(g, inletBounds.toFloat(), inlets[i]);
        }

        for (int i = 0; i < outlets.size(); i++) {

            auto outletBounds = Rectangle<int>();
            int const total = outlets.size();
            float const yPosition = ioletBounds.getBottom() - ioletSize / 2.0f;

            if (total == 1 && i == 0) {
                int xPosition = getWidth() < 40 ? ioletBounds.getCentreX() - ioletSize / 2.0f : ioletBounds.getX();

                outletBounds = Rectangle<int>(xPosition, yPosition, ioletSize, ioletSize);

            } else if (total > 1) {
                float const ratio = (ioletBounds.getWidth() - ioletSize) / static_cast<float>(total - 1);
                outletBounds = Rectangle<int>(ioletBounds.getX() + ratio * i, yPosition, ioletSize, ioletSize);
            }

            drawIolet(g, outletBounds.toFloat(), outlets[i]);
        }
    }

    void showObject(String const& name)
    {
        auto objectInfo = library.getObjectInfo(name);
        bool valid = name.isNotEmpty() && objectInfo.isValid();

        // openHelp.setVisible(valid);
        openReference.setVisible(valid);
        objectDragArea.setVisible(valid);

        inlets.clear();
        outlets.clear();

        if (!valid) {
            objectName = "";
            unknownInletLayout = false;
            unknownOutletLayout = false;
            repaint();
            return;
        }

        bool hasUnknownInletLayout = false;
        bool hasUnknownOutletLayout = false;

        auto ioletDescriptions = objectInfo.getChildWithName("iolets");
        for (auto iolet : ioletDescriptions) {
            auto variable = iolet.getProperty("variable").toString() == "1";

            if (iolet.getType() == Identifier("inlet")) {
                if (variable)
                    hasUnknownInletLayout = true;
                inlets.push_back(iolet.getProperty("tooltip").toString().contains("(signal)"));
            } else {
                if (variable)
                    hasUnknownOutletLayout = true;
                outlets.push_back(iolet.getProperty("tooltip").toString().contains("(signal)"));
            }
        }

        unknownInletLayout = hasUnknownInletLayout;
        unknownOutletLayout = hasUnknownOutletLayout;

        objectName = name;
        objectDragArea.setObjectName(name);
        categories = "";
        origin = "";

        auto categoriesTree = objectInfo.getChildWithName("categories");

        for (auto category : categoriesTree) {
            auto cat = category.getProperty("name").toString();
            if (pd::Library::objectOrigins.contains(cat)) {
                origin = cat;
            } else {
                categories += cat + ", ";
            }
        }

        if (categories.isEmpty()) {
            categories = "Unknown";
        } else {
            categories = categories.dropLastCharacters(2);
        }

        if (origin.isEmpty()) {
            origin = "Unknown";
        }

        description = objectInfo.getProperty("description").toString();

        if (description.isEmpty()) {
            description = "No description available";
        }

        repaint();
    }

    bool unknownInletLayout = false;
    bool unknownOutletLayout = false;

    String objectName;
    std::vector<bool> inlets;
    std::vector<bool> outlets;

    String origin;
    String categories;
    String description;

    TextButton openHelp = TextButton("Show Help");
    TextButton openReference = TextButton("Show Reference");

    ObjectViewerDragArea objectDragArea;

    pd::Library& library;
    ObjectReferenceDialog& reference;
};

class ObjectSearchComponent : public Component
    , public ListBoxModel
    , public ScrollBar::Listener
    , public KeyListener {

public:
    explicit ObjectSearchComponent(pd::Library& library)
        : library(library), bouncer(listBox.getViewport())
    {
        listBox.setModel(this);
        listBox.setRowHeight(28);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();

        listBox.getViewport()->setScrollBarsShown(true, false, false, false);

        input.setTextToShowWhenEmpty("Type to search for objects", findColour(TextEditor::textColourId).withAlpha(0.5f));
        input.addKeyListener(this);
        input.onTextChange = [this]() {
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
            auto objectInfo = library.getObjectInfo(object);
            if (objectInfo.isValid()) {
                objectDescriptions[object] = objectInfo.getProperty("description").toString();
            } else {
                objectDescriptions[object] = "";
            }
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

    void selectedRowsChanged(int row) override
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
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
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

    void paintListBoxItem(int rowNumber, Graphics& g, int w, int h, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle(4, 2, w - 8, h - 4, Corners::defaultCornerRadius);
        }

        g.setColour(findColour(ComboBox::textColourId));
        String const item = searchResult[rowNumber];

        auto colour = findColour(PlugDataColour::popupMenuTextColourId);
        auto yIndent = jmin<float>(4, h * 0.3f);
        auto leftIndent = 34;
        auto rightIndent = 11;
        auto textWidth = w - leftIndent - rightIndent;

        if (textWidth > 0)
            Fonts::drawStyledText(g, item, leftIndent, yIndent, textWidth, h - yIndent * 2, colour, Semibold, 12, Justification::left);

        auto objectDescription = objectDescriptions[item];

        if (objectDescription.isNotEmpty()) {
            auto font = Font(12);
            auto textLength = font.getStringWidth(item);

            g.setColour(colour);

            leftIndent += textLength;
            auto textWidth = getWidth() - leftIndent - rightIndent;

            g.setFont(font);

            // Draw seperator (which is an en dash)
            g.drawText(String::fromUTF8("  \xe2\x80\x93  ") + objectDescription, Rectangle<int>(leftIndent, yIndent, textWidth, h - yIndent * 2), Justification::left);
        }

        auto dataColour = findColour(PlugDataColour::dataColourId);
        auto signalColour = findColour(PlugDataColour::signalColourId);
        auto type = item.endsWith("~");
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
        auto inputBounds = tableBounds.removeFromTop(40).reduced(42, 5);

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

    std::unordered_map<String, String> objectDescriptions;
};

class ObjectBrowserDialog : public Component {

public:
    ObjectBrowserDialog(Component* pluginEditor, Dialog* parent)
        : editor(dynamic_cast<PluginEditor*>(pluginEditor))
        , objectsList(editor, *editor->pd->objectLibrary, [this](bool shouldFade) { dismiss(shouldFade); })
        , objectReference(editor, true)
        , objectViewer(editor, objectReference, [this](bool shouldFade) { dismiss(shouldFade); })
        , objectSearch(*editor->pd->objectLibrary)
    {
        auto& library = *editor->pd->objectLibrary;

        for (auto& object : library.getAllObjects()) {
            auto info = library.getObjectInfo(object);
            if (!info.isValid())
                continue;

            auto categoriesTree = info.getChildWithName("categories");

            for (auto category : categoriesTree) {
                auto cat = category.getProperty("name").toString();
                objectsByCategory[cat].add(object);
            }
        }

        searchButton.setClickingTogglesState(true);
        searchButton.onClick = [this]() {
            if (searchButton.getToggleState()) {
                objectSearch.startSearching();
            } else {
                objectSearch.stopSearching();
            }
        };

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
        auto objectDescriptions = library.getObjectDescriptions();

        int numEmpty = 0;
        for (auto& object : objectsByCategory["All"]) {
            if (objectDescriptions[object].isEmpty()) {
                std::cout << object << std::endl;
                numEmpty++;
            }
        }

        std::cout << "Num Left:" << numEmpty << std::endl;
        float percentage = 1.0f - (numEmpty / static_cast<float>(objectsByCategory["All"].size()));
        std::cout << "Percentage done:" << percentage << std::endl;
#endif */

        categoriesList.changeCallback = [this](String const& category) {
            objectsList.showObjects(objectsByCategory[category]);
        };

        objectsList.changeCallback = [this](String const& object) {
            objectViewer.showObject(object);
        };

        objectSearch.changeCallback = [this](String const& object) {
            objectViewer.showObject(object);
        };

        categoriesList.initialise(categories);
    }

    void dismiss(bool shouldFade)
    {
        if (shouldFade)
            animator.animateComponent(getParentComponent(), getParentComponent()->getBounds(), 0.0f, 300, false, 0.0f, 0.0f);
        else {
            MessageManager::callAsync([_this = SafePointer(this)]() {
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
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius);

        auto titlebarBounds = getLocalBounds().removeFromTop(40);

        Path p;
        p.addRoundedRectangle(titlebarBounds.getX(), titlebarBounds.getY(), titlebarBounds.getWidth(), titlebarBounds.getHeight(), Corners::windowCornerRadius, Corners::windowCornerRadius, true, true, false, false);

        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillPath(p);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawHorizontalLine(40, 0.0f, getWidth());

        Fonts::drawStyledText(g, "Object Browser", Rectangle<float>(0.0f, 4.0f, getWidth(), 32.0f), findColour(PlugDataColour::panelTextColourId), Semibold, 15, Justification::centred);
    }

private:
    PluginEditor* editor;
    CategoriesListBox categoriesList;
    ObjectsListBox objectsList;

    ObjectReferenceDialog objectReference;
    ObjectViewer objectViewer;
    ObjectSearchComponent objectSearch;

    MainToolbarButton searchButton = MainToolbarButton(Icons::Search);

    ComponentAnimator animator;

    std::unordered_map<String, StringArray> objectsByCategory;
};
