/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// This is based on juce_FileTreeComponent, but I decided to rewrite parts to:
// 1. Sort by folders first
// 2. Improve simplicity and efficiency by not using OS file icons (they look bad anyway)

#include <utility>

#include "Utility/OSUtils.h"
#include "Object.h"

class DocumentBrowserSettings : public Component {
public:
    struct DocumentBrowserSettingsButton : public TextButton {
        const String icon;
        const String description;

        DocumentBrowserSettingsButton(String iconString, String descriptionString)
            : icon(std::move(iconString))
            , description(std::move(descriptionString))
        {
        }

        void paint(Graphics& g) override
        {
            auto colour = findColour(PlugDataColour::toolbarTextColourId);
            if (isMouseOver()) {
                colour = colour.contrasting(0.3f);
            }

            Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(28), colour, 14);

            if (getToggleState()) {
                colour = findColour(PlugDataColour::toolbarActiveColourId);
            }

            Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(8), colour, 14, false);
        }
    };

    DocumentBrowserSettings(std::function<void()> const& chooseCustomLocation, std::function<void()> const& resetDefaultLocation)
    {
        addAndMakeVisible(customLocationButton);
        addAndMakeVisible(restoreLocationButton);

        customLocationButton.onClick = [chooseCustomLocation]() {
            chooseCustomLocation();
        };

        restoreLocationButton.onClick = [resetDefaultLocation]() {
            resetDefaultLocation();
        };

        setSize(180, 54);
    }

    void resized() override
    {
        auto buttonBounds = getLocalBounds();

        int buttonHeight = buttonBounds.getHeight() / 2;

        customLocationButton.setBounds(buttonBounds.removeFromTop(buttonHeight));
        restoreLocationButton.setBounds(buttonBounds.removeFromTop(buttonHeight));
    }

private:
    DocumentBrowserSettingsButton customLocationButton = DocumentBrowserSettingsButton(Icons::Open, "Show custom folder...");
    DocumentBrowserSettingsButton restoreLocationButton = DocumentBrowserSettingsButton(Icons::Restore, "Show default folder");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocumentBrowserSettings)
};

// Base classes for communication between parent and child classes
class DocumentBrowserViewBase : public TreeView
    , public DirectoryContentsDisplayComponent, public DragAndDropContainer {

public:
    explicit DocumentBrowserViewBase(DirectoryContentsList& listToShow)
        : DirectoryContentsDisplayComponent(listToShow)
        , bouncer(getViewport())
    {
    }

    bool shouldDropTextWhenDraggedExternally (const DragAndDropTarget::SourceDetails &sourceDetails, String &text) override
    {
        return false;
    }

    BouncingViewportAttachment bouncer;
};

class DocumentBrowserBase : public Component {

public:
    explicit DocumentBrowserBase(PluginProcessor* processor)
        : pd(processor)
        , updateThread("browserThread")
        , directory(&filter, updateThread)
        , filter("*", "*", "All files")
    {
    }

    virtual bool isSearching() = 0;

    PluginProcessor* pd;
    TimeSliceThread updateThread;
    DirectoryContentsList directory;
    WildcardFileFilter filter;
};

