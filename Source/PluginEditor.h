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
#include "SplitView.h"
#include "Utility/RateReducer.h"
#include "Utility/ModifierKeyListener.h"

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
    ShowSettings,
    ShowReference,
    NumItems
};

enum ObjectIDs {
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
    NewMessbox,
    NewBicoeff,
    NumEssentialObjects,

    NewMetro,
    NewCounter,
    NewSel,
    NewRoute,
    NewExpr,
    NewLoadbang,
    NewPack,
    NewUnpack,
    NewPrint,
    NewNetsend,
    NewNetreceive,
    NewTimer,
    NewDelay,
    NewTimedGate,

    NewCrusher,
    NewDrive,
    NewFlanger,
    NewReverb,
    NewFreeze,
    NewFreqShift,
    NewPhaser,
    NewShaper,
    NewRm,
    NewTremolo,
    NewVibrato,
    NewVocoder,
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

    NewAdsr,
    NewAsr,
    NewCurve,
    NewDecay,
    NewEnvelope,
    NewEnvgen,
    NewLfnoise,
    NewSignalLine,
    NewRamp,
    NewSah,
    NewSignalSlider,
    NewVline,

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
    NewBlocksize,
    NewSamplerate,
    NewSetdsp,

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
    { NewObject, "" },
    { NewComment, "comment" },
    { NewBang, "bng" },
    { NewMessage, "msg" },
    { NewToggle, "tgl" },
    { NewNumbox, "nbx" },
    { NewVerticalSlider, "vsl" },
    { NewHorizontalSlider, "hsl" },
    { NewVerticalRadio, "vradio" },
    { NewHorizontalRadio, "hradio" },
    { NewFloatAtom, "floatatom" },
    { NewSymbolAtom, "symbolatom" },
    { NewListAtom, "listbox" },
    { NewArray, "array" },
    { NewGraphOnParent, "graph" },
    { NewCanvas, "cnv" },
    { NewKeyboard, "keyboard" },
    { NewVUMeterObject, "vu" },
    { NewButton, "button" },
    { NewNumboxTilde, "numbox~" },
    { NewOscilloscope, "oscope~" },
    { NewFunction, "function" },
    { NewMessbox, "messbox" },
    { NewBicoeff, "bicoeff" },

    { NewMetro, "metro 500" },
    { NewCounter, "counter 0 5" },
    { NewSel, "sel" },
    { NewRoute, "route" },
    { NewExpr, "expr" },
    { NewLoadbang, "loadbang" },
    { NewPack, "pack" },
    { NewUnpack, "unpack" },
    { NewPrint, "print" },
    { NewNetsend, "netsend" },
    { NewNetreceive, "netreceive" },
    { NewTimer, "timer" },
    { NewDelay, "delay 1 60 permin" },
    { NewTimedGate, "timed.gate" },

    { NewCrusher, "crusher~ 0.1 0.1" },
    { NewSignalDelay, "delay~ 22050 14700" },
    { NewDrive, "drive~" },
    { NewFlanger, "flanger~ 0.1 20 -0.6" },
    { NewReverb, "free.rev~ 0.7 0.6 0.5 0.7" },
    { NewFreeze, "freeze~" },
    { NewFreqShift, "freq.shift~ 200" },
    { NewPhaser, "phaser~ 6 2 0.7" },
    { NewShaper, "shaper~" },
    { NewRm, "rm~ 150" },
    { NewTremolo, "tremolo~ 2 0.5" },
    { NewVibrato, "vibrato~ 6 30" },
    { NewVocoder, "vocoder~" },

    { NewOsc, "osc~ 440" },
    { NewPhasor, "phasor~" },
    { NewSaw, "saw~ 440" },
    { NewSaw2, "saw2~ 440" },
    { NewSquare, "square~ 440" },
    { NewTriangle, "triangle~ 440" },
    { NewImp, "imp~ 100" },
    { NewImp2, "imp2~ 100" },
    { NewWavetable, "wavetable~" },
    { NewBlOsc, "bl.osc~ saw 1 440" },
    { NewBlSaw, "bl.saw~ 440" },
    { NewBlSaw2, "bl.saw2~ 440" },
    { NewBlSquare, "bl.square~ 440" },
    { NewBlTriangle, "bl.tri~ 440" },
    { NewBlImp, "bl.imp~ 100" },
    { NewBlImp2, "bl.imp2~ 100" },
    { NewBlWavetable, "bl.wavetable~" },

    { NewAdsr, "adsr~ -log 10 800 0 0" },
    { NewAsr, "asr~ -log 10 400" },
    { NewCurve, "curve~" },
    { NewDecay, "decay~ 1000" },
    { NewEnvelope, "envelope~" },
    { NewEnvgen, "envgen~ 100 1 500 0" },
    { NewLfnoise, "lfnoise~ 5" },
    { NewSignalLine, "line~" },
    { NewPhasor, "phasor~ 1" },
    { NewRamp, "ramp~ 0.1 0 1 0" },
    { NewSah, "sah~ 0.5" },
    { NewSignalSlider, "slider~" },
    { NewVline, "vline~" },

