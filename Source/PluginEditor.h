/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>

#include "Dialogs/Dialogs.h"
#include "Sidebar/Sidebar.h"
#include "Statusbar.h"
#include "Tabbar.h"

enum CommandIDs {
    NewProject = 1,
    OpenProject,
    SaveProject,
    SaveProjectAs,
    CloseTab,
    Undo,
    Redo,

    Lock,
    ConnectionStyle,
    ConnectionPathfind,
    ZoomIn,
    ZoomOut,
    ZoomNormal,
    Copy,
    Paste,
    Cut,
    Delete,
    Duplicate,
    Encapsulate,
    CreateConnection,
    SelectAll,
    ShowBrowser,
    Search,
    NextTab,
    PreviousTab,
    ToggleGrid,
    ClearConsole,
    NumItems
};

enum ObjectIDs
{
    NewObject = 100,
    NewComment,
    NewBang,
    NewMessage,
    NewToggle,
    NewNumbox,
    NewVerticalSlider,
    NewHorizontalSlider,
    NewVerticalRadio,
    NewHorizontalRadio,
    NewFloatAtom,
    NewSymbolAtom,
    NewListAtom,
    NewArray,
    NewGraphOnParent,
    NewCanvas,
    NewKeyboard,
    NewVUMeterObject,
    NewButton,
    NewNumboxTilde,
    NewOscilloscope,
    NewFunction,
    
    NewMetro,
    NewTimer,
    NewDelay,
    NewTimedGate,
    NewDateTime,
    NewSignalDelay,
    
    NewOsc,
    NewPhasor,
    NewSaw,
    NewSaw2,
    NewSquare,
    NewTriangle,
    NewImp,
    NewImp2,
    NewWavetable,
    NewBlOsc,
    NewBlSaw,
    NewBlSaw2,
    NewBlSquare,
    NewBlTriangle,
    NewBlImp,
    NewBlImp2,
    NewBlWavetable,
    
    NewLop,
    NewVcf,
    NewLores,
    NewSvf,
    NewBob,
    NewOnepole,
    NewReson,
    NewAllpass,
    NewComb,
    NewHip,
    
    NewDac,
    NewAdc,
    NewOut,
    
    NewMidiIn,
    NewMidiOut,
    NewNoteIn,
    NewNoteOut,
    NewCtlIn,
    NewCtlOut,
    NewPgmIn,
    NewPgmOut,
    NewSysexIn,
    NewSysexOut,
    NewMtof,
    NewFtom,
    
    NewArraySet,
    NewArrayGet,
    NewArrayDefine,
    NewArraySize,
    NewArrayMin,
    NewArrayMax,
    NewArrayRandom,
    NewArrayQuantile,
    
    NewListAppend,
    NewListPrepend,
    NewListStore,
    NewListSplit,
    NewListTrim,
    NewListLength,
    NewListFromSymbol,
    NewListToSymbol,
    
    NewAdd,
    NewSubtract,
    NewMultiply,
    NewDivide,
    NewModulo,
    NewInverseSubtract,
    NewInverseDivide,
    NewBiggerThan,
    NewSmallerThan,
    NewBiggerThanOrEqual,
    NewSmallerThanOrEqual,
    NewEquals,
    NewNotEquals,
    
    NewSignalAdd,
    NewSignalSubtract,
    NewSignalMultiply,
    NewSignalDivide,
    NewSignalModulo,
    NewSignalInverseSubtract,
    NewSignalInverseDivide,
    NewSignalBiggerThan,
    NewSignalSmallerThan,
    NewSignalBiggerThanOrEqual,
    NewSignalSmallerThanOrEqual,
    NewSignalEquals,
    NewSignalNotEquals,
    
    NumObjects
};

