/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Utility/Config.h"
#include "Utility/Fonts.h"
#include "Utility/OSUtils.h"

#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Pd/Patch.h"

#include "LookAndFeel.h"

#include "Canvas.h"
#include "Connection.h"
#include "Objects/ObjectBase.h" // TODO: We shouldn't need this!
#include "Dialogs/Dialogs.h"
#include "Statusbar.h"
#include "Sidebar/Sidebar.h"
#include "Object.h"

class ZoomLabel : public TextButton
    , public Timer {

    ComponentAnimator labelAnimator;

    bool initRun = true;

public:
    ZoomLabel()
    {
        setInterceptsMouseClicks(false, false);
    }

    void setZoomLevel(float value)
    {
        if (initRun) {
            initRun = false;
            return;
        }

        setButtonText(String(value * 100, 1) + "%");
        startTimer(2000);

        if (!labelAnimator.isAnimating(this)) {
            labelAnimator.fadeIn(this, 200);
        }
    }

    void timerCallback() override
    {
        labelAnimator.fadeOut(this, 200);
        stopTimer();
    }
};

bool wantsNativeDialog()
{
    if (ProjectInfo::isStandalone) {
        return true;
    }

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
    , statusbar(std::make_unique<Statusbar>(&p))
    , zoomLabel(std::make_unique<ZoomLabel>())
    , sidebar(std::make_unique<Sidebar>(&p, this))
    , tooltipWindow(this, 500)
    , tooltipShadow(DropShadow(Colour(0, 0, 0).withAlpha(0.2f), 4, { 0, 0 }), Corners::defaultCornerRadius)
    , splitView(this)
    , openedDialog(nullptr)
{
    mainMenuButton.setButtonText(Icons::Menu);
    undoButton.setButtonText(Icons::Undo);
    redoButton.setButtonText(Icons::Redo);
    addObjectMenuButton.setButtonText(Icons::Add);
    hideSidebarButton.setButtonText(Icons::Hide);
    pinButton.setButtonText(Icons::Pin);

    setResizable(true, false);

    // In the standalone, the resizer handling is done on the window class
    if (!ProjectInfo::isStandalone) {
        cornerResizer = std::make_unique<MouseRateReducedComponent<ResizableCornerComponent>>(this, getConstrainer());
        cornerResizer->setAlwaysOnTop(true);
        addAndMakeVisible(cornerResizer.get());
    }

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

    splitZoomScale.referTo(settingsFile->getPropertyAsValue("split_zoom"));
    splitZoomScale.addListener(this);

    addAndMakeVisible(*statusbar);
    addAndMakeVisible(splitView);
    addAndMakeVisible(*sidebar);

    for (auto* button : std::vector<TextButton*> { &mainMenuButton, &undoButton, &redoButton, &addObjectMenuButton, &pinButton, &hideSidebarButton }) {
        button->getProperties().set("Style", "LargeIcon");
        button->setConnectedEdges(12);
        addAndMakeVisible(button);
    }

    // Show settings
    mainMenuButton.setTooltip("Settings");
    mainMenuButton.onClick = [this]() {
        Dialogs::showMainMenu(this, &mainMenuButton);
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

        sidebar->showSidebar(show);
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
    pinButton.onClick = [this]() { sidebar->pinSidebar(pinButton.getToggleState()); };

    addAndMakeVisible(hideSidebarButton);

    sidebar->setSize(250, pd->lastUIHeight - statusbar->getHeight());
    setSize(pd->lastUIWidth, pd->lastUIHeight);

    // Set minimum bounds
    setResizeLimits(835, 305, 999999, 999999);

    sidebar->toFront(false);

    // Make sure existing console messages are processed
    sidebar->updateConsole();
    updateCommandStatus();

    addModifierKeyListener(statusbar.get());

    addChildComponent(*zoomLabel);

    // Initialise zoom factor
    valueChanged(zoomScale);
    valueChanged(splitZoomScale);
}
PluginEditor::~PluginEditor()
{
    setConstrainer(nullptr);

    zoomScale.removeListener(this);
    splitZoomScale.removeListener(this);
    theme.removeListener(this);

    pd->lastLeftTab = splitView.getLeftTabbar()->getCurrentTabIndex();
    pd->lastRightTab = splitView.getRightTabbar()->getCurrentTabIndex();
}

void PluginEditor::paint(Graphics& g)
{
    g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);

    auto baseColour = findColour(PlugDataColour::toolbarBackgroundColourId);

    bool rounded = wantsRoundedCorners();

    if (rounded) {
        // Toolbar background
        g.setColour(baseColour);
        g.fillRect(0, 10, getWidth(), toolbarHeight - 9);
        g.fillRoundedRectangle(0.0f, 0.0f, getWidth(), toolbarHeight, Corners::windowCornerRadius);

        // Statusbar background
        g.setColour(baseColour);
        g.fillRect(0, getHeight() - statusbar->getHeight(), getWidth(), statusbar->getHeight() - 12);
        g.fillRoundedRectangle(0.0f, getHeight() - statusbar->getHeight(), getWidth(), statusbar->getHeight(), Corners::windowCornerRadius);
    } else {
        // Toolbar background
        g.setColour(baseColour);
        g.fillRect(0, 0, getWidth(), toolbarHeight);

        // Statusbar background
        g.setColour(baseColour);
        g.fillRect(0, getHeight() - statusbar->getHeight(), getWidth(), statusbar->getHeight());
    }

    g.setColour(findColour(PlugDataColour::outlineColourId));
    g.drawLine(0.0f, toolbarHeight, static_cast<float>(getWidth()), toolbarHeight, 1.0f);
}

// Paint file drop outline
void PluginEditor::paintOverChildren(Graphics& g)
{
    // Never want to be drawing over a dialog window
    if (openedDialog)
        return;

    if (isDraggingFile) {
        g.setColour(findColour(PlugDataColour::scrollbarThumbColourId));
        g.drawRect(getLocalBounds().reduced(1), 2.0f);
    }
}

void PluginEditor::resized()
{
    splitView.setBounds(0, toolbarHeight, (getWidth() - sidebar->getWidth()) + 1, getHeight() - toolbarHeight - (statusbar->getHeight()));
    sidebar->setBounds(getWidth() - sidebar->getWidth(), toolbarHeight, sidebar->getWidth(), getHeight() - toolbarHeight);
    statusbar->setBounds(0, getHeight() - statusbar->getHeight(), getWidth() - sidebar->getWidth(), statusbar->getHeight());

    auto useLeftButtons = SettingsFile::getInstance()->getProperty<bool>("macos_buttons");
    auto useNonNativeTitlebar = ProjectInfo::isStandalone && !SettingsFile::getInstance()->getProperty<bool>("native_window");
    auto offset = useLeftButtons && useNonNativeTitlebar ? 70 : 0;

    mainMenuButton.setBounds(20 + offset, 0, toolbarHeight, toolbarHeight);
    undoButton.setBounds(90 + offset, 0, toolbarHeight, toolbarHeight);
    redoButton.setBounds(160 + offset, 0, toolbarHeight, toolbarHeight);
    addObjectMenuButton.setBounds(230 + offset, 0, toolbarHeight, toolbarHeight);

    auto windowControlsOffset = (useNonNativeTitlebar && !useLeftButtons) ? 170.0f : 70.0f;

    if (!ProjectInfo::isStandalone) {
        int const resizerSize = 18;
        cornerResizer->setBounds(getWidth() - resizerSize,
            getHeight() - resizerSize,
            resizerSize, resizerSize);
    }

    int hidePosition = getWidth() - windowControlsOffset;
    int pinPosition = hidePosition - 65;

    hideSidebarButton.setBounds(hidePosition, 0, 70, toolbarHeight);
    pinButton.setBounds(pinPosition, 0, 70, toolbarHeight);

    pd->lastUIWidth = getWidth();
    pd->lastUIHeight = getHeight();

    zoomLabel->setTopLeftPosition(5, statusbar->getY() - 28);
    zoomLabel->setSize(55, 23);
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

    auto event = e.getEventRelativeTo(getCurrentCanvas()->viewport);

    auto& scale = splitView.isRightTabbarActive() ? splitZoomScale : zoomScale;

    float value = static_cast<float>(scale.getValue());

    // Apply and limit zoom
    value = std::clamp(value * scrollFactor, 0.5f, 2.0f);

    scale = value;
}

void PluginEditor::mouseDown(MouseEvent const& e)
{
    // no window dragging by toolbar in plugin!
    if (!ProjectInfo::isStandalone)
        return;

    if (e.getNumberOfClicks() >= 2) {

#if JUCE_MAC
        if (isMaximised) {
            getPeer()->setBounds(unmaximisedSize, false);
        } else {
            unmaximisedSize = getTopLevelComponent()->getBounds();
            auto userArea = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            getPeer()->setBounds(userArea, false);
        }

        isMaximised = !isMaximised;
        return;

#else
        findParentComponentOfClass<DocumentWindow>()->maximiseButtonPressed();
#endif
    }

    if (e.getPosition().getY() < toolbarHeight) {
        if (auto* window = findParentComponentOfClass<DocumentWindow>()) {
            if (!window->isUsingNativeTitleBar())
                windowDragger.startDraggingComponent(window, e.getEventRelativeTo(window));
        }
    }
}

void PluginEditor::mouseDrag(MouseEvent const& e)
{
    if (!ProjectInfo::isStandalone)
        return;

    if (!isMaximised) {
        if (auto* window = findParentComponentOfClass<DocumentWindow>()) {
            if (!window->isUsingNativeTitleBar())
                windowDragger.dragComponent(window, e.getEventRelativeTo(window), nullptr);
        }
    }
}

bool PluginEditor::isInterestedInFileDrag(StringArray const& files)
{
    if (openedDialog)
        return false;

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
    
    auto* cnv = getCurrentCanvas();
    
    if(cnv->patch.isSubpatch())
    {
        for(auto& parentCanvas : canvases)
        {
            if(cnv->patch.getRoot() == parentCanvas->patch.getPointer()) {
                cnv = parentCanvas;
            }
        }
    }
    
    if (cnv->patch.getCurrentFile().existsAsFile()) {
        cnv->patch.savePatch();
        SettingsFile::getInstance()->addToRecentlyOpened(cnv->patch.getCurrentFile());
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

void PluginEditor::closeAllTabs()
{
    bool complete = false;
    while (!complete) {
        closeTab(canvases.getFirst());
        if (canvases.size() == 0)
            complete = true;
    }
}

void PluginEditor::closeTab(Canvas* cnv)
{
    if (!cnv || !cnv->getTabbar())
        return;

    auto* tabbar = cnv->getTabbar();
    auto const tabIdx = cnv->getTabIndex();

    auto* patch = &cnv->patch;

    sidebar->hideParameters();

    cnv->getTabbar()->removeTab(tabIdx);

    pd->patches.removeObject(patch, false);

    canvases.removeObject(cnv);

    if (patch->closePatchOnDelete) {
        delete patch;
    }

    tabbar->setCurrentTabIndex(tabIdx > 0 ? tabIdx - 1 : tabIdx);

    if (auto* leftCnv = splitView.getLeftTabbar()->getCurrentCanvas()) {
        leftCnv->tabChanged();
    }
    if (auto* rightCnv = splitView.getRightTabbar()->getCurrentCanvas()) {
        rightCnv->tabChanged();
    }

    splitView.closeEmptySplits();
    updateCommandStatus();

    pd->savePatchTabPositions();

    MessageManager::callAsync([_this = SafePointer(this)]() {
        if (!_this)
            return;
        _this->resized();
    });
}

void PluginEditor::addTab(Canvas* cnv)
{
    // Create a pointer to the TabBar in focus
    auto* focusedTabbar = splitView.getActiveTabbar();

    int const newTabIdx = focusedTabbar->getCurrentTabIndex() + 1; // The tab index for the added tab

    // Add tab next to the currently focused tab
    auto patchTitle = cnv->patch.getTitle();
    focusedTabbar->addTab(patchTitle, findColour(ResizableWindow::backgroundColourId), cnv->viewport, true, newTabIdx);

    // Open help files and references in Locked Mode
    if (patchTitle.contains("-help") || patchTitle.equalsIgnoreCase("reference"))
        cnv->locked.setValue(true);

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

    pd->savePatchTabPositions();
}

void PluginEditor::valueChanged(Value& v)
{
    // Update zoom
    if (v.refersToSameSourceAs(zoomScale) || v.refersToSameSourceAs(splitZoomScale)) {
        float scale = static_cast<float>(v.getValue());

        if (scale == 0) {
            scale = 1.0f;
            zoomScale = 1.0f;
        }

        auto lastMousePosition = Point<int>();
        if (auto* cnv = getCurrentCanvas()) {
            lastMousePosition = cnv->getMouseXYRelative();
        }

        auto* tabbar = v.refersToSameSourceAs(zoomScale) ? splitView.getLeftTabbar() : splitView.getRightTabbar();

        for (int i = 0; i < tabbar->getNumTabs(); i++) {
            if (auto* cnv = tabbar->getCanvas(i)) {
                cnv->hideSuggestions();
                cnv->setTransform(AffineTransform().scaled(scale));
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

        zoomLabel->setZoomLevel(scale);
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

        bool isDragging = cnv->dragState.didStartDragging && !cnv->isDraggingLasso && statusbar->locked == var(false);
        for (auto& connection : cnv->getSelectionOfType<Connection>()) {
            allSegmented = allSegmented && connection->isSegmented();
            allNotSegmented = allNotSegmented && !connection->isSegmented();
            hasSelection = true;
        }

        statusbar->connectionStyleButton->setEnabled(!isDragging && hasSelection);
        statusbar->connectionPathfind->setEnabled(!isDragging && hasSelection);
        statusbar->connectionStyleButton->setToggleState(!isDragging && hasSelection && allSegmented, dontSendNotification);

        statusbar->lockButton->setEnabled(!isDragging);
        statusbar->attachToCanvas(cnv);

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

        statusbar->lockButton->setEnabled(true);
        statusbar->presentationButton->setEnabled(true);
        statusbar->gridButton->setEnabled(true);
        statusbar->centreButton->setEnabled(true);

        statusbar->directionButton->setEnabled(true);

        addObjectMenuButton.setEnabled(!locked);
    } else {
        statusbar->connectionStyleButton->setEnabled(false);
        statusbar->connectionPathfind->setEnabled(false);
        statusbar->lockButton->setEnabled(false);

        statusbar->lockButton->setEnabled(false);
        statusbar->presentationButton->setEnabled(false);
        statusbar->gridButton->setEnabled(false);
        statusbar->centreButton->setEnabled(false);

        statusbar->directionButton->setEnabled(false);

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
        isDragging = cnv->dragState.didStartDragging && !cnv->isDraggingLasso && statusbar->locked == var(false);
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
        result.setActive(hasCanvas && !isDragging && statusbar->connectionStyleButton->isEnabled());
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

    std::map<ObjectIDs, std::pair<int, int>> defaultShortcuts;

    switch (OSUtils::getKeyboardLayout()) {
    case OSUtils::KeyboardLayout::QWERTY:
        defaultShortcuts = {
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
        break;
    case OSUtils::KeyboardLayout::AZERTY:
        defaultShortcuts = {
            { NewObject, { 38, cmdMod } },
            { NewComment, { 40, cmdMod } },
            { NewBang, { 66, cmdMod | shiftMod } },
            { NewMessage, { 233, cmdMod } },
            { NewToggle, { 84, cmdMod | shiftMod } },
            { NewNumbox, { 73, cmdMod | shiftMod } },
            { NewVerticalSlider, { 86, cmdMod | shiftMod } },
            { NewHorizontalSlider, { 74, cmdMod | shiftMod } },
            { NewVerticalRadio, { 68, cmdMod | shiftMod } },
            { NewHorizontalRadio, { 73, cmdMod | shiftMod } },
            { NewFloatAtom, { 34, cmdMod } },
            { NewListAtom, { 39, cmdMod } },
            { NewArray, { 65, cmdMod | shiftMod } },
            { NewGraphOnParent, { 71, cmdMod | shiftMod } },
            { NewCanvas, { 67, cmdMod | shiftMod } },
            { NewVUMeterObject, { 85, cmdMod | shiftMod } }
        };
        break;

    default:
        break;
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
    switch (info.commandID) {
    case CommandIDs::NewProject: {
        newProject();
        return true;
    }
    case CommandIDs::OpenProject: {
        openProject();
        return true;
    }
    case CommandIDs::ShowBrowser: {
        sidebar->showPanel(sidebar->isShowingBrowser() ? 0 : 1);
        return true;
    }
    case CommandIDs::Search: {
        sidebar->showPanel(3);
        return true;
    }
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
        statusbar->lockButton->triggerClick();
        return true;
    }
    case CommandIDs::ConnectionPathfind: {
        cnv->patch.startUndoSequence("ConnectionPathFind");

        statusbar->connectionStyleButton->setToggleState(true, sendNotification);
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
        auto& scale = splitView.isRightTabbarActive() ? splitZoomScale : zoomScale;
        float newScale = static_cast<float>(scale.getValue()) + 0.1f;
        scale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.5f, 2.0f) * 10.))) / 10.;

        return true;
    }
    case CommandIDs::ZoomOut: {
        auto& scale = splitView.isRightTabbarActive() ? splitZoomScale : zoomScale;
        float newScale = static_cast<float>(scale.getValue()) - 0.1f;
        scale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.5f, 2.0f) * 10.))) / 10.;

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
        sidebar->clearConsole();
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
    if (!ProjectInfo::isStandalone)
        return false;

    if (auto* window = findParentComponentOfClass<DocumentWindow>()) {
        return !window->isUsingNativeTitleBar() && Desktop::canUseSemiTransparentWindows();
    } else {
        return true;
    }
}

float PluginEditor::getZoomScale()
{
    return static_cast<float>(splitView.isRightTabbarActive() ? splitZoomScale.getValue() : zoomScale.getValue());
}

float PluginEditor::getZoomScaleForCanvas(Canvas* cnv)
{
    return static_cast<float>(getZoomScaleValueForCanvas(cnv).getValue());
}

Value& PluginEditor::getZoomScaleValueForCanvas(Canvas* cnv)
{
    if (cnv->getTabbar() == splitView.getRightTabbar()) {
        return splitZoomScale;
    }

    return zoomScale;
}
