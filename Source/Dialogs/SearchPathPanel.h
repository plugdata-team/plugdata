/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

bool wantsNativeDialog();

class SearchPathComponent : public Component
    , public FileDragAndDropTarget
    , private ListBoxModel {
        
        class AddPathButton : public Component {
            
            bool mouseIsOver = false;
            
        public:
            std::function<void()> onClick = [](){};
            
            void paint(Graphics& g) override {
                
                auto bounds = getLocalBounds().reduced(5, 2);
                auto textBounds = bounds;
                auto iconBounds = textBounds.removeFromLeft(textBounds.getHeight());
                
                auto& lnf = dynamic_cast<PlugDataLook&>(getLookAndFeel());
                
                auto colour = findColour(PlugDataColour::panelTextColourId);
                if(mouseIsOver) {
                    g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
                    g.fillRoundedRectangle(bounds.toFloat(), PlugDataLook::defaultCornerRadius);
                    
                    colour = findColour(PlugDataColour::panelActiveTextColourId);
                }
                
                g.setColour(colour);
                g.setFont(lnf.iconFont.withHeight(14));
                g.drawText(Icons::Add, iconBounds, Justification::centred);
                
                g.setFont(Font(14));
                PlugDataLook::drawText(g, "Add search path", textBounds, Justification::centredLeft, colour);
            }
            
            void mouseEnter(const MouseEvent& e) override
            {
                mouseIsOver = true;
                repaint();
            }
            
            void mouseExit(const MouseEvent& e) override
            {
                mouseIsOver = false;
                repaint();
            }
            
            void mouseUp(const MouseEvent& e) override
            {
                onClick();
            }
        };
        
public:
    std::unique_ptr<Dialog> confirmationDialog;
    

    /** Creates an empty FileSearchPathListObject. */
    SearchPathComponent(ValueTree libraryTree)
        : tree(std::move(libraryTree))
    {
        listBox.setOutlineThickness(0);
        listBox.setRowHeight(25);

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
        removeButton.setName("statusbar:remove");

        changeButton.setTooltip("Edit search path");
        changeButton.setName("statusbar:change");
        addAndMakeVisible(changeButton);
        changeButton.setConnectedEdges(12);
        changeButton.onClick = [this] { editSelected(); };

        upButton.setTooltip("Move selection up");
        upButton.setName("statusbar:up");
        addAndMakeVisible(upButton);
        upButton.setConnectedEdges(12);
        upButton.onClick = [this] { moveSelection(-1); };

        upButton.setTooltip("Move selection down");
        downButton.setName("statusbar:down");
        addAndMakeVisible(downButton);
        downButton.setConnectedEdges(12);
        downButton.onClick = [this] { moveSelection(1); };

        resetButton.setTooltip("Reset to default");
        resetButton.setName("statusbar:reset");
        addAndMakeVisible(resetButton);
        resetButton.setConnectedEdges(12);
        resetButton.onClick = [this]() {
            Dialogs::showOkayCancelDialog(&confirmationDialog, getParentComponent(), "Are you sure you want to reset all the search paths?",
                [this](int result) {
                    if (result == 0)
                        return;

                    paths.clear();
                
                    for(const auto& dir : pd::Library::defaultPaths) {
                        paths.add(dir.getFullPathName());
                    }

                    internalChange();
                });
        };

        // Load state from valuetree
        externalChange();
    }

    /** Changes the current path. */
    void updatePath(ValueTree& tree)
    {
        paths.clear();

        for (auto child : tree) {
            paths.addIfNotAlreadyThere(child.getProperty("Path").toString());
        }

        internalChange();
    }

    /** Sets a file or directory to be the default starting point for the browser to show.

     This is only used if the current file hasn't been set.
     */
    void setDefaultBrowseTarget(File const& newDefaultDirectory)

    {
        defaultBrowseTarget = newDefaultDirectory;
    }

    int getNumRows() override
    {
        return paths.size();
    };

    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({ 4.0f, 1.0f, width - 8.0f, height - 2.0f }, PlugDataLook::defaultCornerRadius);
        }

        auto colour = rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId);

        g.setFont(Font(14));

        PlugDataLook::drawText(g, paths[rowNumber], 12, 0, width - 9, height, Justification::centredLeft, colour);
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
        chooser = std::make_unique<FileChooser>(TRANS("Change folder..."), paths[row], "*", wantsNativeDialog());
        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync(chooserFlags,
            [this, row](FileChooser const& fc) {
                if (fc.getResult() == File {})
                    return;

                paths.remove(row);
                paths.addIfNotAlreadyThere(fc.getResult().getFullPathName(), row);
                internalChange();
            });
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
        resetButton.setBounds(statusbarBounds.removeFromRight(statusbarHeight));
        
        updateButtons();
    }

    bool isInterestedInFileDrag(StringArray const&) override
    {
        return true;
    }

    void filesDropped(StringArray const& filenames, int x, int y) override
    {
        for (int i = filenames.size(); --i >= 0;) {
            const File f(filenames[i]);
            if (f.isDirectory()) {
                paths.add(f.getFullPathName());
                internalChange();
            }
        }
    }

