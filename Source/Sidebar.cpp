/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <JuceHeader.h>
#include "Pd/PdInstance.h"
#include "LookAndFeel.h"
#include "PluginProcessor.h"

#include "Sidebar.h"

// MARK: Document Browser

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


// MARK: Inspector

struct Inspector : public PropertyPanel
{
    void paint(Graphics& g) override
    {
        
        g.setColour(findColour(PlugDataColour::toolbarColourId));
        g.fillRect(getLocalBounds().withHeight(getTotalContentHeight()));
    }
    
    void loadParameters(ObjectParameters& params)
    {
        StringArray names = {"General", "Appearance", "Label", "Extra"};
        
        clear();
        
        auto createPanel = [](int type, const String& name, Value* value, int idx, std::vector<String>& options) -> PropertyComponent*
        {
            switch (type)
            {
                case tString:
                    return new EditableComponent<String>(name, *value, idx);
                case tFloat:
                    return new EditableComponent<float>(name, *value, idx);
                case tInt:
                    return new EditableComponent<int>(name, *value, idx);
                case tColour:
                    return new ColourComponent(name, *value, idx);
                case tBool:
                    return new BoolComponent(name, *value, idx, options);
                case tCombo:
                    return new ComboComponent(name, *value, idx, options);
                default:
                    return new EditableComponent<String>(name, *value, idx);
            }
        };
        
        for (int i = 0; i < 4; i++)
        {
            Array<PropertyComponent*> panels;
            
            int idx = 0;
            for (auto& [name, type, category, value, options] : params)
            {
                if (static_cast<int>(category) == i)
                {
                    panels.add(createPanel(type, name, value, idx, options));
                    idx++;
                }
            }
            if (!panels.isEmpty())
            {
                addSection(names[i], panels);
            }
        }
    }
    
    struct InspectorProperty : public PropertyComponent
    {
        int idx;
        
        InspectorProperty(const String& propertyName, int i) : PropertyComponent(propertyName, 23), idx(i)
        {
        }
        
        void paint(Graphics& g) override
        {
            auto bg = idx & 1 ? findColour(PlugDataColour::toolbarColourId) : findColour(PlugDataColour::canvasColourId);
            
            g.fillAll(bg);
            getLookAndFeel().drawPropertyComponentLabel(g, getWidth(), getHeight() * 0.9, *this);
        }
        
        void refresh() override{};
    };
    
    struct ComboComponent : public InspectorProperty
    {
        ComboComponent(const String& propertyName, Value& value, int idx, std::vector<String> options) : InspectorProperty(propertyName, idx)
        {
            for (int n = 0; n < options.size(); n++)
            {
                comboBox.addItem(options[n], n + 1);
            }
            
            comboBox.setName("inspector:combo");
            comboBox.getSelectedIdAsValue().referTo(value);
            
            addAndMakeVisible(comboBox);
        }
        
        void resized() override
        {
            comboBox.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }
        
    private:
        ComboBox comboBox;
    };
    
    struct BoolComponent : public InspectorProperty
    {
        BoolComponent(const String& propertyName, Value& value, int idx, std::vector<String> options) : InspectorProperty(propertyName, idx)
        {
            toggleButton.setClickingTogglesState(true);
            
            toggleButton.setConnectedEdges(12);
            
            toggleButton.getToggleStateValue().referTo(value);
            toggleButton.setButtonText(static_cast<bool>(value.getValue()) ? options[1] : options[0]);
            
            toggleButton.setName("inspector:toggle");
            
            addAndMakeVisible(toggleButton);
            
            toggleButton.onClick = [this, value, options]() mutable { toggleButton.setButtonText(toggleButton.getToggleState() ? options[1] : options[0]); };
        }
        
        void resized() override
        {
            toggleButton.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }
        
    private:
        TextButton toggleButton;
    };
    
    struct ColourComponent : public InspectorProperty, public ChangeListener
    {
        ColourComponent(const String& propertyName, Value& value, int idx) : InspectorProperty(propertyName, idx), currentColour(value)
        {
            String strValue = currentColour.toString();
            if (strValue.length() > 2)
            {
                button.setButtonText(String("#") + strValue.substring(2).toUpperCase());
            }
            button.setConnectedEdges(12);
            button.setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            
            addAndMakeVisible(button);
            updateColour();
            
            button.onClick = [this]()
            {
                std::unique_ptr<ColourSelector> colourSelector = std::make_unique<ColourSelector>(ColourSelector::showColourAtTop | ColourSelector::showSliders | ColourSelector::showColourspace);
                colourSelector->setName("background");
                colourSelector->setCurrentColour(findColour(TextButton::buttonColourId));
                colourSelector->addChangeListener(this);
                colourSelector->setSize(300, 400);
                colourSelector->setColour(ColourSelector::backgroundColourId, findColour(PlugDataColour::toolbarColourId));
                
                colourSelector->setCurrentColour(Colour::fromString(currentColour.toString()));
                
                CallOutBox::launchAsynchronously(std::move(colourSelector), button.getScreenBounds(), nullptr);
            };
        }
        
