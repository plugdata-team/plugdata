#include <utility>

/*
 // Copyright (c) 2021-2023 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

// Draws a trigger button in the style of the PropertiesPanel, though it's meant to be used outside PropertiesPanel itself
class ActionButton : public Component {

    bool mouseIsOver = false;
    bool roundTop;

public:
    ActionButton(String iconToShow, String textToShow, bool roundOnTop = false)
        : roundTop(roundOnTop)
        , icon(std::move(iconToShow))
        , text(std::move(textToShow))
    {
    }

    std::function<void()> onClick = []() {};

    void paint(Graphics& g) override
    {
        auto bounds = getLocalBounds();
        auto textBounds = bounds;
        auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

        auto colour = findColour(PlugDataColour::panelTextColourId);
        if (mouseIsOver) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));

            Path p;
            p.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, true, true);
            g.fillPath(p);
        }

        Fonts::drawIcon(g, icon, iconBounds, colour, 12);
        Fonts::drawText(g, text, textBounds, colour, 15);
    }

    void mouseEnter(MouseEvent const& e) override
    {
        mouseIsOver = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        mouseIsOver = false;
        repaint();
    }

    void mouseUp(MouseEvent const& e) override
    {
        onClick();
    }

    String icon;
    String text;
};

class SearchPathPanel : public Component
    , public TextEditor::Listener
    , public FileDragAndDropTarget
    , private ListBoxModel {
public:
    std::unique_ptr<Dialog> confirmationDialog;

    std::function<void()> onChange = []() {};

    /** Creates an empty FileSearchPathListObject. */
    SearchPathPanel()
    {
        listBox.setOutlineThickness(0);
        listBox.setRowHeight(32);

        listBox.setModel(this);
        addAndMakeVisible(listBox);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setColour(ListBox::outlineColourId, Colours::transparentBlack);

        addAndMakeVisible(addButton);
        addButton.onClick = [this] { addPath(); };

        removeButton.setTooltip("Remove search path");
        addAndMakeVisible(removeButton);
        removeButton.onClick = [this] { deleteSelected(); };
        removeButton.setConnectedEdges(12);

        changeButton.setTooltip("Edit search path");
        addAndMakeVisible(changeButton);
        changeButton.setConnectedEdges(12);
        changeButton.onClick = [this] { editSelected(); };

        upButton.setTooltip("Move selection up");
        addAndMakeVisible(upButton);
        upButton.setConnectedEdges(12);
        upButton.onClick = [this] { moveSelection(-1); };

        downButton.setTooltip("Move selection down");
        addAndMakeVisible(downButton);
        downButton.setConnectedEdges(12);
        downButton.onClick = [this] { moveSelection(1); };

        addChildComponent(editor);
        editor.addListener(this);

        editor.setColour(TextEditor::backgroundColourId, findColour(PlugDataColour::panelActiveBackgroundColourId));
        editor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);

        editor.setFont(Font(15));

        addAndMakeVisible(resetButton);
        resetButton.onClick = [this]() {
            Dialogs::showOkayCancelDialog(&confirmationDialog, findParentComponentOfClass<Dialog>(), "Are you sure you want to reset all the search paths?",
                [this](int result) {
                    if (result == 0)
                        return;

                    paths.clear();

                    for (const auto& dir : pd::Library::defaultPaths) {
                        paths.add(dir.getFullPathName());
                    }

                    internalChange();
                });
        };

        listBox.getViewport()->setScrollBarsShown(false, false, false, false);

        // Load state from settings file
        externalChange();
    }

    int getNumRows() override
    {
        return paths.size();
    }

    std::pair<int, int> getContentXAndWidth()
    {
        auto desiredContentWidth = 600;
        auto marginWidth = (getWidth() - desiredContentWidth) / 2;
        return { marginWidth, desiredContentWidth };
    }

    void paint(Graphics& g) override
    {
        auto [x, width] = getContentXAndWidth();
        int height = (getNumRows() + 1) * 32;

        // Draw area behind reset button
        auto resetButtonBounds = resetButton.getBounds().toFloat();

        Path p;
        p.addRoundedRectangle(resetButtonBounds.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 7);

        g.setColour(findColour(PlugDataColour::panelForegroundColourId));
        g.fillRoundedRectangle(resetButtonBounds, Corners::largeCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(resetButtonBounds, Corners::largeCornerRadius, 1.0f);

        // Draw area behind properties
        auto propertyBounds = Rectangle<float>(x, 90.0f, width, height);

        p = Path();
        p.addRoundedRectangle(propertyBounds.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 7);

        g.setColour(findColour(PlugDataColour::panelForegroundColourId));
        g.fillRoundedRectangle(propertyBounds, Corners::largeCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(propertyBounds, Corners::largeCornerRadius, 1.0f);

        Fonts::drawStyledText(g, "Search paths", x, 0, width - 4, 36.0f, findColour(PropertyComponent::labelTextColourId), Semibold, 15.0f);
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto [x, newWidth] = getContentXAndWidth();

        if (rowIsSelected) {

            bool roundTop = rowNumber == 0;
            Path p;
            p.addRoundedRectangle(x, 0.0f, newWidth, height, Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, false, false);

            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillPath(p);
        }

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
        g.drawHorizontalLine(height - 1.0f, x, x + newWidth);

        Fonts::drawText(g, paths[rowNumber], x + 12, 0, width - 9, height, findColour(PlugDataColour::panelTextColourId), 15);
    }

    void deleteKeyPressed(int row) override
    {
        if (isPositiveAndBelow(row, paths.size())) {
            paths.remove(row);
            internalChange();
        }
    }

    void returnKeyPressed(int row) override
    {
        if (!changeButton.isEnabled())
            return;

        editor.setVisible(true);
        editor.grabKeyboardFocus();
        editor.setText(paths[listBox.getSelectedRow()]);
        editor.selectAll();

        rowBeingEdited = row;

        resized();
        repaint();
    }

    void listBoxItemDoubleClicked(int row, MouseEvent const&) override
    {
        returnKeyPressed(row);
    }

    void selectedRowsChanged(int lastRowSelected) override
    {
        updateButtons();
    }

    void resized() override
    {
        listBox.setBounds(0, 90, getWidth(), getHeight() - 90);

        auto [x, width] = getContentXAndWidth();
        resetButton.setBounds(x, 39, width, 32.0f);

        if (editor.isVisible()) {
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), true) + listBox.getPosition();
            editor.setBounds(x + 6, selectionBounds.getY() + 2, width - 12, selectionBounds.getHeight() - 2);
        }

        updateButtons();
    }

    bool isInterestedInFileDrag(StringArray const&) override
    {
        return true;
    }

    void filesDropped(StringArray const& filenames, int x, int y) override
    {
        for (int i = filenames.size(); --i >= 0;) {
            File const f(filenames[i]);
            if (f.isDirectory()) {
                paths.add(f.getFullPathName());
                internalChange();
            }
        }
    }

