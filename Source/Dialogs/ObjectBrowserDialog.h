/*
 // Copyright (c) 2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
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
    }

    std::function<void(String const&)> changeCallback;
};

struct ObjectsListBox : public ListBox
    , public ListBoxModel {

    ObjectsListBox(pd::Library& library)
    {
        setOutlineThickness(0);
        setRowHeight(50);

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
    }

    std::unordered_map<String, String> descriptions;
    StringArray objects;
    std::function<void(String const&)> changeCallback;
};

struct ObjectViewer : public Component {
    ObjectViewer(pd::Library& objectLibrary)
        : library(objectLibrary)
    {
        addChildComponent(openHelp);
        addChildComponent(createObject);

        createObject.setConnectedEdges(Button::ConnectedOnBottom);
        openHelp.setConnectedEdges(Button::ConnectedOnTop);

        // We only need to respond to explicit repaints anyway!
        setBufferedToImage(true);
    }

    void resized() override
    {
        auto buttonBounds = getLocalBounds().removeFromBottom(50).reduced(60, 0).translated(0, -30);
        createObject.setBounds(buttonBounds.removeFromTop(25));
        openHelp.setBounds(buttonBounds.translated(0, -1));
    }

    void paint(Graphics& g) override
    {
        auto const font = Font(15.0f);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(5, 0, 5, getHeight());

        if (objectName.isEmpty())
            return;

        auto infoBounds = getLocalBounds().withTrimmedBottom(100).reduced(20);
        auto objectDisplayBounds = infoBounds.removeFromTop(300).reduced(60);

        auto* lnf = dynamic_cast<PlugDataLook*>(&getLookAndFeel());
        if (!lnf)
            return;

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(lnf->boldFont.withHeight(16.0f));
        g.drawText(objectName, getLocalBounds().removeFromTop(100).translated(0, 20), Justification::centred);

        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);
        g.drawText("Category:", infoBounds.removeFromTop(25), Justification::left);
        g.drawText("Description:", infoBounds.removeFromTop(25), Justification::left);

        if (unknownLayout) {
            return;
        }

        int const ioletSize = 8;

        int const ioletWidth = (ioletSize + 5) * std::max(inlets.size(), outlets.size());
        int const textWidth = font.getStringWidth(objectName);
        int const width = std::max(ioletWidth, textWidth) + 14;

        auto outlineBounds = objectDisplayBounds.withSizeKeepingCentre(width, 22).toFloat();
        g.setColour(findColour(PlugDataColour::objectOutlineColourId));
        g.drawRoundedRectangle(outlineBounds, Constants::objectCornerRadius, 1.0f);

        auto textBounds = outlineBounds.reduced(2.0f);
        g.setColour(findColour(PlugDataColour::canvasTextColourId));
        g.setFont(font);
        g.drawText(objectName, textBounds, Justification::centred);

        auto ioletBounds = outlineBounds.reduced(10, 0);

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
        openHelp.setVisible(valid);

        if (!valid) {
            objectName = "";
            unknownLayout = false;
            inlets.clear();
            outlets.clear();
            repaint();
            return;
        }

        auto inletDescriptions = library.getInletDescriptions()[name];
        auto outletDescriptions = library.getOutletDescriptions()[name];

        inlets.resize(inletDescriptions.size());
        outlets.resize(outletDescriptions.size());

        bool hasUnknownLayout = false;

        for (int i = 0; i < inlets.size(); i++) {
            inlets[i] = inletDescriptions[i].first.contains("(signal)");
            if (inletDescriptions[i].second)
                hasUnknownLayout = true;
        }
        for (int i = 0; i < outlets.size(); i++) {
            outlets[i] = outletDescriptions[i].first.contains("(signal)");
            if (outletDescriptions[i].second)
                hasUnknownLayout = true;
        }

        unknownLayout = hasUnknownLayout;
        objectName = name;

        repaint();
    }

    bool unknownLayout = false;

    String objectName;
    std::vector<bool> inlets;
    std::vector<bool> outlets;

    TextButton openHelp = TextButton("Show Help");
    TextButton createObject = TextButton("Create Object");

    pd::Library& library;
};

class ObjectSearchComponent : public Component
    , public ListBoxModel
    , public ScrollBar::Listener
    , public KeyListener {
public:
    ObjectSearchComponent()
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

        closeButton.setName("statusbar:clearsearch");
        closeButton.onClick = [this]() {
            input.clear();
            input.giveAwayKeyboardFocus();
            listBox.setVisible(false);
            setInterceptsMouseClicks(false, true);
            input.repaint();
        };

        input.setInterceptsMouseClicks(true, true);
        closeButton.setAlwaysOnTop(true);

        addAndMakeVisible(closeButton);
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
    }

    void mouseDoubleClick(MouseEvent const& e) override
    {
        int row = listBox.getSelectedRow();

        if (isPositiveAndBelow(row, searchResult.size())) {
            if (listBox.getRowPosition(row, true).contains(e.getEventRelativeTo(&listBox).getPosition())) {
                openFile(searchResult.getReference(row));
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
        g.setFont(getLookAndFeel().getTextButtonFont(closeButton, 30));
        g.setColour(findColour(PlugDataColour::sidebarTextColourId));

        g.drawText(Icons::Search, 0, 0, 30, 30, Justification::centred);

        if (input.getText().isEmpty()) {
            g.setFont(Font(14));
            g.setColour(findColour(PlugDataColour::sidebarTextColourId).withAlpha(0.5f));

            g.drawText("Type to search documentation", 30, 0, 300, 30, Justification::centredLeft);
        }
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int w, int h, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            g.fillRoundedRectangle(4, 2, w - 8, h - 4, Constants::smallCornerRadius);
        }

        g.setColour(rowIsSelected ? findColour(PlugDataColour::sidebarActiveTextColourId) : findColour(ComboBox::textColourId));
        const String item = searchResult[rowNumber].getFileName();

        g.setFont(Font());
        g.drawText(item, 28, 0, w - 4, h, Justification::centredLeft, true);

        g.setFont(getLookAndFeel().getTextButtonFont(closeButton, 23));
        g.drawText(Icons::File, 12, 0, 24, 24, Justification::centredLeft);
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

        /*
        if (query.isEmpty())
            return;

        auto addFile = [this, &query](File const& file) {
            auto fileName = file.getFileName();

            if (!file.hasFileExtension("pd"))
                return;

            // Insert in front if the query matches a whole word
            if (fileName.containsWholeWordIgnoreCase(query)) {
                searchResult.insert(0, file);
            }
            // Insert in back if it contains the query
            else if (fileName.containsIgnoreCase(query)) {
                searchResult.add(file);
            }
        };

        for (int i = 0; i < searchPath.getNumFiles(); i++) {
            auto file = searchPath.getFile(i);

            if (file.isDirectory()) {
                for (auto& child : RangedDirectoryIterator(file, true)) {
                    addFile(child.getFile());
                }
            } else {
                addFile(file);
            }
        }

        listBox.updateContent();

        if (listBox.getSelectedRow() == -1)
            listBox.selectRow(0, true, true); */
    }

    bool hasSelection()
    {
        return listBox.isVisible() && isPositiveAndBelow(listBox.getSelectedRow(), searchResult.size());
    }
    bool isSearching()
    {
        return listBox.isVisible();
    }

    File getSelection()
    {
        int row = listBox.getSelectedRow();

        if (isPositiveAndBelow(row, searchResult.size())) {
            return searchResult[row];
        }

        return File();
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(28);

        tableBounds.removeFromTop(4);

        input.setBounds(inputBounds);

        closeButton.setBounds(inputBounds.removeFromRight(30));

        listBox.setBounds(tableBounds);
    }

    std::function<void(File&)> openFile;