private:
    

    StringArray paths;
    File defaultBrowseTarget;
    std::unique_ptr<FileChooser> chooser;

    ListBox listBox;

    TextButton upButton = TextButton(Icons::Up);
    TextButton downButton = TextButton(Icons::Down);

    AddPathButton addButton;
    TextButton removeButton = TextButton(Icons::Clear);
    TextButton resetButton = TextButton(Icons::Refresh);
    TextButton changeButton = TextButton(Icons::Edit);

    ValueTree tree;

    void externalChange()
    {
        paths.clear();

        for (auto child : tree) {
            if (child.hasType("Path")) {
                paths.addIfNotAlreadyThere(child.getProperty("Path").toString());
            }
        }

        listBox.updateContent();
        listBox.repaint();
        updateButtons();
    }
    void internalChange()
    {
        tree.removeAllChildren(nullptr);

        for (int p = 0; p < paths.size(); p++) {
            auto dir = File(paths[p]);
            if (dir.isDirectory()) {
                auto newPath = ValueTree("Path");
                newPath.setProperty("Path", dir.getFullPathName(), nullptr);
                tree.appendChild(newPath, nullptr);
            }
        }

        listBox.updateContent();
        listBox.repaint();
        updateButtons();
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
        upButton.setEnabled(!readOnlyPath);
        downButton.setEnabled(!readOnlyPath);

        if (anythingSelected) {
            auto selectionBounds = listBox.getRowPosition(listBox.getSelectedRow(), false).translated(0, 3);
            auto buttonHeight = selectionBounds.getHeight();

            selectionBounds.removeFromRight(5);

            removeButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
            changeButton.setBounds(selectionBounds.removeFromRight(buttonHeight - 4));

            selectionBounds.removeFromRight(5);

            downButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
            upButton.setBounds(selectionBounds.removeFromRight(buttonHeight));
        }
        
        auto addButtonBounds = listBox.getRowPosition(getNumRows(), false).translated(0, 5).withHeight(25);
        addButton.setBounds(addButtonBounds);
    }

    void addPath()
    {
        auto start = defaultBrowseTarget;

        if (start == File())
            start = paths[0];

        if (start == File())
            start = File::getCurrentWorkingDirectory();

        chooser = std::make_unique<FileChooser>(TRANS("Add a folder..."), start, "*");
        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync(chooserFlags,
            [this](FileChooser const& fc) {
                if (fc.getResult() == File {})
                    return;

                paths.addIfNotAlreadyThere(fc.getResult().getFullPathName(), listBox.getSelectedRow());
                internalChange();
            });
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchPathComponent)
};
