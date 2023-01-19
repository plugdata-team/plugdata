/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "ObjectReferenceDialog.h"
#include "../Canvas.h"

struct CategoriesListBox : public ListBox
    , public ListBoxModel {

    StringArray categories = { "All", "Audio", "More" };

    CategoriesListBox()
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
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, Constants::defaultCornerRadius);
        }

        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId));

        g.setFont(Font(15));

        g.drawText(categories[rowNumber], 12, 0, width - 9, height, Justification::centredLeft, true);
    }

    void initialise(StringArray newCategories)
    {
        categories = newCategories;
        updateContent();
        repaint();

        selectRow(0, true, true);
    }

    std::function<void(String const&)> changeCallback;
};

struct ObjectsListBox : public ListBox
    , public ListBoxModel {

    ObjectsListBox(pd::Library& library)
    {
        setOutlineThickness(0);
        setRowHeight(45);

        setModel(this);

        setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        setColour(ListBox::outlineColourId, Colours::transparentBlack);

        descriptions = library.getObjectDescriptions();
    }

    int getNumRows() override
    {
        return objects.size();
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto objectName = objects[rowNumber];
        auto objectDescription = descriptions[objectName];

        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if (!lnf)
            return;

        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, Constants::defaultCornerRadius);
        }

        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId));

        g.setFont(lnf->boldFont.withHeight(14));

        auto textBounds = Rectangle<int>(0, 0, width, height).reduced(18, 6);

        g.drawText(objectName, textBounds.removeFromTop(textBounds.proportionOfHeight(0.5f)), Justification::centredLeft, true);

        g.setFont(lnf->defaultFont.withHeight(14));

        g.drawText(objectDescription, textBounds, Justification::centredLeft, true);
    }

    void selectedRowsChanged(int row) override
    {
        changeCallback(objects[row]);
    }

    void showObjects(StringArray objectsToShow)
    {
        objects = objectsToShow;
        updateContent();
        repaint();
        
        selectRow(0, true, true);
    }

    std::unordered_map<String, String> descriptions;
    StringArray objects;
    std::function<void(String const&)> changeCallback;
};