private:
    ListBox listBox;

    Array<File> searchResult;
    TextEditor input;
    TextButton closeButton = TextButton(Icons::Clear);
};

struct ObjectBrowserDialog : public Component {

    ObjectBrowserDialog(Component* pluginEditor, Dialog* parent)
        : editor(dynamic_cast<PluginEditor*>(pluginEditor))
        , objectsList(editor->pd->objectLibrary)
        , objectViewer(editor->pd->objectLibrary)
    {
        objectsByCategory = editor->pd->objectLibrary.getObjectCategories();

        addAndMakeVisible(categoriesList);
        addAndMakeVisible(objectsList);
        addAndMakeVisible(objectViewer);
        addAndMakeVisible(objectSearch);

        objectsByCategory["All"] = StringArray();

        StringArray categories;
        for (auto [category, objects] : objectsByCategory) {
            objects.sort(true);
            objectsByCategory["All"].addArray(objects);
            categories.add(category);
        }

        objectsByCategory["All"].sort(true);
        categories.sort(true);
        categories.move(categories.indexOf("All"), 0);

        categoriesList.changeCallback = [this](String const& category) {
            objectsList.showObjects(objectsByCategory[category]);
        };

        objectsList.changeCallback = [this](String const& object) {
            objectViewer.showObject(object);
        };

        categoriesList.initialise(categories);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced(1);
        objectViewer.setBounds(b.removeFromRight(300));
        objectSearch.setBounds(b);
        b.removeFromTop(35);

        categoriesList.setBounds(b.removeFromLeft(150));
        objectsList.setBounds(b);
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

    ObjectViewer objectViewer;
    ObjectSearchComponent objectSearch;

    std::unordered_map<String, StringArray> objectsByCategory;
    std::unordered_map<String, String> objectDescriptions;
};