class DocumentBrowserItem : public TreeViewItem
    , private AsyncUpdater
    , private ChangeListener {
public:
    DocumentBrowserItem(DocumentBrowserViewBase& treeComp, DirectoryContentsList* parentContents, int indexInContents, int indexInParent, File f)
        : file(std::move(f))
        , owner(treeComp)
        , parentContentsList(parentContents)
        , subContentsList(nullptr, false)
    {
        DirectoryContentsList::FileInfo fileInfo;

        if (parentContents != nullptr && parentContents->getFileInfo(indexInContents, fileInfo)) {
            fileSize = File::descriptionOfSizeInBytes(fileInfo.fileSize);
            isDirectory = fileInfo.isDirectory;
        } else {
            isDirectory = true;
        }
    }

    ~DocumentBrowserItem() override
    {
        clearSubItems();
        removeSubContentsList();
    }

    void paintOpenCloseButton(Graphics& g, Rectangle<float> const& area, Colour backgroundColour, bool isMouseOver) override
    {
        Path p;
        p.startNewSubPath(0.0f, 0.0f);
        p.lineTo(0.5f, 0.5f);
        p.lineTo(isOpen() ? 1.0f : 0.0f, isOpen() ? 0.0f : 1.0f);

        auto arrowArea = area.reduced(5, 9).translated(4, 0).toFloat();

        if (!isOpen()) {
            arrowArea = arrowArea.reduced(1);
        }

        g.setColour(isSelected() ? getOwnerView()->findColour(PlugDataColour::sidebarActiveTextColourId) : getOwnerView()->findColour(PlugDataColour::sidebarTextColourId).withAlpha(isMouseOver ? 0.7f : 1.0f));
        g.strokePath(p, PathStrokeType(1.5f, PathStrokeType::curved, PathStrokeType::rounded), p.getTransformToScaleToFit(arrowArea, true));
    }

    bool mightContainSubItems() override
    {
        return isDirectory;
    }
    String getUniqueName() const override
    {
        return file.getFullPathName();
    }
    int getItemHeight() const override
    {
        return 26;
    }
    var getDragSourceDescription() override
    {
        if (file.existsAsFile()) {
            return var(String(file.getFileName()));
        }
        
        return var(String());
    }

    void itemOpennessChanged(bool isNowOpen) override
    {
        if (isNowOpen) {
            clearSubItems();

            isDirectory = file.isDirectory();

            if (isDirectory) {
                if (subContentsList == nullptr && parentContentsList != nullptr) {
                    auto l = new DirectoryContentsList(parentContentsList->getFilter(), parentContentsList->getTimeSliceThread());

                    l->setDirectory(file, parentContentsList->isFindingDirectories(), parentContentsList->isFindingFiles());

                    setSubContentsList(l, true);
                }

                changeListenerCallback(nullptr);
            }
        }

        setSelected(true, true, sendNotificationSync);
        owner.repaint();
    }

    void removeSubContentsList()
    {
        if (subContentsList != nullptr) {
            subContentsList->removeChangeListener(this);
            subContentsList.reset();
        }
    }

    void setSubContentsList(DirectoryContentsList* newList, bool const canDeleteList)
    {
        removeSubContentsList();

        subContentsList = OptionalScopedPointer<DirectoryContentsList>(newList, canDeleteList);
        newList->addChangeListener(this);
    }

    void changeListenerCallback(ChangeBroadcaster*) override
    {
        rebuildItemsFromContentList();
    }

    void rebuildItemsFromContentList()
    {
        clearSubItems();

        if (isOpen() && subContentsList != nullptr) {
            int idx = 0;
            // Sort by folders first
            for (int i = 0; i < subContentsList->getNumFiles(); ++i) {
                if (subContentsList->getFile(i).isDirectory() && !subContentsList->getFile(i).getFileName().startsWithChar('.')) {
                    addSubItem(new DocumentBrowserItem(owner, subContentsList, i, idx, subContentsList->getFile(i)));
                    idx++;
                }
            }
            for (int i = 0; i < subContentsList->getNumFiles(); ++i) {
                if (subContentsList->getFile(i).existsAsFile() && !subContentsList->getFile(i).getFileName().startsWithChar('.')) {
                    addSubItem(new DocumentBrowserItem(owner, subContentsList, i, idx, subContentsList->getFile(i)));
                    idx++;
                }
            }
        }
    }

    void paintItem(Graphics& g, int width, int height) override
    {
        int const x = 24;

        auto colour = isSelected() ? owner.findColour(PlugDataColour::sidebarActiveTextColourId) : owner.findColour(PlugDataColour::sidebarTextColourId);

        if (isDirectory) {
            Fonts::drawIcon(g, Icons::Folder, Rectangle<int>(6, 2, x - 4, height - 4), colour, 12, false);
        } else {
            Fonts::drawIcon(g, Icons::File, Rectangle<int>(6, 2, x - 4, height - 4), colour, 12, false);
        }

        Fonts::drawFittedText(g, file.getFileName(), x, 0, width - x, height, colour);
    }

    String getAccessibilityName() override
    {
        return file.getFileName();
    }

    bool selectFile(File const& target)
    {
        if (file == target) {
            setSelected(true, true);
            return true;
        }

        if (target.isAChildOf(file)) {
            setOpen(true);

            for (int maxRetries = 500; --maxRetries > 0;) {
                for (int i = 0; i < getNumSubItems(); ++i)
                    if (auto* f = dynamic_cast<DocumentBrowserItem*>(getSubItem(i)))
                        if (f->selectFile(target))
                            return true;

                // if we've just opened and the contents are still loading, wait for it..
                if (subContentsList != nullptr && subContentsList->isStillLoading()) {
                    Thread::sleep(10);
                    rebuildItemsFromContentList();
                } else {
                    break;
                }
            }
        }

        return false;
    }

    void itemClicked(MouseEvent const& e) override
    {
        owner.sendMouseClickMessage(file, e);
    }

    void itemDoubleClicked(MouseEvent const& e) override
    {
        TreeViewItem::itemDoubleClicked(e);

        owner.sendDoubleClickMessage(file);
    }

    void itemSelectionChanged(bool) override
    {
        owner.sendSelectionChangeMessage();
    }

    void handleAsyncUpdate() override
    {
        owner.repaint();
    }

    const File file;

private:
    DocumentBrowserViewBase& owner;
    DirectoryContentsList* parentContentsList;
    OptionalScopedPointer<DirectoryContentsList> subContentsList;
    bool isDirectory;
    String fileSize;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocumentBrowserItem)
};