struct ObjectViewer : public Component {
    ObjectViewer(PluginEditor* editor, ObjectReferenceDialog& objectReference)
        : reference(objectReference)
        , library(editor->pd->objectLibrary)
    {
        addChildComponent(openHelp);
        addChildComponent(openReference);
        addChildComponent(createObject);

        createObject.onClick = [this, editor]() {
            MessageManager::callAsync([_this = SafePointer(this), editor, cnv = SafePointer(editor->getCurrentCanvas())]() {
                if (!cnv || !_this)
                    return;

                auto lastPosition = cnv->viewport->getViewArea().getConstrainedPoint(cnv->lastMousePosition - Point<int>(Object::margin, Object::margin));

                cnv->attachNextObjectToMouse = true;
                cnv->objects.add(new Object(cnv, _this->objectName, lastPosition));

                // Closes this dialog
                editor->openedDialog.reset(nullptr);
            });
        };

        openReference.onClick = [this]() {
            reference.showObject(objectName);
        };

        openHelp.onClick = [this, editor]() {

        };

        openHelp.setVisible(false);

        Array<TextButton*> buttons = { &openHelp, &openReference, &createObject };

        for (auto* button : buttons) {
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::panelBackgroundColourId));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::panelActiveBackgroundColourId));
        }

        // We only need to respond to explicit repaints anyway!
        setBufferedToImage(true);
    }

    void resized() override
    {
        auto buttonBounds = getLocalBounds().removeFromBottom(60).reduced(30, 0).translated(0, -30);
        createObject.setBounds(buttonBounds.removeFromTop(25));
        buttonBounds.removeFromTop(5);
        openReference.setBounds(buttonBounds.removeFromTop(25));
        buttonBounds.removeFromTop(5);
        openHelp.setBounds(buttonBounds.removeFromTop(25));
    }

    void paint(Graphics& g) override
    {
        auto const font = Font(15.0f);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(5, 0, 5, getHeight());

        if (objectName.isEmpty())
            return;

        auto infoBounds = getLocalBounds().withTrimmedBottom(100).reduced(20);
        auto objectDisplayBounds = infoBounds.removeFromTop(100).reduced(60);

        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if (!lnf)
            return;

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(16.0f));
        g.drawText(objectName, getLocalBounds().removeFromTop(35).translated(0, 4), Justification::centred);

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);

        auto numInlets = unknownInletLayout ? "Unknown" : String(inlets.size());
        auto numOutlets = unknownOutletLayout ? "Unknown" : String(outlets.size());

        StringArray infoNames = { "Category:", "Type:", "Num. Inlets:", "Num. Outlets:" };
        StringArray infoText = { category, objectName.contains("~") ? String("Signal") : String("Data"), numInlets, numOutlets };

        for (int i = 0; i < infoNames.size(); i++) {
            auto localBounds = infoBounds.removeFromTop(25);
            g.drawText(infoNames[i], localBounds.removeFromLeft(90), Justification::topLeft);
            g.drawText(infoText[i], localBounds, Justification::topLeft);
        }

        auto descriptionBounds = infoBounds.removeFromTop(25);
        g.drawText("Description: ", descriptionBounds.removeFromLeft(90), Justification::topLeft);

        g.drawFittedText(description, descriptionBounds.withHeight(180), Justification::topLeft, 5, 1.0f);

        if (!unknownInletLayout && !unknownOutletLayout) {
            drawObject(g, objectDisplayBounds);
        } else {
            auto questionMarkBounds = objectDisplayBounds.withSizeKeepingCentre(48, 48);
            g.drawRoundedRectangle(questionMarkBounds.toFloat(), 6.0f, 3.0f);
            g.setFont(Font(40));
            g.drawText("?", questionMarkBounds, Justification::centred);
        }
    }

    void drawObject(Graphics& g, Rectangle<int> objectRect)
    {
        auto const font = Font(15.0f);

        int const ioletSize = 8;
        int const ioletWidth = (ioletSize + 4) * std::max(inlets.size(), outlets.size());
        int const textWidth = font.getStringWidth(objectName);
        int const width = std::max(ioletWidth, textWidth) + 14;

        auto outlineBounds = objectRect.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRoundedRectangle(outlineBounds, Constants::objectCornerRadius, 1.0f);

        auto textBounds = outlineBounds.reduced(2.0f);
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);
        g.drawText(objectName, textBounds, Justification::centred);

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
            g.setColour(inlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.fillEllipse(inletBounds.toFloat());

            g.setColour(findColour(PlugDataColour::objectOutlineColourId));
            g.drawEllipse(inletBounds.toFloat(), 1.0f);
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

            g.setColour(outlets[i] ? findColour(PlugDataColour::signalColourId) : findColour(PlugDataColour::dataColourId));
            g.fillEllipse(outletBounds.toFloat());

            g.setColour(findColour(PlugDataColour::objectOutlineColourId));
            g.drawEllipse(outletBounds.toFloat(), 1.0f);
        }
    }

    void showObject(String name)
    {
        bool valid = name.isNotEmpty();
        createObject.setVisible(valid);
        // openHelp.setVisible(valid);
        openReference.setVisible(valid);

        if (!valid) {
            objectName = "";
            unknownInletLayout = false;
            unknownOutletLayout = false;
            inlets.clear();
            outlets.clear();
            repaint();
            return;
        }

        auto inletDescriptions = library.getInletDescriptions()[name];
        auto outletDescriptions = library.getOutletDescriptions()[name];

        inlets.resize(inletDescriptions.size());
        outlets.resize(outletDescriptions.size());

        bool hasUnknownInletLayout = false;

        for (int i = 0; i < inlets.size(); i++) {
            inlets[i] = inletDescriptions[i].first.contains("(signal)");
            if (inletDescriptions[i].second)
                hasUnknownInletLayout = true;
        }

        bool hasUnknownOutletLayout = false;
        for (int i = 0; i < outlets.size(); i++) {
            outlets[i] = outletDescriptions[i].first.contains("(signal)");
            if (outletDescriptions[i].second)
                hasUnknownOutletLayout = true;
        }

        unknownInletLayout = hasUnknownInletLayout;
        unknownOutletLayout = hasUnknownOutletLayout;

        objectName = name;
        category = "";

        // Inverse lookup :(
        for (auto const& [cat, objects] : library.getObjectCategories()) {
            if (objects.contains(name)) {
                category = cat;
            }
        }

        if (category.isEmpty())
            category = "Unknown";

        description = library.getObjectDescriptions()[name];

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

    String category;
    String description;

    TextButton openHelp = TextButton("Show Help");
    TextButton openReference = TextButton("Show Reference");
    TextButton createObject = TextButton("Create Object");

    pd::Library& library;
    ObjectReferenceDialog& reference;
};

