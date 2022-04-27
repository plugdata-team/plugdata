/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

class FileSearchComponent : public Component, public TableListBoxModel
{
    
public:
    FileSearchComponent(DirectoryContentsList& directory) : searchPath(directory)
    {
        table.setModel(this);
        table.setColour(ListBox::backgroundColourId, findColour(ResizableWindow::backgroundColourId));
        table.setRowHeight(25);
        table.setOutlineThickness(0);
        table.deselectAllRows();
        
        table.getHeader().setStretchToFitActive(true);
        table.setHeaderHeight(0);
        table.getHeader().addColumn("Results", 1, 800, 50, 800, TableHeaderComponent::defaultFlags);
        
        table.getViewport()->setScrollBarsShown(true, false, false, false);
        
        input.setName("sidebar::searcheditor");
        
        input.onTextChange = [this](){
            bool notEmpty = input.getText().isNotEmpty();
            table.setVisible(notEmpty);
            setInterceptsMouseClicks(notEmpty, true);
            updateResults(input.getText());
        };
        
        closeButton.setName("statusbar:clearsearch");
        closeButton.onClick = [this](){
            input.clear();
            input.giveAwayKeyboardFocus();
            table.setVisible(false);
            setInterceptsMouseClicks(false, true);
            input.repaint();
        };
        
        closeButton.setAlwaysOnTop(true);
        
        addAndMakeVisible(closeButton);
        addAndMakeVisible(table);
        addAndMakeVisible(input);
        
        table.addMouseListener(this, true);
        table.setVisible(false);
        

        input.setJustification(Justification::centredLeft);
        input.setBorder({1, 23, 3, 1});
        
        setInterceptsMouseClicks(false, true);
    }
    
    void mouseDoubleClick(const MouseEvent& e) override
    {
        int row = table.getSelectedRow();
        
        if(isPositiveAndBelow(row, searchResult.size())) {
            if(table.getRowPosition(row, true).contains(e.getEventRelativeTo(&table).getPosition())) {
                openFile(searchResult.getReference(row));
            }
        }
    }
    
    void paintOverChildren(Graphics& g) override {
        g.setFont(getLookAndFeel().getTextButtonFont(closeButton, 30));
        g.setColour(findColour(PlugDataColour::textColourId));
        
        g.drawText(Icons::Search, 0, 0, 30, 30, Justification::centred);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 28, getWidth(), 28);
        g.drawLine(0.5f, 0, 0.5f, getHeight());
    }
    
    
    // This is overloaded from TableListBoxModel, and should fill in the background of the whole row
    void paintRowBackground(Graphics& g, int row, int w, int h, bool rowIsSelected) override
    {
        if (rowIsSelected)
        {
            g.setColour(findColour(PlugDataColour::highlightColourId));
        }
        else
        {
            g.setColour(findColour(row & 1 ? PlugDataColour::canvasColourId : PlugDataColour::toolbarColourId));
        }
        
        g.fillRect(1, 0, w - 3, h);
    }
    
    // Overloaded from TableListBoxModel
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
    {
        g.setColour(rowIsSelected ? Colours::white : findColour(ComboBox::textColourId));
        const String item = searchResult[rowNumber].getFileName();
        
        g.setFont(Font());
        g.drawText(item, 24, 0, width - 4, height, Justification::centredLeft, true);
        
        g.setFont(getLookAndFeel().getTextButtonFont(closeButton, 23));
        g.drawText(Icons::File, 8, 2, 24, 24, Justification::centredLeft);
    }
    
    int getNumRows() override
    {
        return searchResult.size();
    }
    
    Component* refreshComponentForCell(int rowNumber, int columnId, bool /*isRowSelected*/, Component* existingComponentToUpdate) override
    {
        delete existingComponentToUpdate;
        return nullptr;
    }
    
    void clearSearchResults() {
        searchResult.clear();
    }
    
    
    void updateResults(String query) {
        clearSearchResults();
        
        if(query.isEmpty()) return;
        
        auto addFile = [this, &query](const File& file){
            
            auto fileName = file.getFileName();
            
            if(!file.hasFileExtension("pd")) return;
            
            // Insert in front if the query matches a whole word
            if(fileName.containsWholeWordIgnoreCase(query)) {
                searchResult.insert(0, file);
            }
            // Insert in back if it contains the query
            else if(fileName.containsIgnoreCase(query)) {
                searchResult.add(file);
            }
        };
        
        for(int i = 0; i < searchPath.getNumFiles(); i++) {
            auto file = searchPath.getFile(i);
            
            if(file.isDirectory()) {
                for(auto& child : RangedDirectoryIterator(file, true)) {
                    addFile(child.getFile());
                }
            }
            else {
                addFile(file);
            }
        }
        
        table.updateContent();
        
        if(table.getSelectedRow() == -1) table.selectRow(0, true, true);
    }
    
    bool isSearchingAndHasSelection() {
        return table.isVisible() && isPositiveAndBelow(table.getSelectedRow(), searchResult.size());
    }
    
    File getSelection() {
        int row = table.getSelectedRow();
        
        if(isPositiveAndBelow(row, searchResult.size())) {
            return searchResult[row];
        }
    }
    
    
    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(28);
        
        input.setBounds(inputBounds);
        
        closeButton.setBounds(inputBounds.removeFromRight(30));
        
        tableBounds.removeFromLeft(Sidebar::dragbarWidth);
        table.setBounds(tableBounds);
    }
    
    std::function<void(File&)> openFile;
    