class DocumentBrowserView : public DocumentBrowserViewBase
    , public FileBrowserListener
    , public ScrollBar::Listener
    , public Timer {
public:
    /** Creates a listbox to show the contents of a specified directory.
     */
    DocumentBrowserView(DirectoryContentsList& listToShow, DocumentBrowserBase* parent)
        : DocumentBrowserViewBase(listToShow)
        , browser(parent)
        , lastUpdateTime(listToShow.getDirectory().getLastModificationTime())
        , itemHeight(24)
    {
        setIndentSize(16);
        setRootItemVisible(false);
        refresh();
        addListener(this);
        getViewport()->getVerticalScrollBar().addListener(this);
        getViewport()->setScrollBarsShown(true, false);
        startTimer(1500);
    }

    void timerCallback() override
    {
        auto lastModificationTime = directoryContentsList.getDirectory().getLastModificationTime();
        if (lastModificationTime > lastUpdateTime) {
            refresh();
            lastUpdateTime = lastModificationTime;
        }
    }

    /** Destructor. */
    ~DocumentBrowserView() override
    {
        deleteRootItem();
    }

    /** Scrolls this view to the top. */
    void scrollToTop() override
    {
        getViewport()->getVerticalScrollBar().setCurrentRangeStart(0);
    }
    /** If the specified file is in the list, it will become the only selected item
        (and if the file isn't in the list, all other items will be deselected). */
    void setSelectedFile(File const& target) override
    {
        if (auto* t = dynamic_cast<DocumentBrowserItem*>(getRootItem()))
            if (!t->selectFile(target))
                clearSelectedItems();
    }

    /** Returns the number of files the user has got selected.
        @see getSelectedFile
    */
    int getNumSelectedFiles() const override
    {
        return TreeView::getNumSelectedItems();
    }

    /** Returns one of the files that the user has currently selected.
        The index should be in the range 0 to (getNumSelectedFiles() - 1).
        @see getNumSelectedFiles
    */
    File getSelectedFile(int index = 0) const override
    {
        if (auto* item = dynamic_cast<DocumentBrowserItem const*>(getSelectedItem(index)))
            return item->file;

        return {};
    }

    /** Deselects any files that are currently selected. */
    void deselectAllFiles() override
    {
        clearSelectedItems();
    }

    /** Updates the files in the list. */
    void refresh()
    {
        directoryContentsList.refresh();

        // Mouse events during update can cause a crash!
        setEnabled(false);

        // Prevents crash!
        setRootItemVisible(false);

        deleteRootItem();

        auto root = new DocumentBrowserItem(*this, nullptr, 0, 0, directoryContentsList.getDirectory());

        root->setSubContentsList(&directoryContentsList, false);
        setRootItem(root);

        setInterceptsMouseClicks(true, true);
        setEnabled(true);
    }

    void paint(Graphics& g) override
    {
        // Paint selected row
        if (getNumSelectedFiles()) {
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));

            auto* selectedItem = getSelectedItem(0);
            if (selectedItem == getRootItem())
                return;

            auto y = selectedItem->getItemPosition(true).getY();

            // Fix for bouncing viewport
            if (auto* holder = getViewport()->getChildComponent(0)) {
                y += holder->getTransform().getTranslationY();
            }

            auto selectedRect = Rectangle<float>(3.5f, y + 2.0f, getWidth() - 6.0f, 22.0f);

            PlugDataLook::fillSmoothedRectangle(g, selectedRect, Corners::defaultCornerRadius);
        }
    }
    // Paint file drop outline
    void paintOverChildren(Graphics& g) override
    {
        if (isDraggingFile) {
            g.setColour(findColour(PlugDataColour::scrollbarThumbColourId));
            g.drawRect(getLocalBounds().reduced(1), 2.0f);
        }
    }

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& d) override
    {
        bouncer.mouseWheelMove(e, d);
        repaint();
    }
        
    void dragOperationStarted (const DragAndDropTarget::SourceDetails &) override
    {
        DragAndDropContainer::performExternalDragDropOfFiles ({getSelectedFile().getFullPathName()}, false, this, nullptr);
        setCurrentDragImage(ScaledImage());
    }
        

    /** Callback when the user double-clicks on a file in the browser. */
    void fileDoubleClicked(File const& file) override
    {
        if (file.isDirectory()) {
            file.revealToUser();
        } else if (file.existsAsFile() && file.hasFileExtension("pd")) {
            browser->pd->loadPatch(file, findParentComponentOfClass<PluginEditor>());
            SettingsFile::getInstance()->addToRecentlyOpened(file);
            lastUpdateTime = Time::getCurrentTime() + RelativeTime(2.0f);
        } else if (file.existsAsFile()) {
            auto* editor = findParentComponentOfClass<PluginEditor>();
            if (auto* cnv = editor->getCurrentCanvas()) {
                auto lastPosition = cnv->viewport->getViewArea().getConstrainedPoint(cnv->getMouseXYRelative() - Point<int>(Object::margin, Object::margin));
                auto filePath = file.getFullPathName().replaceCharacter('\\', '/');
                cnv->objects.add(new Object(cnv, "msg " + filePath, lastPosition));
            }
        }
    }

    void selectionChanged() override
    {
        browser->repaint();
    }

    void fileClicked(File const&, MouseEvent const&) override { }
    void browserRootChanged(File const&) override { }

    bool isInterestedInFileDrag(StringArray const& files) override
    {
        if (!browser->isVisible())
            return false;

        if (browser->isSearching()) {
            return false;
        }

        for (auto& path : files) {
            auto file = File(path);
            if (file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
                return true;
            }
        }

        return false;
    }

    void filesDropped(StringArray const& files, int x, int y) override
    {
        for (auto& path : files) {
            auto file = File(path);

            if (file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
                auto alias = browser->directory.getDirectory().getChildFile(file.getFileName());

                if (alias.exists()) continue;
                
#if JUCE_WINDOWS
                // Symlinks on Windows are weird!
                if (file.isDirectory()) {

                    // Create NTFS directory junction
                    OSUtils::createJunction(alias.getFullPathName().replaceCharacters("/", "\\").toStdString(), file.getFullPathName().toStdString());
                } else {
                    // Create hard link
                    OSUtils::createHardLink(alias.getFullPathName().replaceCharacters("/", "\\").toStdString(), file.getFullPathName().toStdString());
                }

#else
                file.createSymbolicLink(alias, true);
#endif
            }
        }

        browser->directory.refresh();

        isDraggingFile = false;
        repaint();
    }

    void fileDragEnter(StringArray const&, int, int) override
    {
        isDraggingFile = true;
        repaint();
    }

    void fileDragExit(StringArray const&) override
    {
        isDraggingFile = false;
        repaint();
    }