private:
    void externalChange()
    {
        paths.clear();

        for (auto child : SettingsFile::getInstance()->getPathsTree()) {
            if (child.hasType("Path")) {
                paths.addIfNotAlreadyThere(child.getProperty("Path").toString());
            }
        }

        listBox.updateContent();
        listBox.repaint();
        updateButtons();
        onChange();
    }
    void internalChange()
    {
        auto pathsTree = SettingsFile::getInstance()->getPathsTree();
        pathsTree.removeAllChildren(nullptr);

        for (auto const& path : paths) {
            auto dir = File(path);
            if (dir.isDirectory()) {
                auto newPath = ValueTree("Path");
                newPath.setProperty("Path", dir.getFullPathName(), nullptr);
                pathsTree.appendChild(newPath, nullptr);
            }
        }

        listBox.updateContent();
        listBox.repaint();
        updateButtons();
        onChange();
    }

    void updateButtons()
    {
        bool const anythingSelected = listBox.getNumSelectedRows() > 0;
        bool const readOnlyPath = pd::Library::defaultPaths.contains(paths[listBox.getSelectedRow()]);

        removeButton.setVisible(anythingSelected);
        changeButton.setVisible(anythingSelected);
        upButton.setVisible(anythingSelected);
        downButton.setVisible(anythingSelected);

        removeButton.setEnabled(!readOnlyPath);
        changeButton.setEnabled(!readOnlyPath);

        if (anythingSelected) {
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), false) + listBox.getPosition();
            selectionBounds = selectionBounds.reduced(0, 2);
            auto buttonHeight = selectionBounds.getHeight();

            selectionBounds.removeFromRight(50);

            removeButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
            changeButton.setBounds(selectionBounds.removeFromRight(buttonHeight));

            downButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
            upButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
        }

        auto [x, width] = getContentXAndWidth();

        auto addButtonBounds = Rectangle<int>(x, 90.0f + (getNumRows() * 32), width, 32);
        addButton.setBounds(addButtonBounds);
    }

    void addPath()
    {
        auto start = defaultBrowseTarget;

        if (start == File())
            start = paths[0];

        if (start == File())
            start = File::getCurrentWorkingDirectory();

        Dialogs::showOpenDialog([this](URL url) {
            auto result = url.getLocalFile();
            if (result.exists()) {
                paths.addIfNotAlreadyThere(result.getFullPathName(), listBox.getSelectedRow());
                internalChange();
            }
        },
            false, true, "", "PathBrowser", getTopLevelComponent());
    }

    void deleteSelected()
    {
        deleteKeyPressed(listBox.getSelectedRow());
        internalChange();
    }

    void editSelected()
    {
        if (!changeButton.isEnabled())
            return;

        auto row = listBox.getSelectedRow();

        Dialogs::showOpenDialog([this, row](URL url) {
            auto result = url.getLocalFile();
            if (result.exists()) {
                paths.remove(row);
                paths.addIfNotAlreadyThere(result.getFullPathName(), row);
                internalChange();
            }
        },
            false, true, "", "PathBrowser", getTopLevelComponent());

        internalChange();
    }

    void moveSelection(int delta)
    {
        jassert(delta == -1 || delta == 1);
        auto currentRow = listBox.getSelectedRow();

        if (isPositiveAndBelow(currentRow, paths.size())) {
            auto newRow = jlimit(0, paths.size() - 1, currentRow + delta);
            if (currentRow != newRow) {
                paths.move(currentRow, newRow);
                listBox.selectRow(newRow);
                internalChange();
            }
        }
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        if (isPositiveAndBelow(rowBeingEdited, paths.size())) {
            paths.set(rowBeingEdited, editor.getText());
            editor.setVisible(false);
            internalChange();
            rowBeingEdited = -1;
        }
    }

    void textEditorFocusLost(TextEditor& ed) override
    {
        if (isPositiveAndBelow(rowBeingEdited, paths.size())) {
            paths.set(rowBeingEdited, editor.getText());
            editor.setVisible(false);
            internalChange();
            rowBeingEdited = -1;
        }
    }

    TextEditor editor;
    int rowBeingEdited = -1;

    StringArray paths;
    File defaultBrowseTarget;

    ListBox listBox;

    SmallIconButton upButton = SmallIconButton(Icons::Up);
    SmallIconButton downButton = SmallIconButton(Icons::Down);

    ActionButton addButton = ActionButton(Icons::Add, "Add search path");
    ActionButton resetButton = ActionButton(Icons::Reset, "Reset to default search paths", true);
    SmallIconButton removeButton = SmallIconButton(Icons::Clear);
    SmallIconButton changeButton = SmallIconButton(Icons::Edit);

    ValueTree tree;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchPathPanel)
};

