/*
 // Copyright (c) 2015-2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// This is based on juce_FileTreeComponent, but I decided to rewrite parts to:
// 1. Sort by folders first
// 2. Improve simplicity and efficiency by not using OS file icons (they look bad anyway)

#include "../Utility/FileSystemWatcher.h"

#if JUCE_WINDOWS
#    include <filesystem>

#    if _WIN64
extern "C" {
// Need this to create directory junctions on Windows
unsigned int WinExec(char const* lpCmdLine, unsigned int uCmdShow);
}
#    endif
#endif

bool wantsNativeDialog();

// Base classes for communication between parent and child classes
struct DocumentBrowserViewBase : public TreeView
    , public DirectoryContentsDisplayComponent {
    DocumentBrowserViewBase(DirectoryContentsList& listToShow)
        : DirectoryContentsDisplayComponent(listToShow) {};
};

struct DocumentBrowserBase : public Component {
    DocumentBrowserBase(PlugDataAudioProcessor* processor)
        : pd(processor)
        , filter("*", "*", "All files")
        , updateThread("browserThread")
        , directory(&filter, updateThread) {

        };

    virtual bool isSearching() = 0;

    PlugDataAudioProcessor* pd;
    TimeSliceThread updateThread;
    DirectoryContentsList directory;
    WildcardFileFilter filter;
};

//==============================================================================
class DocumentBrowserItem : public TreeViewItem
    , private AsyncUpdater
    , private ChangeListener {
public:
    DocumentBrowserItem(DocumentBrowserViewBase& treeComp, DirectoryContentsList* parentContents, int indexInContents, int indexInParent, File const& f)
        : file(f)
        , owner(treeComp)
        , parentContentsList(parentContents)
        , indexInContentsList(indexInContents)
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
        p.addTriangle(0.0f, 0.0f, 1.0f, isOpen() ? 0.0f : 0.5f, isOpen() ? 0.5f : 0.0f, 1.0f);
        g.setColour(isSelected() ? getOwnerView()->findColour(PlugDataColour::panelActiveTextColourId) : getOwnerView()->findColour(PlugDataColour::panelTextColourId).withAlpha(isMouseOver ? 0.7f : 1.0f));
        
        auto pathArea = area.translated(4, 0);
        g.fillPath(p, p.getTransformToScaleToFit(pathArea.reduced(2, pathArea.getHeight() / 4), true));
    }

    //==============================================================================
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
        return 24;
    }
    var getDragSourceDescription() override
    {
        return String();
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
                if (subContentsList->getFile(i).isDirectory()) {
                    addSubItem(new DocumentBrowserItem(owner, subContentsList, i, idx, subContentsList->getFile(i)));
                    idx++;
                }
            }
            for (int i = 0; i < subContentsList->getNumFiles(); ++i) {
                if (subContentsList->getFile(i).existsAsFile()) {
                    addSubItem(new DocumentBrowserItem(owner, subContentsList, i, idx, subContentsList->getFile(i)));
                    idx++;
                }
            }
        }
    }

    void paintItem(Graphics& g, int width, int height) override
    {
        int const x = 35;

        if (isSelected())
            g.setColour(owner.findColour(DirectoryContentsDisplayComponent::highlightedTextColourId));
        else
            g.setColour(owner.findColour(DirectoryContentsDisplayComponent::textColourId));

        g.setFont(dynamic_cast<PlugDataLook*>(&owner.getLookAndFeel())->iconFont);

        if (isDirectory) {
            g.drawFittedText(Icons::Folder, Rectangle<int>(2, 2, x - 4, height - 4), Justification::centred, 1);
        } else {
            g.drawFittedText(Icons::File, Rectangle<int>(2, 2, x - 4, height - 4), Justification::centred, 1);
        }

        g.setFont(Font());
        g.drawFittedText(file.getFileName(), x, 0, width - x, height, Justification::centredLeft, 1);
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
    int indexInContentsList;
    OptionalScopedPointer<DirectoryContentsList> subContentsList;
    bool isDirectory;
    String fileSize;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocumentBrowserItem)
};

class DocumentBrowserView : public DocumentBrowserViewBase
    , public FileBrowserListener
    , public ScrollBar::Listener {
public:
    //==============================================================================
    /** Creates a listbox to show the contents of a specified directory.
     */
    DocumentBrowserView(DirectoryContentsList& listToShow, DocumentBrowserBase* parent)
        : DocumentBrowserViewBase(listToShow)
        , itemHeight(24)
        , browser(parent)
    {
        setRootItemVisible(false);
        refresh();
        addListener(this);
        getViewport()->getVerticalScrollBar().addListener(this);
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

    //==============================================================================
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
        int selectedIdx = -1;

        // Paint selected row
        if (getNumSelectedFiles()) {
            selectedIdx = getSelectedItem(0)->getRowNumberInTree();
        }

        PlugDataLook::paintStripes(g, 24, getViewport()->getHeight(), *this, selectedIdx, getViewport()->getViewPositionY());
    }
    // Paint file drop outline
    void paintOverChildren(Graphics& g) override
    {
        if (isDraggingFile) {
            g.setColour(findColour(PlugDataColour::panelBackgroundColourId));
            g.drawRect(getLocalBounds().reduced(1), 2.0f);
        }
    }

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    /** Callback when the user double-clicks on a file in the browser. */
    void fileDoubleClicked(File const& file) override
    {
        if (file.isDirectory()) {
            file.revealToUser();
        } else if (file.existsAsFile() && file.hasFileExtension("pd")) {
            browser->pd->loadPatch(file);
        } else if (file.existsAsFile()) {
            if (auto* editor = dynamic_cast<PlugDataPluginEditor*>(browser->pd->getActiveEditor())) {
                auto* cnv = editor->getCurrentCanvas();
                cnv->attachNextObjectToMouse = true;

                auto lastPosition = cnv->viewport->getViewArea().getConstrainedPoint(cnv->getMouseXYRelative() - Point<int>(Object::margin, Object::margin));
                auto filePath = file.getFullPathName().replaceCharacter('\\', '/');
                cnv->objects.add(new Object(cnv, "msg " + filePath, lastPosition));
            }
        }
    }

    void selectionChanged() override
    {
        browser->repaint();
    };
    void fileClicked(File const&, MouseEvent const&) override {};
    void browserRootChanged(File const&) override {};

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