private:
    
    TableListBox table;
    
    DirectoryContentsList& searchPath;
    Array<File> searchResult;
    TextEditor input;
    TextButton closeButton = TextButton(Icons::Clear);
};

struct DocumentBrowser : public Component, public Timer
{
    
    struct FileTree : public FileTreeComponent, public FileBrowserListener, public ScrollBar::Listener
    {
        
        FileTree(DirectoryContentsList& directory, DocumentBrowser* parent) : FileTreeComponent(directory), browser(parent) {
            
            setItemHeight(23);
            
            addListener(this);
            getViewport()->getVerticalScrollBar().addListener(this);
        }
        
        // Paint file drop outline
        void paintOverChildren(Graphics& g) override
        {
            if(isDraggingFile) {
                g.setColour(findColour(PlugDataColour::highlightColourId));
                g.drawRect(getLocalBounds().reduced(1), 2.0f);
            }
        }
        
        void scrollBarMoved (ScrollBar *scrollBarThatHasMoved, double newRangeStart) override
        {
            repaint();
        }
        
        void paint(Graphics& g) override {
            
            // Paint the rest of the stripes and make sure that the stripes extend to the left
            int itemHeight = getItemHeight();
            int totalHeight = std::max(getViewport()->getViewedComponent()->getHeight(), getHeight());
            for(int i = 0; i < (totalHeight / itemHeight) + 1; i++)
            {
                
                int y = i * itemHeight - getViewport()->getViewPositionY();
                int height = itemHeight;
                
                if(y + itemHeight > getHeight() - 28) {
                    height = (getHeight() - 28) - (y - itemHeight);
                }
                if(height <= 0) break;
                
                if(getNumSelectedItems() == 1 && getSelectedItem(0) == getItemOnRow(i)) {
                    g.setColour(findColour(PlugDataColour::highlightColourId));
                }
                else {
                    g.setColour(findColour(i & 1 ? PlugDataColour::canvasColourId : PlugDataColour::toolbarColourId));
                }
                
                g.fillRect(0, y, getWidth(), height);
            }
        }

        
        /** Callback when the user double-clicks on a file in the browser. */
        void fileDoubleClicked (const File& file) override {
            browser->pd->loadPatch(file);
        }
        void selectionChanged() override {
            browser->repaint();
        };
        void fileClicked (const File&, const MouseEvent&) override {};
        void browserRootChanged (const File&) override {};
        
        bool isInterestedInFileDrag (const StringArray &files) override {
           
            if(!browser->isVisible()) return false;
            
            if(browser->searchComponent.isSearchingAndHasSelection()) {
                return false;
            }
            
            for(auto& path : files) {
                auto file = File(path);
                if(file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
                    return true;
                }
            }
        }

        void filesDropped (const StringArray &files, int x, int y) override
        {
            for(auto& path : files) {
                auto file = File(path);
                
                if(file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
                    auto alias = browser->directory.getDirectory().getChildFile(file.getFileName());
                    
                    if(alias.exists()) alias.deleteFile();
                    File::createSymbolicLink(alias, file.getFullPathName(), false);
                }
            }
            
            browser->timerCallback();
            
            isDraggingFile = false;
            repaint();
        }

        void fileDragEnter (const StringArray&, int, int) override
        {
            isDraggingFile = true;
            repaint();
        }

        void fileDragExit (const StringArray&) override
        {
            isDraggingFile = false;
            repaint();
        }
        
    private:
        DocumentBrowser* browser;
        bool isDraggingFile = false;
        
    };
    
