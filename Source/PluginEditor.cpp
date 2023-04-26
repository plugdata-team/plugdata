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

#include "Presets.h" // TODO: temporary, remove later

#include "Pd/Patch.h"

#include "LookAndFeel.h"
#include "Palettes.h"

#include "Canvas.h"
#include "Connection.h"
#include "Objects/ObjectBase.h" // TODO: We shouldn't need this!
#include "Dialogs/Dialogs.h"
#include "Statusbar.h"
#include "Sidebar/Sidebar.h"
#include "Object.h"
#include "PluginMode.h"

class ZoomLabel : public TextButton
    , public Timer {

    ComponentAnimator labelAnimator;

    // Ignore two times, when setting zoom scale and when setting split zoom scale
    int initRun = 2;

public:
    ZoomLabel()
    {
        setInterceptsMouseClicks(false, false);
    }

    void setZoomLevel(float value)
    {
        if (initRun > 0) {
            initRun--;
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

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p)
    , pd(&p)
    , statusbar(std::make_unique<Statusbar>(&p))
    , zoomLabel(std::make_unique<ZoomLabel>())
    , sidebar(std::make_unique<Sidebar>(&p, this))
    , splitView(this)
    , openedDialog(nullptr)
    , tooltipWindow(this, [](Component* c) {
        if (auto* cnv = c->findParentComponentOfClass<Canvas>()) {
            return !getValue<bool>(cnv->locked) || getValue<bool>(cnv->paletteDragMode);
        }

        return true;
    })
{
    mainMenuButton.setButtonText(Icons::Menu);
    undoButton.setButtonText(Icons::Undo);
    redoButton.setButtonText(Icons::Redo);
    addObjectMenuButton.setButtonText(Icons::Add);
    hideSidebarButton.setButtonText(Icons::Hide);
    pinButton.setButtonText(Icons::Pin);

    editButton.setButtonText(Icons::Edit);
    runButton.setButtonText(Icons::Lock);
    presentButton.setButtonText(Icons::Presentation);

    setResizable(true, false);
    
    // In the standalone, the resizer handling is done on the window class
    if (ProjectInfo::isStandalone) {
        
        // We need to attach this to the top-level component, which is not yet know during the constructor
        // So we postpone initialisation of the resizer until PluginEditor has a parent component
        MessageManager::callAsync([this](){
            borderResizer = std::make_unique<MouseRateReducedComponent<ResizableBorderComponent>>(getTopLevelComponent(), getConstrainer());
            borderResizer->setAlwaysOnTop(true);
            addAndMakeVisible(borderResizer.get());
        });
    }
    else {
        cornerResizer = std::make_unique<MouseRateReducedComponent<ResizableCornerComponent>>(this, getConstrainer());
        cornerResizer->setAlwaysOnTop(true);
        addAndMakeVisible(cornerResizer.get());
    }

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

    palettes = std::make_unique<Palettes>(this);

    addChildComponent(*palettes);
    addAndMakeVisible(*statusbar);

    addAndMakeVisible(splitView);
    addAndMakeVisible(*sidebar);

    for (auto* button : std::vector<TextButton*> { &mainMenuButton, &undoButton, &redoButton, &addObjectMenuButton, &pinButton, &hideSidebarButton }) {
        button->getProperties().set("Style", "LargeIcon");
        // button->setConnectedEdges(Button::Conn);
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

    // Edit, run and presentation mode buttons
    for (auto* button : std::vector<TextButton*> { &editButton, &runButton, &presentButton }) {
        button->onClick = [this]() {
            if (auto* cnv = getCurrentCanvas()) {
                if (editButton.getToggleState()) {
                    cnv->presentationMode.setValue(false);
                    cnv->locked.setValue(false);
                } else if (runButton.getToggleState()) {
                    cnv->presentationMode.setValue(false);
                    cnv->locked.setValue(true);
                } else if (presentButton.getToggleState()) {
                    cnv->presentationMode.setValue(true);
                    cnv->locked.setValue(true);
                }
            }
        };

        button->getProperties().set("Style", "LargeIcon");
        button->setClickingTogglesState(true);
        button->setRadioGroupId(2200);
        addAndMakeVisible(button);
    }
    editButton.setToggleState(true, sendNotification);

    editButton.setTooltip("Edit mode");
    runButton.setTooltip("Run mode");
    presentButton.setTooltip("Presentation mode");

    editButton.setConnectedEdges(Button::ConnectedOnRight);
    runButton.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
    presentButton.setConnectedEdges(Button::ConnectedOnLeft);

    // Hide sidebar
    hideSidebarButton.setTooltip("Hide Sidebar");
    hideSidebarButton.getProperties().set("Style", "LargeIcon");
    hideSidebarButton.setClickingTogglesState(true);
    hideSidebarButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
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

    addModifierKeyListener(this);

    // Restore Plugin Mode View
    if (pd->isInPluginMode())
        enablePluginMode(nullptr);
}

PluginEditor::~PluginEditor()
{
    pd->savePatchTabPositions();
    
    setVisible(false); // We can check the visible flag to detect if the pluginEditor is closing
    setConstrainer(nullptr);

    theme.removeListener(this);

    pd->lastLeftTab = splitView.getLeftTabbar()->getCurrentTabIndex();
    pd->lastRightTab = splitView.getRightTabbar()->getCurrentTabIndex();
}

void PluginEditor::setZoomLabelLevel(float value)
{
    zoomLabel->setZoomLevel(value);
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
    if (pd->isInPluginMode())
        return;

    auto paletteWidth = palettes->isExpanded() ? palettes->getWidth() : 26;
    if (!palettes->isVisible())
        paletteWidth = 0;

    palettes->setBounds(0, toolbarHeight, palettes->getWidth(), getHeight() - toolbarHeight - (statusbar->getHeight()));

    splitView.setBounds(paletteWidth, toolbarHeight, (getWidth() - sidebar->getWidth() - paletteWidth) + 1, getHeight() - toolbarHeight - Statusbar::statusbarHeight);
    sidebar->setBounds(getWidth() - sidebar->getWidth(), toolbarHeight, sidebar->getWidth(), getHeight() - toolbarHeight);
    statusbar->setBounds(0, getHeight() - Statusbar::statusbarHeight, getWidth() - sidebar->getWidth(), statusbar->getHeight());

    auto useLeftButtons = SettingsFile::getInstance()->getProperty<bool>("macos_buttons");
    auto useNonNativeTitlebar = ProjectInfo::isStandalone && !SettingsFile::getInstance()->getProperty<bool>("native_window");
    auto offset = useLeftButtons && useNonNativeTitlebar ? 70 : 0;
#if JUCE_MAC
    if (auto standalone = ProjectInfo::isStandalone ? dynamic_cast<DocumentWindow*>(getTopLevelComponent()) : nullptr)
        offset = standalone->isFullScreen() ? 0 : offset;
#endif

    zoomLabel->setBounds(paletteWidth + 5, getHeight() - Statusbar::statusbarHeight - 36, 55, 23);

    mainMenuButton.setBounds(20 + offset, 0, toolbarHeight, toolbarHeight);
    undoButton.setBounds(80 + offset, 0, toolbarHeight, toolbarHeight);
    redoButton.setBounds(140 + offset, 0, toolbarHeight, toolbarHeight);
    addObjectMenuButton.setBounds(200 + offset, 0, toolbarHeight, toolbarHeight);

    auto startX = (getWidth() / 2) - (toolbarHeight * 1.5);

    editButton.setBounds(startX, 0, toolbarHeight, toolbarHeight);
    runButton.setBounds(startX + toolbarHeight - 1, 0, toolbarHeight, toolbarHeight);
    presentButton.setBounds(startX + (2 * toolbarHeight) - 2, 0, toolbarHeight, toolbarHeight);

    auto windowControlsOffset = (useNonNativeTitlebar && !useLeftButtons) ? 150.0f : 60.0f;

    if (borderResizer && ProjectInfo::isStandalone) {
        borderResizer->setBounds(getLocalBounds());
    }
    else if(cornerResizer){
        int const resizerSize = 18;
        cornerResizer->setBounds(getWidth() - resizerSize,
            getHeight() - resizerSize,
            resizerSize, resizerSize);
    }

    int hidePosition = getWidth() - windowControlsOffset;
    int pinPosition = hidePosition - 60;

    hideSidebarButton.setBounds(hidePosition, 0, toolbarHeight, toolbarHeight);
    pinButton.setBounds(pinPosition, 0, toolbarHeight, toolbarHeight);

    pd->lastUIWidth = getWidth();
    pd->lastUIHeight = getHeight();
}

void PluginEditor::parentSizeChanged()
{
    if (!ProjectInfo::isStandalone)
        return;

    auto* standalone = dynamic_cast<DocumentWindow*>(getTopLevelComponent());
    // Hide TitleBar Buttons in Plugin Mode
    bool visible = !pd->isInPluginMode();
#if JUCE_MAC
    if (!standalone->isUsingNativeTitleBar()) {
        // & hide TitleBar buttons when fullscreen on MacOS
        visible = visible && !standalone->isFullScreen();
        standalone->getCloseButton()->setVisible(visible);
        standalone->getMinimiseButton()->setVisible(visible);
        standalone->getMaximiseButton()->setVisible(visible);
    } else if (!visible && !standalone->isFullScreen()) {
        // Hide TitleBar Buttons in Plugin Mode if using native title bar
        if (ComponentPeer* peer = standalone->getPeer())
            OSUtils::HideTitlebarButtons(peer->getNativeHandle(), true, true, true);
    } else {
        // Show TitleBar Buttons
        if (ComponentPeer* peer = standalone->getPeer())
            OSUtils::HideTitlebarButtons(peer->getNativeHandle(), false, false, false);
    }
#else
    if (!standalone->isUsingNativeTitleBar()) {
        // Hide/Show TitleBar Buttons in Plugin Mode
        standalone->getCloseButton()->setVisible(visible);
        standalone->getMinimiseButton()->setVisible(visible);
        standalone->getMaximiseButton()->setVisible(visible);
    }
#endif

    resized();
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
    auto patch = pd->loadPatch(pd::Instance::defaultPatch);
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

    openChooser = std::make_unique<FileChooser>("Choose file to open", File(SettingsFile::getInstance()->getProperty<String>("last_filechooser_path")), "*.pd", SettingsFile::getInstance()->wantsNativeDialog());

    openChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, openFunc);
}

void PluginEditor::saveProjectAs(std::function<void()> const& nestedCallback)
{
    saveChooser = std::make_unique<FileChooser>("Select a save file", File(SettingsFile::getInstance()->getProperty<String>("last_filechooser_path")), "*.pd", SettingsFile::getInstance()->wantsNativeDialog());

    // The warnAboutOverwriting flag causes the save dialog not to show at all on some Linux distros, so we better disable it
#if JUCE_LINUX
    auto saveFlags = FileBrowserComponent::saveMode;
#else
    auto saveFlags = FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting;
#endif

    saveChooser->launchAsync(saveFlags,
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
    for (auto patch : pd->patches) {
        patch->deselectAll();
    }

    auto* cnv = getCurrentCanvas();

    if (cnv->patch.isSubpatch()) {
        for (auto& parentCanvas : canvases) {
            if (cnv->patch.getRoot() == parentCanvas->patch.getPointer()) {
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

Canvas* PluginEditor::getCurrentCanvas(bool canBePalette)
{
    if (canBePalette && palettes && palettes->hasFocus()) {
        if (auto* cnv = palettes->getCurrentCanvas()) {
            if (!getValue<bool>(cnv->paletteDragMode)) {
                return cnv;
            }
        }
    }

    return getActiveTabbar()->getCurrentCanvas();
}

void PluginEditor::closeAllTabs(bool quitAfterComplete)
{
    auto* canvas = canvases.getLast();
    if (!canvas) {
        if(quitAfterComplete)
        {
            JUCEApplication::quit();
        }
        return;
    }

    auto* tabbar = canvas->getTabbar();
    auto* patch = &canvas->patch;

    auto deleteFunc = [this, tabbar, canvas, patch, quitAfterComplete]() {
        if (!canvas) {
            return;
        }

        closeTab(canvas);
        closeAllTabs(quitAfterComplete);
    };

    if (canvas) {
        MessageManager::callAsync([this, canvas, patch, deleteFunc]() mutable {
            if (patch->isDirty()) {
                Dialogs::showSaveDialog(&openedDialog, this, patch->getTitle(),
                    [this, canvas, deleteFunc](int result) mutable {
                        if (!canvas)
                            return;
                        if (result == 2)
                            saveProject([&deleteFunc]() mutable { deleteFunc(); });
                        else if (result == 1)
                            deleteFunc();
                    });
            } else {
                deleteFunc();
            }
        });
    }
}

void PluginEditor::closeTab(Canvas* cnv)
{
    if (!cnv || !cnv->getTabbar())
        return;

    auto* tabbar = cnv->getTabbar();
    auto const tabIdx = cnv->getTabIndex();

    auto patch = cnv->refCountedPatch;

    sidebar->hideParameters();

    if (tabbar->getCurrentTabIndex() == tabIdx)
        tabbar->setCurrentTabIndex(tabIdx > 0 ? tabIdx - 1 : tabIdx);

    cnv->getTabbar()->removeTab(tabIdx);
    canvases.removeObject(cnv);

    pd->patches.removeFirstMatchingValue(patch);

    if (auto* leftCnv = splitView.getLeftTabbar()->getCurrentCanvas()) {
        leftCnv->tabChanged();
    }
    if (auto* rightCnv = splitView.getRightTabbar()->getCurrentCanvas()) {
        rightCnv->tabChanged();
    }

    pd->updateObjectImplementations();

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

        if (cnv) {
            MessageManager::callAsync([this, cnv = SafePointer(cnv)]() mutable {
                // Don't show save dialog, if patch is still open in another view
                if (cnv && cnv->patch.isDirty()) {
                    Dialogs::showSaveDialog(&openedDialog, this, cnv->patch.getTitle(),
                        [this, cnv](int result) mutable {
                            if (!cnv)
                                return;
                            if (result == 2)
                                saveProject([this, cnv]() mutable { closeTab(cnv); });
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
    cnv->jumpToOrigin();

    if(cnv->patch.openInPluginMode)
    {
        enablePluginMode(cnv);
    }

    pd->savePatchTabPositions();
}

void PluginEditor::valueChanged(Value& v)
{
    // Update theme
    if (v.refersToSameSourceAs(theme)) {
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
    pd->titleChanged();

    if (auto* cnv = getCurrentCanvas()) {
        // Update connection style button
        bool allSegmented = true;
        bool allNotSegmented = true;
        bool hasSelection = false;

        bool locked = getValue<bool>(cnv->locked);

        bool isDragging = cnv->dragState.didStartDragging && !cnv->isDraggingLasso && cnv->locked == var(false);
        for (auto& connection : cnv->getSelectionOfType<Connection>()) {
            allSegmented = allSegmented && connection->isSegmented();
            allNotSegmented = allNotSegmented && !connection->isSegmented();
            hasSelection = true;
        }

        statusbar->connectionStyleButton.setEnabled(!isDragging && hasSelection);
        statusbar->connectionPathfind.setEnabled(!isDragging && hasSelection);
        statusbar->connectionStyleButton.setToggleState(!isDragging && hasSelection && allSegmented, dontSendNotification);

        if (getValue<bool>(cnv->presentationMode)) {
            presentButton.setToggleState(true, dontSendNotification);
        } else if (getValue<bool>(cnv->locked)) {
            runButton.setToggleState(true, dontSendNotification);
        } else {
            editButton.setToggleState(true, dontSendNotification);
        }

        auto* patchPtr = cnv->patch.getPointer();
        if (!patchPtr)
            return;

        // First on pd's thread, get undo status
        pd->enqueueFunction(
            [this, patchPtr, isDragging, deletionCheck = SafePointer(this), locked]() mutable {
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

        editButton.setEnabled(true);
        runButton.setEnabled(true);
        presentButton.setEnabled(true);

        statusbar->centreButton.setEnabled(true);
        statusbar->fitAllButton.setEnabled(true);

        addObjectMenuButton.setEnabled(!locked);
    } else {

        editButton.setEnabled(false);
        runButton.setEnabled(false);
        presentButton.setEnabled(false);

        statusbar->connectionStyleButton.setEnabled(false);
        statusbar->connectionPathfind.setEnabled(false);
        statusbar->centreButton.setEnabled(false);
        statusbar->fitAllButton.setEnabled(false);

        undoButton.setEnabled(false);
        redoButton.setEnabled(false);
        addObjectMenuButton.setEnabled(false);
    }
}

ApplicationCommandTarget* PluginEditor::getNextCommandTarget()
{
    return nullptr;
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
    bool hasObjectSelection = false;
    bool hasSelection = false;
    bool isDragging = false;
    bool hasCanvas = false;
    bool locked = true;
    bool canConnect = false;

    if (auto* cnv = getCurrentCanvas(true)) {
        auto selectedObjects = cnv->getSelectionOfType<Object>();
        auto selectedConnections = cnv->getSelectionOfType<Connection>();

        hasObjectSelection = !selectedObjects.isEmpty();
        hasSelection = hasObjectSelection || !selectedConnections.isEmpty();
        isDragging = cnv->dragState.didStartDragging && !cnv->isDraggingLasso && cnv->locked == var(false);
        hasCanvas = true;

        locked = getValue<bool>(cnv->locked);
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
        result.setActive((hasCanvas) && !isDragging);
        break;
    }
    case CommandIDs::ConnectionStyle: {
        result.setInfo("Connection style", "Set connection style", "Edit", 0);
        result.setActive(hasCanvas && !isDragging && statusbar->connectionStyleButton.isEnabled());
        break;
    }
    case CommandIDs::ZoomIn: {
        result.setInfo("Zoom in", "Zoom in", "View", 0);
        result.addDefaultKeypress(61, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::ZoomOut: {
        result.setInfo("Zoom out", "Zoom out", "View", 0);
        result.addDefaultKeypress(45, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::ZoomNormal: {
        result.setInfo("Zoom 100%", "Revert zoom to 100%", "View", 0);
        result.addDefaultKeypress(48, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::ZoomToFitAll: {
        result.setInfo("Zoom to fit all", "Fit all objects in the viewport", "View", 0);
        result.addDefaultKeypress(41, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::GoToOrigin: {
        result.setInfo("Go to origin", "Jump to canvas origin", "View", 0);
        result.addDefaultKeypress(57, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging);
        break;
    }
    case CommandIDs::Copy: {
        result.setInfo("Copy", "Copy", "Edit", 0);
        result.addDefaultKeypress(67, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !locked && hasObjectSelection && !isDragging);
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
        result.setActive(hasCanvas && !isDragging && !locked && hasObjectSelection);
        break;
    }
    case CommandIDs::CreateConnection: {
        result.setInfo("Create connection", "Create a connection between selected objects", "Edit", 0);
        result.addDefaultKeypress(75, ModifierKeys::commandModifier);
        result.setActive(canConnect);
        break;
    }
    case CommandIDs::RemoveConnections: {
        result.setInfo("Remove Connections", "Delete all connections from selection", "Edit", 0);
        result.addDefaultKeypress(75, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas && !isDragging && !locked && hasSelection);
        break;
    }
    case CommandIDs::SelectAll: {
        result.setInfo("Select all", "Select all objects and connections", "Edit", 0);
        result.addDefaultKeypress(65, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && !locked);
        break;
    }
    case CommandIDs::ShowBrowser: {
        result.setInfo("Show Browser", "Open documentation browser panel", "View", 0);
        result.addDefaultKeypress(66, ModifierKeys::commandModifier);
        result.setActive(true);
        break;
    }
    case CommandIDs::ToggleSidebar: {
        result.setInfo("Toggle Sidebar", "Show or hide the sidebar", "View", 0);
        result.addDefaultKeypress(93, ModifierKeys::commandModifier);
        result.setActive(true);
        break; 
    }
    case CommandIDs::TogglePalettes: {
        result.setInfo("Toggle Palettes", "Show or hide palettes", "View", 0);
        result.addDefaultKeypress(91, ModifierKeys::commandModifier);
        result.setActive(true);
        break; 
    }
    case CommandIDs::Search: {
        result.setInfo("Search Current Patch", "Search for objects in current patch", "View", 0);
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
    case CommandIDs::ToggleSnapping: {
        result.setInfo("Toggle Snapping", "Toggle object snapping", "Edit", 0);
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
        result.setInfo("Open Settings", "Open settings panel", "View", 0);
        result.addDefaultKeypress(44, ModifierKeys::commandModifier); // Cmd + , to open settings
        result.setActive(true);
        break;
    }
    case CommandIDs::ShowReference: {
        result.setInfo("Open Reference", "Open reference panel", "View", 0);
        result.addDefaultKeypress(KeyPress::F1Key, ModifierKeys::noModifiers); // f1 to open reference

        if (auto* cnv = getCurrentCanvas(true)) {
            auto selection = cnv->getSelectionOfType<Object>();
            bool enabled = selection.size() == 1 && selection[0]->gui && selection[0]->gui->getType().isNotEmpty();
            result.setActive(enabled);
        } else {
            result.setActive(false);
        }
        break;
    }
    case CommandIDs::OpenObjectBrowser: {
        result.setInfo("Open Object Browser", "Open object browser dialog", "View", 0);
        result.addDefaultKeypress(63, ModifierKeys::shiftModifier); // shift + ? to open object browser
        result.setActive(true);
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
    case CommandIDs::ToggleSidebar: {
        hideSidebarButton.triggerClick();
        return true;
    }
    case CommandIDs::TogglePalettes: {
        auto value = SettingsFile::getInstance()->getProperty<int>("show_palettes");
        SettingsFile::getInstance()->setProperty("show_palettes", !value);
        resized();
        return true;
    }
    case CommandIDs::Search: {
        sidebar->showPanel(3);
        return true;
    }
    case CommandIDs::ToggleSnapping: {
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
    }

    auto* cnv = getCurrentCanvas();

    if (!cnv)
        return false;

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

        if (cnv) {
            MessageManager::callAsync([this, cnv = SafePointer(cnv)]() mutable {
                if (cnv && cnv->patch.isDirty()) {
                    Dialogs::showSaveDialog(&openedDialog, this, cnv->patch.getTitle(),
                        [this, cnv](int result) mutable {
                            if (!cnv)
                                return;
                            if (result == 2)
                                saveProject([this, cnv]() mutable { closeTab(cnv); });
                            else if (result == 1)
                                closeTab(cnv);
                        });
                } else {
                    closeTab(cnv);
                }
            });
        }

        return true;
    }
    case CommandIDs::Copy: {
        cnv = getCurrentCanvas(true);
        cnv->copySelection();
        return true;
    }
    case CommandIDs::Paste: {
        cnv = getCurrentCanvas(true);
        cnv->pasteSelection();
        return true;
    }
    case CommandIDs::Cut: {
        cnv = getCurrentCanvas(true);
        cnv->cancelConnectionCreation();
        cnv->copySelection();
        cnv->removeSelection();
        return true;
    }
    case CommandIDs::Delete: {
        cnv = getCurrentCanvas(true);
        cnv->cancelConnectionCreation();
        cnv->removeSelection();
        return true;
    }
    case CommandIDs::Duplicate: {
        cnv = getCurrentCanvas(true);
        cnv->duplicateSelection();
        return true;
    }
    case CommandIDs::Encapsulate: {
        cnv = getCurrentCanvas(true);
        cnv->encapsulateSelection();
        return true;
    }
    case CommandIDs::CreateConnection: {
        cnv = getCurrentCanvas(true);
        return cnv->connectSelectedObjects();
    }
    case CommandIDs::RemoveConnections: {
        cnv = getCurrentCanvas(true);
        cnv->removeSelectedConnections();
        return true;
    }
    case CommandIDs::SelectAll: {
        cnv = getCurrentCanvas(true);
        for (auto* object : cnv->objects) {
            cnv->setSelected(object, true, false);
        }
        for (auto* con : cnv->connections) {
            cnv->setSelected(con, true, false);
        }
        updateCommandStatus();
        return true;
    }
    case CommandIDs::Lock: {
        cnv->locked = !getValue<bool>(cnv->locked);
        cnv->presentationMode = false;
        return true;
    }
    case CommandIDs::ConnectionPathfind: {
        cnv = getCurrentCanvas(true);
        cnv->patch.startUndoSequence("ConnectionPathFind");

        statusbar->connectionStyleButton.setToggleState(true, sendNotification);
        for (auto* con : cnv->connections) {
            if (con->isSelected()) {
                con->applyBestPath();
            }
        }

        cnv->patch.endUndoSequence("ConnectionPathFind");

        return true;
    }
    case CommandIDs::ZoomIn: {
        auto& scale = getCurrentCanvas()->zoomScale;
        float newScale = getValue<float>(scale) + 0.1f;
        scale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.2f, 3.0f) * 10.))) / 10.;

        return true;
    }
    case CommandIDs::ZoomOut: {
        auto& scale = getCurrentCanvas()->zoomScale;
        float newScale = getValue<float>(scale) - 0.1f;
        scale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.2f, 3.0f) * 10.))) / 10.;

        return true;
    }
    case CommandIDs::ZoomNormal: {
        auto& scale = getCurrentCanvas()->zoomScale;
        scale = 1.0f;
        return true;
    }
    case CommandIDs::ZoomToFitAll: {
        cnv = getCurrentCanvas(true);
        cnv->zoomToFitAll();
        return true;
    }
    case CommandIDs::GoToOrigin: {
        cnv = getCurrentCanvas(true);
        cnv->jumpToOrigin();
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
    case CommandIDs::ShowReference: {
        if (auto* cnv = getCurrentCanvas(true)) {
            auto selection = cnv->getSelectionOfType<Object>();
            if (selection.size() != 1 || !selection[0]->gui || selection[0]->gui->getType().isEmpty()) {
                return false;
            }

            Dialogs::showObjectReferenceDialog(&openedDialog, this, selection[0]->gui->getType());

            return true;
        }

        return false;
    }
    case CommandIDs::OpenObjectBrowser: {
        Dialogs::showObjectBrowserDialog(&openedDialog, this);
        return true;
    }
    case ObjectIDs::NewArray: {

        cnv = getCurrentCanvas(true);

        Dialogs::showArrayDialog(&openedDialog, this,
            [this](int result, String const& name, String const& size) {
                if (result) {
                    auto* cnv = getCurrentCanvas(true);
                    auto* object = new Object(cnv, "graph " + name + " " + size, cnv->viewport->getViewArea().getCentre());
                    cnv->objects.add(object);
                }
            });
        return true;
    }

    default: {

        cnv = getCurrentCanvas(true);

        // Get viewport area, compensate for zooming
        auto viewArea = cnv->viewport->getViewArea() / std::sqrt(std::abs(cnv->getTransform().getDeterminant()));
        auto lastPosition = viewArea.getConstrainedPoint(cnv->lastMousePosition - Point<int>(Object::margin, Object::margin));

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

    // Since this is called in a paint routine, use reinterpret_cast instead of dynamic_cast for efficiency
    // For the standalone, the top-level component should always be DocumentWindow derived!
    if (auto* window = reinterpret_cast<DocumentWindow*>(getTopLevelComponent())) {
        return !window->isUsingNativeTitleBar() && ProjectInfo::canUseSemiTransparentWindows();
    } else {
        return true;
    }
}

void PluginEditor::enablePluginMode(Canvas* cnv)
{
    if (!cnv) {
        if (pd->isInPluginMode()) {
            MessageManager::callAsync([_this = SafePointer(this), this]() {
                if (!_this)
                    return;
                
                // Restore Plugin Mode View
                for(auto* canvas : canvases)
                {
                    if(canvas && canvas->patch.openInPluginMode)
                    {
                        enablePluginMode(canvas);
                    }
                }
            });
        } else {
            return;
        }
    } else {
        cnv->patch.openInPluginMode = true;
        pluginMode = std::make_unique<PluginMode>(cnv);
    }
}

// At the top-level, always catch all keypresses
// This makes sure you can't accidentally do a DAW keyboard shortcut with plugdata open
// Since objects like "keyname" need to be able to respond to any key as well,
// it would be annoying to hear the bloop sound for every key that isn't a valid command
bool PluginEditor::keyPressed(KeyPress const& key)
{
    return key.getKeyCode() != KeyPress::tabKey;
}

void PluginEditor::commandKeyChanged(bool isHeld)
{
    if (isHeld) {
        runButton.setToggleState(true, dontSendNotification);
    } else if (auto* cnv = getCurrentCanvas()) {
        if (!getValue<bool>(cnv->locked)) {
            editButton.setToggleState(true, dontSendNotification);
        }
    }
}

void PluginEditor::quit(bool askToSave)
{
    jassert(ProjectInfo::isStandalone);

    if (askToSave) {
        auto* window = dynamic_cast<DocumentWindow*>(getTopLevelComponent());
        window->closeButtonPressed();
    } else {
        JUCEApplication::quit();
    }
}