    { NewLop, "lop~ 100" },
    { NewVcf, "vcf~" },
    { NewLores, "lores~ 800 0.1" },
    { NewSvf, "svf~ 800 0.1" },
    { NewBob, "bob~" },
    { NewOnepole, "onepole~ 800" },
    { NewReson, "reson~ 1 800 23" },
    { NewAllpass, "allpass~ 100 60 0.7" },
    { NewComb, "comb~ 500 12 0.5 1 0.5" },
    { NewHip, "hip~ 800" },

    { NewDac, "dac~" },
    { NewAdc, "adc~" },
    { NewOut, "out~" },
    { NewBlocksize, "blocksize~" },
    { NewSamplerate, "samplerate~" },
    { NewSetdsp, "setdsp~" },

    { NewMidiIn, "midiin" },
    { NewMidiOut, "midiout" },
    { NewNoteIn, "notein" },
    { NewNoteOut, "noteout" },
    { NewCtlIn, "ctlin" },
    { NewCtlOut, "ctlout" },
    { NewPgmIn, "pgmin" },
    { NewPgmOut, "pgmout" },
    { NewSysexIn, "sysexin" },
    { NewSysexOut, "sysexout" },
    { NewMtof, "mtof" },
    { NewFtom, "ftom" },

    { NewArraySet, "array set" },
    { NewArrayGet, "array get" },
    { NewArrayDefine, "array define" },
    { NewArraySize, "array size" },
    { NewArrayMin, "array min" },
    { NewArrayMax, "array max" },
    { NewArrayRandom, "array random" },
    { NewArrayQuantile, "array quantile" },

    { NewListAppend, "list append" },
    { NewListPrepend, "list prepend" },
    { NewListStore, "list store" },
    { NewListSplit, "list split" },
    { NewListTrim, "list trim" },
    { NewListLength, "list length" },
    { NewListFromSymbol, "list fromsymbol" },
    { NewListToSymbol, "list tosymbol" },

    { NewAdd, "+" },
    { NewSubtract, "-" },
    { NewMultiply, "*" },
    { NewDivide, "/" },
    { NewModulo, "%" },
    { NewInverseSubtract, "!-" },
    { NewInverseDivide, "!/" },

    { NewBiggerThan, ">" },
    { NewSmallerThan, "<" },
    { NewBiggerThanOrEqual, ">=" },
    { NewSmallerThanOrEqual, "<=" },
    { NewEquals, "==" },
    { NewNotEquals, "!=" },

    { NewSignalAdd, "+~" },
    { NewSignalSubtract, "-~" },
    { NewSignalMultiply, "*~" },
    { NewSignalDivide, "/~" },
    { NewSignalModulo, "%~" },
    { NewSignalInverseSubtract, "!-~" },
    { NewSignalInverseDivide, "!/~" },
    { NewSignalBiggerThan, ">~" },
    { NewSignalSmallerThan, "<~" },
    { NewSignalBiggerThanOrEqual, ">=~" },
    { NewSignalSmallerThanOrEqual, "<=~" },
    { NewSignalEquals, "==~" },
    { NewSignalNotEquals, "!=~" },
};

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

class WelcomeButton;
class Canvas;
class TabComponent;
class PluginProcessor;
class PluginEditor : public AudioProcessorEditor
    , public Value::Listener
    , public ApplicationCommandTarget
    , public ApplicationCommandManager
    , public FileDragAndDropTarget
    , public ModifierKeyBroadcaster
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

    void addTab(Canvas* cnv);
    void closeTab(Canvas* cnv);
    
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
    
    void updateSplitOutline();
    TabComponent* getActiveTabbar();

    PluginProcessor* pd;

    OwnedArray<Canvas, CriticalSection> canvases;
    Sidebar sidebar;
    Statusbar statusbar;

    std::atomic<bool> canUndo = false, canRedo = false;

    std::unique_ptr<Dialog> openedDialog = nullptr;

    Value theme;

    Value hvccMode;
    Value autoconnect;

    SplitView splitView;
    DrawableRectangle selectedSplitRect;
    
    Value zoomScale;
    Value splitZoomScale;
    
private:
    
    std::unique_ptr<FileChooser> saveChooser;
    std::unique_ptr<FileChooser> openChooser;
    
#ifdef PLUGDATA_STANDALONE
    static constexpr int toolbarHeight = 40;
#else
    static constexpr int toolbarHeight = 35;
#endif

    TextButton mainMenuButton, undoButton, redoButton, addObjectMenuButton, pinButton, hideSidebarButton;

    TooltipWindow tooltipWindow;
    StackDropShadower tooltipShadow;

    TextButton seperators[8];

    ZoomLabel zoomLabel;

#if PLUGDATA_STANDALONE && JUCE_MAC
    Rectangle<int> unmaximisedSize;
#endif

    bool isMaximised = false;
    bool isDraggingFile = false;

#if !PLUGDATA_STANDALONE
    std::unique_ptr<MouseRateReducedComponent<ResizableCornerComponent>> cornerResizer;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
