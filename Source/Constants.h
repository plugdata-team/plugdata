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
    inline static const String SidePanel = "i";
    inline static const String ShowSidePanel = "j";
    inline static const String Clear = "k";
    inline static const String ClearText = "l";
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
    inline static const String DevTools = "{";

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
    inline static const String ExportState = "^";
    inline static const String Trash = "~";
    inline static const String Fullscreen = "&";
    inline static const String Eyedropper = "@";

    inline static const String Reset = "'";
    inline static const String More = ".";
    inline static const String MIDI = "`";
    inline static const String PluginMode = "=";

    inline static const String Reorder = "(";
    inline static const String Object = ")";
    
    inline static const String Copy = "0";
    inline static const String Paste = "1";
    inline static const String Duplicate = "2";
    inline static const String Cut = "3";

    inline static const String Heart = "4";
    inline static const String Download = "5";

    // ================== OBJECT ICONS ==================
    // default
    inline static const String GlyphEmpty = CharPointer_UTF8("Â");
    inline static const String GlyphMessage = CharPointer_UTF8("Ä");
    inline static const String GlyphFloatBox = CharPointer_UTF8("Ã");
    inline static const String GlyphSymbolBox = CharPointer_UTF8("Å");
    inline static const String GlyphListBox = CharPointer_UTF8("Æ");
    inline static const String GlyphComment = CharPointer_UTF8("Ç");

    // ui
    inline static const String GlyphBang = CharPointer_UTF8("¡");
    inline static const String GlyphToggle = CharPointer_UTF8("¢");
    inline static const String GlyphButton = CharPointer_UTF8("£");
    inline static const String GlyphKnob = CharPointer_UTF8("¤");
    inline static const String GlyphNumber = CharPointer_UTF8("¥");
    inline static const String GlyphHSlider = CharPointer_UTF8("¨");
    inline static const String GlyphVSlider = CharPointer_UTF8("©");
    inline static const String GlyphHRadio = CharPointer_UTF8("¦");
    inline static const String GlyphVRadio = CharPointer_UTF8("§");
    inline static const String GlyphCanvas = CharPointer_UTF8("ª");
    inline static const String GlyphKeyboard = CharPointer_UTF8("«");
    inline static const String GlyphVUMeter = CharPointer_UTF8("¬");
    inline static const String GlyphArray = CharPointer_UTF8("®");
    inline static const String GlyphGOP = CharPointer_UTF8("¯");
    inline static const String GlyphOscilloscope = CharPointer_UTF8("°");
    inline static const String GlyphFunction = CharPointer_UTF8("±");
    inline static const String GlyphMessbox = CharPointer_UTF8("µ"); // alternates: ² ´
    inline static const String GlyphBicoeff = CharPointer_UTF8("³");

    // general
    inline static const String GlyphMetro = CharPointer_UTF8("ä");
    inline static const String GlyphCounter = CharPointer_UTF8("æ");
    inline static const String GlyphSelect = CharPointer_UTF8("ç");
    inline static const String GlyphRoute = CharPointer_UTF8("è");
    inline static const String GlyphExpr = CharPointer_UTF8("å");
    inline static const String GlyphLoadbang = CharPointer_UTF8("é");
    inline static const String GlyphPack = CharPointer_UTF8("ê");
    inline static const String GlyphUnpack = CharPointer_UTF8("ë");
    inline static const String GlyphPrint = CharPointer_UTF8("ì");
    inline static const String GlyphNetsend = CharPointer_UTF8("î");
    inline static const String GlyphNetreceive = CharPointer_UTF8("í");
    inline static const String GlyphTimer = CharPointer_UTF8("ï");
    inline static const String GlyphDelay = CharPointer_UTF8("ð");

    // MIDI
    inline static const String GlyphMidiIn = CharPointer_UTF8("ć");
    inline static const String GlyphMidiOut = CharPointer_UTF8("Ĉ");
    inline static const String GlyphNoteIn = CharPointer_UTF8("ĉ");
    inline static const String GlyphNoteOut = CharPointer_UTF8("Ċ");
    inline static const String GlyphCtlIn = CharPointer_UTF8("ċ");
    inline static const String GlyphCtlOut = CharPointer_UTF8("Č");
    inline static const String GlyphPgmIn = CharPointer_UTF8("č");
    inline static const String GlyphPgmOut = CharPointer_UTF8("Ď");
    inline static const String GlyphSysexIn = CharPointer_UTF8("ď");
    inline static const String GlyphSysexOut = CharPointer_UTF8("Đ");
    inline static const String GlyphMtof = CharPointer_UTF8("đ");
    inline static const String GlyphFtom = CharPointer_UTF8("Ē");

    // IO~
    inline static const String GlyphAdc = CharPointer_UTF8("Ī");
    inline static const String GlyphDac = CharPointer_UTF8("ī");
    inline static const String GlyphOut = CharPointer_UTF8("Ĭ");
    inline static const String GlyphBlocksize = CharPointer_UTF8("ĭ");
    inline static const String GlyphSamplerate = CharPointer_UTF8("Į");
    inline static const String GlyphSetDsp = CharPointer_UTF8("į");

    // OSC~
    inline static const String GlyphOsc = CharPointer_UTF8("ō");
    inline static const String GlyphPhasor = CharPointer_UTF8("Ŏ");
    inline static const String GlyphSaw = CharPointer_UTF8("ŏ");
    inline static const String GlyphSaw2 = CharPointer_UTF8("Ő");
    inline static const String GlyphSquare = CharPointer_UTF8("ő");
    inline static const String GlyphTriangle = CharPointer_UTF8("Œ");
    inline static const String GlyphImp = CharPointer_UTF8("œ");
    inline static const String GlyphImp2 = CharPointer_UTF8("Ŕ");
    inline static const String GlyphWavetable = CharPointer_UTF8("ŕ");
    inline static const String GlyphPlaits = CharPointer_UTF8("Ŗ");

    inline static const String GlyphOscBL = CharPointer_UTF8("ŗ");
    inline static const String GlyphSawBL = CharPointer_UTF8("Ř");
    inline static const String GlyphSawBL2 = CharPointer_UTF8("ř");
    inline static const String GlyphSquareBL = CharPointer_UTF8("Ś");
    inline static const String GlyphTriBL = CharPointer_UTF8("ś");
    inline static const String GlyphImpBL = CharPointer_UTF8("Ŝ");
    inline static const String GlyphImpBL2 = CharPointer_UTF8("ŝ");
    inline static const String GlyphWavetableBL = CharPointer_UTF8("Ş");

    // effects~
    inline static const String GlyphCrusher = CharPointer_UTF8("ƙ");
    inline static const String GlyphDelayEffect = CharPointer_UTF8("ƚ");
    inline static const String GlyphDrive = CharPointer_UTF8("ƛ");
    inline static const String GlyphFlanger = CharPointer_UTF8("Ɯ");
    inline static const String GlyphReverb = CharPointer_UTF8("Ɲ");
    inline static const String GlyphFreeze = CharPointer_UTF8("ƞ");
    inline static const String GlyphRingmod = CharPointer_UTF8("Ƣ");
};

enum PlugDataColour {
    toolbarBackgroundColourId,
    toolbarTextColourId,
    toolbarActiveColourId,
    toolbarHoverColourId,
    toolbarOutlineColourId,

    tabBackgroundColourId,
    tabTextColourId,
    activeTabBackgroundColourId,
    activeTabTextColourId,

    canvasBackgroundColourId,
    canvasTextColourId,
    canvasDotsColourId,

    guiObjectBackgroundColourId,
    guiObjectInternalOutlineColour,
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
    levelMeterBackgroundColourId,
    levelMeterThumbColourId,

    panelBackgroundColourId,
    panelForegroundColourId,
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
    NewKnob,
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
    { NewKnob, "knob" },
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
    inline static float const windowCornerRadius = 12.5f;
    inline static float const largeCornerRadius = 8.0f;
    inline static float const defaultCornerRadius = 5.0f;
    inline static float objectCornerRadius = 2.75f;
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
