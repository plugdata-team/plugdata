/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#if PLUGDATA_STANDALONE
#    include "Standalone/PlugDataWindow.h"
#endif

#include "PluginEditor.h"

#include "LookAndFeel.h"

#include "Canvas.h"
#include "Connection.h"
#include "Dialogs/Dialogs.h"

bool wantsNativeDialog()
{
#if PLUGDATA_STANDALONE
    return true;
#endif

    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");
    File settingsFile = homeDir.getChildFile("Settings.xml");

    auto settingsTree = ValueTree::fromXml(settingsFile.loadFileAsString());

    if (!settingsTree.hasProperty("NativeDialog")) {
        return true;
    }

    return static_cast<bool>(settingsTree.getProperty("NativeDialog"));
}

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p)
    , pd(&p)
    , statusbar(&p)
    , sidebar(&p, this)
    , tooltipWindow(this, 500)
    , tooltipShadow(DropShadow(Colour(0, 0, 0).withAlpha(0.2f), 4, { 0, 0 }), PlugDataLook::defaultCornerRadius)
    , splitView(this)
{
    mainMenuButton.setButtonText(Icons::Menu);
    undoButton.setButtonText(Icons::Undo);
    redoButton.setButtonText(Icons::Redo);
    addObjectMenuButton.setButtonText(Icons::Add);
    hideSidebarButton.setButtonText(Icons::Hide);
    pinButton.setButtonText(Icons::Pin);

    setResizable(true, false);

    // In the standalone, the resizer handling is done on the window class
#if !PLUGDATA_STANDALONE
    cornerResizer = std::make_unique<MouseRateReducedComponent<ResizableCornerComponent>>(this, getConstrainer());
    cornerResizer->setAlwaysOnTop(true);
    addAndMakeVisible(cornerResizer.get());
#endif

    tooltipWindow.setOpaque(false);
    tooltipWindow.setLookAndFeel(&pd->lnf.get());

    tooltipShadow.setOwner(&tooltipWindow);

    addKeyListener(getKeyMappings());

    setWantsKeyboardFocus(true);
    registerAllCommandsForTarget(this);

    for (auto& seperator : seperators) {
        addChildComponent(&seperator);
    }

    auto* settingsFile = SettingsFile::getInstance();

    auto keymap = settingsFile->getKeyMapTree();
    if (keymap.isValid()) {
        auto xmlStr = keymap.getProperty("keyxml").toString();
        auto elt = XmlDocument(xmlStr).getDocumentElement();

        if (elt) {
            getKeyMappings()->restoreFromXml(*elt);
        }
    } else {
        settingsFile->getValueTree().appendChild(ValueTree("KeyMap"), nullptr);
    }

    autoconnect.referTo(settingsFile->getPropertyAsValue("autoconnect"));

    theme.referTo(settingsFile->getPropertyAsValue("theme"));
    theme.addListener(this);

    if (!settingsFile->hasProperty("hvcc_mode"))
        settingsFile->setProperty("hvcc_mode", false);
    hvccMode.referTo(settingsFile->getPropertyAsValue("hvcc_mode"));

    zoomScale.referTo(settingsFile->getPropertyAsValue("zoom"));
    zoomScale.addListener(this);

    addAndMakeVisible(statusbar);
    addAndMakeVisible(splitView);
    addAndMakeVisible(sidebar);

    for (auto* button : std::vector<TextButton*> { &mainMenuButton, &undoButton, &redoButton, &addObjectMenuButton, &pinButton, &hideSidebarButton }) {
        button->getProperties().set("Style", "LargeIcon");
        button->setConnectedEdges(12);
        addAndMakeVisible(button);
    }

    // Show settings
    mainMenuButton.setTooltip("Settings");
    mainMenuButton.onClick = [this]() {
#ifdef PLUGDATA_STANDALONE
        Dialogs::showMainMenu(this, &mainMenuButton);
#else
        Dialogs::showMainMenu(this, &mainMenuButton);
#endif
    };

    addAndMakeVisible(mainMenuButton);

    //  Undo button
    undoButton.setTooltip("Undo");
    undoButton.onClick = [this]() { getCurrentCanvas()->undo(); };
    addAndMakeVisible(undoButton);

    // Redo button
    redoButton.setTooltip("Redo");
    redoButton.onClick = [this]() { getCurrentCanvas()->redo(); };
    addAndMakeVisible(redoButton);

    // New object button
    addObjectMenuButton.setTooltip("Create object");
    addObjectMenuButton.onClick = [this]() { Dialogs::showObjectMenu(this, &addObjectMenuButton); };
    addAndMakeVisible(addObjectMenuButton);

    // Hide sidebar
    hideSidebarButton.setTooltip("Hide Sidebar");
    hideSidebarButton.getProperties().set("Style", "LargeIcon");
    hideSidebarButton.setClickingTogglesState(true);
    hideSidebarButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    hideSidebarButton.setConnectedEdges(12);
    hideSidebarButton.onClick = [this]() {
        bool show = !hideSidebarButton.getToggleState();

        sidebar.showSidebar(show);
        hideSidebarButton.setButtonText(show ? Icons::Hide : Icons::Show);
        hideSidebarButton.setTooltip(show ? "Hide Sidebar" : "Show Sidebar");
        pinButton.setVisible(show);

        repaint();
        resized();
    };

    // Hide pin sidebar panel
    pinButton.setTooltip("Pin Panel");
    pinButton.getProperties().set("Style", "LargeIcon");
    pinButton.setClickingTogglesState(true);
    pinButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    pinButton.setConnectedEdges(12);
    pinButton.onClick = [this]() { sidebar.pinSidebar(pinButton.getToggleState()); };

    addAndMakeVisible(hideSidebarButton);

    sidebar.setSize(250, pd->lastUIHeight - 40);
    setSize(pd->lastUIWidth, pd->lastUIHeight);

    // Set minimum bounds
    setResizeLimits(835, 305, 999999, 999999);

    sidebar.toFront(false);

    // Make sure existing console messages are processed
    sidebar.updateConsole();
    updateCommandStatus();
    
    addModifierKeyListener(&statusbar);

    addChildComponent(zoomLabel);

    // Initialise zoom factor
    valueChanged(zoomScale);
    
    selectedSplitRect.setStrokeThickness(1.0f);
    selectedSplitRect.setInterceptsMouseClicks(false, false);
    selectedSplitRect.setFill(Colours::transparentBlack);
    addAndMakeVisible(selectedSplitRect);
}
PluginEditor::~PluginEditor()
{
    setConstrainer(nullptr);

    zoomScale.removeListener(this);
    theme.removeListener(this);
    
    pd->lastLeftTab = splitView.getLeftTabbar()->getCurrentTabIndex();
    pd->lastRightTab = splitView.getRightTabbar()->getCurrentTabIndex();
}

