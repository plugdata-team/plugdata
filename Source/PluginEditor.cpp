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
#include "Sidebar/Palettes.h"
#include "Utility/Autosave.h"

#include "Utility/RateReducer.h"
#include "Utility/StackShadow.h"

#include "Canvas.h"
#include "Connection.h"
#include "Dialogs/ConnectionMessageDisplay.h"
#include "Dialogs/Dialogs.h"
#include "Statusbar.h"
#include "Components/WelcomePanel.h"
#include "Sidebar/Sidebar.h"
#include "Object.h"
#include "PluginMode.h"
#include "Components/TouchSelectionHelper.h"

#if ENABLE_TESTING
void runTests(PluginEditor* editor);
#endif

#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <nanovg.h>

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p)
    , pd(&p)
    , sidebar(std::make_unique<Sidebar>(&p, this))
    , statusbar(std::make_unique<Statusbar>(&p))
    , openedDialog(nullptr)
    , nvgSurface(this)
    , pluginConstrainer(*getConstrainer())
    , autosave(std::make_unique<Autosave>(pd))
    , tooltipWindow(nullptr, [](Component* c) {
        if (auto* cnv = c->findParentComponentOfClass<Canvas>()) {
            return !getValue<bool>(cnv->locked);
        }

        return true;
    })
    , tabComponent(this)
    , pluginMode(nullptr)
    , touchSelectionHelper(std::make_unique<TouchSelectionHelper>(this))
{
    keyboardLayout = OSUtils::getKeyboardLayout();
    
#if JUCE_IOS
    // constrainer.setMinimumSize(100, 100);
    // pluginConstrainer.setMinimumSize(100, 100);
    // setResizable(true, false);
#else
    // if we are inside a DAW / host set up the border resizer now
    if (!ProjectInfo::isStandalone) {
        // NEVER touch pluginConstrainer outside of plugin mode!
        pluginConstrainer.setMinimumSize(850, 650);
        setUseBorderResizer(true);
    } else {
        constrainer.setMinimumSize(850, 650);
    }
#endif

    mainMenuButton.setButtonText(Icons::Menu);
    undoButton.setButtonText(Icons::Undo);
    redoButton.setButtonText(Icons::Redo);
    pluginModeButton.setButtonText(Icons::PluginMode);

    editButton.setButtonText(Icons::Edit);
    runButton.setButtonText(Icons::Lock);
    presentButton.setButtonText(Icons::Presentation);

    addKeyListener(commandManager.getKeyMappings());

    welcomePanel = std::make_unique<WelcomePanel>(this);
    addAndMakeVisible(*welcomePanel);
    welcomePanel->setAlwaysOnTop(true);

    setWantsKeyboardFocus(true);
    commandManager.registerAllCommandsForTarget(this);

    for (auto& seperator : seperators) {
        addChildComponent(&seperator);
    }

    auto* settingsFile = SettingsFile::getInstance();
    PlugDataLook::setDefaultFont(settingsFile->getProperty<String>("default_font"));

    auto keymap = settingsFile->getKeyMapTree();
    if (keymap.isValid()) {
        auto xmlStr = keymap.getProperty("keyxml").toString();
        auto elt = XmlDocument(xmlStr).getDocumentElement();

        if (elt) {
            commandManager.getKeyMappings()->restoreFromXml(*elt);
        }
    } else {
        settingsFile->getValueTree().appendChild(ValueTree("KeyMap"), nullptr);
    }

    autoconnect.referTo(settingsFile->getPropertyAsValue("autoconnect"));
    theme.referTo(settingsFile->getPropertyAsValue("theme"));
    theme.addListener(this);

    palettes = std::make_unique<Palettes>(this);

    addChildComponent(*palettes);
    addAndMakeVisible(*statusbar);

    addAndMakeVisible(*sidebar);
    sidebar->toBehind(statusbar.get());
    addAndMakeVisible(tabComponent);

    calloutArea = std::make_unique<CalloutArea>(this);
    calloutArea->setVisible(true);
    calloutArea->setAlwaysOnTop(true);
    calloutArea->setInterceptsMouseClicks(true, true);

    setOpaque(false);

    for (auto* button : std::vector<MainToolbarButton*> {
             &mainMenuButton,
                 &undoButton,
                 &redoButton,
                 &addObjectMenuButton,
#if !JUCE_IOS
                 &pluginModeButton,
#endif
         }) {
        addAndMakeVisible(button);
    }

    // Show settings
    mainMenuButton.setTooltip("Main menu");
    mainMenuButton.onClick = [this]() {
        Dialogs::showMainMenu(this, &mainMenuButton);
    };

    addAndMakeVisible(mainMenuButton);

    //  Undo button
    undoButton.isUndo = true;
    undoButton.onClick = [this]() { getCurrentCanvas()->undo(); };
    addAndMakeVisible(undoButton);

    // Redo button
    redoButton.isRedo = true;
    redoButton.onClick = [this]() { getCurrentCanvas()->redo(); };
    addAndMakeVisible(redoButton);

    // New object button
    addObjectMenuButton.setButtonText(Icons::AddObject);
    addObjectMenuButton.setTooltip("Add object");
    addObjectMenuButton.onClick = [this]() { Dialogs::showObjectMenu(this, &addObjectMenuButton); };
    addAndMakeVisible(addObjectMenuButton);

    // Edit, run and presentation mode buttons
    for (auto* button : std::vector<ToolbarRadioButton*> { &editButton, &runButton, &presentButton }) {
        button->onClick = [this]() {
            if (auto* cnv = getCurrentCanvas()) {
                if (editButton.getToggleState()) {
                    cnv->presentationMode.setValue(false);
                    cnv->locked.setValue(false);
                } else if (runButton.getToggleState()) {
                    cnv->locked.setValue(true);
                    cnv->presentationMode.setValue(false);
                } else if (presentButton.getToggleState()) {
                    cnv->locked.setValue(true);
                    cnv->presentationMode.setValue(true);
                }
            }
        };

        button->setClickingTogglesState(true);
        button->setRadioGroupId(hash("edit_run_present"));
        addAndMakeVisible(button);
    }
    editButton.setToggleState(true, sendNotification);

    editButton.setTooltip("Edit mode");
    runButton.setTooltip("Run mode");
    presentButton.setTooltip("Presentation mode");

    editButton.setConnectedEdges(Button::ConnectedOnRight);
    runButton.setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
    presentButton.setConnectedEdges(Button::ConnectedOnLeft);

    // Enter plugin mode
    pluginModeButton.setTooltip("Enter plugin mode");
    pluginModeButton.setColour(ComboBox::outlineColourId, findColour(TextButton::buttonColourId));
    pluginModeButton.onClick = [this]() {
        if (auto* cnv = getCurrentCanvas()) {
            tabComponent.openInPluginMode(cnv->refCountedPatch);
        }
    };

    sidebar->setSize(250, pd->lastUIHeight - statusbar->getHeight());

    setSize(pd->lastUIWidth, pd->lastUIHeight);

    sidebar->toFront(false);

    // Make sure existing console messages are processed
    sidebar->updateConsole(0, false);
    updateCommandStatus();

    addModifierKeyListener(statusbar.get());

    addAndMakeVisible(&callOutSafeArea);
    callOutSafeArea.setAlwaysOnTop(true);
    callOutSafeArea.setInterceptsMouseClicks(false, true);

    addModifierKeyListener(this);

    connectionMessageDisplay = std::make_unique<ConnectionMessageDisplay>(this);
    connectionMessageDisplay->addToDesktop(ComponentPeer::windowIsTemporary | ComponentPeer::windowIgnoresKeyPresses | ComponentPeer::windowIgnoresMouseClicks);
    if (!ProjectInfo::isStandalone) {
        connectionMessageDisplay->setAlwaysOnTop(true);
    }

    // This cannot be done in MidiDeviceManager's constructor because SettingsFile is not yet initialised at that time
    if (ProjectInfo::isStandalone) {
        auto* midiDeviceManager = ProjectInfo::getMidiDeviceManager();
        midiDeviceManager->loadMidiOutputSettings();
    }

    // This is necessary on Linux to make PluginEditor grab keyboard focus on startup
    // It also appears to be necessary for some DAWs, like Logic
    ::Timer::callAfterDelay(100, [_this = SafePointer(this)]() {
        if (!_this)
            return;

        if (auto* window = _this->getTopLevelComponent()) {
            window->toFront(false);
        }
        _this->grabKeyboardFocus();
    });

    ObjectThemeManager::get()->updateTheme();

    addChildComponent(nvgSurface);
    nvgSurface.toBehind(&tabComponent);

    editorIndex = ProjectInfo::isStandalone ? numEditors++ : 0;

#if JUCE_IOS
    addAndMakeVisible(touchSelectionHelper.get());
    touchSelectionHelper->setAlwaysOnTop(true);
#endif

#if ENABLE_TESTING
        // Call after window is ready
        ::Timer::callAfterDelay(200, [this](){
            runTests(this);
        });
#endif

    pd->messageDispatcher->setBlockMessages(false);
    pd->objectLibrary->waitForInitialisationToFinish();
}