        void updateColour()
        {
            auto colour = Colour::fromString(currentColour.toString());
            
            button.setColour(TextButton::buttonColourId, colour);
            button.setColour(TextButton::buttonOnColourId, colour.brighter());
            
            auto textColour = colour.getPerceivedBrightness() > 0.5 ? Colours::black : Colours::white;
            
            // make sure text is readable
            button.setColour(TextButton::textColourOffId, textColour);
            button.setColour(TextButton::textColourOnId, textColour);
            
            button.setButtonText(String("#") + currentColour.toString().substring(2).toUpperCase());
        }
        
        void changeListenerCallback(ChangeBroadcaster* source) override
        {
            auto* cs = dynamic_cast<ColourSelector*>(source);
            
            auto colour = cs->getCurrentColour();
            currentColour = colour.toString();
            
            updateColour();
        }
        
        ~ColourComponent() override = default;
        
        void resized() override
        {
            button.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }
        
    private:
        TextButton button;
        Value& currentColour;
    };
    
    template <typename T>
    struct EditableComponent : public InspectorProperty
    {
        Label label;
        Value& property;
        
        float dragValue = 0.0f;
        bool shift = false;
        int decimalDrag = 0;
        
        Point<float> lastDragPos;
        
        EditableComponent(String propertyName, Value& value, int idx) : InspectorProperty(propertyName, idx), property(value)
        {
            addAndMakeVisible(label);
            label.setEditable(true, false);
            label.getTextValue().referTo(property);
            label.addMouseListener(this, true);
            
            label.setFont(Font(14));
            
            label.onEditorShow = [this]()
            {
                auto* editor = label.getCurrentTextEditor();
                
                if constexpr (std::is_floating_point<T>::value)
                {
                    editor->setInputRestrictions(0, "0123456789.-");
                }
                else if constexpr (std::is_integral<T>::value)
                {
                    editor->setInputRestrictions(0, "0123456789-");
                }
            };
        }
        
        void mouseDown(const MouseEvent& e) override
        {
            if constexpr (!std::is_arithmetic<T>::value) return;
            if (label.isBeingEdited()) return;
            
            shift = e.mods.isShiftDown();
            dragValue = label.getText().getFloatValue();
            
            lastDragPos = e.position;
            
            const auto textArea = label.getBorderSize().subtractedFrom(label.getBounds());
            
            GlyphArrangement glyphs;
            glyphs.addFittedText(label.getFont(), label.getText(), textArea.getX(), 0., textArea.getWidth(), getHeight(), Justification::centredLeft, 1, label.getMinimumHorizontalScale());
            
            double decimalX = getWidth();
            for (int i = 0; i < glyphs.getNumGlyphs(); ++i)
            {
                auto const& glyph = glyphs.getGlyph(i);
                if (glyph.getCharacter() == '.')
                {
                    decimalX = glyph.getRight();
                }
            }
            
            bool isDraggingDecimal = e.x > decimalX;
            
            if constexpr (std::is_integral<T>::value) isDraggingDecimal = false;
            
            decimalDrag = isDraggingDecimal ? 6 : 0;
            
            if (isDraggingDecimal)
            {
                GlyphArrangement decimalsGlyph;
                static const String decimalStr("000000");
                
                decimalsGlyph.addFittedText(label.getFont(), decimalStr, decimalX, 0, getWidth(), getHeight(), Justification::centredLeft, 1, label.getMinimumHorizontalScale());
                
                for (int i = 0; i < decimalsGlyph.getNumGlyphs(); ++i)
                {
                    auto const& glyph = decimalsGlyph.getGlyph(i);
                    if (e.x <= glyph.getRight())
                    {
                        decimalDrag = i + 1;
                        break;
                    }
                }
            }
        }
        
        void mouseUp(const MouseEvent& e) override
        {
            setMouseCursor(MouseCursor::NormalCursor);
            updateMouseCursor();
        }
        