void PluginEditor::paint(Graphics& g)
{
    selectedSplitRect.setStrokeFill(findColour(PlugDataColour::dataColourId));
    
    g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), PlugDataLook::windowCornerRadius);

    auto baseColour = findColour(PlugDataColour::toolbarBackgroundColourId);

    bool rounded = wantsRoundedCorners();

    if (rounded) {
        // Toolbar background
        g.setColour(baseColour);
        g.fillRect(0, 10, getWidth(), toolbarHeight - 9);
        g.fillRoundedRectangle(0.0f, 0.0f, getWidth(), toolbarHeight, PlugDataLook::windowCornerRadius);

        // Statusbar background
        g.setColour(baseColour);
        g.fillRect(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight() - 12);
        g.fillRoundedRectangle(0.0f, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight(), PlugDataLook::windowCornerRadius);
    } else {
        // Toolbar background
        g.setColour(baseColour);
        g.fillRect(0, 0, getWidth(), toolbarHeight);

        // Statusbar background
        g.setColour(baseColour);
        g.fillRect(0, getHeight() - statusbar.getHeight(), getWidth(), statusbar.getHeight());
    }

    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0.0f, toolbarHeight, static_cast<float>(getWidth()), toolbarHeight, 1.0f);
}

// Paint file drop outline
void PluginEditor::paintOverChildren(Graphics& g)
{
    // Never want to be drawing over a dialog window
    if(openedDialog) return;
    
    if (isDraggingFile) {
        g.setColour(findColour(PlugDataColour::scrollbarThumbColourId));
        g.drawRect(getLocalBounds().reduced(1), 2.0f);
    }
    
    
    
}

void PluginEditor::resized()
{
    splitView.setBounds(0, toolbarHeight, (getWidth() - sidebar.getWidth()) + 1, getHeight() - toolbarHeight - (statusbar.getHeight()));
    sidebar.setBounds(getWidth() - sidebar.getWidth(), toolbarHeight, sidebar.getWidth(), getHeight() - toolbarHeight);
    statusbar.setBounds(0, getHeight() - statusbar.getHeight(), getWidth() - sidebar.getWidth(), statusbar.getHeight());
    

    mainMenuButton.setBounds(20, 0, toolbarHeight, toolbarHeight);
    undoButton.setBounds(90, 0, toolbarHeight, toolbarHeight);
    redoButton.setBounds(160, 0, toolbarHeight, toolbarHeight);
    addObjectMenuButton.setBounds(230, 0, toolbarHeight, toolbarHeight);

#ifdef PLUGDATA_STANDALONE
    auto useNativeTitlebar = SettingsFile::getInstance()->getProperty<bool>("native_window");
    auto windowControlsOffset = useNativeTitlebar ? 70.0f : 170.0f;
#else
    int const resizerSize = 18;
    cornerResizer->setBounds(getWidth() - resizerSize,
        getHeight() - resizerSize,
        resizerSize, resizerSize);

    auto windowControlsOffset = 70.0f;
#endif

    int hidePosition = getWidth() - windowControlsOffset;
    int pinPosition = hidePosition - 65;

    hideSidebarButton.setBounds(hidePosition, 0, 70, toolbarHeight);
    pinButton.setBounds(pinPosition, 0, 70, toolbarHeight);

    pd->lastUIWidth = getWidth();
    pd->lastUIHeight = getHeight();

    zoomLabel.setTopLeftPosition(5, statusbar.getY() - 28);
    zoomLabel.setSize(55, 23);

    if (auto* cnv = getCurrentCanvas()) {
        cnv->checkBounds();
    }
    
    updateSplitOutline();
}

void PluginEditor::mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel)
{
    if (e.mods.isCommandDown()) {
        mouseMagnify(e, 1.0f / (1.0f - wheel.deltaY));
    }
}