class ObjectSearchComponent : public Component
    , public ListBoxModel
    , public ScrollBar::Listener
    , public KeyListener {
public:
    ObjectSearchComponent(pd::Library& library)
    {
        listBox.setModel(this);
        listBox.setRowHeight(28);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();

        listBox.getViewport()->setScrollBarsShown(true, false, false, false);

        input.setName("sidebar::searcheditor");
        input.addKeyListener(this);
        input.onTextChange = [this]() {
            bool notEmpty = input.getText().isNotEmpty();
            if (listBox.isVisible() != notEmpty) {
                listBox.setVisible(notEmpty);
                getParentComponent()->repaint();
            }

            setInterceptsMouseClicks(notEmpty, true);
            updateResults(input.getText());
        };

        clearButton.setName("statusbar:clearsearch");
        clearButton.onClick = [this]() {
            input.clear();
            input.giveAwayKeyboardFocus();
            listBox.setVisible(false);
            setInterceptsMouseClicks(false, true);
            input.repaint();
            changeCallback("");
        };

        input.setInterceptsMouseClicks(true, true);
        clearButton.setAlwaysOnTop(true);

        addAndMakeVisible(clearButton);
        addAndMakeVisible(listBox);
        addAndMakeVisible(input);

        listBox.addMouseListener(this, true);
        listBox.setVisible(false);

        input.setJustification(Justification::centredLeft);
        input.setBorder({ 1, 23, 3, 1 });
        input.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);

        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

        listBox.getViewport()->getVerticalScrollBar().addListener(this);

        setInterceptsMouseClicks(false, true);

        objectDescriptions = library.getObjectDescriptions();

        for (auto& object : library.getAllObjects()) {
            if (!objectDescriptions.count(object)) {
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
        if (listBox.isVisible()) {
            g.fillAll(findColour(PlugDataColour::panelBackgroundColourId));
        }
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setFont(getLookAndFeel().getTextButtonFont(clearButton, 30));
        g.setColour(findColour(PlugDataColour::sidebarTextColourId));

        g.drawText(Icons::Search, 0, 0, 30, 30, Justification::centred);

        if (input.getText().isEmpty()) {
            g.setFont(Font(14));
            g.setColour(findColour(PlugDataColour::sidebarTextColourId).withAlpha(0.5f));

            g.drawText("Type to search for objects", 30, 0, 300, 30, Justification::centredLeft);
        }
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int w, int h, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle(4, 2, w - 8, h - 4, Constants::smallCornerRadius);
        }

        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(ComboBox::textColourId));
        const String item = searchResult[rowNumber];

        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        auto font = lnf->semiBoldFont.withHeight(12.0f);
        g.setFont(font);

        g.setColour(rowIsSelected ? findColour(PlugDataColour::popupMenuActiveTextColourId) : findColour(PlugDataColour::popupMenuTextColourId));

        auto yIndent = jmin<float>(4, h * 0.3f);
        auto fontHeight = roundToInt(font.getHeight() * 0.6f);
        auto leftIndent = 34;
        auto rightIndent = 11;
        auto textWidth = w - leftIndent - rightIndent;

        if (textWidth > 0)
            g.drawFittedText(item, leftIndent, yIndent, textWidth, h - yIndent * 2, Justification::left, 2);

        font = lnf->defaultFont.withHeight(12);
        g.setFont(font);

        auto objectDescription = objectDescriptions[item];

        if (objectDescription.isNotEmpty()) {
            auto textLength = font.getStringWidth(item);

            g.setColour(rowIsSelected ? findColour(PlugDataColour::popupMenuActiveTextColourId) : findColour(PlugDataColour::popupMenuTextColourId));

            leftIndent += textLength;
            auto textWidth = getWidth() - leftIndent - rightIndent;

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
        g.fillRoundedRectangle(iconbound.toFloat(), Constants::smallCornerRadius);

        g.setColour(Colours::white);
        g.setFont(font.withHeight(type ? 12 : 10));
        g.drawFittedText(type ? "~" : "pd", iconbound.reduced(1), Justification::centred, 1);
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
    }

    void updateResults(String query)
    {
        clearSearchResults();

        if (query.isEmpty())
            return;

        for (auto& item : objectDescriptions) {
            // Insert in front if the query matches a whole word
            if (item.first.containsWholeWord(query) || item.second.containsWholeWord(query)) {
                searchResult.insert(0, item.first);
            }
            // Insert in back if it contains the query
            else if (item.first.containsIgnoreCase(query) || item.second.containsIgnoreCase(query)) {
                searchResult.add(item.first);
            }
        }

        listBox.updateContent();
        listBox.repaint();
        
        if (listBox.getSelectedRow() == -1)
            listBox.selectRow(0, true, true);

        selectedRowsChanged(listBox.getSelectedRow());
    }

    bool hasSelection()
    {
        return listBox.isVisible() && isPositiveAndBelow(listBox.getSelectedRow(), searchResult.size());
    }
    bool isSearching()
    {
        return listBox.isVisible();
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(28);

        tableBounds.removeFromTop(4);

        input.setBounds(inputBounds);

        clearButton.setBounds(inputBounds.removeFromRight(25));

        listBox.setBounds(tableBounds);
    }

    std::function<void(String const&)> changeCallback;

private:
    ListBox listBox;

    Array<String> searchResult;
    TextEditor input;
    TextButton clearButton = TextButton(Icons::Clear);

    std::unordered_map<String, String> objectDescriptions;
};

