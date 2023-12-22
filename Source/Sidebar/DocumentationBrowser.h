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
#include "Utility/Autosave.h"
#include "Utility/ValueTreeViewer.h"
#include "Object.h"

class DocumentBrowserSettings : public Component {
public:
    struct DocumentBrowserSettingsButton : public TextButton {
        String const icon;
        String const description;

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

class DocumentationBrowser : public Component, public FileDragAndDropTarget, private FileSystemWatcher::Listener,  private Thread, public KeyListener {

public:
    explicit DocumentationBrowser(PluginProcessor* processor)
        : Thread("Documentation Browser Thread"), pd(processor)
    {
        auto location = ProjectInfo::appDataDir;

        if (SettingsFile::getInstance()->hasProperty("browser_path")) {
            auto customLocation = File(pd->settingsFile->getProperty<String>("browser_path"));
            if (customLocation.exists()) {
                location = customLocation;
            }
        }
        searchInput.setBackgroundColour(PlugDataColour::sidebarActiveBackgroundColourId);
        searchInput.addKeyListener(this);
        searchInput.onTextChange = [this]() {
            fileList.setFilterString(searchInput.getText());
        };
        
        fsWatcher.addListener(this);

        searchInput.setJustification(Justification::centredLeft);
        searchInput.setBorder({ 1, 23, 5, 1 });
        searchInput.setTextToShowWhenEmpty("Type to search documentation", findColour(PlugDataColour::sidebarTextColourId).withAlpha(0.5f));
        searchInput.setInterceptsMouseClicks(true, true);
        addAndMakeVisible(searchInput);
        
        fileList.onClick = [this](ValueTree& tree){
            auto file = File(tree.getProperty("Path").toString());
            if (file.existsAsFile() && file.hasFileExtension("pd")) {
                pd->loadPatch(file, findParentComponentOfClass<PluginEditor>());
                SettingsFile::getInstance()->addToRecentlyOpened(file);
            }
            else if(file.isDirectory())
            {
                file.revealToUser();
            }
        };
        
        fileList.onDragStart = [this](ValueTree& tree){
            DragAndDropContainer::performExternalDragDropOfFiles({ tree.getProperty("Path") }, false, this, nullptr);
        };
        
        updateContent();
        addAndMakeVisible(fileList);
    }
    