void PluginEditor::mouseMagnify(MouseEvent const& e, float scrollFactor)
{
    auto* cnv = getCurrentCanvas();

    if (!cnv)
        return;

    auto* viewport = getCurrentCanvas()->viewport;

    auto event = e.getEventRelativeTo(viewport);

    float value = static_cast<float>(zoomScale.getValue());

    // Apply and limit zoom
    value = std::clamp(value * scrollFactor, 0.5f, 2.0f);

    zoomScale = value;
}

#if PLUGDATA_STANDALONE

void PluginEditor::mouseDown(MouseEvent const& e)
{

    if (e.getNumberOfClicks() >= 2) {

#    if JUCE_MAC
        if (isMaximised) {
            getPeer()->setBounds(unmaximisedSize, false);
        } else {
            unmaximisedSize = getTopLevelComponent()->getBounds();
            auto userArea = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            getPeer()->setBounds(userArea, false);
        }

        isMaximised = !isMaximised;
        return;

#    else
        findParentComponentOfClass<PlugDataWindow>()->maximiseButtonPressed();
#    endif
    }

    if (e.getPosition().getY() < toolbarHeight) {
        if (auto* window = findParentComponentOfClass<PlugDataWindow>()) {
            if (!window->isUsingNativeTitleBar())
                windowDragger.startDraggingComponent(window, e.getEventRelativeTo(window));
        }
    }
}

void PluginEditor::mouseDrag(MouseEvent const& e)
{
    if (!isMaximised) {
        if (auto* window = findParentComponentOfClass<PlugDataWindow>()) {
            if (!window->isUsingNativeTitleBar())
                windowDragger.dragComponent(window, e.getEventRelativeTo(window), nullptr);
        }
    }
}
#endif

bool PluginEditor::isInterestedInFileDrag(StringArray const& files)
{
    if(openedDialog) return false;
    
    for (auto& path : files) {
        auto file = File(path);
        if (file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
            return true;
        }
    }

    return false;
}

void PluginEditor::filesDropped(StringArray const& files, int x, int y)
{
    for (auto& path : files) {
        auto file = File(path);
        if (file.exists() && (file.isDirectory() || file.hasFileExtension("pd"))) {
            pd->loadPatch(file);
            SettingsFile::getInstance()->addToRecentlyOpened(file);
        }
    }

    isDraggingFile = false;
    repaint();
}
void PluginEditor::fileDragEnter(StringArray const&, int, int)
{
    isDraggingFile = true;
    repaint();
}

void PluginEditor::fileDragExit(StringArray const&)
{
    isDraggingFile = false;
    repaint();
}

void PluginEditor::newProject()
{
    auto* patch = pd->loadPatch(pd::Instance::defaultPatch);
    patch->setTitle("Untitled Patcher");
}

void PluginEditor::openProject()
{
    auto openFunc = [this](FileChooser const& f) {
        File openedFile = f.getResult();

        if (openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
            SettingsFile::getInstance()->setProperty("last_filechooser_path", openedFile.getParentDirectory().getFullPathName());

            pd->loadPatch(openedFile);
            SettingsFile::getInstance()->addToRecentlyOpened(openedFile);
        }
    };

    openChooser = std::make_unique<FileChooser>("Choose file to open", File(SettingsFile::getInstance()->getProperty<String>("last_filechooser_path")), "*.pd", wantsNativeDialog());

    openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
}

void PluginEditor::saveProjectAs(std::function<void()> const& nestedCallback)
{
    saveChooser = std::make_unique<FileChooser>("Select a save file", File(SettingsFile::getInstance()->getProperty<String>("last_filechooser_path")), "*.pd", wantsNativeDialog());

    saveChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting,
        [this, nestedCallback](FileChooser const& f) mutable {
            File result = saveChooser->getResult();

            if (result.getFullPathName().isNotEmpty()) {
                SettingsFile::getInstance()->setProperty("last_filechooser_path", result.getParentDirectory().getFullPathName());

                result.deleteFile();
                result = result.withFileExtension(".pd");

                getCurrentCanvas()->patch.savePatch(result);
                SettingsFile::getInstance()->addToRecentlyOpened(result);
            }

            nestedCallback();
        });
}

void PluginEditor::saveProject(std::function<void()> const& nestedCallback)
{
    for (auto* patch : pd->patches) {
        patch->deselectAll();
    }

    if (getCurrentCanvas()->patch.getCurrentFile().existsAsFile()) {
        getCurrentCanvas()->patch.savePatch();
        SettingsFile::getInstance()->addToRecentlyOpened(getCurrentCanvas()->patch.getCurrentFile());
        nestedCallback();
    } else {
        saveProjectAs(nestedCallback);
    }
}

TabComponent* PluginEditor::getActiveTabbar()
{
    return splitView.getActiveTabbar();
}

Canvas* PluginEditor::getCurrentCanvas()
{
    return getActiveTabbar()->getCurrentCanvas();
}

