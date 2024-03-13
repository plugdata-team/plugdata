/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_opengl/juce_opengl.h>

#include "Utility/Fonts.h"
#include "Utility/ModifierKeyListener.h"
#include "Components/CheckedTooltip.h"
#include "Components/ZoomableDragAndDropContainer.h"
#include "Utility/OfflineObjectRenderer.h"
#include "Utility/WindowDragger.h"

#include "Tabbar/SplitView.h"
#include "Dialogs/OverlayDisplaySettings.h"
#include "Dialogs/SnapSettings.h"

#include "Utility/ObjectThemeManager.h"

class ConnectionMessageDisplay;
class Sidebar;
class Statusbar;
class ZoomLabel;
class Dialog;
class Canvas;
class TabComponent;
class PluginProcessor;
class Palettes;
class Autosave;
class PluginMode;
class TouchSelectionHelper;
class PluginEditor : public AudioProcessorEditor
    , public Value::Listener
    , public ApplicationCommandTarget
    , public FileDragAndDropTarget
    , public ModifierKeyBroadcaster
    , public ModifierKeyListener
    , public ZoomableDragAndDropContainer
    , public AsyncUpdater
{
public:
    explicit PluginEditor(PluginProcessor&);

    ~PluginEditor() override;

    void paint(Graphics& g) override;
    void paintOverChildren(Graphics& g) override;

    void initialiseCanvasRenderer();
    
    bool isActiveWindow() override;

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
    void closeAllTabs(
        bool quitAfterComplete = false, Canvas* patchToExclude = nullptr, std::function<void()> afterComplete = []() {});

    void quit(bool askToSave);

    Canvas* getCurrentCanvas();

    // Part of the ZoomableDragAndDropContainer, we give it the splitview
    // so it can check if the drag image is over the entire splitview
    // otherwise some objects inside the splitview will trigger a zoom
    SplitView* getSplitView() override;

    void modifierKeysChanged(ModifierKeys const& modifiers) override;

    void valueChanged(Value& v) override;

    void updateCommandStatus();
    void handleAsyncUpdate() override;
    
    bool isInterestedInFileDrag(StringArray const& files) override;
    void filesDropped(StringArray const& files, int x, int y) override;
    void fileDragEnter(StringArray const&, int, int) override;
    void fileDragMove(StringArray const& files, int x, int y) override;
    void fileDragExit(StringArray const&) override;

    void createNewWindow(TabBarButtonComponent* tabButton) override;

    DragAndDropTarget* findNextDragAndDropTarget(Point<int> screenPos) override;

    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(Array<CommandID>& commands) override;
    void getCommandInfo(CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform(InvocationInfo const& info) override;

    bool wantsRoundedCorners();

    bool keyPressed(KeyPress const& key) override;

    void enablePluginMode(Canvas* cnv);

    void commandKeyChanged(bool isHeld) override;
    void setZoomLabelLevel(float value);
    void setUseBorderResizer(bool shouldUse);
    void showTouchSelectionHelper(bool shouldBeShown);
    
    bool highlightSearchTarget(void* target, bool openNewTabIfNeeded);


    TabComponent* getActiveTabbar();

    PluginProcessor* pd;

    std::unique_ptr<ConnectionMessageDisplay> connectionMessageDisplay;

    OwnedArray<Canvas, CriticalSection> canvases;
    std::unique_ptr<Sidebar> sidebar;
    std::unique_ptr<Statusbar> statusbar;

    std::unique_ptr<Dialog> openedDialog;

    std::unique_ptr<PluginMode> pluginMode;

    Value theme;

    Value hvccMode;
    Value autoconnect;

    SplitView splitView;
    DrawableRectangle selectedSplitRect;

    std::unique_ptr<Palettes> palettes;

    std::unique_ptr<ZoomLabel> zoomLabel;

    OfflineObjectRenderer offlineRenderer;
    
    NVGcontext* nvg = nullptr;
    Component openGLView;
    std::unique_ptr<OpenGLContext> glContext;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;

    // used to display callOutBoxes only in a safe area between top & bottom toolbars
    Component callOutSafeArea;

    ComponentBoundsConstrainer constrainer;
    ComponentBoundsConstrainer& pluginConstrainer;

    std::unique_ptr<Autosave> autosave;
    ApplicationCommandManager commandManager;
        
    inline static ObjectThemeManager objectManager;
    static ObjectThemeManager* getObjectManager() { return &objectManager; };

private:
    float pixelScale = 2.0f;
    
    std::unique_ptr<TouchSelectionHelper> touchSelectionHelper;

    // Used by standalone to handle dragging the window
    WindowDragger windowDragger;

    int const toolbarHeight = ProjectInfo::isStandalone ? 40 : 35;

    MainToolbarButton mainMenuButton, undoButton, redoButton, addObjectMenuButton, pluginModeButton;
    ToolbarRadioButton editButton, runButton, presentButton;

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
    
    struct FrameSync : public juce::Timer
    {
        FrameSync(std::function<void()> callback) : cb(callback) {
            startTimerHz(60);
        };
        
        void timerCallback() override
        {
            cb();
        }
        
        std::function<void()> cb;
    };
    
    std::unique_ptr<FrameSync> frameSync;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
