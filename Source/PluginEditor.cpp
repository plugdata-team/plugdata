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
{
    toolbarButtons = { new TextButton(Icons::Menu),
        new TextButton(Icons::Undo),
        new TextButton(Icons::Redo),
        new TextButton(Icons::Add),
        new TextButton(Icons::Hide),
        new TextButton(Icons::Pin) };

#if PLUGDATA_STANDALONE
    // In the standalone, the resizer handling is done on the window class
    setResizable(true, false);
#else
    setResizable(true, true);
#endif

    tooltipWindow.setOpaque(false);
    tooltipWindow.setLookAndFeel(&pd->lnf.get());

    tooltipShadow.setOwner(&tooltipWindow);

    addKeyListener(getKeyMappings());

    pd->settingsTree.addListener(this);

    setWantsKeyboardFocus(true);
    registerAllCommandsForTarget(this);

    for (auto& seperator : seperators) {
        addChildComponent(&seperator);
    }

    auto keymap = p.settingsTree.getChildWithName("Keymap");
    if (keymap.isValid()) {
        auto xmlStr = keymap.getProperty("keyxml").toString();
        auto elt = XmlDocument(xmlStr).getDocumentElement();

        if (elt) {
            getKeyMappings()->restoreFromXml(*elt);
        }
    } else {
        p.settingsTree.appendChild(ValueTree("Keymap"), nullptr);
    }

    autoconnect.referTo(pd->settingsTree.getPropertyAsValue("AutoConnect", nullptr));

    theme.referTo(pd->settingsTree.getPropertyAsValue("Theme", nullptr));
    theme.addListener(this);

    if (!pd->settingsTree.hasProperty("HvccMode"))
        pd->settingsTree.setProperty("HvccMode", false, nullptr);
    hvccMode.referTo(pd->settingsTree.getPropertyAsValue("HvccMode", nullptr));

    zoomScale.referTo(pd->settingsTree.getPropertyAsValue("Zoom", nullptr));
    zoomScale.addListener(this);

    addAndMakeVisible(statusbar);

    tabbar.newTab = [this]() {
        newProject();
    };

    tabbar.openProject = [this]() {
        openProject();
    };

    tabbar.onTabChange = [this](int idx) {
        if (idx == -1 || pd->isPerformingGlobalSync)
            return;

        sidebar.tabChanged();

        // update GraphOnParent when changing tabs
        for (auto* object : getCurrentCanvas()->objects) {
            if (!object->gui)
                continue;
            if (auto* cnv = object->gui->getCanvas())
                cnv->synchronise();
        }

        auto* cnv = getCurrentCanvas();
        if (cnv->patch.getPointer()) {
            cnv->patch.setCurrent();
        }

        cnv->synchronise();
        cnv->updateGuiValues();
        cnv->updateDrawables();

        updateCommandStatus();
    };

    tabbar.setOutline(0);
    addAndMakeVisible(tabbar);
    addAndMakeVisible(sidebar);

    for (auto* button : toolbarButtons) {
        button->setName("toolbar:button");
        button->setConnectedEdges(12);
        addAndMakeVisible(button);
    }

    // Show settings
    toolbarButton(Settings)->setTooltip("Settings");
    toolbarButton(Settings)->onClick = [this]() {
#ifdef PLUGDATA_STANDALONE
        // Initialise settings dialog for DAW and standalone
        auto* pluginHolder = findParentComponentOfClass<PlugDataWindow>()->getPluginHolder();

        Dialogs::createSettingsDialog(pd, &pluginHolder->deviceManager, toolbarButton(Settings), pd->settingsTree);
#else
        Dialogs::createSettingsDialog(pd, nullptr, toolbarButton(Settings), pd->settingsTree);
#endif
    };

    addAndMakeVisible(toolbarButton(Settings));

    //  Undo button
    toolbarButton(Undo)->setTooltip("Undo");
    toolbarButton(Undo)->onClick = [this]() { getCurrentCanvas()->undo(); };
    addAndMakeVisible(toolbarButton(Undo));

    // Redo button
    toolbarButton(Redo)->setTooltip("Redo");
    toolbarButton(Redo)->onClick = [this]() { getCurrentCanvas()->redo(); };
    addAndMakeVisible(toolbarButton(Redo));

    // New object button
    toolbarButton(Add)->setTooltip("Create object");
    toolbarButton(Add)->onClick = [this]() { Dialogs::showObjectMenu(this, toolbarButton(Add)); };
    addAndMakeVisible(toolbarButton(Add));

    // Hide sidebar
    toolbarButton(Hide)->setTooltip("Hide Sidebar");
    toolbarButton(Hide)->setName("toolbar:hide");
    toolbarButton(Hide)->setClickingTogglesState(true);
    toolbarButton(Hide)->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    toolbarButton(Hide)->setConnectedEdges(12);
    toolbarButton(Hide)->onClick = [this]() {
        bool show = !toolbarButton(Hide)->getToggleState();

        sidebar.showSidebar(show);
        toolbarButton(Hide)->setButtonText(show ? Icons::Hide : Icons::Show);
        toolbarButton(Hide)->setTooltip(show ? "Hide Sidebar" : "Show Sidebar");
        toolbarButton(Pin)->setVisible(show);

        repaint();
        resized();
    };

    // Hide pin sidebar panel
    toolbarButton(Pin)->setTooltip("Pin Panel");
    toolbarButton(Pin)->setName("toolbar:pin");
    toolbarButton(Pin)->setClickingTogglesState(true);
    toolbarButton(Pin)->setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    toolbarButton(Pin)->setConnectedEdges(12);
    toolbarButton(Pin)->onClick = [this]() { sidebar.pinSidebar(toolbarButton(Pin)->getToggleState()); };

    addAndMakeVisible(toolbarButton(Hide));

    sidebar.setSize(250, pd->lastUIHeight - 40);
    setSize(pd->lastUIWidth, pd->lastUIHeight);

    // Set minimum bounds
    setResizeLimits(835, 305, 999999, 999999);

    tabbar.toFront(false);
    sidebar.toFront(false);

    // Make sure existing console messages are processed
    sidebar.updateConsole();
    updateCommandStatus();

    addChildComponent(zoomLabel);

    // Initialise zoom factor
    valueChanged(zoomScale);
}
PluginEditor::~PluginEditor()
{
    setConstrainer(nullptr);

    pd->settingsTree.removeListener(this);
    zoomScale.removeListener(this);
    theme.removeListener(this);

    pd->lastTab = tabbar.getCurrentTabIndex();
}