PluginEditor::~PluginEditor()
{
    theme.removeListener(this);

    if (auto* window = dynamic_cast<PlugDataWindow*>(getTopLevelComponent())) {
        ProjectInfo::closeWindow(window); // Make sure plugdatawindow gets cleaned up
    }

    if(!ProjectInfo::isStandalone)
    {
        // Block incoming gui messages from pd if there is no active editor
        pd->messageDispatcher->setBlockMessages(true);
    }
}

void PluginEditor::setUseBorderResizer(bool shouldUse)
{
    if (shouldUse) {
        if (ProjectInfo::isStandalone) {
            if (!borderResizer) {
                borderResizer = std::make_unique<MouseRateReducedComponent<ResizableBorderComponent>>(getTopLevelComponent(), &constrainer);
                borderResizer->setAlwaysOnTop(true);
                addAndMakeVisible(borderResizer.get());
            }
            borderResizer->setVisible(true);
            resized(); // Makes sure resizer gets resized

            if (isInPluginMode()) {
                borderResizer->toBehind(pluginMode.get());
            }
        } else {
            if (!cornerResizer) {
                cornerResizer = std::make_unique<MouseRateReducedComponent<ResizableCornerComponent>>(this, &pluginConstrainer);
                cornerResizer->setAlwaysOnTop(true);
            }
            addAndMakeVisible(cornerResizer.get());
        }
    } else {
        if (ProjectInfo::isStandalone && borderResizer) {
            borderResizer->setVisible(false);
        }
    }
}