        void mouseDrag(const MouseEvent& e) override
        {
            if constexpr (!std::is_arithmetic<T>::value) return;
            if (label.isBeingEdited()) return;
            
            setMouseCursor(MouseCursor::NoCursor);
            updateMouseCursor();
            
            const int decimal = decimalDrag + e.mods.isShiftDown();
            const float increment = (decimal == 0) ? 1. : (1. / std::pow(10., decimal));
            const float deltaY = e.y - lastDragPos.y;
            lastDragPos = e.position;
            
            dragValue += increment * -deltaY;
            
            // truncate value and set
            double newValue = dragValue;
            
            if (decimal > 0)
            {
                const int sign = (newValue > 0) ? 1 : -1;
                unsigned int ui_temp = (newValue * std::pow(10, decimal)) * sign;
                newValue = (((double)ui_temp) / std::pow(10, decimal) * sign);
            }
            else
            {
                newValue = static_cast<int64_t>(newValue);
            }
            
            label.setText(formatNumber(newValue), NotificationType::sendNotification);
        }
        
        String formatNumber(float value)
        {
            String text;
            text << value;
            if (!text.containsChar('.')) text << '.';
            return text;
        }
        
        void resized() override
        {
            label.setBounds(getLocalBounds().removeFromRight(getWidth() / 2));
        }
    };
};

// MARK: Console
struct Console : public Component
{
    explicit Console(pd::Instance* instance)
    {
        // Viewport takes ownership
        console = new ConsoleComponent(instance, buttons, viewport);
        
        addComponentListener(console);
        
        viewport.setViewedComponent(console);
        viewport.setScrollBarsShown(true, false);
        
        console->setVisible(true);
        
        addAndMakeVisible(viewport);
        
        std::vector<String> tooltips = {"Clear logs", "Restore logs", "Show errors", "Show messages", "Enable autoscroll"};
        
        std::vector<std::function<void()>> callbacks = {
            [this]() { console->clear(); }, [this]() { console->restore(); }, [this]() {
                console->update(); }, [this]() {
                    console->update(); }, [this]() {
                        console->update(); },
            
        };
        
        int i = 0;
        for (auto& button : buttons)
        {
            button.setName("statusbar:console");
            button.setConnectedEdges(12);
            addAndMakeVisible(button);
            
            button.onClick = callbacks[i];
            button.setTooltip(tooltips[i]);
            
            i++;
        }
        
        for (int n = 2; n < 5; n++)
        {
            buttons[n].setClickingTogglesState(true);
            buttons[n].setToggleState(true, sendNotification);
        }
        
        resized();
    }
    
    ~Console() override
    {
        removeComponentListener(console);
    }
    
    void resized() override
    {
        FlexBox fb;
        fb.flexWrap = FlexBox::Wrap::noWrap;
        fb.justifyContent = FlexBox::JustifyContent::flexStart;
        fb.alignContent = FlexBox::AlignContent::flexStart;
        fb.flexDirection = FlexBox::Direction::row;
        
        for (auto& b : buttons)
        {
            auto item = FlexItem(b).withMinWidth(8.0f).withMinHeight(8.0f).withMaxHeight(27);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add(item);
        }
        
        auto bounds = getLocalBounds().toFloat();
        
        fb.performLayout(bounds.removeFromBottom(28));
        viewport.setBounds(bounds.toNearestInt());
        console->resized();
    }
    
    void update()
    {
        console->update();
    }
    
    void deselect()
    {
        console->selectedItem = -1;
        repaint();
    }
    
    struct ConsoleComponent : public Component, public ComponentListener
    {
        std::array<TextButton, 5>& buttons;
        Viewport& viewport;
        
        pd::Instance* pd;  // instance to get console messages from
        
        ConsoleComponent(pd::Instance* instance, std::array<TextButton, 5>& b, Viewport& v) : buttons(b), viewport(v), pd(instance)
        {
            setWantsKeyboardFocus(true);
            update();
        }
        
        void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override
        {
            setSize(viewport.getWidth(), getHeight());
            repaint();
        }
        
        void focusLost(FocusChangeType cause) override
        {
            selectedItem = -1;
        }
        
        bool keyPressed(const KeyPress& key) override
        {
            if (isPositiveAndBelow(selectedItem, pd->consoleMessages.size()))
            {
                // Copy console item
                if (key == KeyPress('c', ModifierKeys::commandModifier, 0))
                {
                    SystemClipboard::copyTextToClipboard(pd->consoleMessages[selectedItem].first);
                    return true;
                }
            }
            
            return false;
        }
        