void PluginEditor::paint(Graphics& g)
{
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
    if (isDraggingFile) {
        g.setColour(findColour(PlugDataColour::scrollbarThumbColourId));
        g.drawRect(getLocalBounds().reduced(1), 2.0f);
    }
}

void PluginEditor::resized()
{
    sidebar.setBounds(getWidth() - sidebar.getWidth(), toolbarHeight, sidebar.getWidth(), getHeight() - toolbarHeight);

    tabbar.setBounds(0, toolbarHeight, (getWidth() - sidebar.getWidth()) + 1, getHeight() - toolbarHeight - (statusbar.getHeight()));

    statusbar.setBounds(0, getHeight() - statusbar.getHeight(), getWidth() - sidebar.getWidth(), statusbar.getHeight());

    toolbarButton(Settings)->setBounds(20, 0, toolbarHeight, toolbarHeight);
    toolbarButton(Undo)->setBounds(100, 0, toolbarHeight, toolbarHeight);
    toolbarButton(Redo)->setBounds(180, 0, toolbarHeight, toolbarHeight);
    toolbarButton(Add)->setBounds(260, 0, toolbarHeight, toolbarHeight);

#ifdef PLUGDATA_STANDALONE
    auto useNativeTitlebar = static_cast<bool>(pd->settingsTree.getProperty("NativeWindow"));
    auto windowControlsOffset = useNativeTitlebar ? 70.0f : 170.0f;
#else
    auto windowControlsOffset = 70.0f;
#endif
    

    int hidePosition = getWidth() - windowControlsOffset;
    int pinPosition = hidePosition - 65;

    toolbarButton(Hide)->setBounds(hidePosition, 0, 70, toolbarHeight);
    toolbarButton(Pin)->setBounds(pinPosition, 0, 70, toolbarHeight);

    pd->lastUIWidth = getWidth();
    pd->lastUIHeight = getHeight();

    zoomLabel.setTopLeftPosition(5, statusbar.getY() - 28);
    zoomLabel.setSize(55, 23);

    if (auto* cnv = getCurrentCanvas()) {
        cnv->checkBounds();
    }

    repaint();
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
        auto* window = getTopLevelComponent();
        windowDragger.startDraggingComponent(window, e.getEventRelativeTo(window));
    }
}

