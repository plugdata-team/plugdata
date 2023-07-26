/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Utility/Fonts.h"
#include "Utility/RateReducer.h" // TODO: move to impl
#include "Utility/ModifierKeyListener.h"
#include "Utility/CheckedTooltip.h"
#include "Utility/StackShadow.h" // TODO: move to impl
#include "Utility/ZoomableDragAndDropContainer.h"
#include "Utility/OfflineObjectRenderer.h"
#include "SplitView.h"           // TODO: move to impl
#include "Dialogs/OverlayDisplaySettings.h"
#include "Dialogs/SnapSettings.h"

class ConnectionMessageDisplay;
class Sidebar;
class Statusbar;
class ZoomLabel;
class Dialog;
class WelcomeButton;
class Canvas;
class TabComponent;
class PluginProcessor;
class Palettes;
class PluginMode;
class PluginEditor : public AudioProcessorEditor
    , public Value::Listener
    , public ApplicationCommandTarget
    , public ApplicationCommandManager
    , public FileDragAndDropTarget
    , public ModifierKeyBroadcaster
    , public ModifierKeyListener
    , public ZoomableDragAndDropContainer
{
public:
    enum ToolbarButtonType {
        Settings = 0,
        Undo,
        Redo,
        Add,
        Hide,
        Pin
    };

    explicit PluginEditor(PluginProcessor&);

    ~PluginEditor() override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void resized() override;
    void parentSizeChanged() override;

    // For dragging parent window
    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    void newProject();
    void openProject();
    void saveProject(std::function<void()> const& nestedCallback = []() {});
    void saveProjectAs(std::function<void()> const& nestedCallback = []() {});

    void addTab(Canvas* cnv, int splitIdx = -1);
    void closeTab(Canvas* cnv);
    void closeAllTabs(bool quitAfterComplete = false);

    void quit(bool askToSave);

    Canvas* getCurrentCanvas();

    // Part of the ZoomableDragAndDropContainer, we give it the splitview
    // so it can check if the drag image is over the entire splitview
    // otherwise some objects inside the splitview will trigger a zoom
    SplitView* getSplitView() override;

    void modifierKeysChanged(ModifierKeys const& modifiers) override;

    void valueChanged(Value& v) override;

    void updateCommandStatus();

    bool isInterestedInFileDrag(StringArray const& files) override;
    void filesDropped(StringArray const& files, int x, int y) override;
    void fileDragEnter(StringArray const&, int, int) override;
    void fileDragExit(StringArray const&) override;

    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform(InvocationInfo const& info) override;

    bool wantsRoundedCorners();

    bool keyPressed(KeyPress const& key) override;

    void enablePluginMode(Canvas* cnv);

    void commandKeyChanged(bool isHeld) override;

    void setZoomLabelLevel(float value);

    TabComponent* getActiveTabbar();

    PluginProcessor* pd;

    std::unique_ptr<ConnectionMessageDisplay> connectionMessageDisplay;

    OwnedArray<Canvas, CriticalSection> canvases;
    std::unique_ptr<Sidebar> sidebar;
    std::unique_ptr<Statusbar> statusbar;

    bool canUndo = false, canRedo = false;

    std::unique_ptr<Dialog> openedDialog;

    std::unique_ptr<PluginMode> pluginMode;

    Value theme;

    Value hvccMode;
    Value autoconnect;

    SplitView splitView;
    DrawableRectangle selectedSplitRect;

    std::unique_ptr<Palettes> palettes;

    std::unique_ptr<ZoomLabel> zoomLabel;

    ComponentBoundsConstrainer* defaultConstrainer;
    OfflineObjectRenderer offlineRenderer;

private:

    // Used by standalone to handle dragging the window
    ComponentDragger windowDragger;

    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

    int const toolbarHeight = ProjectInfo::isStandalone ? 40 : 35;

    TextButton mainMenuButton, undoButton, redoButton, addObjectMenuButton, hideSidebarButton, pluginModeButton;
    TextButton editButton, runButton, presentButton;

    CheckedTooltip tooltipWindow;

    TextButton seperators[8];

#if JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif

    bool isMaximised = false;
    bool isDraggingFile = false;

    // Used in plugin
    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;

    // Used in standalone
    std::unique_ptr<MouseRateReducedComponent<ResizableBorderComponent>> borderResizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