void PluginEditor::closeTab(Canvas* cnv, bool neverDeletePatch)
{
    if(!cnv || !cnv->getTabbar()) return;
    
    auto* tabbar = cnv->getTabbar();
    const int tabIdx = cnv->getTabIndex();
    const int currentTabIdx = tabbar->getCurrentTabIndex();
    auto* patch = &cnv->patch;
    
    cnv->getTabbar()->removeTab(tabIdx);
    if(!neverDeletePatch && cnv->closePatchAlongWithCanvas) {
        patch->close();
    }
    canvases.removeObject(cnv);
    if(!neverDeletePatch) pd->patches.removeObject(patch);

    if (currentTabIdx == tabIdx) {
        if (currentTabIdx != tabbar->getNumTabs()) {
            // Set the focused tab to the next one
            tabbar->setCurrentTabIndex(currentTabIdx, true);
        } else {
            // Unless it's the last, then set it to the previous one
            tabbar->setCurrentTabIndex(currentTabIdx - 1, true);
        }
    }
    
    splitView.closeEmptySplits();
    updateCommandStatus();
    
    pd->savePatchTabPositions();
    
    MessageManager::callAsync([_this = SafePointer(this)]()
    {
        if(!_this) return;
        _this->resized();
    });
}

void PluginEditor::addTab(Canvas* cnv)
{
    // Create a pointer to the TabBar in focus
    auto* focusedTabbar = splitView.getActiveTabbar();
    
    int const newTabIdx = focusedTabbar->getCurrentTabIndex() + 1; // The tab index for the added tab
    
    // Add tab next to the currently focused tab
    focusedTabbar->addTab(cnv->patch.getTitle(), findColour(ResizableWindow::backgroundColourId), cnv->viewport, true, newTabIdx);

    focusedTabbar->setCurrentTabIndex(newTabIdx);
    focusedTabbar->setTabBackgroundColour(newTabIdx, Colours::transparentBlack);

    auto* tabButton = focusedTabbar->getTabbedButtonBar().getTabButton(newTabIdx);
    tabButton->setTriggeredOnMouseDown(true);

    auto* closeTabButton = new TextButton(Icons::Clear);
    closeTabButton->getProperties().set("Style", "Icon");
    closeTabButton->getProperties().set("FontScale", 0.44f);
    closeTabButton->setColour(TextButton::buttonColourId, Colour());
    closeTabButton->setColour(TextButton::buttonOnColourId, Colour());
    closeTabButton->setColour(ComboBox::outlineColourId, Colour());
    closeTabButton->setConnectedEdges(12);
    closeTabButton->setSize(28, 28);

    // Add the close button to the tab button
    tabButton->setExtraComponent(closeTabButton, TabBarButton::beforeText);

    closeTabButton->onClick = [this, focusedTabbar, tabButton]() mutable {
        // We cant use the index from earlier because it might have changed!
        const int tabIdx = tabButton->getIndex();
        auto* cnv = focusedTabbar->getCanvas(tabIdx);

        if (tabIdx == -1)
            return;

        auto deleteFunc = [this, cnv]() {
            closeTab(cnv);
        };

        if (cnv) {
            MessageManager::callAsync([this, cnv, deleteFunc]() mutable {
                // Don't show save dialog, if patch is still open in another view
                if (cnv->patch.isDirty()) {
                    Dialogs::showSaveDialog(&openedDialog, this, cnv->patch.getTitle(),
                        [this, cnv, deleteFunc](int result) mutable {
                            if (!cnv)
                                return;
                            if (result == 2)
                                saveProject([&deleteFunc]() mutable { deleteFunc(); });
                            else if (result == 1)
                                closeTab(cnv);
                        });
                } else {
                    closeTab(cnv);
                }
            });
        }
    };

    cnv->setVisible(true);
    updateSplitOutline();
    
    pd->savePatchTabPositions();
}

void PluginEditor::valueChanged(Value& v)
{
    // Update zoom
    if (v.refersToSameSourceAs(zoomScale)) {
        float scale = static_cast<float>(v.getValue());

        if (scale == 0) {
            scale = 1.0f;
            zoomScale = 1.0f;
        }

        transform = AffineTransform().scaled(scale);

        auto lastMousePosition = Point<int>();
        if (auto* cnv = getCurrentCanvas()) {
            lastMousePosition = cnv->getMouseXYRelative();
        }

        for (auto& canvas : canvases) {
            if (!canvas->isGraph) {
                canvas->hideSuggestions();
                canvas->setTransform(transform);
            }
        }
        if (auto* cnv = getCurrentCanvas()) {
            cnv->checkBounds();

            if (!cnv->viewport)
                return;

            auto totalBounds = Rectangle<int>();

            for (auto* object : cnv->getSelectionOfType<Object>()) {
                totalBounds = totalBounds.getUnion(object->getBoundsInParent().reduced(Object::margin));
            }

            // Check if we have any selection, if so, zoom towards that
            if (!totalBounds.isEmpty()) {
                auto pos = totalBounds.getCentre() * scale;
                pos.x -= cnv->viewport->getViewWidth() * 0.5f;
                pos.y -= cnv->viewport->getViewHeight() * 0.5f;
                cnv->viewport->setViewPosition(pos);
            }
            // If we don't have a selection, zoom towards mouse cursor
            else if (totalBounds.isEmpty() && cnv->getLocalBounds().contains(lastMousePosition)) {
                auto pos = lastMousePosition - cnv->getMouseXYRelative();
                pos = pos + cnv->viewport->getViewPosition();
                cnv->viewport->setViewPosition(pos);
            }

            // Otherwise don't adjust viewport position
        }

        zoomLabel.setZoomLevel(scale);
    }
    // Update theme
    else if (v.refersToSameSourceAs(theme)) {
        pd->setTheme(theme.toString());
        getTopLevelComponent()->repaint();
    }
}