    DocumentBrowser(PlugDataAudioProcessor* processor) :
    filter("*.pd", "*", "pure-data files"),
    updateThread("browserThread"),
    directory(&filter, updateThread),
    fileList(directory, this),
    pd(processor),
    searchComponent(directory)
    {
        auto location = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");
        
        auto customLocation = File(pd->settingsTree.getProperty("BrowserPath"));
        if(customLocation.exists()) {
            location = customLocation;
        }
        
        directory.setDirectory(location, true, true);
        
        updateThread.startThread();
        timerCallback();
        
        addAndMakeVisible(fileList);

        searchComponent.openFile = [this](File& file){
            if(file.existsAsFile()) {
                pd->loadPatch(file);
            }
        };
        
        
        revealButton.setName("statusbar:reveal");
        loadFolderButton.setName("statusbar:browserpathopen");
        resetFolderButton.setName("statusbar:browserpathreset");
        
#if JUCE_MAC
        String revealTip = "Show in Finder";
#elif JUCE_WINDOWS
        String revealTip = "Show in Explorer";
#else
        String revealTip = "Show in file browser";
#endif
        
        revealButton.setTooltip(revealTip);
        addAndMakeVisible(revealButton);
        addAndMakeVisible(loadFolderButton);
        addAndMakeVisible(resetFolderButton);
        
        loadFolderButton.onClick = [this](){
            
            openChooser = std::make_unique<FileChooser>("Open...",  directory.getDirectory().getFullPathName(), "", true);
            
            openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                                     [this](FileChooser const& fileChooser)
                                     {
                auto const file = fileChooser.getResult();
                if(file.exists()) {
                    auto path =  file.getFullPathName();
                    pd->settingsTree.setProperty("BrowserPath", path, nullptr);
                    directory.setDirectory(path, true, true);
                }
            });
        };
        
        resetFolderButton.onClick = [this](){
            auto location = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData");
            auto path =  location.getFullPathName();
            pd->settingsTree.setProperty("BrowserPath", path, nullptr);
            directory.setDirectory(path, true, true);
        };
        
        revealButton.onClick = [this](){
            if(searchComponent.isSearchingAndHasSelection()) {
                searchComponent.getSelection().revealToUser();
            }
            else if(fileList.getSelectedFile().exists()) {
                fileList.getSelectedFile().revealToUser();
            }
        };
        
        addAndMakeVisible(searchComponent);
        
        
        if(!fileList.getSelectedFile().exists()) fileList.moveSelectedRow(1);
    }
    
    ~DocumentBrowser() {
        fileList.removeMouseListener(this);
        updateThread.stopThread(1000);
    }
    
    void visibilityChanged() override
    {
        if(isVisible()) {
            timerCallback();
            startTimer(500);
        }
        else {
            stopTimer();
        }
    }

    void timerCallback() override
    {
        if(directory.getDirectory().getLastModificationTime() > lastUpdateTime)
        {
            lastUpdateTime = directory.getDirectory().getLastModificationTime();
            directory.refresh();
            fileList.refresh();
            
            for(int i = 0; i < fileList.getNumRowsInTree(); i++) {
                auto* item = fileList.getItemOnRow(i);
                item->setDrawsInLeftMargin(true);
            }
            
        
        }
    }
    
    bool hitTest(int x, int y) override
    {
        if(x < 5) return false;
        
        return true;
    }
    
    
    void resized() override {
        fileList.setBounds(getLocalBounds().withHeight(getHeight() - 58).withY(30).withLeft(5));
        
        searchComponent.setBounds(getLocalBounds().withHeight(getHeight() - 28));
        
        FlexBox fb;
        fb.flexWrap = FlexBox::Wrap::noWrap;
        fb.justifyContent = FlexBox::JustifyContent::flexStart;
        fb.alignContent = FlexBox::AlignContent::flexStart;
        fb.flexDirection = FlexBox::Direction::row;
        
        Array<TextButton*> buttons = {&revealButton, &loadFolderButton, &resetFolderButton};
        
        for (auto* b : buttons)
        {
            auto item = FlexItem(*b).withMinWidth(8.0f).withMinHeight(8.0f).withMaxHeight(27);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add(item);
        }
        
        auto bounds = getLocalBounds().toFloat();
        
        fb.performLayout(bounds.removeFromBottom(28));
    }
    
    void paint(Graphics& g) override {
        g.fillAll(findColour(PlugDataColour::toolbarColourId));
        
        // Draggable bar
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRect(getWidth() - Sidebar::dragbarWidth, 0, Sidebar::dragbarWidth + 1, getHeight());
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0, 0, 0, getHeight() - 27.5f);
    }
    

private:
    TextButton revealButton = TextButton(Icons::OpenedFolder);
    TextButton loadFolderButton = TextButton(Icons::Folder);
    TextButton resetFolderButton = TextButton(Icons::Restore);
    
    std::unique_ptr<FileChooser> openChooser;
    PlugDataAudioProcessor* pd;
    
    WildcardFileFilter filter;
    TimeSliceThread updateThread;
    
    Time lastUpdateTime;
    
public:
    DirectoryContentsList directory;
    FileTree fileList;
    
    FileSearchComponent searchComponent;
};