struct ObjectBrowserDialog : public Component {

    ObjectBrowserDialog(Component* pluginEditor, Dialog* parent)
        : editor(dynamic_cast<PluginEditor*>(pluginEditor))
        , objectsList(editor->pd->objectLibrary)
        , objectReference(editor, true)
        , objectViewer(editor, objectReference)
        , objectSearch(editor->pd->objectLibrary)
    {
        auto& library = editor->pd->objectLibrary;
        objectsByCategory = library.getObjectCategories();

        addAndMakeVisible(categoriesList);
        addAndMakeVisible(objectsList);
        addAndMakeVisible(objectViewer);
        addAndMakeVisible(objectSearch);

        addChildComponent(objectReference);

        objectsByCategory["All"] = StringArray();

        StringArray categories;
        for (auto [category, objects] : objectsByCategory) {
            // Sort alphabetically
            objects.sort(true);

            // Add objects from every category to "All"
            objectsByCategory["All"].addArray(objects);
            categories.add(category);
        }

        // Also include undocumented objects
        objectsByCategory["All"].addArray(library.getAllObjects());
        objectsByCategory["All"].removeDuplicates(true);

        // Sort alphabetically
        objectsByCategory["All"].sort(true);
        categories.sort(true);

        // Make sure "All" is the first category
        categories.move(categories.indexOf("All"), 0);

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
#endif

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

    void resized() override
    {
        auto b = getLocalBounds().reduced(1);
        objectViewer.setBounds(b.removeFromRight(260));
        objectSearch.setBounds(b);
        b.removeFromTop(35);

        categoriesList.setBounds(b.removeFromLeft(170));
        objectsList.setBounds(b);

        objectReference.setBounds(getLocalBounds());
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
        g.fillRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Constants::windowCornerRadius);
    }

private:
    PluginEditor* editor;
    CategoriesListBox categoriesList;
    ObjectsListBox objectsList;

    ObjectReferenceDialog objectReference;
    ObjectViewer objectViewer;
    ObjectSearchComponent objectSearch;

    std::unordered_map<String, StringArray> objectsByCategory;
};