void PluginEditor::mouseDrag(MouseEvent const& e)
{
    if (!isMaximised) {
        auto* window = getTopLevelComponent();
        windowDragger.dragComponent(window, e.getEventRelativeTo(window), nullptr);
    }
}
#endif

bool PluginEditor::isInterestedInFileDrag(StringArray const& files)
{
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
            addToRecentlyOpened(file);
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

void PluginEditor::addToRecentlyOpened(File path)
{
    auto recentlyOpened = pd->settingsTree.getChildWithName("RecentlyOpened");

    if (!recentlyOpened.isValid()) {
        recentlyOpened = ValueTree("RecentlyOpened");
        pd->settingsTree.appendChild(recentlyOpened, nullptr);
    }

    if (recentlyOpened.getChildWithProperty("Path", path.getFullPathName()).isValid()) {

        recentlyOpened.getChildWithProperty("Path", path.getFullPathName()).setProperty("Time", Time::getCurrentTime().toMilliseconds(), nullptr);

        int oldIdx = recentlyOpened.indexOf(recentlyOpened.getChildWithProperty("Path", path.getFullPathName()));
        recentlyOpened.moveChild(oldIdx, 0, nullptr);
    } else {
        ValueTree subTree("Path");
        subTree.setProperty("Path", path.getFullPathName(), nullptr);
        subTree.setProperty("Time", Time::getCurrentTime().toMilliseconds(), nullptr);
        recentlyOpened.appendChild(subTree, nullptr);
    }

    while (recentlyOpened.getNumChildren() > 10) {
        auto minTime = Time::getCurrentTime().toMilliseconds();
        int minIdx = -1;

        // Find oldest entry
        for (int i = 0; i < recentlyOpened.getNumChildren(); i++) {
            auto time = static_cast<int>(recentlyOpened.getChild(i).getProperty("Time"));
            if (time < minTime) {
                minIdx = i;
                minTime = time;
            }
        }

        recentlyOpened.removeChild(minIdx, nullptr);
    }
}

void PluginEditor::openProject()
{
    auto openFunc = [this](FileChooser const& f) {
        File openedFile = f.getResult();

        if (openedFile.exists() && openedFile.getFileExtension().equalsIgnoreCase(".pd")) {
            pd->settingsTree.setProperty("LastChooserPath", openedFile.getParentDirectory().getFullPathName(), nullptr);

            pd->loadPatch(openedFile);
            addToRecentlyOpened(openedFile);
        }
    };

    openChooser = std::make_unique<FileChooser>("Choose file to open", File(pd->settingsTree.getProperty("LastChooserPath")), "*.pd", wantsNativeDialog());

    openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
}

void PluginEditor::saveProjectAs(std::function<void()> const& nestedCallback)
{
    saveChooser = std::make_unique<FileChooser>("Select a save file", File(pd->settingsTree.getProperty("LastChooserPath")), "*.pd", wantsNativeDialog());

    saveChooser->launchAsync(FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting,
        [this, nestedCallback](FileChooser const& f) mutable {
            File result = saveChooser->getResult();

            if (result.getFullPathName().isNotEmpty()) {
                pd->settingsTree.setProperty("LastChooserPath", result.getParentDirectory().getFullPathName(), nullptr);

                result.deleteFile();
                result = result.withFileExtension(".pd");

                getCurrentCanvas()->patch.savePatch(result);
                addToRecentlyOpened(result);
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
        addToRecentlyOpened(getCurrentCanvas()->patch.getCurrentFile());
        nestedCallback();
    } else {
        saveProjectAs(nestedCallback);
    }
}

Canvas* PluginEditor::getCurrentCanvas()
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getCurrentContentComponent())) {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }
    return nullptr;
}

Canvas* PluginEditor::getCanvas(int idx)
{
    if (auto* viewport = dynamic_cast<Viewport*>(tabbar.getTabContentComponent(idx))) {
        if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
            return cnv;
        }
    }

    return nullptr;
}