class LibraryLoadPanel : public Component
    , public TextEditor::Listener
    , private ListBoxModel {

public:
    std::unique_ptr<Dialog> confirmationDialog;
    std::function<void()> onChange = []() {};

    LibraryLoadPanel()
    {
        listBox.setOutlineThickness(0);
        listBox.setRowHeight(32);

        listBox.setModel(this);
        addAndMakeVisible(listBox);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setColour(ListBox::outlineColourId, Colours::transparentBlack);

        addAndMakeVisible(addButton);
        addButton.onClick = [this]() { addLibrary(); };

        removeButton.setTooltip("Remove library");
        addAndMakeVisible(removeButton);
        removeButton.onClick = [this] { deleteSelected(); };
        removeButton.setConnectedEdges(12);

        changeButton.setTooltip("Edit library");
        addAndMakeVisible(changeButton);
        changeButton.setConnectedEdges(12);
        changeButton.onClick = [this] { editSelected(); };

        addChildComponent(editor);
        editor.addListener(this);

        editor.setColour(TextEditor::backgroundColourId, findColour(PlugDataColour::panelActiveBackgroundColourId));
        editor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);

        editor.setFont(Font(15));

        listBox.getViewport()->setScrollBarsShown(false, false, false, false);

        // Load state from settings file
        externalChange();
    }

    int getNumRows() override
    {
        return librariesToLoad.size();
    };

    void paint(Graphics& g) override
    {
        auto [x, width] = getContentXAndWidth();
        int height = (getNumRows() + 1) * 32;

        auto propertyBounds = Rectangle<float>(x, 40.0f, width, height);

        Path p;
        p.addRoundedRectangle(propertyBounds.reduced(3.0f), Corners::largeCornerRadius);
        StackShadow::renderDropShadow(g, p, Colour(0, 0, 0).withAlpha(0.4f), 7);

        g.setColour(findColour(PlugDataColour::panelForegroundColourId));
        g.fillRoundedRectangle(propertyBounds, Corners::largeCornerRadius);

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawRoundedRectangle(propertyBounds, Corners::largeCornerRadius, 1.0f);

        Fonts::drawStyledText(g, "Libraries to load", x, 0, width - 4, 36.0f, findColour(PropertyComponent::labelTextColourId), Semibold, 15.0f);
    }

    std::pair<int, int> getContentXAndWidth()
    {
        auto desiredContentWidth = 600;
        auto marginWidth = (getWidth() - desiredContentWidth) / 2;
        return { marginWidth, desiredContentWidth };
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        auto [x, newWidth] = getContentXAndWidth();

        if (rowIsSelected) {

            bool roundTop = rowNumber == 0;
            Path p;
            p.addRoundedRectangle(x, 0.0f, newWidth, height, Corners::largeCornerRadius, Corners::largeCornerRadius, roundTop, roundTop, false, false);

            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillPath(p);
        }

        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId).withAlpha(0.5f));
        g.drawHorizontalLine(height - 1.0f, x, x + newWidth);

        Fonts::drawText(g, librariesToLoad[rowNumber], x + 12, 0, width - 9, height, findColour(PlugDataColour::panelTextColourId), 15);
    }

    void deleteKeyPressed(int row) override
    {
        if (isPositiveAndBelow(row, librariesToLoad.size())) {
            librariesToLoad.remove(row);
            internalChange();
        }
    }

    void returnKeyPressed(int row) override
    {
        editor.setVisible(true);
        editor.grabKeyboardFocus();
        editor.setText(librariesToLoad[listBox.getSelectedRow()]);
        editor.selectAll();

        rowBeingEdited = row;

        resized();
        repaint();
    }

    void listBoxItemDoubleClicked(int row, MouseEvent const&) override
    {

        returnKeyPressed(row);
    }

    void selectedRowsChanged(int lastRowSelected) override
    {
        updateButtons();
    }

    void resized() override
    {
        int const titleHeight = 40;
        listBox.setBounds(0, titleHeight, getWidth(), getHeight());

        if (editor.isVisible()) {
            auto [x, width] = getContentXAndWidth();
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), true) + listBox.getPosition();
            editor.setBounds(x + 6, selectionBounds.getY() + 2, width - 12, selectionBounds.getHeight() - 2);
        }

        updateButtons();
    }

    void externalChange()
    {
        librariesToLoad.clear();

        for (auto child : SettingsFile::getInstance()->getLibrariesTree()) {
            if (child.hasType("Library")) {
                librariesToLoad.addIfNotAlreadyThere(child.getProperty("Name").toString());
            }
        }

        listBox.updateContent();
        listBox.repaint();
        updateButtons();
        onChange();
    }
    void internalChange()
    {
        auto librariesTree = SettingsFile::getInstance()->getLibrariesTree();
        librariesTree.removeAllChildren(nullptr);

        for (auto const& name : librariesToLoad) {
            if (name.isNotEmpty()) {
                auto newLibrary = ValueTree("Library");
                newLibrary.setProperty("Name", name, nullptr);
                librariesTree.appendChild(newLibrary, nullptr);
            }
        }

        listBox.updateContent();
        listBox.repaint();
        updateButtons();
        onChange();
    }

    void updateButtons()
    {
        bool const anythingSelected = listBox.getNumSelectedRows() > 0;

        removeButton.setVisible(anythingSelected);
        changeButton.setVisible(anythingSelected);

        if (anythingSelected) {
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), false) + listBox.getPosition();
            selectionBounds = selectionBounds.reduced(0, 2);
            auto buttonHeight = selectionBounds.getHeight();

            selectionBounds.removeFromRight(50);

            removeButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
            changeButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
        }

        auto [x, width] = getContentXAndWidth();

        auto addButtonBounds = Rectangle<int>(x, 40 + (getNumRows() * 32), width, 32);

        addButton.setBounds(addButtonBounds);
    }

    void addLibrary()
    {
        librariesToLoad.add("");
        internalChange();
        listBox.selectRow(librariesToLoad.size() - 1, true);
        returnKeyPressed(librariesToLoad.size() - 1);
    }

    void deleteSelected()
    {
        deleteKeyPressed(listBox.getSelectedRow());
        internalChange();
    }

    void editSelected()
    {
        returnKeyPressed(listBox.getSelectedRow());
        internalChange();
    }

    void textEditorReturnKeyPressed(TextEditor& ed) override
    {
        if (isPositiveAndBelow(rowBeingEdited, librariesToLoad.size())) {
            librariesToLoad.set(rowBeingEdited, editor.getText());
            editor.setVisible(false);
            internalChange();
            rowBeingEdited = -1;
        }
    }

    void textEditorFocusLost(TextEditor& ed) override
    {
        if (isPositiveAndBelow(rowBeingEdited, librariesToLoad.size())) {
            librariesToLoad.set(rowBeingEdited, editor.getText());
            editor.setVisible(false);
            internalChange();
            rowBeingEdited = -1;
        }
    }