protected:
    DocumentBrowserBase* browser;
    bool isDraggingFile = false;

    Time lastUpdateTime;
    int itemHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocumentBrowserView)
};

class FileSearchComponent : public Component
    , public ListBoxModel
    , public ScrollBar::Listener
    , public KeyListener {
public:
    explicit FileSearchComponent(DirectoryContentsList& directory)
        : bouncer(listBox.getViewport())
        , searchPath(directory)
    {
        listBox.setModel(this);
        listBox.setRowHeight(26);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();

        listBox.getViewport()->setScrollBarsShown(true, false, false, false);

        input.setBackgroundColour(PlugDataColour::sidebarActiveBackgroundColourId);
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

        input.setTextToShowWhenEmpty("Type to search documentation", findColour(PlugDataColour::sidebarTextColourId).withAlpha(0.5f));
        input.setInterceptsMouseClicks(true, true);

        addAndMakeVisible(listBox);
        addAndMakeVisible(input);

        listBox.addMouseListener(this, true);
        listBox.setVisible(false);

        input.setJustification(Justification::centredLeft);
        input.setBorder({ 1, 23, 5, 1 });

        listBox.setColour(ListBox::backgroundColourId, Colours::transparentBlack);

        listBox.getViewport()->getVerticalScrollBar().addListener(this);

        setInterceptsMouseClicks(false, true);

        lookAndFeelChanged();
        repaint();
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
            g.fillAll(findColour(PlugDataColour::sidebarBackgroundColourId));
        }

        g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
        g.fillRoundedRectangle(input.getBounds().reduced(6, 4).toFloat(), Corners::defaultCornerRadius);

    }
        
    void paintOverChildren(Graphics& g) override
    {
        auto backgroundColour =  findColour(PlugDataColour::sidebarBackgroundColourId);
        auto transparentColour = backgroundColour.withAlpha(0.0f);

        // Draw a gradient to fade the content out underneath the search input
        g.setGradientFill(ColourGradient(backgroundColour, 0.0f, 30.0f, transparentColour, 0.0f, 42.0f, false));
        g.fillRect(Rectangle<int>(0, input.getBottom(), getWidth(), 12));
        
        Fonts::drawIcon(g, Icons::Search, 2, 1, 32, findColour(PlugDataColour::sidebarTextColourId), 12);
    }

    void lookAndFeelChanged() override
    {
        input.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        input.setColour(TextEditor::textColourId, findColour(PlugDataColour::sidebarTextColourId));
        input.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int w, int h, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
            PlugDataLook::fillSmoothedRectangle(g, Rectangle<float>(5.5, 1, w - 11, h - 4), Corners::defaultCornerRadius);
        }

        auto colour = rowIsSelected ? findColour(PlugDataColour::sidebarActiveTextColourId) : findColour(ComboBox::textColourId);
        const String item = searchResult[rowNumber].getFileName();

        Fonts::drawText(g, item, h + 4, 0, w - 4, h, colour, 14);
        Fonts::drawIcon(g, Icons::File, 12, 0, h, colour, 12, false);
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

        auto addFile = [this, &query](File const& file) {
            auto fileName = file.getFileName();

            if (!file.hasFileExtension("pd"))
                return;

            if (fileName.containsIgnoreCase(query)) {
                searchResult.add(file);
            }
        };

        for (int i = 0; i < searchPath.getNumFiles(); i++) {
            auto file = searchPath.getFile(i);

            if (file.isDirectory()) {
                for (auto& child : OSUtils::iterateDirectory(file, true, false)) {
                    addFile(child);
                }
            } else {
                addFile(file);
            }
        }

        listBox.updateContent();
        listBox.repaint();

        if (listBox.getSelectedRow() == -1)
            listBox.selectRow(0, true, true);
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

        return {};
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(34);

        tableBounds.removeFromTop(2);

        input.setBounds(inputBounds.reduced(5, 4));
        listBox.setBounds(tableBounds.withTrimmedTop(1));
    }

    std::function<void(File&)> openFile;