void PluginEditor::modifierKeysChanged(ModifierKeys const& modifiers)
{
    setModifierKeys(modifiers);
}

void PluginEditor::updateCommandStatus()
{
    if (auto* cnv = getCurrentCanvas()) {
        // Update connection style button
        bool allSegmented = true;
        bool allNotSegmented = true;
        bool hasSelection = false;

        bool isDragging = cnv->didStartDragging && !cnv->isDraggingLasso && statusbar.locked == var(false);
        for (auto& connection : cnv->getSelectionOfType<Connection>()) {
            allSegmented = allSegmented && connection->isSegmented();
            allNotSegmented = allNotSegmented && !connection->isSegmented();
            hasSelection = true;
        }

        statusbar.connectionStyleButton->setEnabled(!isDragging && hasSelection);
        statusbar.connectionPathfind->setEnabled(!isDragging && hasSelection);
        statusbar.connectionStyleButton->setToggleState(!isDragging && hasSelection && allSegmented, dontSendNotification);

        statusbar.lockButton->setEnabled(!isDragging);
        statusbar.attachToCanvas(cnv);

        auto* patchPtr = cnv->patch.getPointer();
        if (!patchPtr)
            return;

        auto deletionCheck = SafePointer(this);

        bool locked = static_cast<bool>(cnv->locked.getValue());

        // First on pd's thread, get undo status
        pd->enqueueFunction(
            [this, patchPtr, isDragging, deletionCheck, locked]() mutable {
                if (!deletionCheck)
                    return;

                canUndo = libpd_can_undo(patchPtr) && !isDragging && !locked;
                canRedo = libpd_can_redo(patchPtr) && !isDragging && !locked;

                // Set button enablement on message thread
                MessageManager::callAsync(
                    [this, deletionCheck]() mutable {
                        if (!deletionCheck)
                            return;

                        undoButton.setEnabled(canUndo);
                        redoButton.setEnabled(canRedo);

                        // Application commands need to be updated when undo state changes
                        commandStatusChanged();
                    });
            });

        statusbar.lockButton->setEnabled(true);
        statusbar.presentationButton->setEnabled(true);
        statusbar.gridButton->setEnabled(true);

        addObjectMenuButton.setEnabled(!locked);
    } else {
        statusbar.connectionStyleButton->setEnabled(false);
        statusbar.connectionPathfind->setEnabled(false);
        statusbar.lockButton->setEnabled(false);

        statusbar.lockButton->setEnabled(false);
        statusbar.presentationButton->setEnabled(false);
        statusbar.gridButton->setEnabled(false);

        undoButton.setEnabled(false);
        redoButton.setEnabled(false);
        addObjectMenuButton.setEnabled(false);
    }
}

ApplicationCommandTarget* PluginEditor::getNextCommandTarget()
{
    return this;
}

void PluginEditor::getAllCommands(Array<CommandID>& commands)
{
    // Add all command IDs
    for (int n = NewProject; n < NumItems; n++) {
        commands.add(n);
    }

    // Add all object IDs
    for (int n = NewObject; n < NumEssentialObjects; n++) {
        commands.add(n);
    }
}