void PluginEditor::paint(Graphics& g)
{
    auto baseColour = findColour(PlugDataColour::toolbarBackgroundColourId);

    if (ProjectInfo::isStandalone && !isActiveWindow()) {
        baseColour = baseColour.brighter(baseColour.getBrightness() / 2.5f);
    }

    bool rounded = wantsRoundedCorners();

    if (rounded) {
#if JUCE_MAC || JUCE_LINUX
        g.setColour(baseColour);
        g.fillRoundedRectangle(getLocalBounds().toFloat(), Corners::windowCornerRadius);
#else
        g.fillAll(baseColour);
#endif
    } else {
        g.fillAll(baseColour);
    }
}

// Paint file drop outline
void PluginEditor::paintOverChildren(Graphics& g)
{
    // Never want to be drawing over a dialog window
    if (openedDialog)
        return;
    
    if (isDraggingFile) {
        g.setColour(findColour(PlugDataColour::dataColourId));
        g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), Corners::windowCornerRadius, 2.0f);
    }

    auto welcomePanelVisible = !getCurrentCanvas();
    auto tabbarDepth = welcomePanelVisible ? toolbarHeight + 5.5f : toolbarHeight + 30.0f;
    g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
    g.drawLine(palettes->isExpanded() ? palettes->getRight() : 29.0f, tabbarDepth, sidebar->getX() + 1.0f, tabbarDepth);

    // Draw extra lines in case tabbar is not visible. Otherwise some outlines will stop too soon
    if (!getCurrentCanvas()) {
        auto toolbarDepth = welcomePanelVisible ? toolbarHeight + 6 : toolbarHeight;
        g.drawLine(palettes->isExpanded() ? palettes->getRight() : 29.5f, toolbarDepth, palettes->isExpanded() ? palettes->getRight() : 29.5f, toolbarDepth + 30);
        g.drawLine(sidebar->getX() + 0.5f, toolbarDepth, sidebar->getX() + 0.5f, toolbarHeight + 30);
    }
    
    if(pluginMode)
    {
        g.setColour(findColour(PlugDataColour::canvasBackgroundColourId));
        g.fillRect(getLocalBounds().withTrimmedTop(40));
    }
}

void PluginEditor::renderArea(NVGcontext* nvg, Rectangle<int> area)
{
    if (isInPluginMode()) {
        pluginMode->render(nvg);
    } else {
        if (welcomePanel->isVisible()) {
            NVGScopedState scopedState(nvg);
            welcomePanel->render(nvg);
        } else {
            tabComponent.renderArea(nvg, area);

            if (touchSelectionHelper && touchSelectionHelper->isVisible() && area.intersects(touchSelectionHelper->getBounds() - nvgSurface.getPosition())) {
                NVGScopedState scopedState(nvg);
                nvgTranslate(nvg, touchSelectionHelper->getX() - nvgSurface.getX(), touchSelectionHelper->getY() - nvgSurface.getY());
                touchSelectionHelper->render(nvg);
            }
        }
    }
}

CallOutBox& PluginEditor::showCalloutBox(std::unique_ptr<Component> content, Rectangle<int> screenBounds)
{
    class CalloutDeletionListener : public ComponentListener {
        PluginEditor* editor;

    public:
        explicit CalloutDeletionListener(PluginEditor* e)
            : editor(e)
        {
        }

        void componentBeingDeleted(Component& c) override
        {
            c.removeComponentListener(this);
            editor->calloutArea->removeFromDesktop();
            delete this;
        }
    };

    if (ProjectInfo::canUseSemiTransparentWindows()) {
        content->addComponentListener(new CalloutDeletionListener(this));
        calloutArea->addToDesktop(ComponentPeer::windowIsTemporary);
        calloutArea->toFront(true);
        auto bounds = calloutArea->getLocalArea(nullptr, screenBounds);
        return CallOutBox::launchAsynchronously(std::move(content), bounds, calloutArea.get());
    } else {
        return CallOutBox::launchAsynchronously(std::move(content), screenBounds, nullptr);
    }
}

DragAndDropTarget* PluginEditor::findNextDragAndDropTarget(Point<int> screenPos)
{
    return tabComponent.getScreenBounds().contains(screenPos) ? &tabComponent : nullptr;
}