private:
    ListBox listBox;
    BouncingViewportAttachment bouncer;
    DirectoryContentsList& searchPath;
    Array<File> searchResult;
    SearchEditor input;
};

class DocumentationBrowser : public DocumentBrowserBase {

public:
    explicit DocumentationBrowser(PluginProcessor* processor)
        : DocumentBrowserBase(processor)
        , fileList(directory, this)
        , searchComponent(directory)
    {
        auto location = ProjectInfo::appDataDir;

        if (SettingsFile::getInstance()->hasProperty("browser_path")) {
            auto customLocation = File(pd->settingsFile->getProperty<String>("browser_path"));
            if (customLocation.exists()) {
                location = customLocation;
            }
        }

        directory.setDirectory(location, true, true);

        updateThread.startThread();

        addAndMakeVisible(fileList);

        searchComponent.openFile = [this](File& result) {
            if (result.existsAsFile()) {
                pd->loadPatch(result, findParentComponentOfClass<PluginEditor>());
                SettingsFile::getInstance()->addToRecentlyOpened(result);
            }
        };

        addAndMakeVisible(searchComponent);

        if (!fileList.getSelectedFile().exists())
            fileList.moveSelectedRow(1);
    }

    ~DocumentationBrowser() override
    {
        updateThread.stopThread(1000);
    }

