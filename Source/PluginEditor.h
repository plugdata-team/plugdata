/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include <map>

#include "Utility/Fonts.h"
#include "Utility/ModifierKeyListener.h"
#include "Components/CheckedTooltip.h"
#include "Utility/ZoomableDragAndDropContainer.h"
#include "Utility/OfflineObjectRenderer.h"
#include "Utility/WindowDragger.h"
#include "Canvas.h"
#include "Components/Buttons.h"
#include "Components/SearchEditor.h"
#include "TabComponent.h"

#include "Utility/ObjectThemeManager.h"
#include "NVGSurface.h"

class CalloutArea final : public Component
    , public Timer {
public:
    explicit CalloutArea(Component* parent)
        : target(parent)
        , tooltipWindow(this)
    {
        setVisible(true);
        setAlwaysOnTop(true);
        setInterceptsMouseClicks(false, true);
        startTimerHz(3);
    }

    ~CalloutArea() override = default;

    void timerCallback() override
    {
        setBounds(target->getScreenBounds());
    }

    void paint(Graphics& g) override
    {
        if (!ProjectInfo::canUseSemiTransparentWindows()) {
            g.fillAll(findColour(PlugDataColour::popupMenuBackgroundColourId));
        }
    }

private:
    WeakReference<Component> target;
    TooltipWindow tooltipWindow;
};

class ConnectionMessageDisplay;
class Sidebar;
class Statusbar;
class Dialog;
class Canvas;
class TabComponent;
class PluginProcessor;
class Palettes;
class Autosave;
class PluginMode;
class TouchSelectionHelper;
class WelcomePanel;
class PluginEditor final : public AudioProcessorEditor
    , public Value::Listener
    , public ApplicationCommandTarget
    , public FileDragAndDropTarget
    , public ModifierKeyBroadcaster
    , public ModifierKeyListener
    , public ZoomableDragAndDropContainer
    , public AsyncUpdater
    , public Timer
{
public:
    explicit PluginEditor(PluginProcessor&);

    ~PluginEditor() override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void renderArea(NVGcontext* nvg, Rectangle<int> area);

    bool isActiveWindow() override;

    void resized() override;
    void parentSizeChanged() override;
    void parentHierarchyChanged() override;
    void broughtToFront() override;
    
    void timerCallback() override;

    void lookAndFeelChanged() override;

    // For dragging parent window
    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    void showWelcomePanel(bool shouldShow);

    void quit(bool askToSave);

    SmallArray<Canvas*> getCanvases();
    Canvas* getCurrentCanvas();
    
    float getRenderScale() const;

    void modifierKeysChanged(ModifierKeys const& modifiers) override;

    void valueChanged(Value& v) override;

    void updateCommandStatus();
    void handleAsyncUpdate() override;

    void updateSelection(Canvas* cnv);
    void setCommandButtonObject(Object* obj);

    bool isInterestedInFileDrag(StringArray const& files) override;
    void filesDropped(StringArray const& files, int x, int y) override;
    void fileDragEnter(StringArray const&, int, int) override;
    void fileDragMove(StringArray const& files, int x, int y) override;
    void fileDragExit(StringArray const&) override;

    TabComponent& getTabComponent() override;

    DragAndDropTarget* findNextDragAndDropTarget(Point<int> screenPos) override;

    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform(InvocationInfo const& info) override;

    bool wantsRoundedCorners() const;

    bool keyPressed(KeyPress const& key) override;

    CallOutBox& showCalloutBox(std::unique_ptr<Component> content, Rectangle<int> screenBounds);

    static void updateIoletGeometryForAllObjects(PluginProcessor* pd);

    void commandKeyChanged(bool isHeld) override;
    void setUseBorderResizer(bool shouldUse);

    Object* highlightSearchTarget(void* target, bool openNewTabIfNeeded);

    SmallArray<pd::WeakReference> openTextEditors;

    PluginProcessor* pd;

    std::unique_ptr<ConnectionMessageDisplay> connectionMessageDisplay;

    std::unique_ptr<Sidebar> sidebar;
    std::unique_ptr<Statusbar> statusbar;

    std::unique_ptr<Dialog> openedDialog;

    Value theme;
    Value autoconnect;

    std::unique_ptr<Palettes> palettes;

    NVGSurface nvgSurface;

    // used to display callOutBoxes only in a safe area between top & bottom toolbars
    Component callOutSafeArea;

    ComponentBoundsConstrainer constrainer;
    ComponentBoundsConstrainer& pluginConstrainer;

    ApplicationCommandManager commandManager;

    std::unique_ptr<CalloutArea> calloutArea;
    std::unique_ptr<WelcomePanel> welcomePanel;

    CheckedTooltip tooltipWindow;

    int editorIndex;

    bool isInPluginMode() const;

    // Return the canvas currently in plugin mode, otherwise return nullptr
    Canvas* getPluginModeCanvas() const;

private:
    TabComponent tabComponent;

public:
    std::unique_ptr<PluginMode> pluginMode;

private:
    std::unique_ptr<TouchSelectionHelper> touchSelectionHelper;

    // Used by standalone to handle dragging the window
    WindowDragger windowDragger;

    int const toolbarHeight = 34;

    MainToolbarButton mainMenuButton, undoButton, redoButton, addObjectMenuButton, pluginModeButton, welcomePanelSearchButton;
    SettingsToolbarButton recentlyOpenedPanelSelector, libraryPanelSelector;
    ToolbarRadioButton editButton, runButton, presentButton;

    SearchEditor welcomePanelSearchInput;

#if JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif

    bool isMaximised = false;
    bool isDraggingFile = false;

    static inline int numEditors = 0;

    Rectangle<int> workArea;

    // Used in plugin
    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;

    // Used in standalone
    std::unique_ptr<MouseRateReducedComponent<ResizableBorderComponent>> borderResizer;

    OSUtils::KeyboardLayout keyboardLayout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