void PluginEditor::resized()
{
    if (isInPluginMode()) {
        nvgSurface.updateBounds(getLocalBounds().withTrimmedTop(pluginMode->isWindowFullscreen() ? 0 : 40));
        return;
    }

#if JUCE_IOS
    if (auto* window = dynamic_cast<PlugDataWindow*>(getTopLevelComponent())) {
        window->setFullScreen(true);
    }
    if (auto* peer = getPeer()) {
        OSUtils::ScrollTracker::create(peer);
    }
#endif

    auto paletteWidth = palettes->isExpanded() ? palettes->getWidth() : 30;
    if (!palettes->isVisible())
        paletteWidth = 0;

    callOutSafeArea.setBounds(0, toolbarHeight, getWidth(), getHeight() - toolbarHeight - 30);

    statusbar->setBounds(0, getHeight() - Statusbar::statusbarHeight, getWidth(), Statusbar::statusbarHeight);

    auto workAreaHeight = getHeight() - toolbarHeight - Statusbar::statusbarHeight;

    palettes->setBounds(0, toolbarHeight, palettes->getWidth(), workAreaHeight);

    auto workArea = Rectangle<int>(paletteWidth, toolbarHeight, (getWidth() - sidebar->getWidth() - paletteWidth), workAreaHeight);
    tabComponent.setBounds(workArea);
    welcomePanel->setBounds(workArea.withTrimmedTop(4));
    nvgSurface.updateBounds(welcomePanel->isVisible() ? workArea.withTrimmedTop(6) : workArea.withTrimmedTop(31));

    sidebar->setBounds(getWidth() - sidebar->getWidth(), toolbarHeight, sidebar->getWidth(), workAreaHeight);

    auto useLeftButtons = SettingsFile::getInstance()->getProperty<bool>("macos_buttons");
    auto useNonNativeTitlebar = ProjectInfo::isStandalone && !SettingsFile::getInstance()->getProperty<bool>("native_window");
    auto offset = useLeftButtons && useNonNativeTitlebar ? 84 : 15;
#if JUCE_MAC
    if (auto standalone = ProjectInfo::isStandalone ? dynamic_cast<DocumentWindow*>(getTopLevelComponent()) : nullptr)
        offset = standalone->isFullScreen() ? 20 : offset;
#endif
#if JUCE_IOS
    offset += 22;
#endif

    auto const buttonDistance = 56;
    auto const buttonSize = toolbarHeight + 5;
    mainMenuButton.setBounds(offset, 0, buttonSize, buttonSize);
    undoButton.setBounds(buttonDistance + offset, 0, buttonSize, buttonSize);
    redoButton.setBounds((2 * buttonDistance) + offset, 0, buttonSize, buttonSize);
    addObjectMenuButton.setBounds((3 * buttonDistance) + offset, 0, buttonSize, buttonSize);

    auto startX = (getWidth() / 2.0f) - (toolbarHeight * 1.5);

#if JUCE_IOS
    auto touchHelperBounds = getLocalBounds().removeFromBottom(48).withSizeKeepingCentre(192, 48).translated(0, -54);
    if(touchSelectionHelper) touchSelectionHelper->setBounds(touchHelperBounds);
    
    if(OSUtils::isIPad())
    {
        startX += 80.0f; // Otherwise it gets in the way of multitasking controls
    }
#endif
    editButton.setBounds(startX, 1, buttonSize, buttonSize - 2);
    runButton.setBounds(startX + buttonSize - 1, 1, buttonSize, buttonSize - 2);
    presentButton.setBounds(startX + (2 * buttonSize) - 2, 1, buttonSize, buttonSize - 2);

    auto windowControlsOffset = (useNonNativeTitlebar && !useLeftButtons) ? 135.0f : 45.0f;

    if (borderResizer && ProjectInfo::isStandalone) {
        borderResizer->setBounds(getLocalBounds());
    } else if (cornerResizer) {
        int const resizerSize = 18;
        cornerResizer->setBounds(getWidth() - resizerSize + 1,
            getHeight() - resizerSize + 1,
            resizerSize, resizerSize);
    }

    pluginModeButton.setBounds(getWidth() - windowControlsOffset, 0, buttonSize, buttonSize);

    pd->lastUIWidth = getWidth();
    pd->lastUIHeight = getHeight();

    repaint(); // Some outlines are dependent on whether or not the sidebars are expanded, or whether or not a patch is opened
}

bool PluginEditor::isInPluginMode() const
{
    return static_cast<bool>(pluginMode);
}

// Retern the patch that belongs to this editor that's in plugin mode
pd::Patch::Ptr PluginEditor::findPatchInPluginMode()
{
    ScopedLock lock(pd->patches.getLock());

    for (auto& patch : pd->patches) {
        if (editorIndex == patch->windowIndex && patch->openInPluginMode) {
            return patch;
        }
    }
    return nullptr;
}

