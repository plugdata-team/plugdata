/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

bool wantsNativeDialog();

class SearchPathComponent : public Component
, public SettableTooltipClient
, public FileDragAndDropTarget
, private ListBoxModel {
public:
    
    std::unique_ptr<Dialog> confirmationDialog;
    //==============================================================================
    /** Creates an empty FileSearchPathListObject. */
    SearchPathComponent(ValueTree libraryTree)
    : tree(std::move(libraryTree))
    {
        listBox.setOutlineThickness(0);
        listBox.setRowHeight(24);
        
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
                                          [this](int result){
                
                if(result == 0) return;
                
                auto libraryDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library");
                
                auto abstractionsDir = libraryDir.getChildFile("Abstractions");
                auto dekenDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library").getChildFile("Deken");
                
                path = FileSearchPath();
                path.add(abstractionsDir);
                path.add(dekenDir);
                
                internalChange();
            });
        };
        
        // Load state from valuetree
        externalChange();
    }
    
    //==============================================================================
    /** Returns the path as it is currently shown. */
    FileSearchPath const& getPath() const
    {
        return path;
    }
    
    /** Changes the current path. */
    void updatePath(ValueTree& tree)
    {
        path = FileSearchPath();
        
        for (auto child : tree) {
            path.add(File(child.getProperty("Path").toString()));
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
    
    //==============================================================================
    
    int getNumRows() override
    {
        return path.getNumPaths();
    };
    
    void paintListBoxItem(int rowNumber, Graphics& g, int width, int height, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRoundedRectangle({2.0f, 2.0f, width - 4.0f, height - 4.0f}, 5.0f);
        }
        
        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(PlugDataColour::panelTextColourId));
        
        Font f((float)height * 0.6f);
        f.setHorizontalScale(1.0f);
        g.setFont(f);
        
        g.drawText(path[rowNumber].getFullPathName(), 6, 0, width - 6, height, Justification::centredLeft, true);
    }
    
    void deleteKeyPressed(int row) override
    {
        if (isPositiveAndBelow(row, path.getNumPaths())) {
            path.remove(row);
            internalChange();
        }
    }
    
    void returnKeyPressed(int row) override
    {
        chooser = std::make_unique<FileChooser>(TRANS("Change folder..."), path[row], "*", wantsNativeDialog());
        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;
        
        chooser->launchAsync(chooserFlags,
                             [this, row](FileChooser const& fc) {
            if (fc.getResult() == File {})
                return;
            
            path.remove(row);
            path.add(fc.getResult(), row);
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
        
        listBox.setBounds(0, 0, getWidth(), statusbarY);
        
        auto statusbarBounds = Rectangle<int>(2, statusbarY + 6, getWidth() - 6, statusbarHeight);
        
        addButton.setBounds(statusbarBounds.removeFromLeft(statusbarHeight));
        removeButton.setBounds(statusbarBounds.removeFromLeft(statusbarHeight));
        
        downButton.setBounds(statusbarBounds.removeFromRight(statusbarHeight));
        upButton.setBounds(statusbarBounds.removeFromRight(statusbarHeight));
        changeButton.setBounds(statusbarBounds.removeFromRight(statusbarHeight));
        resetButton.setBounds(statusbarBounds.removeFromRight(statusbarHeight));
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
                auto row = listBox.getRowContainingPosition(0, y - listBox.getY());
                path.add(f, row);
                internalChange();
            }
        }
    }
    
private:
    //==============================================================================
    FileSearchPath path;
    File defaultBrowseTarget;
    std::unique_ptr<FileChooser> chooser;
    
    ListBox listBox;
    
    TextButton upButton = TextButton(Icons::Up);
    TextButton downButton = TextButton(Icons::Down);
    
    TextButton addButton = TextButton(Icons::Add);
    TextButton removeButton = TextButton(Icons::Clear);
    TextButton resetButton = TextButton(Icons::Refresh);
    TextButton changeButton = TextButton(Icons::Edit);
    
    ValueTree tree;
    StringArray items;
    
    void externalChange()
    {
        path = FileSearchPath();
        
        for (auto child : tree) {
            if (child.hasType("Path")) {
                path.addIfNotAlreadyThere(File(child.getProperty("Path").toString()));
            }
        }
        
        listBox.updateContent();
        listBox.repaint();
        updateButtons();
    }
    void internalChange()
    {
        tree.removeAllChildren(nullptr);
        
        for (int p = 0; p < path.getNumPaths(); p++) {
            auto dir = path[p];
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
        
        removeButton.setEnabled(anythingSelected);
        changeButton.setEnabled(anythingSelected);
        upButton.setEnabled(anythingSelected);
        downButton.setEnabled(anythingSelected);
    }
    
    void addPath()
    {
        auto start = defaultBrowseTarget;
        
        if (start == File())
            start = path[0];
        
        if (start == File())
            start = File::getCurrentWorkingDirectory();
        
        chooser = std::make_unique<FileChooser>(TRANS("Add a folder..."), start, "*");
        auto chooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories;
        
        chooser->launchAsync(chooserFlags,
                             [this](FileChooser const& fc) {
            if (fc.getResult() == File {})
                return;
            
            path.add(fc.getResult(), listBox.getSelectedRow());
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
        
        if (isPositiveAndBelow(currentRow, path.getNumPaths())) {
            auto newRow = jlimit(0, path.getNumPaths() - 1, currentRow + delta);
            
            if (currentRow != newRow) {
                auto f = path[currentRow];
                path.remove(currentRow);
                path.add(f, newRow);
                listBox.selectRow(newRow);
                internalChange();
            }
        }
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SearchPathComponent)
};