        void update()
        {
            repaint();
            setSize(viewport.getWidth(), std::max<int>(getTotalHeight(), viewport.getHeight()));
            
            if (buttons[4].getToggleState())
            {
                viewport.setViewPositionProportionately(0.0f, 1.0f);
            }
        }
        
        void clear()
        {
            pd->consoleHistory.insert(pd->consoleHistory.end(), pd->consoleMessages.begin(), pd->consoleMessages.end());
            
            pd->consoleMessages.clear();
            
            update();
        }
        
        void restore()
        {
            pd->consoleMessages.insert(pd->consoleMessages.begin(), pd->consoleHistory.begin(), pd->consoleHistory.end());
            pd->consoleHistory.clear();
            update();
        }
        
        void mouseDown(const MouseEvent& e) override
        {
            int totalHeight = 0;
            
            for (int row = 0; row < static_cast<int>(pd->consoleMessages.size()); row++)
            {
                auto& message = pd->consoleMessages[row];
                
                int numLines = getNumLines(message.first, getWidth());
                int height = numLines * 22 + 2;
                
                const Rectangle<int> r(0, totalHeight, getWidth(), height);
                
                if (r.contains(e.getPosition() + viewport.getViewPosition()))
                {
                    selectedItem = row;
                    repaint();
                    break;
                }
                
                totalHeight += height;
            }
        }
        
        void paint(Graphics& g) override
        {
            auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
            g.setFont(font);
            g.fillAll(findColour(PlugDataColour::toolbarColourId));
            
            int totalHeight = 0;
            bool showMessages = buttons[2].getToggleState();
            bool showErrors = buttons[3].getToggleState();
            
            bool rowColour = false;
            
            for (int row = 0; row < static_cast<int>(pd->consoleMessages.size()); row++)
            {
                auto& message = pd->consoleMessages[row];
                
                int numLines = getNumLines(message.first, getWidth());
                int height = numLines * 22 + 2;
                
                const Rectangle<int> r(2, totalHeight, getWidth(), height);
                
                if ((message.second == 1 && !showMessages) || (message.second == 0 && !showErrors))
                {
                    continue;
                }
                
                if (rowColour || row == selectedItem)
                {
                    g.setColour(selectedItem == row ? findColour(PlugDataColour::highlightColourId) : findColour(ResizableWindow::backgroundColourId));
                    
                    g.fillRect(r);
                }
                
                rowColour = !rowColour;
                
                g.setColour(selectedItem == row ? Colours::white : colourWithType(message.second));
                g.drawFittedText(message.first, r.reduced(4, 0), Justification::centredLeft, numLines, 1.0f);
                
                totalHeight += height;
            }
            
            while (totalHeight < viewport.getHeight())
            {
                if (rowColour)
                {
                    const Rectangle<int> r(0, totalHeight, getWidth(), 24);
                    
                    g.setColour(findColour(ResizableWindow::backgroundColourId));
                    g.fillRect(r);
                }
                rowColour = !rowColour;
                totalHeight += 24;
            }
        }
        
        // Get total height of messages, also taking multi-line messages into account
        int getTotalHeight()
        {
            bool showMessages = buttons[2].getToggleState();
            bool showErrors = buttons[3].getToggleState();
            
            auto font = Font(Font::getDefaultSansSerifFontName(), 13, 0);
            int totalHeight = 0;
            
            int numEmpty = 0;
            
            for (auto& message : pd->consoleMessages)
            {
                int numLines = getNumLines(message.first, getWidth());
                int height = numLines * 22 + 2;
                
                if ((message.second == 1 && !showMessages) || (message.second == 0 && !showErrors)) continue;
                
                totalHeight += height;
            }
            
            totalHeight -= numEmpty * 24;
            
            return totalHeight;
        }
        
        void resized() override
        {
            update();
        }
        
        int selectedItem = -1;
        
    private:
        Colour colourWithType(int type)
        {
            if (type == 0)
                return findColour(ComboBox::textColourId);
            else if (type == 1)
                return Colours::orange;
            else
                return Colours::red;
        }
        
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConsoleComponent)
    };
    
    
    
private:
    ConsoleComponent* console;
    Viewport viewport;
    
    std::array<TextButton, 5> buttons = {TextButton(Icons::Clear), TextButton(Icons::Restore), TextButton(Icons::Error), TextButton(Icons::Message), TextButton(Icons::AutoScroll)};
};

