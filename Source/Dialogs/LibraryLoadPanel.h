/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

bool wantsNativeDialog();

class LibraryLoadPanel : public Component
    , public TextEditor::Listener
    , private ListBoxModel {
public:
    std::unique_ptr<Dialog> confirmationDialog;
    //==============================================================================
    /** Creates an empty FileSearchPathListObject. */
    LibraryLoadPanel(ValueTree libraryTree)
        : tree(std::move(libraryTree))
    {
        listBox.setOutlineThickness(0);
        listBox.setRowHeight(25);

        listBox.setModel(this);
        addAndMakeVisible(listBox);
        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);
        listBox.setColour(ListBox::outlineColourId, Colours::transparentBlack);

        addButton.setTooltip("Add search path");
        addButton.setName("statusbar:add");
        addAndMakeVisible(addButton);
        addButton.onClick = [this] { addPath(); };
        addButton.setConnectedEdges(12);

        removeButton.setTooltip("Remove search path");
        addAndMakeVisible(removeButton);
        removeButton.onClick = [this] { deleteSelected(); };
        removeButton.setConnectedEdges(12);
        removeButton.setName("statusbar:remove");

        changeButton.setTooltip("Edit search path");
        changeButton.setName("statusbar:change");
        addAndMakeVisible(changeButton);
        changeButton.setConnectedEdges(12);
        changeButton.onClick = [this] { editSelected(); };

        addChildComponent(editor);
        editor.addListener(this);

        editor.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        editor.setColour(TextEditor::outlineColourId, Colours::transparentBlack);

        editor.setFont(Font(15));

        // Load state from valuetree
        externalChange();
    }

    /** Changes the current path. */
    void updateLibraries(ValueTree& tree)
    {
        librariesToLoad.clear();

        for (auto child : tree) {
            librariesToLoad.add(child.getProperty("Name").toString());
        }

        internalChange();
    }

    //==============================================================================

    int getNumRows() override
    {
        return librariesToLoad.size();
    };

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, PlugDataLook::defaultCornerRadius);
        }

        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId));

        g.setFont(Font(15));

        if (!editor.isVisible() || rowBeingEdited != rowNumber) {
            g.drawText(librariesToLoad[rowNumber], 12, 0, width - 9, height, Justification::centredLeft, true);
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

        auto statusbarBounds = Rectangle<int>(2, statusbarY + 6, getWidth() - 6, statusbarHeight);
        addButton.setBounds(statusbarBounds.removeFromLeft(statusbarHeight));

        if (editor.isVisible()) {
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), false).translated(0, 3);
            editor.setBounds(selectionBounds.withTrimmedRight(80).withTrimmedLeft(6).reduced(1));
        }
    }

private:
    //==============================================================================
    StringArray librariesToLoad;

    ListBox listBox;

    TextButton addButton = TextButton(Icons::Add);
    TextButton removeButton = TextButton(Icons::Clear);
    TextButton changeButton = TextButton(Icons::Edit);

    ValueTree tree;

    TextEditor editor;
    int rowBeingEdited = -1;

    void externalChange()
    {
        librariesToLoad.clear();

        for (auto child : tree) {
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
        tree.removeAllChildren(nullptr);

        for (int i = 0; i < librariesToLoad.size(); i++) {
            auto newPath = ValueTree("Library");
            newPath.setProperty("Name", librariesToLoad[i], nullptr);
            tree.appendChild(newPath, nullptr);
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
            changeButton.setBounds(selectionBounds.removeFromRight(buttonHeight - 4));
        }
    }

    void addPath()
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
