/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

class LibraryLoadPanel : public Component
    , public TextEditor::Listener
    , private ListBoxModel {

    class AddLibraryButton : public Component {

        bool mouseIsOver = false;

    public:
        std::function<void()> onClick = []() {};

        void paint(Graphics& g) override
        {

            auto bounds = getLocalBounds().reduced(5, 2);
            auto textBounds = bounds;
            auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight());

            auto colour = findColour(PlugDataColour::panelTextColourId);
            if (mouseIsOver) {
                g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
                g.fillRoundedRectangle(bounds.toFloat(), Corners::defaultCornerRadius);

                colour = findColour(PlugDataColour::panelActiveTextColourId);
            }

            Fonts::drawIcon(g, Icons::Add, iconBounds, colour, 12);
            Fonts::drawText(g, "Add library to load on startup", textBounds, colour, 14);
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
    };

public:
    std::unique_ptr<Dialog> confirmationDialog;

    LibraryLoadPanel()
    {
        listBox.setOutlineThickness(0);
        listBox.setRowHeight(26);

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
        removeButton.getProperties().set("Style", "SmallIcon");

        changeButton.setTooltip("Edit library");
        changeButton.getProperties().set("Style", "SmallIcon");
        addAndMakeVisible(changeButton);
        changeButton.setConnectedEdges(12);
        changeButton.onClick = [this] { editSelected(); };

        addChildComponent(editor);
        editor.addListener(this);

        editor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);

        editor.setFont(Font(14));

        // Load state from settings file
        externalChange();
    }

    void updateLibraries()
    {
        librariesToLoad.clear();

        for (auto child : tree) {
            librariesToLoad.add(child.getProperty("Name").toString());
        }

        internalChange();
    }

    int getNumRows() override
    {
        return librariesToLoad.size();
    };

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, Corners::defaultCornerRadius);
        }

        if (!editor.isVisible() || rowBeingEdited != rowNumber) {
            auto colour = rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId);
            Fonts::drawText(g, librariesToLoad[rowNumber], 12, 0, width - 9, height, colour, 14);
        }
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
        int const statusbarHeight = 32;
        int const statusbarY = getHeight() - statusbarHeight;

        listBox.setBounds(0, 4, getWidth(), statusbarY);

        if (editor.isVisible()) {
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), false).translated(0, 3);
            editor.setBounds(selectionBounds.withTrimmedRight(80).withTrimmedLeft(6).reduced(1));
        }

        updateButtons();
    }

private:
    StringArray librariesToLoad;

    ListBox listBox;

    AddLibraryButton addButton;
    TextButton removeButton = TextButton(Icons::Clear);
    TextButton changeButton = TextButton(Icons::Edit);

    ValueTree tree;

    TextEditor editor;
    int rowBeingEdited = -1;

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
    }
    void internalChange()
    {
        auto librariesTree = SettingsFile::getInstance()->getLibrariesTree();
        librariesTree.removeAllChildren(nullptr);

        for (int i = 0; i < librariesToLoad.size(); i++) {
            auto newLibrary = ValueTree("Library");
            newLibrary.setProperty("Name", librariesToLoad[i], nullptr);
            librariesTree.appendChild(newLibrary, nullptr);
        }

        listBox.updateContent();
        listBox.repaint();
        updateButtons();
    }

    void updateButtons()
    {
        bool const anythingSelected = listBox.getNumSelectedRows() > 0;

        removeButton.setVisible(anythingSelected);
        changeButton.setVisible(anythingSelected);

        if (anythingSelected) {
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), false).translated(0, 3);
            auto buttonHeight = selectionBounds.getHeight();

            selectionBounds.removeFromRight(5);

            removeButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
            changeButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
        }

        auto addButtonBounds = listBox.getRowPosition(getNumRows(), false).translated(0, 5).withHeight(28);
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LibraryLoadPanel)
};