void PluginEditor::addTab(Canvas* cnv, bool deleteWhenClosed)
{
    tabbar.addTab(cnv->patch.getTitle(), findColour(ResizableWindow::backgroundColourId), cnv->viewport, true);

    int tabIdx = tabbar.getNumTabs() - 1;

    tabbar.setCurrentTabIndex(tabIdx);
    tabbar.setTabBackgroundColour(tabIdx, Colours::transparentBlack);

    auto* tabButton = tabbar.getTabbedButtonBar().getTabButton(tabIdx);
    tabButton->setTriggeredOnMouseDown(true);

    auto* closeTabButton = new TextButton(Icons::Clear);

    closeTabButton->onClick = [this, tabButton, deleteWhenClosed]() mutable {
        // We cant use the index from earlier because it might change!
        int idx = -1;
        for (int i = 0; i < tabbar.getNumTabs(); i++) {
            if (tabbar.getTabbedButtonBar().getTabButton(i) == tabButton) {
                idx = i;
                break;
            }
        }

        if (idx == -1)
            return;

        auto deleteFunc = [this, deleteWhenClosed, idx]() mutable {
            auto* cnv = getCanvas(idx);

            if (!cnv) {
                tabbar.removeTab(idx);
                return;
            }

            auto* patch = &cnv->patch;

            if (deleteWhenClosed) {
                patch->close();
            }

            canvases.removeObject(cnv);
            tabbar.removeTab(idx);
            pd->patches.removeObject(patch);

            tabbar.setCurrentTabIndex(tabbar.getNumTabs() - 1, true);
            updateCommandStatus();
        };

        MessageManager::callAsync(
            [this, deleteFunc, idx]() mutable {
                auto cnv = SafePointer(getCanvas(idx));
                if (cnv && cnv->patch.isDirty()) {
                    Dialogs::showSaveDialog(&openedDialog, this, cnv->patch.getTitle(),
                        [this, deleteFunc, cnv](int result) mutable {
                            if (!cnv)
                                return;
                            if (result == 2) {
                                saveProject([deleteFunc]() mutable { deleteFunc(); });
                            } else if (result == 1) {
                                deleteFunc();
                            }
                        });
                } else if (cnv) {
                    deleteFunc();
                }
            });
    };

    closeTabButton->setName("tab:close");
    closeTabButton->setColour(TextButton::buttonColourId, Colour());
    closeTabButton->setColour(TextButton::buttonOnColourId, Colour());
    closeTabButton->setColour(ComboBox::outlineColourId, Colour());
    closeTabButton->setConnectedEdges(12);
    tabButton->setExtraComponent(closeTabButton, TabBarButton::beforeText);
    closeTabButton->setSize(28, 28);

    tabbar.repaint();

    cnv->setVisible(true);

    updateCommandStatus();
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

void PluginEditor::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property)
{
    pd->settingsChangedInternally = true;
    startTimer(300);
}
void PluginEditor::valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
{
    pd->settingsChangedInternally = true;
    startTimer(300);
}
void PluginEditor::valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved)
{
    pd->settingsChangedInternally = true;
    startTimer(300);
}

void PluginEditor::timerCallback()
{
    // Save settings to file whenever valuetree state changes
    // Use timer to group changes together
    pd->saveSettings();
    stopTimer();
}

