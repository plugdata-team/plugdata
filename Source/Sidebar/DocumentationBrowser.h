/*
 // Copyright (c) 2021-2025 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// This is based on juce_FileTreeComponent, but I decided to rewrite parts to:
// 1. Sort by folders first
// 2. Improve simplicity and efficiency by not using OS file icons (they look bad anyway)

#pragma once

#include "Utility/OSUtils.h"
#include "Utility/ValueTreeViewer.h"
#include "Dialogs/Dialogs.h"

class DocumentBrowserSettings final : public Component {

public:
    struct DocumentBrowserSettingsButton final : public TextButton {
        String const icon;
        String const description;

        DocumentBrowserSettingsButton(String iconString, String descriptionString)
            : icon(std::move(iconString))
            , description(std::move(descriptionString))
        {
        }

        void paint(Graphics& g) override
        {
            auto colour = PlugDataColours::toolbarTextColour;
            if (isMouseOver()) {
                colour = colour.contrasting(0.3f);
            }

            Fonts::drawText(g, description, getLocalBounds().withTrimmedLeft(28), colour, 14);

            if (getToggleState()) {
                colour = PlugDataColours::toolbarActiveColour;
            }

            Fonts::drawIcon(g, icon, getLocalBounds().withTrimmedLeft(8), colour, 14, false);
        }
    };

    DocumentBrowserSettings(std::function<void()> const& chooseCustomLocation, std::function<void()> const& resetDefaultLocation)
    {
        addAndMakeVisible(customLocationButton);
        addAndMakeVisible(restoreLocationButton);

        customLocationButton.onClick = [chooseCustomLocation] {
            chooseCustomLocation();
        };

        restoreLocationButton.onClick = [resetDefaultLocation] {
            resetDefaultLocation();
        };

        setSize(180, 54);
    }

    void resized() override
    {
        auto buttonBounds = getLocalBounds();

        int const buttonHeight = buttonBounds.getHeight() / 2;

        customLocationButton.setBounds(buttonBounds.removeFromTop(buttonHeight));
        restoreLocationButton.setBounds(buttonBounds.removeFromTop(buttonHeight));
    }

private:
    DocumentBrowserSettingsButton customLocationButton = DocumentBrowserSettingsButton(Icons::Open, "Show custom folder...");
    DocumentBrowserSettingsButton restoreLocationButton = DocumentBrowserSettingsButton(Icons::Restore, "Show default folder");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DocumentBrowserSettings)
};

class DocumentationBrowserUpdateThread final : public Thread
    , public ChangeBroadcaster
    , private FileSystemWatcher::Listener
    , private SettingsFileListener
    , public DeletedAtShutdown {

    static inline DocumentationBrowserUpdateThread* instance = nullptr;

public:
    DocumentationBrowserUpdateThread()
        : Thread("Documentation Browser Thread")
    {
        fsWatcher.removeAllFolders();
        fsWatcher.addFolder(File(SettingsFile::getInstance()->getProperty<String>("browser_path")));
        fsWatcher.addListener(this);

        update();
    }

    ~DocumentationBrowserUpdateThread() override
    {

        instance = nullptr;
        stopThread(-1);
    }

    void update()
    {
        // startThread(fileTree.isValid() ? Thread::Priority::low : Thread::Priority::background);
        startThread(Thread::Priority::low);
    }

    ValueTree getCurrentTree() const
    {
        ScopedLock treeLock(fileTreeLock);
        return fileTree.createCopy();
    }

    static DocumentationBrowserUpdateThread* getInstance()
    {
        if (!instance)
            instance = new DocumentationBrowserUpdateThread();
        return instance;
    }

private:
    static inline auto const fileIdentifier = Identifier("File");
    static inline auto const nameIdentifier = Identifier("Name");
    static inline auto const pathIdentifier = Identifier("Path");
    static inline auto const iconIdentifier = Identifier("Icon");

    ValueTree generateDirectoryValueTree(File const& directory)
    {
        static File versionDataDir = ProjectInfo::appDataDir.getChildFile("Versions");
        static File toolchainDir = ProjectInfo::appDataDir.getChildFile("Toolchain");
        static File libraryDir = ProjectInfo::appDataDir.getChildFile("Library");

        if (threadShouldExit() || directory == versionDataDir || directory == toolchainDir || directory == libraryDir) {
            return {};
        }

        ValueTree rootNode("Folder");
        rootNode.setProperty(nameIdentifier, directory.getFileName(), nullptr);
        rootNode.setProperty(pathIdentifier, directory.getFullPathName(), nullptr);
        rootNode.setProperty(iconIdentifier, Icons::Folder, nullptr);

        // visitedDirectories keeps track of dirs we've already processed to prevent infinite loops
        static SmallArray<hash32> visitedDirectories = {};

        auto const directoryHash = OSUtils::getUniqueFileHash(directory.getFullPathName());
        if (!visitedDirectories.contains(directoryHash)) {
            visitedDirectories.add(directoryHash); // Protect against symlink loops!
            for (auto const& subDirectory : OSUtils::iterateDirectory(directory, false, false)) {
                auto pathName = subDirectory.getFullPathName();
                if (OSUtils::isDirectoryFast(pathName) && subDirectory != directory) {
                    ValueTree childNode = generateDirectoryValueTree(subDirectory);
                    if (childNode.isValid())
                        rootNode.appendChild(childNode, nullptr);
                }
            }
            visitedDirectories.pop_back();
        }

        for (auto const& file : OSUtils::iterateDirectory(directory, false, true)) {
            if (file.getFileName().startsWith("."))
                continue;

            ValueTree childNode(fileIdentifier);
            childNode.setProperty(nameIdentifier, file.getFileName(), nullptr);
            childNode.setProperty(pathIdentifier, file.getFullPathName(), nullptr);
            childNode.setProperty(iconIdentifier, Icons::File, nullptr);

            rootNode.appendChild(childNode, nullptr);
        }

        if (threadShouldExit())
            return {};

        struct {
            static int compareElements(ValueTree const& first, ValueTree const& second)
            {
                if (first.getProperty(iconIdentifier) == Icons::File && second.getProperty(iconIdentifier) == Icons::Folder) {
                    return 1;
                }
                if (first.getProperty(iconIdentifier) == Icons::Folder && second.getProperty(iconIdentifier) == Icons::File) {
                    return -1;
                }

                return first.getProperty(nameIdentifier).toString().compareNatural(second.getProperty(nameIdentifier).toString());
            }
        } valueTreeSorter;

        rootNode.sort(valueTreeSorter, nullptr, false);
        return rootNode;
    }

    void settingsChanged(String const& name, var const& value) override
    {
        if (name == "browser_path") {
            fsWatcher.removeAllFolders();
            fsWatcher.addFolder(File(SettingsFile::getInstance()->getProperty<String>("browser_path")));
            update();
        }
    }

    void run() override
    {
        try {
            constexpr int maxRetries = 50;
            int retries = 0;

            while (retries < maxRetries) {
                if (threadShouldExit())
                    break;

                ScopedTryLock const stl(fileTreeLock);
                if (stl.isLocked()) {
                    fileTree = generateDirectoryValueTree(File(SettingsFile::getInstance()->getProperty<String>("browser_path")));
                    break;
                }
                retries++;
                Time::waitForMillisecondCounter(Time::getMillisecondCounter() + 100);
            }

            sendChangeMessage();
        } catch (...) {

            std::cerr << "Failed to update documentation browser" << std::endl;
        }
    }

    void filesystemChanged() override
    {
        update();
    }

    CriticalSection const fileTreeLock;
    ValueTree fileTree;
    FileSystemWatcher fsWatcher;
};

class DocumentationBrowser final : public Component
    , public FileDragAndDropTarget
    , public ChangeListener
    , public KeyListener {

public:
    explicit DocumentationBrowser(PluginProcessor* processor)
        : pd(processor)
    {
        updater = DocumentationBrowserUpdateThread::getInstance();
        updater->addChangeListener(this);
#if JUCE_IOS // Needed to AUv3
        updater->update();
#endif

        searchInput.setBackgroundColour(PlugDataColour::sidebarActiveBackgroundColourId);
        searchInput.addKeyListener(this);
        searchInput.onTextChange = [this] {
            fileList.setFilterString(searchInput.getText());
        };

        searchInput.setJustification(Justification::centredLeft);
        searchInput.setBorder({ 1, 23, 5, 1 });
        searchInput.setTextToShowWhenEmpty("Type to search documentation", PlugDataColours::sidebarTextColour.withAlpha(0.5f));
        searchInput.setInterceptsMouseClicks(true, true);
        addAndMakeVisible(searchInput);

        auto returnAndClickFn = [this](ValueTree const& tree) {
            auto const file = File(tree.getProperty("Path").toString());
            if (file.existsAsFile() && file.hasFileExtension("pd")) {
                auto* editor = findParentComponentOfClass<PluginEditor>();
                editor->getTabComponent().openPatch(URL(file));
            } else if (file.isDirectory()) {
                file.revealToUser();
            } else if (file.existsAsFile()) {
                file.startAsProcess();
            }
        };

        // For search we use return key to maintain focus on the side panel
        // So a user can traverse a patch tree with up/down/return
        // We don't need this for documentation, but we use the same viewer for it,
        // so set both to same function here
        fileList.onReturn = returnAndClickFn;
        fileList.onClick = returnAndClickFn;

        fileList.onDragStart = [this](ValueTree const& tree) {
            DragAndDropContainer::performExternalDragDropOfFiles({ tree.getProperty("Path") }, false, this, nullptr);
        };

        addAndMakeVisible(fileList);
    }

    ~DocumentationBrowser() override
    {
        updater->removeChangeListener(this);
    }

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        fileTree = updater->getCurrentTree();

        if (isVisible()) {
            fileList.setValueTree(fileTree);
        }
    }

    bool isInterestedInFileDrag(StringArray const& files) override
    {
        if (!isVisible())
            return false;

        for (auto& path : files) {
            auto file = File(path);
            if (file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
                return true;
            }
        }

        return false;
    }

    bool keyPressed(KeyPress const& key, Component* originatingComponent) override
    {
        if (key.isKeyCode(KeyPress::upKey) || key.isKeyCode(KeyPress::downKey)) {
            fileList.keyPressed(key, originatingComponent);
            return true;
        }

        return false;
    }

    void visibilityChanged() override
    {
        if (fileList.getValueTree() != fileTree) {
            fileList.setValueTree(fileTree);
        }
    }

    void filesDropped(StringArray const& files, int x, int y) override
    {
        auto const parentDirectory = File(pd->settingsFile->getProperty<String>("browser_path"));

        for (auto& path : files) {
            auto file = File(path);
            if (file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
                auto alias = parentDirectory.getChildFile(file.getFileName());

                if (alias.exists())
                    continue;

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

        updater->update();

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

    bool hitTest(int const x, int y) override
    {
        if (x < 5)
            return false;

        return true;
    }

    void paint(Graphics& g) override
    {
        g.setColour(PlugDataColours::sidebarActiveBackgroundColour);
        g.fillRoundedRectangle(searchInput.getBounds().reduced(6, 4).toFloat(), Corners::defaultCornerRadius);
    }

    void lookAndFeelChanged() override
    {
        searchInput.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        searchInput.setColour(TextEditor::textColourId, PlugDataColours::sidebarTextColour);
        searchInput.setColour(TextEditor::outlineColourId, Colours::transparentBlack);

        searchInput.applyColourToAllText(PlugDataColours::panelTextColour);
    }

    void resized() override
    {
        searchInput.setBounds(getLocalBounds().removeFromTop(34).reduced(5, 4));
        fileList.setBounds(getLocalBounds().withHeight(getHeight() - 32).withY(32).reduced(2, 0));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(PlugDataColours::toolbarOutlineColour);
        g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);

        auto const backgroundColour = PlugDataColours::sidebarBackgroundColour;
        auto const transparentColour = backgroundColour.withAlpha(0.0f);

        // Draw a gradient to fade the content out underneath the search input
        auto const scrollOffset = fileList.getViewport().canScrollVertically();
        g.setGradientFill(ColourGradient(backgroundColour, 0.0f, 26.0f, transparentColour, 0.0f, 42.0f, false));
        g.fillRect(Rectangle<int>(0, searchInput.getBottom(), getWidth() - scrollOffset, 12));

        Fonts::drawIcon(g, Icons::Search, 2, 1, 32, PlugDataColours::sidebarTextColour, 12);

        if (isDraggingFile) {
            g.setColour(PlugDataColours::scrollbarThumbColour);
            g.drawRect(getLocalBounds().reduced(1), 2.0f);
        }
    }

    std::unique_ptr<Component> getExtraSettingsComponent() const
    {
        auto* settingsCalloutButton = new SmallIconButton(Icons::More);
        settingsCalloutButton->setTooltip("Show browser settings");
        settingsCalloutButton->setConnectedEdges(12);
        settingsCalloutButton->onClick = [this, settingsCalloutButton] {
            auto* editor = findParentComponentOfClass<PluginEditor>();
            auto const bounds = settingsCalloutButton->getScreenBounds();
            auto openFolderCallback = [this, editor] {
                Dialogs::showOpenDialog([this](URL const& result) {
                    if (result.getLocalFile().isDirectory()) {
                        pd->settingsFile->setProperty("browser_path", result.toString(false));
                    }
                },
                    false, true, "", "DocumentationFileChooser", editor);
            };

            auto resetFolderCallback = [this] {
                auto const location = ProjectInfo::appDataDir;
                pd->settingsFile->setProperty("browser_path", location.getFullPathName());
            };

            auto docsSettings = std::make_unique<DocumentBrowserSettings>(openFolderCallback, resetFolderCallback);
            editor->showCalloutBox(std::move(docsSettings), bounds);
        };

        return std::unique_ptr<TextButton>(settingsCalloutButton);
    }

private:
    PluginProcessor* pd;

    TextButton revealButton = TextButton(Icons::OpenedFolder);
    TextButton loadFolderButton = TextButton(Icons::Folder);
    TextButton resetFolderButton = TextButton(Icons::Restore);
    TextButton settingsCalloutButton = TextButton();

    ValueTree fileTree;
    ValueTreeViewerComponent fileList = ValueTreeViewerComponent("(Folder)");

    DocumentationBrowserUpdateThread* updater;
    SearchEditor searchInput;

    bool isDraggingFile = false;
};