    ~DocumentationBrowser()
    {
        stopThread(-1);
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
    
   void updateContent()
   {
       fsWatcher.removeAllFolders();
       fsWatcher.addFolder(File(pd->settingsFile->getProperty<String>("browser_path")));
       startThread(Thread::Priority::background);
   }
    
   bool keyPressed(KeyPress const& key, Component* originatingComponent) override
   {
       if (key.isKeyCode(KeyPress::upKey) || key.isKeyCode(KeyPress::downKey)) {
           fileList.keyPressed(key, originatingComponent);
           return true;
       }

       return false;
   }

    
    void filesystemChanged() override
    {
        if(isVisible())
        {
            startThread(Thread::Priority::background);
        }
        else {
            needsUpdate = true;
        }
    }
    
    void visibilityChanged() override
    {
        if(needsUpdate && isVisible())
        {
            startThread(Thread::Priority::low);
            needsUpdate = false;
        }
    }

   void filesDropped(StringArray const& files, int x, int y) override
   {
       auto parentDirectory = File(pd->settingsFile->getProperty<String>("browser_path"));
       
       for (auto& path : files) {
           auto file = File(path);
           if (file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
               auto alias = parentDirectory.getChildFile(file.getFileName());

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

       updateContent();

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
    
    void run() override
    {
        auto tree = generateDirectoryValueTree(File(pd->settingsFile->getProperty<String>("browser_path")));
        MessageManager::callAsync([this, tree](){
            fileTree = tree;
            fileList.setValueTree(tree);
        });
    }
    
    ValueTree generateDirectoryValueTree(const File& directory) {
        
        static File versionDataDir = ProjectInfo::appDataDir.getChildFile("Versions");
        static File toolchainDir = ProjectInfo::appDataDir.getChildFile("Toolchain");
        
        if (threadShouldExit() || !directory.exists() || !directory.isDirectory() || directory == versionDataDir || directory == toolchainDir)
        {
            return ValueTree();
        }
        
        ValueTree rootNode("Folder");
        rootNode.setProperty("Name", directory.getFileName(), nullptr);
        rootNode.setProperty("Path", directory.getFullPathName(), nullptr);
        rootNode.setProperty("Icon", Icons::Folder, nullptr);
        
        // visitedDirectories keeps track of dirs we've already processed to prevent infinite loops
        static Array<File> visitedDirectories = {};
    
        // Protect against symlink loops!
        if (!visitedDirectories.contains(directory)) {
            for (const auto& subDirectory : OSUtils::iterateDirectory(directory, false, false)) {
                visitedDirectories.add(directory);
                
                if(subDirectory.isDirectory() && subDirectory != directory) {
                    ValueTree childNode = generateDirectoryValueTree(subDirectory);
                    if(childNode.isValid()) rootNode.appendChild(childNode, nullptr);
                }
                
                visitedDirectories.removeLast();
            }
        }
        
        for (const auto& file : OSUtils::iterateDirectory(directory, false, true)) {
            if(file.getFileName().startsWith(".") || file.isDirectory()) continue;
            
            ValueTree childNode("File");
            childNode.setProperty("Name", file.getFileName(), nullptr);
            childNode.setProperty("Path", file.getFullPathName(), nullptr);
            childNode.setProperty("Icon", Icons::File, nullptr);

            rootNode.appendChild(childNode, nullptr);
        }
        
        struct {
            int compareElements (const ValueTree& first, const ValueTree& second)
            {
                if(first.getProperty("Icon") == Icons::File && second.getProperty("Icon") == Icons::Folder)
                {
                    return 1;
                }
                if(first.getProperty("Icon") == Icons::Folder && second.getProperty("Icon") == Icons::File)
                {
                    return -1;
                }
                
                return first.getProperty("Name").toString().compareNatural(second.getProperty("Name").toString());
            }
        } valueTreeSorter;
        
        rootNode.sort(valueTreeSorter, nullptr, false);
        return rootNode;
    }

    bool isSearching()
    {
        return searchInput.hasKeyboardFocus(false);
    }

    bool hitTest(int x, int y) override
    {
        if (x < 5)
            return false;

        return true;
    }
    
    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::sidebarActiveBackgroundColourId));
        g.fillRoundedRectangle(searchInput.getBounds().reduced(6, 4).toFloat(), Corners::defaultCornerRadius);
    }

    void lookAndFeelChanged() override
    {
        searchInput.setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
        searchInput.setColour(TextEditor::textColourId, findColour(PlugDataColour::sidebarTextColourId));
        searchInput.setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    }

    void resized() override
    {
        searchInput.setBounds(getLocalBounds().removeFromTop(34).reduced(5, 4));
        fileList.setBounds(getLocalBounds().withHeight(getHeight() - 32).withY(32).reduced(2, 0));
    }

    void paintOverChildren(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        g.drawLine(0.5f, 0, 0.5f, getHeight() - 27.5f);
        
        auto backgroundColour = findColour(PlugDataColour::sidebarBackgroundColourId);
        auto transparentColour = backgroundColour.withAlpha(0.0f);

        // Draw a gradient to fade the content out underneath the search input
        auto scrollOffset = fileList.getViewport().canScrollVertically();
        g.setGradientFill(ColourGradient(backgroundColour, 0.0f, 26.0f, transparentColour, 0.0f, 42.0f, false));
        g.fillRect(Rectangle<int>(0, searchInput.getBottom(), getWidth() - scrollOffset, 12));

        Fonts::drawIcon(g, Icons::Search, 2, 1, 32, findColour(PlugDataColour::sidebarTextColourId), 12);
        
        if (isDraggingFile) {
            g.setColour(findColour(PlugDataColour::scrollbarThumbColourId));
            g.drawRect(getLocalBounds().reduced(1), 2.0f);
        }
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
                Dialogs::showOpenDialog([this](File& result) {
                    if (result.isDirectory()) {
                        pd->settingsFile->setProperty("browser_path", result.getFullPathName());
                        updateContent();
                    }
                },
                    false, true, "", "DocumentationFileChooser");
            };

            auto resetFolderCallback = [this]() {
                auto location = ProjectInfo::appDataDir;
                pd->settingsFile->setProperty("browser_path", location.getFullPathName());
                updateContent();
            };

            auto docsSettings = std::make_unique<DocumentBrowserSettings>(openFolderCallback, resetFolderCallback);
            CallOutBox::launchAsynchronously(std::move(docsSettings), bounds, editor);
        };

        return std::unique_ptr<TextButton>(settingsCalloutButton);
    }

private:
    PluginProcessor* pd;
    
    bool needsUpdate = false;
    
    FileSystemWatcher fsWatcher;
    
    TextButton revealButton = TextButton(Icons::OpenedFolder);
    TextButton loadFolderButton = TextButton(Icons::Folder);
    TextButton resetFolderButton = TextButton(Icons::Restore);
    TextButton settingsCalloutButton = TextButton();
    
    ValueTree fileTree;
    ValueTreeViewerComponent fileList = ValueTreeViewerComponent("(Folder)");
    SearchEditor searchInput;
    
    bool isDraggingFile = false;
};