    bool isSearching() override
    {
        return searchComponent.isSearching();
    }

    bool hitTest(int x, int y) override
    {
        if (x < 5)
            return false;

        return true;
    }

    void resized() override
    {
        searchComponent.setBounds(getLocalBounds());
        fileList.setBounds(getLocalBounds().withHeight(getHeight() - 32).withY(32).reduced(2, 0));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);
    }

    std::unique_ptr<Component> getExtraSettingsComponent()
    {
        auto* settingsCalloutButton = new SmallIconButton(Icons::More);
        settingsCalloutButton->setTooltip("Show browser settings");
        settingsCalloutButton->setConnectedEdges(12);
        settingsCalloutButton->onClick = [this, settingsCalloutButton]() {
            auto* editor = findParentComponentOfClass<PluginEditor>();
            auto* sidebar = getParentComponent();
            auto bounds = editor->getLocalArea(sidebar, settingsCalloutButton->getBounds());
            auto openFolderCallback = [this]() {
                Dialogs::showOpenDialog([this](File& result){
                    if (result.exists()) {
                        const auto& path = result.getFullPathName();
                        pd->settingsFile->setProperty("browser_path", path);
                        directory.setDirectory(path, true, true);
                    }
                }, false, true, "", "DocumentationFileChooser");
            };

            auto resetFolderCallback = [this]() {
                auto location = ProjectInfo::appDataDir;
                const auto& path = location.getFullPathName();
                pd->settingsFile->setProperty("browser_path", path);
                directory.setDirectory(path, true, true);
            };

            auto docsSettings = std::make_unique<DocumentBrowserSettings>(openFolderCallback, resetFolderCallback);
            CallOutBox::launchAsynchronously(std::move(docsSettings), bounds, editor);
        };

        return std::unique_ptr<TextButton>(settingsCalloutButton);
    }

private:
    TextButton revealButton = TextButton(Icons::OpenedFolder);
    TextButton loadFolderButton = TextButton(Icons::Folder);
    TextButton resetFolderButton = TextButton(Icons::Restore);

    TextButton settingsCalloutButton = TextButton();

public:
    DocumentBrowserView fileList;
    FileSearchComponent searchComponent;
};