void PluginEditor::parentSizeChanged()
{
    if (!ProjectInfo::isStandalone)
        return;

    auto* standalone = dynamic_cast<PlugDataWindow*>(getTopLevelComponent());
    // Hide TitleBar Buttons in Plugin Mode
    bool visible = !isInPluginMode();
#if JUCE_MAC
    if (!standalone->useNativeTitlebar()) {
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
    if (!standalone->useNativeTitlebar()) {
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
        if (auto* window = findParentComponentOfClass<PlugDataWindow>()) {
            if (!window->useNativeTitlebar())
                windowDragger.startDraggingWindow(window, e.getEventRelativeTo(window));
        }
    }
}

void PluginEditor::mouseDrag(MouseEvent const& e)
{
    if (!ProjectInfo::isStandalone)
        return;

    if (!isMaximised) {
        if (auto* window = findParentComponentOfClass<PlugDataWindow>()) {
            if (!window->useNativeTitlebar())
                windowDragger.dragWindow(window, e.getEventRelativeTo(window), nullptr);
        }
    }
}

bool PluginEditor::isInterestedInFileDrag(StringArray const& files)
{
    if (openedDialog)
        return false;

    for (auto& path : files) {
        auto file = File(path);
        if (file.exists()) {
            return true;
        }
    }

    return false;
}

void PluginEditor::fileDragMove(StringArray const& files, int x, int y)
{
    for (auto& path : files) {
        auto file = File(path);
        if (file.exists() && file.hasFileExtension("pd") && !isDraggingFile) {
            isDraggingFile = true;
            repaint();
            return;
        }
    }

    bool wasDraggingFile = isDraggingFile;
    if (auto* cnv = tabComponent.getCanvasAtScreenPosition(localPointToGlobal(Point<int>(x, y)))) {
        if (wasDraggingFile) {
            isDraggingFile = false;
            repaint();
        }

        tabComponent.setActiveSplit(cnv);
        return;
    } else {
        if (!wasDraggingFile) {
            isDraggingFile = true;
        }
        repaint();
    }
}

void PluginEditor::filesDropped(StringArray const& files, int x, int y)
{
    // First check for .pd files
    bool openedPdFiles = false;
    for (auto& path : files) {
        auto file = File(path);
        if (file.exists() && file.hasFileExtension("pd")) {
            openedPdFiles = true;
            autosave->checkForMoreRecentAutosave(file, this, [this, file]() {
                tabComponent.openPatch(URL(file));
                SettingsFile::getInstance()->addToRecentlyOpened(file);
            });
        }
    }

    if (auto* cnv = tabComponent.getCanvasAtScreenPosition(localPointToGlobal(Point<int>(x, y)))) {
        for (auto& path : files) {
            auto file = File(path);
            if (file.exists() && !openedPdFiles) {
                auto position = cnv->getLocalPoint(this, Point<int>(x, y));
                auto filePath = file.getFullPathName().replaceCharacter('\\', '/').replace(" ", "\\ ");

                auto* object = cnv->objects.add(new Object(cnv, "msg " + filePath, position));
                object->hideEditor();
            }
        }
    }

    isDraggingFile = false;
    repaint();
}
void PluginEditor::fileDragEnter(StringArray const&, int, int)
{
    // isDraggingFile = true;
    repaint();
}

void PluginEditor::fileDragExit(StringArray const&)
{
    isDraggingFile = false;
    repaint();
}

TabComponent& PluginEditor::getTabComponent()
{
    return tabComponent;
}

bool PluginEditor::isActiveWindow()
{
    bool isDraggingTab = ZoomableDragAndDropContainer::isDragAndDropActive();
    return !ProjectInfo::isStandalone || isDraggingTab || (TopLevelWindow::getActiveTopLevelWindow() == getTopLevelComponent());
}

Array<Canvas*> PluginEditor::getCanvases()
{
    return tabComponent.getCanvases();
}

Canvas* PluginEditor::getCurrentCanvas()
{
    return tabComponent.getCurrentCanvas();
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

// Updates command status asynchronously
void PluginEditor::handleAsyncUpdate()
{
    tabComponent.repaint(); // So tab dirty titles can be reflected

    if (auto* cnv = getCurrentCanvas()) {
        bool locked = getValue<bool>(cnv->locked);
        bool isDragging = cnv->dragState.didStartDragging && !cnv->isDraggingLasso && cnv->locked == var(false);

        if (getValue<bool>(cnv->presentationMode)) {
            presentButton.setToggleState(true, dontSendNotification);
        } else if (locked) {
            runButton.setToggleState(true, dontSendNotification);
        } else {
            editButton.setToggleState(true, dontSendNotification);
        }

        auto currentUndoState = cnv->patch.canUndo() && !isDragging && !locked;
        auto currentRedoState = cnv->patch.canRedo() && !isDragging && !locked;

        undoButton.setEnabled(currentUndoState);
        redoButton.setEnabled(currentRedoState);

        // Application commands need to be updated when undo state changes
        commandManager.commandStatusChanged();

        pluginModeButton.setEnabled(true);

        editButton.setEnabled(true);
        runButton.setEnabled(true);
        presentButton.setEnabled(true);

        statusbar->setHasActiveCanvas(true);
        addObjectMenuButton.setEnabled(true);
#if JUCE_IOS
        if(!locked)
        {
            touchSelectionHelper->show();
        }
        else {
            touchSelectionHelper->setVisible(false);
        }
#endif
    } else {
        pluginModeButton.setEnabled(false);

        editButton.setEnabled(false);
        runButton.setEnabled(false);
        presentButton.setEnabled(false);

        statusbar->setHasActiveCanvas(false);

        undoButton.setEnabled(false);
        redoButton.setEnabled(false);
        addObjectMenuButton.setEnabled(false);
#if JUCE_IOS
        touchSelectionHelper->setVisible(false);
#endif
    }
#if JUCE_IOS
    nvgSurface.invalidateAll(); // Make sure touch selection helper is repainted
#endif
}

void PluginEditor::updateCommandStatus()
{
    statusbar->updateZoomLevel();

    // Make sure patches update their undo/redo state information soon
    pd->updatePatchUndoRedoState();
    AsyncUpdater::triggerAsyncUpdate();
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
    for (int n = NewObject; n < NumObjects; n++) {
        commands.add(n);
    }

    commands.add(StandardApplicationCommandIDs::quit);
}

void PluginEditor::getCommandInfo(CommandID const commandID, ApplicationCommandInfo& result)
{

    if (commandID == StandardApplicationCommandIDs::quit) {
        result.setInfo("Quit",
            "Quits the application",
            "Application", 0);

        result.defaultKeypresses.add(KeyPress('q', ModifierKeys::commandModifier, 0));
        return;
    }

    bool hasObjectSelection = false;
    bool hasConnectionSelection = false;
    bool hasSelection = false;
    bool isDragging = false;
    bool hasCanvas = false;
    bool locked = true;
    bool canUndo = false;
    bool canRedo = false;

    if (auto* cnv = getCurrentCanvas()) {
        auto selectedObjects = cnv->getSelectionOfType<Object>();
        auto selectedConnections = cnv->getSelectionOfType<Connection>();

        hasObjectSelection = !selectedObjects.isEmpty();
        hasConnectionSelection = !selectedConnections.isEmpty();

        hasSelection = hasObjectSelection || hasConnectionSelection;
        isDragging = cnv->dragState.didStartDragging && !cnv->isDraggingLasso && cnv->locked == var(false);
        hasCanvas = true;

        canUndo = cnv->patch.canUndo() && !isDragging;
        canRedo = cnv->patch.canRedo() && !isDragging;

        locked = getValue<bool>(cnv->locked);
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
        result.setActive(canUndo);
        break;
    }
    case CommandIDs::Redo: {
        result.setInfo("Redo", "Redo action", "General", 0);
        result.addDefaultKeypress(90, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(canRedo);
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
        result.setActive(hasCanvas && !isDragging && hasConnectionSelection);
        break;
    }
    case CommandIDs::ConnectionStyle: {
        result.setInfo("Connection style", "Set connection style", "Edit", 0);
        result.addDefaultKeypress(76, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && hasConnectionSelection);
        break;
    }
    case CommandIDs::PanDragKey: {
        result.setInfo("Pan drag key", "Pan drag key", "View", ApplicationCommandInfo::dontTriggerAlertSound);
        result.addDefaultKeypress(KeyPress::spaceKey, ModifierKeys::noModifiers);
        result.setActive(hasCanvas && !isDragging && !isInPluginMode());
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
    case CommandIDs::Tidy: {
        result.setInfo("Tidy", "Tidy objects", "Edit", 0);
        result.addDefaultKeypress(82, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        result.setActive(hasCanvas && !isDragging && !locked && hasSelection);
        break;
    }
    case CommandIDs::Triggerize: {
        result.setInfo("Triggerize", "Triggerize objects", "Edit", 0);
        result.addDefaultKeypress(84, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && !locked && hasSelection);
        break;
    }
    case CommandIDs::Duplicate: {
        result.setInfo("Duplicate", "Duplicate selection", "Edit", 0);
        result.addDefaultKeypress(68, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && !locked && hasSelection);
        break;
    }
    case CommandIDs::CreateConnection: {
        result.setInfo("Create connection", "Create a connection between selected objects", "Edit", 0);
        result.addDefaultKeypress(75, ModifierKeys::commandModifier);
        result.setActive(hasCanvas && !isDragging && !locked && hasSelection);
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
        result.addDefaultKeypress(125, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
#else
        result.addDefaultKeypress(KeyPress::pageDownKey, ModifierKeys::commandModifier);
#endif

        break;
    }
    case CommandIDs::PreviousTab: {
        result.setInfo("Previous Tab", "Show the previous tab", "View", 0);
        result.setActive(true);
#if JUCE_MAC
        result.addDefaultKeypress(123, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
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

        if (auto* cnv = getCurrentCanvas()) {
            auto selection = cnv->getSelectionOfType<Object>();
            bool enabled = selection.size() == 1 && selection[0]->getType().isNotEmpty();
            result.setActive(enabled);
        } else {
            result.setActive(false);
        }
        break;
    }
    case CommandIDs::ShowHelp: {
        result.setInfo("Open Help", "Open help file", "View", 0);
        result.addDefaultKeypress(KeyPress::F2Key, ModifierKeys::noModifiers); // f1 to open reference

        if (auto* cnv = getCurrentCanvas()) {
            auto selection = cnv->getSelectionOfType<Object>();
            bool enabled = selection.size() == 1 && selection[0]->getType().isNotEmpty();
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
    case CommandIDs::ToggleDSP: {
        result.setInfo("Toggle DSP", "Enables or disables audio DSP", "Edit", 0);
        result.setActive(true);
        break;
    }
    default:
        break;
    }

    if (commandID >= ObjectIDs::NewObject) {
        static auto const cmdMod = ModifierKeys::commandModifier;
        static auto const shiftMod = ModifierKeys::shiftModifier;

        std::map<ObjectIDs, std::pair<int, int>> defaultShortcuts;

        switch (keyboardLayout) {
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
                { NewVUMeter, { 85, cmdMod | shiftMod } }
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
                { NewVUMeter, { 85, cmdMod | shiftMod } }
            };
            break;

        default:
            break;
        }
        
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
    switch (info.commandID) {
    case StandardApplicationCommandIDs::quit: {
        if (ProjectInfo::isStandalone) {
            quit(true);
            return true;
        }

        return false;
    }
    case CommandIDs::NewProject: {
        tabComponent.newPatch();
        return true;
    }
    case CommandIDs::OpenProject: {
        tabComponent.openPatch();
        return true;
    }
    case CommandIDs::ShowBrowser: {
        sidebar->showPanel(sidebar->isShowingBrowser() ? 0 : 1);
        return true;
    }
    case CommandIDs::ToggleSidebar: {
        sidebar->showSidebar(sidebar->isHidden());
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
        cnv->save();
        return true;
    }
    case CommandIDs::SaveProjectAs: {
        cnv->saveAs();
        return true;
    }
    case CommandIDs::CloseTab: {
        if (cnv) {
            MessageManager::callAsync([this, cnv = SafePointer(cnv)]() mutable {
                if (cnv && cnv->patch.isDirty()) {
                    Dialogs::showAskToSaveDialog(
                        &openedDialog, this, cnv->patch.getTitle(),
                        [this, cnv](int result) mutable {
                            if (!cnv)
                                return;
                            if (result == 2)
                                cnv->save([this, cnv]() mutable { tabComponent.closeTab(cnv); });
                            else if (result == 1)
                                tabComponent.closeTab(cnv);
                        },
                        0, true);
                } else {
                    tabComponent.closeTab(cnv);
                }
            });
        }

        return true;
    }
    case CommandIDs::Copy: {
        cnv = getCurrentCanvas();
        cnv->copySelection();
        return true;
    }
    case CommandIDs::Paste: {
        cnv = getCurrentCanvas();
        cnv->pasteSelection();
        return true;
    }
    case CommandIDs::Cut: {
        cnv = getCurrentCanvas();
        cnv->cancelConnectionCreation();
        cnv->copySelection();
        cnv->removeSelection();
        return true;
    }
    case CommandIDs::Delete: {
        cnv = getCurrentCanvas();
        cnv->cancelConnectionCreation();
        cnv->removeSelection();
        return true;
    }
    case CommandIDs::Duplicate: {
        cnv = getCurrentCanvas();
        cnv->duplicateSelection();
        return true;
    }
    case CommandIDs::Encapsulate: {
        cnv = getCurrentCanvas();
        cnv->encapsulateSelection();
        return true;
    }
    case CommandIDs::Tidy:
    {
        cnv = getCurrentCanvas();
        cnv->tidySelection();
        return true;
    }
    case CommandIDs::Triggerize: {
        cnv = getCurrentCanvas();
        cnv->triggerizeSelection();
        return true;
    }
    case CommandIDs::CreateConnection: {
        cnv = getCurrentCanvas();
        cnv->connectSelection();
        return true;
    }
    case CommandIDs::RemoveConnections: {
        cnv = getCurrentCanvas();
        cnv->removeSelectedConnections();
        return true;
    }
    case CommandIDs::SelectAll: {
        cnv = getCurrentCanvas();
        for (auto* object : cnv->objects) {
            cnv->setSelected(object, true, false);
        }
        for (auto* con : cnv->connections) {
            cnv->setSelected(con, true, false);
        }
        updateCommandStatus();
        cnv->updateSidebarSelection();
        return true;
    }
    case CommandIDs::Lock: {
        cnv->locked = !getValue<bool>(cnv->locked);
        cnv->presentationMode = false;
        return true;
    }
    case CommandIDs::ConnectionStyle: {
        bool noneSegmented = true;
        for (auto* con : cnv->getSelectionOfType<Connection>()) {
            if (con->isSegmented())
                noneSegmented = false;
        }

        for (auto* con : cnv->getSelectionOfType<Connection>()) {
            con->setSegmented(noneSegmented);
        }

        return true;
    }
    case CommandIDs::ConnectionPathfind: {
        cnv = getCurrentCanvas();
        cnv->patch.startUndoSequence("ConnectionPathFind");

        for (auto* con : cnv->getSelectionOfType<Connection>()) {
            con->applyBestPath();
        }

        cnv->patch.endUndoSequence("ConnectionPathFind");
        return true;
    }
    case CommandIDs::ZoomIn: {
        auto* viewport = dynamic_cast<CanvasViewport*>(cnv->viewport.get());
        if (!viewport)
            return false;
        float newScale = getValue<float>(getCurrentCanvas()->zoomScale) + 0.1f;
        newScale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.25f, 3.0f) * 10.))) / 10.;
        viewport->magnify(newScale);
        return true;
    }
    case CommandIDs::ZoomOut: {
        auto* viewport = dynamic_cast<CanvasViewport*>(cnv->viewport.get());
        if (!viewport)
            return false;
        float newScale = getValue<float>(getCurrentCanvas()->zoomScale) - 0.1f;
        newScale = static_cast<float>(static_cast<int>(round(std::clamp(newScale, 0.25f, 3.0f) * 10.))) / 10.;
        viewport->magnify(newScale);
        return true;
    }
    case CommandIDs::ZoomNormal: {
        auto& scale = getCurrentCanvas()->zoomScale;
        scale = 1.0f;
        return true;
    }
    case CommandIDs::ZoomToFitAll: {
        cnv = getCurrentCanvas();
        cnv->zoomToFitAll();
        return true;
    }
    case CommandIDs::GoToOrigin: {
        cnv = getCurrentCanvas();
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
        tabComponent.nextTab();
        return true;
    }
    case CommandIDs::PreviousTab: {
        tabComponent.previousTab();
        return true;
    }
    case CommandIDs::ShowReference: {
        if (auto* cnv = getCurrentCanvas()) {
            auto selection = cnv->getSelectionOfType<Object>();
            if (selection.size() != 1 || selection[0]->getType().isEmpty()) {
                return false;
            }

            Dialogs::showObjectReferenceDialog(&openedDialog, this, selection[0]->getType());

            return true;
        }

        return false;
    }
    case CommandIDs::ShowHelp: {
        if (auto* cnv = getCurrentCanvas()) {
            auto selection = cnv->getSelectionOfType<Object>();
            if (selection.size() != 1 || selection[0]->getType().isEmpty()) {
                return false;
            }
            selection[0]->openHelpPatch();
            return true;
        }

        return false;
    }
    case CommandIDs::OpenObjectBrowser: {
        Dialogs::showObjectBrowserDialog(&openedDialog, this);
        return true;
    }
    case CommandIDs::ToggleDSP: {
        if (pd_getdspstate()) {
            pd->releaseDSP();
        } else {
            pd->startDSP();
        }

        return true;
    }
    default: {
        cnv = getCurrentCanvas();
        if (!cnv->viewport)
            return false;

        // This should close any opened editors before creating a new object
        cnv->grabKeyboardFocus();

        // Get viewport area, compensate for zooming
        auto viewArea = cnv->viewport->getViewArea() / std::sqrt(std::abs(cnv->getTransform().getDeterminant()));
        auto lastPosition = viewArea.getConstrainedPoint(cnv->getMouseXYRelative() - Point<int>(Object::margin, Object::margin));

        auto ID = static_cast<ObjectIDs>(info.commandID);

        if (objectNames.count(ID)) {
            if (cnv->getSelectionOfType<Object>().size() == 1) {
                // if 1 object is selected, create new object beneath selected
                auto obj = cnv->getSelectionOfType<Object>()[0];
                obj->hideEditor(); // If it's still open, it might overwrite lastSelectedObject
                cnv->lastSelectedObject = obj;
                if (obj) {
                    cnv->objects.add(new Object(cnv, objectNames.at(ID),
                        Point<int>(
                            // place beneath object + Object::margin
                            obj->getX() + Object::margin,
                            obj->getY() + obj->getHeight())));
                }
            } else if ((cnv->getSelectionOfType<Object>().size() == 0) && (cnv->getSelectionOfType<Connection>().size() == 1)) { // Autopatching: insert object in connection. Should document this better!
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
    if (auto* window = reinterpret_cast<PlugDataWindow*>(getTopLevelComponent())) {
        return !window->useNativeTitlebar() && !window->isMaximised() && ProjectInfo::canUseSemiTransparentWindows();
    } else {
        return true;
    }
}

// At the top-level, always catch all keypresses
// This makes sure you can't accidentally do a DAW keyboard shortcut with plugdata open
// Since objects like "keyname" need to be able to respond to any key as well,
// it would be annoying to hear the bloop sound for every key that isn't a valid command
bool PluginEditor::keyPressed(KeyPress const& key)
{
    if (!getCurrentCanvas())
        return false;

    // Claim tab keys on canvas to prevent cycling selection
    // The user might want to catch the tab key with an object, this behaviour just gets in the way
    // We do still want to allow tab cycling on other components, so if canvas doesn't have focus, don't grab the tab key
    return getCurrentCanvas()->hasKeyboardFocus(true) || (key.getKeyCode() != KeyPress::tabKey && key.getKeyCode() != KeyPress::spaceKey);
}

void PluginEditor::parentHierarchyChanged()
{
    if (isShowing() || isOnDesktop())
        grabKeyboardFocus();
}

void PluginEditor::broughtToFront()
{
    if (isShowing() || isOnDesktop())
        grabKeyboardFocus();

    if (openedDialog)
        openedDialog->toFront(true);
}

void PluginEditor::lookAndFeelChanged()
{
    ObjectThemeManager::get()->updateTheme();
}

void PluginEditor::commandKeyChanged(bool isHeld)
{
    if (isHeld && !presentButton.getToggleState()) {
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
        nvgSurface.detachContext();
        JUCEApplication::quit();
    }
}

// Finds an object, then centres and selects it, to indicate it's the target of a search action
// If you set "openNewTabIfNeeded" to true, it will open a new tab if the object you're trying to highlight is not currently visible
// Returns true if successful. If "openNewTabIfNeeded" it should always return true as long as target is valid
bool PluginEditor::highlightSearchTarget(void* target, bool openNewTabIfNeeded)
{
    std::function<t_glist*(t_glist*, void*)> findSearchTargetRecursively;
    findSearchTargetRecursively = [&findSearchTargetRecursively](t_glist* glist, void* target) -> t_glist* {
        for (auto* y = glist->gl_list; y; y = y->g_next) {
            if (pd_class(&y->g_pd) == canvas_class) {
                if (auto* subpatch = findSearchTargetRecursively(reinterpret_cast<t_glist*>(y), target)) {
                    return subpatch;
                }
            }
            if (y == target) {
                return glist;
            }
        }

        return nullptr;
    };

    ScopedLock audioLock(pd->audioLock);

    t_glist* targetCanvas = nullptr;
    for (auto* glist = pd_getcanvaslist(); glist; glist = glist->gl_next) {
        auto* found = findSearchTargetRecursively(glist, target);
        if (found) {
            targetCanvas = found;
            break;
        }
    }

    if (!targetCanvas)
        return false;

    for (auto* cnv : getCanvases()) {
        if (cnv->patch.getPointer().get() == targetCanvas) {
            Object* found = nullptr;
            for (auto* object : cnv->objects) {
                if (object->getPointer() == target) {
                    found = object;
                    break;
                }
            }

            if (found) {

                cnv->deselectAll();
                cnv->setSelected(found, true);

                auto* viewport = cnv->viewport.get();
                if (!viewport)
                    return false;

                auto scale = getValue<float>(cnv->zoomScale);
                auto pos = found->getBounds().getCentre() * scale;

                pos.x -= viewport->getViewWidth() * 0.5f;
                pos.y -= viewport->getViewHeight() * 0.5f;

                viewport->setViewPosition(pos);
                tabComponent.showTab(cnv);

                return true;
            }
        }
    }

    if (openNewTabIfNeeded) {
        auto* cnv = tabComponent.openPatch(new pd::Patch(pd::WeakReference(targetCanvas, pd), pd, false));

        Object* found = nullptr;
        for (auto* object : cnv->objects) {
            if (object->getPointer() == target) {
                found = object;
                break;
            }
        }

        if (found) {

            cnv->deselectAll();
            cnv->setSelected(found, true);

            auto* viewport = cnv->viewport.get();
            if (!viewport)
                return false;

            auto scale = getValue<float>(cnv->zoomScale);
            auto pos = found->getBounds().getCentre() * scale;

            pos.x -= viewport->getViewWidth() * 0.5f;
            pos.y -= viewport->getViewHeight() * 0.5f;

            viewport->setViewPosition(pos);
            tabComponent.showTab(cnv);
        }

        return true;
    }

    return false;
}
