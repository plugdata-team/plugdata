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
#include "TabComponent.h"

#include "Utility/ObjectThemeManager.h"
#include "NVGSurface.h"

class CalloutArea : public Component, public Timer {
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

    ~CalloutArea() = default;

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
class PluginEditor : public AudioProcessorEditor
    , public Value::Listener
    , public ApplicationCommandTarget
    , public FileDragAndDropTarget
    , public ModifierKeyBroadcaster
    , public ModifierKeyListener
    , public ZoomableDragAndDropContainer
    , public AsyncUpdater {
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

    void lookAndFeelChanged() override;

    // For dragging parent window
    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    void quit(bool askToSave);

    Array<Canvas*> getCanvases();
    Canvas* getCurrentCanvas();

    void modifierKeysChanged(ModifierKeys const& modifiers) override;

    void valueChanged(Value& v) override;

    void updateCommandStatus();
    void handleAsyncUpdate() override;

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

    bool wantsRoundedCorners();

    bool keyPressed(KeyPress const& key) override;

    CallOutBox& showCalloutBox(std::unique_ptr<Component> content, Rectangle<int> screenBounds);

    void commandKeyChanged(bool isHeld) override;
    void setUseBorderResizer(bool shouldUse);

    bool highlightSearchTarget(void* target, bool openNewTabIfNeeded);

    Array<pd::WeakReference> openTextEditors;

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

    std::unique_ptr<Autosave> autosave;
    ApplicationCommandManager commandManager;

    std::unique_ptr<CalloutArea> calloutArea;
    std::unique_ptr<WelcomePanel> welcomePanel;

    CheckedTooltip tooltipWindow;

    int editorIndex;

    pd::Patch::Ptr findPatchInPluginMode();

    bool isInPluginMode() const;

private:
    TabComponent tabComponent;

public:
    std::unique_ptr<PluginMode> pluginMode;

private:
    std::unique_ptr<TouchSelectionHelper> touchSelectionHelper;

    // Used by standalone to handle dragging the window
    WindowDragger windowDragger;

    int const toolbarHeight = 34;

    MainToolbarButton mainMenuButton, undoButton, redoButton, addObjectMenuButton, pluginModeButton;
    ToolbarRadioButton editButton, runButton, presentButton;
    TextButton seperators[8];

#if JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif

    bool isMaximised = false;
    bool isDraggingFile = false;

    static inline int numEditors = 0;

    // Used in plugin
    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;

    // Used in standalone
    std::unique_ptr<MouseRateReducedComponent<ResizableBorderComponent>> borderResizer;

    OSUtils::KeyboardLayout keyboardLayout;
        
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