const std::map<ObjectIDs, String> objectNames {
    {NewObject, ""},
    {NewComment, "comment"},
    {NewBang, "bng"},
    {NewMessage, "msg"},
    {NewToggle, "tgl"},
    {NewNumbox, "nbx"},
    {NewVerticalSlider, "vsl"},
    {NewHorizontalSlider, "hsl"},
    {NewVerticalRadio, "vradio"},
    {NewHorizontalRadio, "hradio"},
    {NewFloatAtom, "floatatom"},
    {NewSymbolAtom, "symbolatom"},
    {NewListAtom, "listatom"},
    {NewArray, "array"},
    {NewGraphOnParent, "graph"},
    {NewCanvas, "cnv"},
    {NewKeyboard, "keyboard"},
    {NewVUMeterObject, "vu"},
    {NewButton, "button"},
    {NewNumboxTilde, "numbox~"},
    {NewOscilloscope, "oscope~"},
    {NewFunction, "function"},
    
    {NewMetro, "metro"},
    {NewTimer, "timer"},
    {NewDelay, "delay"},
    {NewTimedGate, "timed.gate"},
    {NewDateTime, "datetime"},
    {NewSignalDelay, "delay~"},
    
    {NewOsc, "osc~"},
    {NewPhasor, "phasor~"},
    {NewSaw, "saw~"},
    {NewSaw2, "saw2~"},
    {NewSquare, "square~"},
    {NewTriangle, "triangle~"},
    {NewImp, "imp~"},
    {NewImp2, "imp2~"},
    {NewWavetable, "wavetable~"},
    {NewBlOsc, "bl.osc~"},
    {NewBlSaw, "bl.saw~"},
    {NewBlSaw2, "bl.saw2~"},
    {NewBlSquare, "bl.square~"},
    {NewBlTriangle, "bl.triangle~"},
    {NewBlImp, "bl.imp~"},
    {NewBlImp2, "bl.imp2~"},
    {NewBlWavetable, "bl.wavetable~"},
    
    {NewLop, "lop~"},
    {NewVcf, "vcf~"},
    {NewLores, "lores~"},
    {NewSvf, "svf~"},
    {NewBob, "bob~"},
    {NewOnepole, "onepole~"},
    {NewReson, "reson~"},
    {NewAllpass, "allpass~"},
    {NewComb, "comb~"},
    {NewHip, "hip~"},
    
    {NewDac, "dac~"},
    {NewAdc, "adc~"},
    {NewOut, "out~"},
    
    {NewMidiIn, "midiin"},
    {NewMidiOut, "midiout"},
    {NewNoteIn, "notein"},
    {NewNoteOut, "noteout"},
    {NewCtlIn, "ctlin"},
    {NewCtlOut, "ctlout"},
    {NewPgmIn, "pgmin"},
    {NewPgmOut, "pgmout"},
    {NewSysexIn, "sysexin"},
    {NewSysexOut, "sysexout"},
    {NewMtof, "mtof"},
    {NewFtom, "ftom"},
    
    {NewArraySet, "array set"},
    {NewArrayGet, "array get"},
    {NewArrayDefine, "array define"},
    {NewArraySize, "array size"},
    {NewArrayMin, "array min"},
    {NewArrayMax, "array max"},
    {NewArrayRandom, "array random"},
    {NewArrayQuantile, "array quantile"},
    
    {NewListAppend, "list append"},
    {NewListPrepend, "list prepend"},
    {NewListStore, "list store"},
    {NewListSplit, "list split"},
    {NewListTrim, "list trim"},
    {NewListLength, "list length"},
    {NewListFromSymbol, "list fromsymbol"},
    {NewListToSymbol,  "list tosymbol"},
    
    {NewAdd, "+"},
    {NewSubtract, "-"},
    {NewMultiply, "*"},
    {NewDivide, "/"},
    {NewModulo, "%"},
    {NewInverseSubtract, "!-"},
    {NewInverseDivide, "!/"},

    {NewBiggerThan, ">"},
    {NewSmallerThan, "<"},
    {NewBiggerThanOrEqual, ">="},
    {NewSmallerThanOrEqual, "<="},
    {NewEquals, "=="},
    {NewNotEquals, "!="},
    
    {NewSignalAdd, "+~"},
    {NewSignalSubtract, "-~"},
    {NewSignalMultiply, "*~"},
    {NewSignalDivide, "/~"},
    {NewSignalModulo, "%~"},
    {NewSignalInverseSubtract, "!-~"},
    {NewSignalInverseDivide, "!/~"},
    {NewSignalBiggerThan, ">~"},
    {NewSignalSmallerThan, "<~"},
    {NewSignalBiggerThanOrEqual, ">=~"},
    {NewSignalSmallerThanOrEqual, "<=~"},
    {NewSignalEquals, "==~"},
    {NewSignalNotEquals, "!=~"},
};

struct ZoomLabel : public TextButton
    , public Timer {
    ZoomLabel()
    {
        setInterceptsMouseClicks(false, false);
    }

    void setZoomLevel(float value)
    {
        setButtonText(String(value * 100, 1) + "%");
        startTimer(2000);

        if (!labelAnimator.isAnimating(this)) {
            labelAnimator.fadeIn(this, 200);
        }
    }

    void timerCallback() override
    {
        labelAnimator.fadeOut(this, 200);
    }

    ComponentAnimator labelAnimator;
};

struct WelcomeButton;
class Canvas;
class PluginProcessor;
class PluginEditor : public AudioProcessorEditor
    , public Value::Listener
    , public ValueTree::Listener
    , public ApplicationCommandTarget
    , public ApplicationCommandManager
    , public Timer
    , public FileDragAndDropTarget {
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

#ifdef PLUGDATA_STANDALONE
    // For dragging parent window
    void mouseDrag(MouseEvent const& e) override;
    void mouseDown(MouseEvent const& e) override;

    ComponentDragger windowDragger;
#endif

    void newProject();
    void openProject();
    void saveProject(std::function<void()> const& nestedCallback = []() {});
    void saveProjectAs(std::function<void()> const& nestedCallback = []() {});

    void addTab(Canvas* cnv, bool deleteWhenClosed = false);

    Canvas* getCurrentCanvas();
    Canvas* getCanvas(int idx);

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

    void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, Identifier const& property) override;
    void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override;
    void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;

    void addToRecentlyOpened(File path);
        
    void timerCallback() override;

    TextButton* toolbarButton(ToolbarButtonType type)
    {
        return toolbarButtons[static_cast<int>(type)];
    }

    bool wantsRoundedCorners();

    PluginProcessor* pd;

    AffineTransform transform;

    TabComponent tabbar;
    OwnedArray<Canvas, CriticalSection> canvases;
    Sidebar sidebar;
    Statusbar statusbar;

    std::atomic<bool> canUndo = false, canRedo = false;

    std::unique_ptr<Dialog> openedDialog = nullptr;

    Value theme;
    Value zoomScale;

    Value hvccMode;
    Value autoconnect;

private:
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;

#ifdef PLUGDATA_STANDALONE
    static constexpr int toolbarHeight = 45;
#else
    static constexpr int toolbarHeight = 40;
#endif

    OwnedArray<TextButton> toolbarButtons;

    TooltipWindow tooltipWindow;
    StackDropShadower tooltipShadow;

    TextButton seperators[8];

    ZoomLabel zoomLabel;

#if PLUGDATA_STANDALONE && JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif

    bool isMaximised = false;
    bool isDraggingFile = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
