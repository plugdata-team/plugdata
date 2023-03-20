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
#include "Utility/StackShadow.h" // TODO: move to impl
#include "SplitView.h"           // TODO: move to impl

class Sidebar;
class Statusbar;
class ZoomLabel;
class Dialog;
class WelcomeButton;
class Canvas;
class TabComponent;
class PluginProcessor;
class Palettes;
class PluginEditor : public AudioProcessorEditor
    , public Value::Listener
    , public ApplicationCommandTarget
    , public ApplicationCommandManager
    , public FileDragAndDropTarget
    , public ModifierKeyBroadcaster {
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

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override;
    void mouseMagnify(MouseEvent const& e, float scaleFactor) override;

    // For dragging parent window
    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    void newProject();
    void openProject();
    void saveProject(std::function<void()> const& nestedCallback = []() {});
    void saveProjectAs(std::function<void()> const& nestedCallback = []() {});

    void addTab(Canvas* cnv);
    void closeTab(Canvas* cnv);
    void closeAllTabs();

    Canvas* getCurrentCanvas();

    void modifierKeysChanged(ModifierKeys const& modifiers) override;

    void valueChanged(Value& v) override;

    void updateCommandStatus();

    bool isInterestedInFileDrag(StringArray const& files) override;
    void filesDropped(StringArray const& files, int x, int y) override;
    void fileDragEnter(StringArray const&, int, int) override;
    void fileDragExit(StringArray const&) override;

    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform(InvocationInfo const& info) override;

    bool wantsRoundedCorners();

    float getZoomScale();
    float getZoomScaleForCanvas(Canvas* cnv);
    Value& getZoomScaleValueForCanvas(Canvas* cnv);

    TabComponent* getActiveTabbar();

    PluginProcessor* pd;

    OwnedArray<Canvas, CriticalSection> canvases;
    std::unique_ptr<Sidebar> sidebar;
    std::unique_ptr<Statusbar> statusbar;

    std::atomic<bool> canUndo = false, canRedo = false;

    std::unique_ptr<Dialog> openedDialog;

    Value theme;

    Value hvccMode;
    Value autoconnect;

    SplitView splitView;
    DrawableRectangle selectedSplitRect;

    Value zoomScale;
    Value splitZoomScale;

private:
    // Used by standalone to handle dragging the window
    ComponentDragger windowDragger;

    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;
        
    std::unique_ptr<Palettes> palettes;

    int const toolbarHeight = ProjectInfo::isStandalone ? 40 : 35;

    TextButton mainMenuButton, undoButton, redoButton, addObjectMenuButton, pinButton, hideSidebarButton;
    TextButton editButton, runButton, presentButton;

    TooltipWindow tooltipWindow;
    StackDropShadower tooltipShadow;

    TextButton seperators[8];

    std::unique_ptr<ZoomLabel> zoomLabel;

#if JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif

    bool isMaximised = false;
    bool isDraggingFile = false;

    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