void PluginEditor::modifierKeysChanged(ModifierKeys const& modifiers)
{
    statusbar.modifierKeysChanged(modifiers);
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

                        toolbarButton(Undo)->setEnabled(canUndo);
                        toolbarButton(Redo)->setEnabled(canRedo);

                        // Application commands need to be updated when undo state changes
                        commandStatusChanged();
                    });
            });

        statusbar.lockButton->setEnabled(true);
        statusbar.presentationButton->setEnabled(true);
        statusbar.gridButton->setEnabled(true);

        toolbarButton(Add)->setEnabled(!locked);
    } else {
        statusbar.connectionStyleButton->setEnabled(false);
        statusbar.connectionPathfind->setEnabled(false);
        statusbar.lockButton->setEnabled(false);

        statusbar.lockButton->setEnabled(false);
        statusbar.presentationButton->setEnabled(false);
        statusbar.gridButton->setEnabled(false);

        toolbarButton(Undo)->setEnabled(false);
        toolbarButton(Redo)->setEnabled(false);
        toolbarButton(Add)->setEnabled(false);
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
    for (int n = NewObject; n < NumObjects; n++) {
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
        result.setActive(hasCanvas && !isDragging && !static_cast<bool>(statusbar.presentationMode.getValue()));
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

#if JUCE_MAC
        result.addDefaultKeypress(KeyPress::rightKey, ModifierKeys::commandModifier);
#else
        result.addDefaultKeypress(KeyPress::pageDownKey, ModifierKeys::commandModifier);
#endif

        break;
    }
    case CommandIDs::PreviousTab: {
        result.setInfo("Previous Tab", "Show the previous tab", "View", 0);

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
    }

    static auto const cmdMod = ModifierKeys::commandModifier;
    static auto const shiftMod = ModifierKeys::shiftModifier;
    static const std::map<ObjectIDs, std::pair<int, int>> defaultShortcuts = {
        { NewObject, { 49, cmdMod } },
        { NewComment, { 53, cmdMod } },
        { NewBang, { 66, cmdMod | shiftMod } },
        { NewMessage, { 50, cmdMod } },
        { NewToggle, { 84, cmdMod | shiftMod } },
        { NewNumbox, { 70, cmdMod | shiftMod } },
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

        result.setInfo("New " + name, "Create new " + name, "Objects", 0);
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
        if (tabbar.getNumTabs() == 0)
            return true;

        int currentIdx = tabbar.getCurrentTabIndex();
        auto* closeTabButton = dynamic_cast<TextButton*>(tabbar.getTabbedButtonBar().getTabButton(currentIdx)->getExtraComponent());

        // Virtually click the close button
        closeTabButton->triggerClick();

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
        cnv->copySelection();
        cnv->removeSelection();
        return true;
    }
    case CommandIDs::Delete: {
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
        statusbar.connectionStyleButton->setToggleState(true, sendNotification);
        for (auto* con : cnv->connections) {
            if (cnv->isSelected(con)) {
                con->findPath();
                con->updatePath();
            }
        }

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
        int currentIdx = canvases.indexOf(cnv) + 1;

        if (currentIdx >= canvases.size())
            currentIdx -= canvases.size();
        if (currentIdx < 0)
            currentIdx += canvases.size();

        tabbar.setCurrentTabIndex(currentIdx);

        return true;
    }
    case CommandIDs::PreviousTab: {
        int currentIdx = canvases.indexOf(cnv) - 1;

        if (currentIdx >= canvases.size())
            currentIdx -= canvases.size();
        if (currentIdx < 0)
            currentIdx += canvases.size();

        tabbar.setCurrentTabIndex(currentIdx);

        return true;
    }
    case CommandIDs::ToggleGrid: {
        auto value = static_cast<bool>(pd->settingsTree.getProperty("GridEnabled"));
        pd->settingsTree.setProperty("GridEnabled", !value, nullptr);

        return true;
    }
    case CommandIDs::ClearConsole: {
        auto value = static_cast<bool>(pd->settingsTree.getProperty("GridEnabled"));
        pd->settingsTree.setProperty("GridEnabled", !value, nullptr);

        sidebar.clearConsole();

        return true;
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
                cnv->patch.startUndoSequence("Object in connection");
                cnv->lastSelectedConnection = cnv->getSelectionOfType<Connection>().getFirst();
                auto outobj = cnv->getSelectionOfType<Connection>().getFirst()->outobj;
                cnv->objects.add(new Object(cnv, objectNames.at(ID),
                    Point<int>(
                        // place beneath outlet object + Object::margin
                        cnv->lastSelectedConnection->getX() + (cnv->lastSelectedConnection->getWidth() / 2) - 12,
                        cnv->lastSelectedConnection->getY() + (cnv->lastSelectedConnection->getHeight() / 2) - 12)));
                cnv->patch.endUndoSequence("Object in connection");
            } else {
                // if 0 or several objects are selected, create new object at mouse position
                cnv->objects.add(new Object(cnv, objectNames.at(ID), lastPosition));
            }
            cnv->deselectAll();
            cnv->setSelected(cnv->objects[cnv->objects.size() - 1], true); // Select newly created object
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