private:
    StringArray librariesToLoad;

    ListBox listBox;

    ActionButton addButton = ActionButton(Icons::Add, "Add library to load on startup");
    SmallIconButton removeButton = SmallIconButton(Icons::Clear);
    SmallIconButton changeButton = SmallIconButton(Icons::Edit);

    ValueTree tree;

    TextEditor editor;
    int rowBeingEdited = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryLoadPanel)
};

class PathsAndLibrariesPanel : public SettingsDialogPanel
    , public ComponentListener {
public:
    PathsAndLibrariesPanel()
    {
        container.addAndMakeVisible(searchPathsPanel);
        container.addAndMakeVisible(libraryLoadPanel);

        searchPathsPanel.onChange = [this]() {
            updateBounds();
        };

        libraryLoadPanel.onChange = [this]() {
            updateBounds();
        };

        container.setVisible(true);
        viewport.setViewedComponent(&container, false);

        addAndMakeVisible(viewport);
        resized();

        viewport.setScrollBarsShown(true, false, false, false);
    }

private:
    void updateBounds()
    {
        searchPathsPanel.setSize(getWidth(), 96.0f + (searchPathsPanel.getNumRows() + 1) * 32.0f);
        libraryLoadPanel.setBounds(libraryLoadPanel.getX(), searchPathsPanel.getBottom() + 4.0f, getWidth(), 52.0f + (libraryLoadPanel.getNumRows() + 1) * 32.0f);

        container.setBounds(getLocalBounds().getUnion(searchPathsPanel.getBounds()).getUnion(libraryLoadPanel.getBounds()));
    }

    void resized() override
    {
        viewport.setBounds(getLocalBounds());
        updateBounds();
    }

    SearchPathPanel searchPathsPanel;
    LibraryLoadPanel libraryLoadPanel;

    Component container;
    BouncingViewport viewport;
};
