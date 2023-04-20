/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Config.h"
#include <juce_data_structures/juce_data_structures.h>

struct Icons {
    inline static const String Open = "b";
    inline static const String Save = "c";
    inline static const String SaveAs = "d";
    inline static const String Undo = "e";
    inline static const String Redo = "f";
    inline static const String Add = "g";
    inline static const String Settings = "h";
    inline static const String Hide = "i";
    inline static const String Show = "i";
    inline static const String Clear = "k";
    inline static const String ClearLarge = "l";
    inline static const String Lock = "m";
    inline static const String Unlock = "n";
    inline static const String ConnectionStyle = "o";
    inline static const String Power = "p";
    inline static const String Audio = "q";
    inline static const String Search = "r";
    inline static const String Wand = "s";
    inline static const String Pencil = "t";
    inline static const String Grid = "u";
    inline static const String Pin = "v";
    inline static const String Keyboard = "w";
    inline static const String Folder = "x";
    inline static const String OpenedFolder = "y";
    inline static const String File = "z";
    inline static const String New = "z";
    inline static const String AutoScroll = "A";
    inline static const String Restore = "B";
    inline static const String Error = "C";
    inline static const String Message = "D";
    inline static const String Parameters = "E";
    inline static const String Presentation = "F";
    inline static const String Externals = "G";
    inline static const String Refresh = "H";
    inline static const String Up = "I";
    inline static const String Down = "J";
    inline static const String Edit = "K";
    inline static const String ThinDown = "L";
    inline static const String Sine = "M";
    inline static const String Documentation = "N";
    inline static const String AddCircled = "O";
    inline static const String Console = "P";
    inline static const String GitHub = "Q";
    inline static const String Wrench = "R";
    inline static const String Back = "S";
    inline static const String Forward = "T";
    inline static const String Library = "U";
    inline static const String Menu = "V";
    inline static const String Info = "W";
    inline static const String History = "X";
    inline static const String Protection = "Y";
    inline static const String Direction = "{";

    inline static const String SavePatch = "Z";
    inline static const String ClosePatch = "[";
    inline static const String CloseAllPatches = "]";
    inline static const String Centre = "}";
    inline static const String FitAll = ">";
    inline static const String Eye = "|";
    inline static const String Magnet = "%";
    inline static const String SnapEdges = "#";
    inline static const String SnapCorners = "\"";
    inline static const String SnapCenters = "$";
    inline static const String DragCopyMode = "^";
    inline static const String Trash = "~";
    inline static const String Fullscreen = "&";
    inline static const String Eyedropper = "@";
};

enum PlugDataColour {
    toolbarBackgroundColourId,
    toolbarTextColourId,
    toolbarActiveColourId,
    toolbarHoverColourId,

    tabBackgroundColourId,
    tabTextColourId,
    activeTabBackgroundColourId,
    activeTabTextColourId,

    canvasBackgroundColourId,
    canvasTextColourId,
    canvasDotsColourId,

    guiObjectBackgroundColourId,
    textObjectBackgroundColourId,

    objectOutlineColourId,
    objectSelectedOutlineColourId,
    commentTextColourId,
    outlineColourId,

    ioletAreaColourId,
    ioletOutlineColourId,

    dataColourId,
    connectionColourId,
    signalColourId,

    dialogBackgroundColourId,

    sidebarBackgroundColourId,
    sidebarTextColourId,
    sidebarActiveBackgroundColourId,
    sidebarActiveTextColourId,

    levelMeterActiveColourId,
    levelMeterInactiveColourId,
    levelMeterTrackColourId,
    levelMeterThumbColourId,

    panelBackgroundColourId,
    panelTextColourId,
    panelActiveBackgroundColourId,
    panelActiveTextColourId,
    searchBarColourId,

    popupMenuBackgroundColourId,
    popupMenuActiveBackgroundColourId,
    popupMenuTextColourId,
    popupMenuActiveTextColourId,

    sliderThumbColourId,
    scrollbarThumbColourId,
    resizeableCornerColourId,
    gridLineColourId,
    caretColourId,

    /* iteration hack */
    numberOfColours
};

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
    ZoomToFitAll,
    GoToOrigin,
    Copy,
    Paste,
    Cut,
    Delete,
    Duplicate,
    Encapsulate,
    CreateConnection,
    RemoveConnections,
    SelectAll,
    ShowBrowser,
    ToggleSidebar,
    TogglePalettes,
    Search,
    NextTab,
    PreviousTab,
    ToggleSnapping,
    ClearConsole,
    ShowSettings,
    ShowReference,
    OpenObjectBrowser,
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
    NewPlaits,
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
    { NewPlaits, "plaits~ -model 0 440 0.25 0.5 0.1" },
    { NewBlOsc, "bl.osc~ saw 1 440" },
    { NewBlSaw, "bl.saw~ 440" },
    { NewBlSaw2, "bl.saw2~ 440" },
    { NewBlSquare, "bl.square~ 440" },
    { NewBlTriangle, "bl.tri~ 440" },
    { NewBlImp, "bl.imp~ 100" },
    { NewBlImp2, "bl.imp2~ 100" },
    { NewBlWavetable, "bl.wavetable~" },

    { NewAdsr, "adsr~ 10 800 0 0" },
    { NewAsr, "asr~ 10 400" },
    { NewCurve, "curve~" },
    { NewDecay, "decay~ 1000" },
    { NewEnvelope, "envelope~" },
    { NewEnvgen, "envgen~ 100 1 500 0" },
    { NewLfnoise, "lfnoise~ 5" },
    { NewSignalLine, "line~" },
    { NewPhasor, "phasor~ 1" },
    { NewRamp, "ramp~ 0.1 0 1 0" },
    { NewSah, "sah~ 0.5" },
    { NewSignalSlider, "slide~ 41 41" },
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

struct Corners {
    inline static float const windowCornerRadius = 7.5f;
    inline static float const defaultCornerRadius = 6.0f;
    inline static float const smallCornerRadius = 4.0f;
    inline static float objectCornerRadius = 2.75f;
};

enum ParameterType {
    tString,
    tInt,
    tFloat,
    tColour,
    tBool,
    tCombo,
    tRange,
    tFont
};

enum ParameterCategory {
    cGeneral,
    cAppearance,
    cLabel,
    cExtra
};

enum Overlay {
    None = 0,
    Origin = 1,
    Border = 2,
    Index = 4,
    Coordinate = 8,
    ActivationState = 16,
    Order = 32,
    Direction = 64
};

enum OverlayItem {
    OverlayOrigin = 0,
    OverlayBorder,
    OverlayIndex,
    // Coordinate,
    // ActivationState,
    OverlayDirection,
    OverlayOrder
};

using ObjectParameter = std::tuple<String, ParameterType, ParameterCategory, Value*, std::vector<String>>; // name, type and pointer to value, list of items only for combobox and bool

using ObjectParameters = std::vector<ObjectParameter>;                                                     // List of elements and update function