Sidebar::Sidebar(PlugDataAudioProcessor* instance)
{
    // Can't use RAII because unique pointer won't compile with forward declarations
    console = new Console(instance);
    inspector = new Inspector;
    browser = new DocumentBrowser(instance);
    
    addAndMakeVisible(console);
    addAndMakeVisible(inspector);
    addChildComponent(browser);
    
    browser->setAlwaysOnTop(true);
    browser->addMouseListener(this, true);
    
    setBounds(getParentWidth() - lastWidth, 40, lastWidth, getParentHeight() - 40);
}

Sidebar::~Sidebar()
{
    browser->removeMouseListener(this);
    delete console;
    delete inspector;
    delete browser;
}

void Sidebar::paint(Graphics& g)
{
    int sWidth = sidebarHidden ? dragbarWidth : std::max(dragbarWidth, getWidth());
    
    // Sidebar
    g.setColour(findColour(PlugDataColour::toolbarColourId));
    g.fillRect(getWidth() - sWidth, 0, sWidth, getHeight() - 28);
    
    // Draggable bar
    g.setColour(findColour(PlugDataColour::toolbarColourId));
    g.fillRect(getWidth() - sWidth, 0, dragbarWidth + 1, getHeight());
    
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);
}

void Sidebar::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromLeft(dragbarWidth);
    
    console->setBounds(bounds);
    inspector->setBounds(bounds);
    browser->setBounds(getLocalBounds());
}

void Sidebar::mouseDown(const MouseEvent& e)
{
    Rectangle<int> dragBar(0, dragbarWidth, getWidth(), getHeight());
    if (dragBar.contains(e.getPosition()) && !sidebarHidden)
    {
        draggingSidebar = true;
        dragStartWidth = getWidth();
    }
    else
    {
        draggingSidebar = false;
    }
}

void Sidebar::mouseDrag(const MouseEvent& e)
{
    if (draggingSidebar)
    {
        int newWidth = dragStartWidth - e.getDistanceFromDragStartX();
        newWidth = std::min(newWidth, getParentWidth() / 2);
        
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
        getParentComponent()->resized();
    }
    
}

void Sidebar::mouseUp(const MouseEvent& e)
{
    if (draggingSidebar)
    {
        // getCurrentCanvas()->checkBounds(); fix this
        draggingSidebar = false;
    }
}

void Sidebar::mouseMove(const MouseEvent& e)
{
    if(e.getPosition().getX() < dragbarWidth) {
        setMouseCursor(MouseCursor::LeftRightResizeCursor);
    }
    else {
        setMouseCursor(MouseCursor::NormalCursor);
    }
}

void Sidebar::mouseExit(const MouseEvent& e)
{
    setMouseCursor(MouseCursor::NormalCursor);
}



void Sidebar::showBrowser(bool show)
{
    browser->setVisible(show);
    pinned = show;
}

bool Sidebar::isShowingBrowser() {
    return browser->isVisible();
};

void Sidebar::showSidebar(bool show)
{
    sidebarHidden = !show;
    
    if (!show)
    {
        lastWidth = getWidth();
        int newWidth = dragbarWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
    else
    {
        int newWidth = lastWidth;
        setBounds(getParentWidth() - newWidth, getY(), newWidth, getHeight());
    }
}

void Sidebar::pinSidebar(bool pin)
{
    pinned = pin;
    
    if (!pinned && lastParameters.empty())
    {
        hideParameters();
    }
}

bool Sidebar::isPinned()
{
    return pinned;
}

void Sidebar::showParameters(ObjectParameters& params)
{
    lastParameters = params;
    inspector->loadParameters(params);
    
    if (!pinned)
    {
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}

void Sidebar::showParameters()
{
    inspector->loadParameters(lastParameters);
    
    if (!pinned)
    {
        browser->setVisible(false);
        inspector->setVisible(true);
        console->setVisible(false);
    }
}
void Sidebar::hideParameters()
{
    
    if (!pinned)
    {
        inspector->setVisible(false);
        console->setVisible(true);
    }
    
    if (pinned)
    {
        ObjectParameters params = {};
        inspector->loadParameters(params);
    }
    
    console->deselect();
}

bool Sidebar::isShowingConsole() const noexcept
{
    return console->isVisible();
}

void Sidebar::updateConsole()
{
    console->update();
}