void PluginEditor::getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result)
{
    bool hasBoxSelection = false;
    bool hasSelection = false;
    bool isDragging = false;
    bool hasCanvas = false;
    bool locked = true;
    bool canConnect = false;

    if (auto* cnv = getCurrentCanvas()) {
        auto selectedBoxes = cnv->getSelectionOfType<Object>();
        auto selectedConnections = cnv->getSelectionOfType<Connection>();

        hasBoxSelection = !selectedBoxes.isEmpty();
        hasSelection = hasBoxSelection || !selectedConnections.isEmpty();
        isDragging = cnv->didStartDragging && !cnv->isDraggingLasso && statusbar.locked == var(false);
        hasCanvas = true;

        locked = static_cast<bool>(cnv->locked.getValue());
        canConnect = cnv->canConnectSelectedObjects();
    }

    switch (commandID) {
    case CommandIDs::NewProject: {
        result.setInfo("New patch", "Create a new patch", "General", 0);
        result.addDefaultKeypress(78, ModifierKeys::commandModifier);
        break;
    }
    case CommandIDs::OpenProject: {
        result.setInfo("Open patch...", "Open a patch", "General", 0);
        result.addDefaultKeypress(79, ModifierKeys::commandModifier);
        break;
    }
    case CommandIDs::SaveProject: {
        result.setInfo("Save patch", "Save patch at current location", "General", 0);
        result.addDefaultKeypress(83, ModifierKeys::commandModifier);
        result.setActive(hasCanvas);
        break;
    }
    case CommandIDs::SaveProjectAs: {
        result.setInfo("Save patch as...", "Save patch in chosen location", "General", 0);
        result.addDefaultKeypress(83, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas);
        break;
    }
    case CommandIDs::CloseTab: {
        result.setInfo("Close tab", "Close currently opened tab", "General", 0);
        result.addDefaultKeypress(87, ModifierKeys::commandModifier);
        result.setActive(hasCanvas);
        break;
    }
    case CommandIDs::Undo: {
        result.setInfo("Undo", "Undo action", "General", 0);
        result.addDefaultKeypress(90, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && canUndo);

        break;
    }

    case CommandIDs::Redo: {
        result.setInfo("Redo", "Redo action", "General", 0);
        result.addDefaultKeypress(90, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas && !isDragging && canRedo);
        break;
    }
    case CommandIDs::Lock: {
        result.setInfo("Run mode", "Run Mode", "Edit", 0);
        result.addDefaultKeypress(69, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::ConnectionPathfind: {
        result.setInfo("Tidy connection", "Find best path for connection", "Edit", 0);
        result.addDefaultKeypress(89, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::ConnectionStyle: {
        result.setInfo("Connection style", "Set connection style", "Edit", 0);
        result.setActive(hasCanvas && !isDragging && statusbar.connectionStyleButton->isEnabled());
        break;
    }
    case CommandIDs::ZoomIn: {
        result.setInfo("Zoom in", "Zoom in", "Edit", 0);
        result.addDefaultKeypress(61, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::ZoomOut: {
        result.setInfo("Zoom out", "Zoom out", "Edit", 0);
        result.addDefaultKeypress(45, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::ZoomNormal: {
        result.setInfo("Zoom 100%", "Revert zoom to 100%", "Edit", 0);
        result.addDefaultKeypress(33, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::Copy: {
        result.setInfo("Copy", "Copy", "Edit", 0);
        result.addDefaultKeypress(67, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !locked && hasBoxSelection && !isDragging);
        break;
    }
    case CommandIDs::Cut: {
        result.setInfo("Cut", "Cut selection", "Edit", 0);
        result.addDefaultKeypress(88, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !locked && hasSelection && !isDragging);
        break;
    }
    case CommandIDs::Paste: {
        result.setInfo("Paste", "Paste", "Edit", 0);
        result.addDefaultKeypress(86, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !locked && !isDragging);
        break;
    }
    case CommandIDs::Delete: {
        result.setInfo("Delete", "Delete selection", "Edit", 0);
        result.addDefaultKeypress(KeyPress::backspaceKey, ModifierKeys::noModifiers);
        result.addDefaultKeypress(KeyPress::deleteKey, ModifierKeys::noModifiers);

        result.setActive(hasCanvas && !isDragging && !locked && hasSelection);
        break;
    }
    case CommandIDs::Encapsulate: {
        result.setInfo("Encapsulate", "Encapsulate objects", "Edit", 0);
        result.addDefaultKeypress(69, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas && !isDragging && !locked && hasSelection);
        break;
    }
    case CommandIDs::Duplicate: {

        result.setInfo("Duplicate", "Duplicate selection", "Edit", 0);
        result.addDefaultKeypress(68, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && !locked && hasBoxSelection);
        break;
    }
    case CommandIDs::CreateConnection: {
        result.setInfo("Create connection", "Create a connection between selected objects", "General", 0);
        result.addDefaultKeypress(75, ModifierKeys::commandModifier);
        result.setActive(canConnect);
        break;
    }
    case CommandIDs::SelectAll: {
        result.setInfo("Select all", "Select all objects and connections", "Edit", 0);
        result.addDefaultKeypress(65, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && !locked);
        break;
    }
    case CommandIDs::ShowBrowser: {
        result.setInfo("Show Browser", "Open documentation browser panel", "Edit", 0);
        result.addDefaultKeypress(66, ModifierKeys::commandModifier);
        result.setActive(true);
        break;
    }
    case CommandIDs::Search: {
        result.setInfo("Search Current Patch", "Search for objects in current patch", "Edit", 0);
        result.addDefaultKeypress(70, ModifierKeys::commandModifier);
        result.setActive(true);
        break;
    }
    case CommandIDs::NextTab: {
        result.setInfo("Next Tab", "Show the next tab", "View", 0);
        result.setActive(true);
        
#if JUCE_MAC
        result.addDefaultKeypress(KeyPress::rightKey, ModifierKeys::commandModifier);
#else
        result.addDefaultKeypress(KeyPress::pageDownKey, ModifierKeys::commandModifier);
#endif
        break;
    }
    case CommandIDs::PreviousTab: {
        result.setInfo("Previous Tab", "Show the previous tab", "View", 0);
        result.setActive(true);
        
#if JUCE_MAC
        result.addDefaultKeypress(KeyPress::leftKey, ModifierKeys::commandModifier);
#else
        result.addDefaultKeypress(KeyPress::pageUpKey, ModifierKeys::commandModifier);
#endif
        break;
    }
    case CommandIDs::ToggleGrid: {
        result.setInfo("Toggle grid", "Toggle grid enablement", "Edit", 0);
        result.addDefaultKeypress(103, ModifierKeys::commandModifier);
        result.setActive(true);
        break;
    }
    case CommandIDs::ClearConsole: {
        result.setInfo("Clear Console", "Clear the console", "Edit", 0);
        result.addDefaultKeypress(76, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(true);
        break;
    }
    case CommandIDs::ShowSettings: {
        result.setInfo("Open Settings", "Open settings panel", "Edit", 0);
        result.addDefaultKeypress(44, ModifierKeys::commandModifier); // Cmd + , to open settings
        result.setActive(true);
        break;
    }
    case CommandIDs::ShowReference: {
        result.setInfo("Open Reference", "Open reference panel", "Edit", 0);
        result.addDefaultKeypress(KeyPress::F1Key, ModifierKeys::noModifiers); // f1 to open settings

        if (auto* cnv = getCurrentCanvas()) {
            auto selection = cnv->getSelectionOfType<Object>();
            bool enabled = selection.size() == 1 && selection[0]->gui && selection[0]->gui->getType().isNotEmpty();
            result.setActive(enabled);
        } else {
            result.setActive(false);
        }
        break;
    }
    }

    static auto const cmdMod = ModifierKeys::commandModifier;
    static auto const shiftMod = ModifierKeys::shiftModifier;
    static const std::map<ObjectIDs, std::pair<int, int>> defaultShortcuts = {
        { NewObject, { 49, cmdMod } },
        { NewComment, { 53, cmdMod } },
        { NewBang, { 66, cmdMod | shiftMod } },
        { NewMessage, { 50, cmdMod } },
        { NewToggle, { 84, cmdMod | shiftMod } },
        { NewNumbox, { 78, cmdMod | shiftMod } },
        { NewVerticalSlider, { 86, cmdMod | shiftMod } },
        { NewHorizontalSlider, { 74, cmdMod | shiftMod } },
        { NewVerticalRadio, { 68, cmdMod | shiftMod } },
        { NewHorizontalRadio, { 73, cmdMod | shiftMod } },
        { NewFloatAtom, { 51, cmdMod } },
        { NewListAtom, { 52, cmdMod } },
        { NewArray, { 65, cmdMod | shiftMod } },
        { NewGraphOnParent, { 71, cmdMod | shiftMod } },
        { NewCanvas, { 67, cmdMod | shiftMod } },
        { NewVUMeterObject, { 85, cmdMod | shiftMod } }
    };

    if (commandID >= ObjectIDs::NewObject) {
        auto name = objectNames.at(static_cast<ObjectIDs>(commandID));

        if (name.isEmpty())
            name = "object";

        bool essential = commandID > NumEssentialObjects;

        result.setInfo("New " + name, "Create new " + name, essential ? "Objects" : "Extra", 0);
        result.setActive(hasCanvas && !isDragging && !locked);

        if (defaultShortcuts.count(static_cast<ObjectIDs>(commandID))) {
            auto [key, mods] = defaultShortcuts.at(static_cast<ObjectIDs>(commandID));
            result.addDefaultKeypress(key, mods);
        }
    }
}

bool PluginEditor::perform(InvocationInfo const& info)
{
    if (info.commandID == CommandIDs::NewProject) {
        newProject();
        return true;
    } else if (info.commandID == CommandIDs::OpenProject) {
        openProject();
        return true;
    } else if (info.commandID == CommandIDs::ShowBrowser) {
        sidebar.showPanel(sidebar.isShowingBrowser() ? 0 : 1);
        return true;
    } else if (info.commandID == CommandIDs::Search) {
        sidebar.showPanel(3);
        return true;
    }

    auto* cnv = getCurrentCanvas();

    if (!cnv)
        return false;

    auto lastPosition = cnv->viewport->getViewArea().getConstrainedPoint(cnv->lastMousePosition - Point<int>(Object::margin, Object::margin));

    switch (info.commandID) {
    case CommandIDs::SaveProject: {
        saveProject();
        return true;
    }
    case CommandIDs::SaveProjectAs: {
        saveProjectAs();
        return true;
    }
    case CommandIDs::CloseTab: {
        
        if (splitView.getActiveTabbar()->getNumTabs() == 0)
            return true;
        
        closeTab(getCurrentCanvas());

        return true;

       
    }
    case CommandIDs::Copy: {
        cnv->copySelection();
        return true;
    }
    case CommandIDs::Paste: {
        cnv->pasteSelection();
        return true;
    }
    case CommandIDs::Cut: {
        cnv->cancelConnectionCreation();
        cnv->copySelection();
        cnv->removeSelection();
        return true;
    }
    case CommandIDs::Delete: {
        cnv->cancelConnectionCreation();
        cnv->removeSelection();
        return true;
    }
    case CommandIDs::Duplicate: {
        cnv->duplicateSelection();
        return true;
    }
    case CommandIDs::Encapsulate: {
        cnv->encapsulateSelection();
        return true;
    }
    case CommandIDs::CreateConnection: {
        return cnv->connectSelectedObjects();
    }
    case CommandIDs::SelectAll: {
        for (auto* object : cnv->objects) {
            cnv->setSelected(object, true);
        }
        for (auto* con : cnv->connections) {
            cnv->setSelected(con, true);
        }
        return true;
    }
    case CommandIDs::Lock: {
        statusbar.lockButton->triggerClick();
        return true;
    }
    case CommandIDs::ConnectionPathfind: {
        cnv->patch.startUndoSequence("ConnectionPathFind");

        statusbar.connectionStyleButton->setToggleState(true, sendNotification);
        for (auto* con : cnv->connections) {
            if (cnv->isSelected(con)) {
                con->findPath();
                con->updatePath();
            }
        }

        cnv->patch.endUndoSequence("ConnectionPathFind");

        return true;
    }
    case CommandIDs::ZoomIn: {
        float newScale = static_cast<float>(zoomScale.getValue()) + 0.1f;
        zoomScale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.5f, 2.0f) * 10.))) / 10.;

        return true;
    }
    case CommandIDs::ZoomOut: {
        float newScale = static_cast<float>(zoomScale.getValue()) - 0.1f;
        zoomScale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.5f, 2.0f) * 10.))) / 10.;

        return true;
    }
    case CommandIDs::ZoomNormal: {
        zoomScale = 1.0f;
        return true;
    }
    case CommandIDs::Undo: {
        cnv->undo();
        return true;
    }
    case CommandIDs::Redo: {
        cnv->redo();
        return true;
    }
    case CommandIDs::NextTab: {
        
        auto* tabbar = cnv->getTabbar();
        
        int currentIdx = cnv->getTabIndex() + 1;

        if (currentIdx >= tabbar->getNumTabs())
            currentIdx -= tabbar->getNumTabs();
        if (currentIdx < 0)
            currentIdx += tabbar->getNumTabs();

        tabbar->setCurrentTabIndex(currentIdx);

        return true;
    }
    case CommandIDs::PreviousTab: {
        auto* tabbar = cnv->getTabbar();
        
        int currentIdx = cnv->getTabIndex() - 1;

        if (currentIdx >= tabbar->getNumTabs())
            currentIdx -= tabbar->getNumTabs();
        if (currentIdx < 0)
            currentIdx += tabbar->getNumTabs();

        tabbar->setCurrentTabIndex(currentIdx);

        return true;
    }
    case CommandIDs::ToggleGrid: {
        auto value = SettingsFile::getInstance()->getProperty<int>("grid_enabled");
        SettingsFile::getInstance()->setProperty("grid_enabled", !value);

        return true;
    }
    case CommandIDs::ClearConsole: {
        sidebar.clearConsole();
        return true;
    }
    case CommandIDs::ShowSettings: {
        Dialogs::showSettingsDialog(this);

        return true;
    }
    case CommandIDs::ShowReference: {
        if (auto* cnv = getCurrentCanvas()) {
            auto selection = cnv->getSelectionOfType<Object>();
            if (selection.size() != 1 || !selection[0]->gui || selection[0]->gui->getType().isEmpty()) {
                return false;
            }

            Dialogs::showObjectReferenceDialog(&openedDialog, this, selection[0]->gui->getType());

            return true;
        }

        return false;
    }
    case ObjectIDs::NewArray: {

        Dialogs::showArrayDialog(&openedDialog, this,
            [this](int result, String const& name, String const& size) {
                if (result) {
                    auto* cnv = getCurrentCanvas();
                    auto* object = new Object(cnv, "graph " + name + " " + size, cnv->viewport->getViewArea().getCentre());
                    cnv->objects.add(object);
                }
            });
        return true;
    }

    default: {

        auto ID = static_cast<ObjectIDs>(info.commandID);

        if (objectNames.count(ID)) {
            if (cnv->getSelectionOfType<Object>().size() == 1) {
                // if 1 object is selected, create new object beneath selected
                auto obj = cnv->lastSelectedObject = cnv->getSelectionOfType<Object>()[0];
                if (obj) {
                    cnv->objects.add(new Object(cnv, objectNames.at(ID),
                        Point<int>(
                            // place beneath object + Object::margin
                            obj->getX() + Object::margin,
                            obj->getY() + obj->getHeight())));
                }
            } else if ((cnv->getSelectionOfType<Object>().size() == 0) && (cnv->getSelectionOfType<Connection>().size() == 1)) {
                // if 1 connection is selected, create new object in the middle of connection
                cnv->patch.startUndoSequence("ObjectInConnection");
                cnv->lastSelectedConnection = cnv->getSelectionOfType<Connection>().getFirst();
                auto outobj = cnv->getSelectionOfType<Connection>().getFirst()->outobj;
                cnv->objects.add(new Object(cnv, objectNames.at(ID),
                    Point<int>(
                        // place beneath outlet object + Object::margin
                        cnv->lastSelectedConnection->getX() + (cnv->lastSelectedConnection->getWidth() / 2) - 12,
                        cnv->lastSelectedConnection->getY() + (cnv->lastSelectedConnection->getHeight() / 2) - 12)));
                cnv->patch.endUndoSequence("ObjectInConnection");
            } else {
                // if 0 or several objects are selected, create new object at mouse position
                cnv->objects.add(new Object(cnv, objectNames.at(ID), lastPosition));
            }
            cnv->deselectAll();
            if (auto obj = cnv->objects.getLast())
                cnv->setSelected(obj, true); // Select newly created object
            return true;
        }

        return false;
    }
    }
}

bool PluginEditor::wantsRoundedCorners()
{
#if PLUGDATA_STANDALONE
    if (auto* window = findParentComponentOfClass<PlugDataWindow>()) {
        return !window->isUsingNativeTitleBar() && Desktop::canUseSemiTransparentWindows();
    } else {
        return true;
    }
#else
    return false;
#endif
}

void PluginEditor::updateSplitOutline()
{
    auto* tabbar = splitView.getActiveTabbar();
    if (splitView.isSplitEnabled() && tabbar) {
        if(auto* cnv = tabbar->getCurrentCanvas()) {
            bool isOnLeft = tabbar == splitView.getLeftTabbar();

            auto bounds = getLocalArea(tabbar, tabbar->getLocalBounds()).withTrimmedTop(tabbar->getTabBarDepth()).toFloat().withTrimmedRight(isOnLeft ? 0.0f : 0.5f).withTrimmedLeft(isOnLeft ? 0.5f : 0.0f).withTrimmedBottom(-0.5f);
            selectedSplitRect.setRectangle(Parallelogram<float>(bounds));
            selectedSplitRect.toFront(false);
            selectedSplitRect.setVisible(true);
        }
    }
    else
    {
        selectedSplitRect.setVisible(false);
    }
}