#if JUCE_WINDOWS
                if (alias.exists())
                    alias.deleteRecursively();

                // Symlinks on Windows are weird!
                if (file.isDirectory()) {
                    // Create directory junction command
                    auto aliasCommand = "cmd.exe /k mklink /J " + alias.getFullPathName().replaceCharacters("/", "\\") + " " + file.getFullPathName();
                    // Execute command
#    if _WIN64
                    WinExec(aliasCommand.toRawUTF8(), 0);
#    else
                    system(aliasCommand.fromFirstOccurrenceOf("/k", false, false).toRawUTF8());
#    endif
                } else {
                    // Create hard link
                    std::filesystem::create_hard_link(file.getFullPathName().toStdString(), alias.getFullPathName().toStdString());
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

private:
    //==============================================================================

    DocumentBrowserBase* browser;
    bool isDraggingFile = false;

    int itemHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocumentBrowserView)
};

class FileSearchComponent : public Component
    , public ListBoxModel
    , public ScrollBar::Listener {
public:
    FileSearchComponent(DirectoryContentsList& directory)
        : searchPath(directory)
    {
        listBox.setModel(this);
        listBox.setRowHeight(24);
        listBox.setOutlineThickness(0);
        listBox.deselectAllRows();

        listBox.getViewport()->setScrollBarsShown(true, false, false, false);

        input.setName("sidebar::searcheditor");

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

    void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
    {
        repaint();
    }

    void paint(Graphics& g) override
    {
        if (listBox.isVisible()) {
            PlugDataLook::paintStripes(g, 24, listBox.getHeight() + 24, *this, -1, listBox.getViewport()->getViewPositionY() - 4);
        }

    }

    void paintOverChildren(Graphics& g) override
    {
        g.setFont(getLookAndFeel().getTextButtonFont(closeButton, 30));
        g.setColour(findColour(PlugDataColour::panelTextColourId));

        g.drawText(Icons::Search, 0, 0, 30, 30, Justification::centred);
    }

    void paintListBoxItem(int rowNumber, Graphics& g, int w, int h, bool rowIsSelected) override
    {
        if (rowIsSelected) {
            g.setColour(findColour(PlugDataColour::panelActiveBackgroundColourId));
            g.fillRect(1, 0, w - 3, h);
        }

        g.setColour(rowIsSelected ? findColour(PlugDataColour::panelActiveTextColourId) : findColour(ComboBox::textColourId));
        const String item = searchResult[rowNumber].getFileName();

        g.setFont(Font());
        g.drawText(item, 24, 0, w - 4, h, Justification::centredLeft, true);

        g.setFont(getLookAndFeel().getTextButtonFont(closeButton, 23));
        g.drawText(Icons::File, 8, 2, 24, 24, Justification::centredLeft);
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

        return File();
    }

    void resized() override
    {
        auto tableBounds = getLocalBounds();
        auto inputBounds = tableBounds.removeFromTop(28);

        input.setBounds(inputBounds);

        closeButton.setBounds(inputBounds.removeFromRight(30));

        tableBounds.removeFromTop(28);
        listBox.setBounds(tableBounds);
    }

    std::function<void(File&)> openFile;

private:
    ListBox listBox;

    DirectoryContentsList& searchPath;
    Array<File> searchResult;
    TextEditor input;
    TextButton closeButton = TextButton(Icons::Clear);
};

struct DocumentBrowser : public DocumentBrowserBase
    , public FileSystemWatcher::Listener {
    DocumentBrowser(PlugDataAudioProcessor* processor)
        : DocumentBrowserBase(processor)
        , fileList(directory, this)
        , searchComponent(directory)
    {
        auto location = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library");

        if (pd->settingsTree.hasProperty("BrowserPath")) {
            auto customLocation = File(pd->settingsTree.getProperty("BrowserPath"));
            if (customLocation.exists()) {
                location = customLocation;
            }
        }

        watcher.addFolder(location);
        directory.setDirectory(location, true, true);

        updateThread.startThread();

        addAndMakeVisible(fileList);

        watcher.addListener(this);

        searchComponent.openFile = [this](File& file) {
            if (file.existsAsFile()) {
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

        loadFolderButton.setTooltip("Choose location to show");
        addAndMakeVisible(loadFolderButton);

        resetFolderButton.setTooltip("Reset to default location");
        addAndMakeVisible(resetFolderButton);

        loadFolderButton.onClick = [this]() {
            openChooser = std::make_unique<FileChooser>("Open...", directory.getDirectory().getFullPathName(), "", wantsNativeDialog());

            openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                [this](FileChooser const& fileChooser) {
                    auto const file = fileChooser.getResult();
                    if (file.exists()) {
                        auto path = file.getFullPathName();
                        pd->settingsTree.setProperty("BrowserPath", path, nullptr);
                        directory.setDirectory(path, true, true);
                        watcher.addFolder(file);
                    }
                });
        };

        resetFolderButton.onClick = [this]() {
            auto location = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library");
            auto path = location.getFullPathName();
            pd->settingsTree.setProperty("BrowserPath", path, nullptr);
            directory.setDirectory(path, true, true);
        };

        revealButton.onClick = [this]() {
            if (searchComponent.hasSelection()) {
                searchComponent.getSelection().revealToUser();
            } else if (fileList.getSelectedFile().exists()) {
                fileList.getSelectedFile().revealToUser();
            }
        };

        addAndMakeVisible(searchComponent);

        if (!fileList.getSelectedFile().exists())
            fileList.moveSelectedRow(1);
    }

    ~DocumentBrowser()
    {
        updateThread.stopThread(1000);
    }

    // Called when folder changes
    void fsChangeCallback() override
    {
        directory.refresh();
        fileList.refresh();
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
        searchComponent.setBounds(getLocalBounds().withHeight(getHeight() - 28));
        
        fileList.setBounds(getLocalBounds().withHeight(getHeight() - 58).withY(28).withLeft(5));

        auto fb = FlexBox(FlexBox::Direction::row, FlexBox::Wrap::noWrap, FlexBox::AlignContent::flexStart, FlexBox::AlignItems::stretch, FlexBox::JustifyContent::flexStart);

        Array<TextButton*> buttons = { &revealButton, &loadFolderButton, &resetFolderButton };

        for (auto* b : buttons) {
            auto item = FlexItem(*b).withMinWidth(8.0f).withMinHeight(8.0f).withMaxHeight(27);
            item.flexGrow = 1.0f;
            item.flexShrink = 1.0f;
            fb.items.add(item);
        }

        auto bounds = getLocalBounds().toFloat();

        fb.performLayout(bounds.removeFromBottom(28));
    }

    void paint(Graphics& g) override
    {
        // Background for statusbar part
        g.setColour(findColour(PlugDataColour::toolbarBackgroundColourId));
        g.fillRoundedRectangle(0, getHeight() - 28, getWidth(), 28, 6.0f);
    }

    void paintOverChildren(Graphics& g) override
    {
        // Draggable bar
        g.setColour(findColour(PlugDataColour::panelBackgroundOffsetColourId));
        g.fillRect(0, 28, Sidebar::dragbarWidth + 1, getHeight());

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);

        g.setColour(findColour(PlugDataColour::outlineColourId));
        g.drawLine(0, 28, getWidth(), 28);
    }

private:
    TextButton revealButton = TextButton(Icons::OpenedFolder);
    TextButton loadFolderButton = TextButton(Icons::Folder);
    TextButton resetFolderButton = TextButton(Icons::Restore);

    std::unique_ptr<FileChooser> openChooser;

public:
    DocumentBrowserView fileList;
    FileSearchComponent searchComponent;
    FileSystemWatcher watcher;
};
