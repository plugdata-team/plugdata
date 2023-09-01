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
    inline static const String AddObject = ";";
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
    inline static const String Compass = "+";

    inline static const String Reorder = "(";
    inline static const String Object = ":";

    inline static const String Heart = ",";
    inline static const String Download = "-";

    inline static const String Copy = "0";
    inline static const String Paste = "1";
    inline static const String Duplicate = "2";
    inline static const String Cut = "3";

    inline static const String AlignLeft = "4";
    inline static const String AlignRight = "5";
    inline static const String AlignVCentre = "6";
    inline static const String AlignHDistribute = "/";
    inline static const String AlignTop = "7";
    inline static const String AlignBottom = "8";
    inline static const String AlignHCentre = "9";
    inline static const String AlignVDistribute = "*";

    // ================== OBJECT ICONS ==================

    // generic
    inline static const String GlyphGenericSignal = CharPointer_UTF8("\xc3\x80");
    inline static const String GlyphGeneric = CharPointer_UTF8("\xc3\x81");

    // default
    inline static const String GlyphEmptyObject = CharPointer_UTF8("\xc3\x82");
    inline static const String GlyphMessage = CharPointer_UTF8("\xc3\x84");
    inline static const String GlyphFloatBox = CharPointer_UTF8("\xc3\x83");
    inline static const String GlyphSymbolBox = CharPointer_UTF8("\xc3\x85");
    inline static const String GlyphListBox = CharPointer_UTF8("\xc3\x86");
    inline static const String GlyphComment = CharPointer_UTF8("\xc3\x87");

    // ui
    inline static const String GlyphBang = CharPointer_UTF8("\xc2\xa1");
    inline static const String GlyphToggle = CharPointer_UTF8("\xc2\xa2");
    inline static const String GlyphButton = CharPointer_UTF8("\xc2\xa3");
    inline static const String GlyphKnob = CharPointer_UTF8("\xc2\xa4");
    inline static const String GlyphNumber = CharPointer_UTF8("\xc2\xa5");
    inline static const String GlyphHSlider = CharPointer_UTF8("\xc2\xa8");
    inline static const String GlyphVSlider = CharPointer_UTF8("\xc2\xa9");
    inline static const String GlyphHRadio = CharPointer_UTF8("\xc2\xa6");
    inline static const String GlyphVRadio = CharPointer_UTF8("\xc2\xa7");
    inline static const String GlyphCanvas = CharPointer_UTF8("\xc2\xaa");
    inline static const String GlyphKeyboard = CharPointer_UTF8("\xc2\xab");
    inline static const String GlyphVUMeter = CharPointer_UTF8("\xc2\xac");
    inline static const String GlyphArray = CharPointer_UTF8("\xc2\xae");
    inline static const String GlyphGOP = CharPointer_UTF8("\xc2\xaf");
    inline static const String GlyphOscilloscope = CharPointer_UTF8("\xc2\xb0");
    inline static const String GlyphFunction = CharPointer_UTF8("\xc2\xb1");
    inline static const String GlyphMessbox = CharPointer_UTF8("\xc2\xb5");
    inline static const String GlyphBicoeff = CharPointer_UTF8("\xc2\xb3");

    // general
    inline static const String GlyphMetro = CharPointer_UTF8("\xc3\xa4");
    inline static const String GlyphCounter = CharPointer_UTF8("\xc3\xa6");
    inline static const String GlyphSelect = CharPointer_UTF8("\xc3\xa7");
    inline static const String GlyphRoute = CharPointer_UTF8("\xc3\xa8");
    inline static const String GlyphExpr = CharPointer_UTF8("\xc3\xb5");
    inline static const String GlyphLoadbang = CharPointer_UTF8("\xc3\xa9");
    inline static const String GlyphPack = CharPointer_UTF8("\xc3\xaa");
    inline static const String GlyphUnpack = CharPointer_UTF8("\xc3\xab");
    inline static const String GlyphPrint = CharPointer_UTF8("\xc3\xac");
    inline static const String GlyphNetsend = CharPointer_UTF8("\xc3\xae");
    inline static const String GlyphNetreceive = CharPointer_UTF8("\xc3\xad");
    inline static const String GlyphTimer = CharPointer_UTF8("\xc3\xb6");
    inline static const String GlyphDelay = CharPointer_UTF8("\xc3\xb7");
    inline static const String GlyphTrigger = CharPointer_UTF8("\xc3\xb1");
    inline static const String GlyphMoses = CharPointer_UTF8("\xc3\xb2");
    inline static const String GlyphSpigot = CharPointer_UTF8("\xc3\xb3");
    inline static const String GlyphBondo = CharPointer_UTF8("\xc3\xb4");
    inline static const String GlyphSfz = CharPointer_UTF8("\xc3\xb8");

    // MIDI
    inline static const String GlyphMidiIn = CharPointer_UTF8("\xc4\x87");
    inline static const String GlyphMidiOut = CharPointer_UTF8("\xc4\x88");
    inline static const String GlyphNoteIn = CharPointer_UTF8("\xc4\x89");
    inline static const String GlyphNoteOut = CharPointer_UTF8("\xc4\x8a");
    inline static const String GlyphCtlIn = CharPointer_UTF8("\xc4\x8b");
    inline static const String GlyphCtlOut = CharPointer_UTF8("\xc4\x8c");
    inline static const String GlyphPgmIn = CharPointer_UTF8("\xc4\x8d");
    inline static const String GlyphPgmOut = CharPointer_UTF8("\xc4\x8e");
    inline static const String GlyphSysexIn = CharPointer_UTF8("\xc4\x8f");
    inline static const String GlyphSysexOut = CharPointer_UTF8("\xc4\x90");
    inline static const String GlyphMtof = CharPointer_UTF8("\xc4\x91");
    inline static const String GlyphFtom = CharPointer_UTF8("\xc4\x92");
    inline static const String GlyphAutotune = CharPointer_UTF8("\xc4\x93");

    // Multi~
    inline static const String GlyphMultiSnake = CharPointer_UTF8("\xc4\xbf");
    inline static const String GlyphMultiGet = CharPointer_UTF8("\xc5\x82");
    inline static const String GlyphMultiPick = CharPointer_UTF8("\xc5\x81");
    inline static const String GlyphMultiSig = CharPointer_UTF8("\xc5\x83");
    inline static const String GlyphMultiMerge = CharPointer_UTF8("\xc5\x84");
    inline static const String GlyphMultiUnmerge = CharPointer_UTF8("\xc5\x85");

    // IO~
    inline static const String GlyphAdc = CharPointer_UTF8("\xc4\xaa");
    inline static const String GlyphDac = CharPointer_UTF8("\xc4\xab");
    inline static const String GlyphOut = CharPointer_UTF8("\xc4\xac");
    inline static const String GlyphBlocksize = CharPointer_UTF8("\xc4\xad");
    inline static const String GlyphSamplerate = CharPointer_UTF8("\xc4\xae");
    inline static const String GlyphSetDsp = CharPointer_UTF8("\xc4\xaf");
    inline static const String GlyphSend = CharPointer_UTF8("\xc4\xb0");
    inline static const String GlyphReceive = CharPointer_UTF8("\xc4\xb1");
    inline static const String GlyphSignalSend = CharPointer_UTF8("\xc4\xb2");
    inline static const String GlyphSignalReceive = CharPointer_UTF8("\xc4\xb3");

    // OSC~
    inline static const String GlyphOsc = CharPointer_UTF8("\xc5\x8d");
    inline static const String GlyphPhasor = CharPointer_UTF8("\xc5\x8e");
    inline static const String GlyphSaw = CharPointer_UTF8("\xc5\x8f");
    inline static const String GlyphSaw2 = CharPointer_UTF8("\xc5\x90");
    inline static const String GlyphSquare = CharPointer_UTF8("\xc5\x91");
    inline static const String GlyphTriangle = CharPointer_UTF8("\xc5\x92");
    inline static const String GlyphImp = CharPointer_UTF8("\xc5\x93");
    inline static const String GlyphImp2 = CharPointer_UTF8("\xc5\x94");
    inline static const String GlyphWavetable = CharPointer_UTF8("\xc5\x95");
    inline static const String GlyphPlaits = CharPointer_UTF8("\xc5\x96");

    inline static const String GlyphOscBL = CharPointer_UTF8("\xc5\x97");
    inline static const String GlyphSawBL = CharPointer_UTF8("\xc5\x98");
    inline static const String GlyphSawBL2 = CharPointer_UTF8("\xc5\x99");
    inline static const String GlyphSquareBL = CharPointer_UTF8("\xc5\x9a");
    inline static const String GlyphTriBL = CharPointer_UTF8("\xc5\x9b");
    inline static const String GlyphImpBL = CharPointer_UTF8("\xc5\x9c");
    inline static const String GlyphImpBL2 = CharPointer_UTF8("\xc5\x9d");
    inline static const String GlyphWavetableBL = CharPointer_UTF8("\xc5\x9e");

    // effects~
    inline static const String GlyphCrusher = CharPointer_UTF8("\xc6\x99");
    inline static const String GlyphDelayEffect = CharPointer_UTF8("\xc6\x9a");
    inline static const String GlyphDrive = CharPointer_UTF8("\xc6\x9b");
    inline static const String GlyphFlanger = CharPointer_UTF8("\xc6\x9c");
    inline static const String GlyphReverb = CharPointer_UTF8("\xc6\x9d");
    inline static const String GlyphFreeze = CharPointer_UTF8("\xc6\x9e");
    inline static const String GlyphRingmod = CharPointer_UTF8("\xc6\xa2");
    inline static const String GlyphSVFilter = CharPointer_UTF8("\xc6\xa6");
    inline static const String GlyphClip = CharPointer_UTF8("\xc6\xa3");
    inline static const String GlyphFold = CharPointer_UTF8("\xc6\xa4");
    inline static const String GlyphWrap = CharPointer_UTF8("\xc6\xa5");
    inline static const String GlyphCombRev = CharPointer_UTF8("\xc6\x9f");
    inline static const String GlyphDuck = CharPointer_UTF8("\xc6\xa0");
    inline static const String GlyphBallance = CharPointer_UTF8("\xc6\xa7");
    inline static const String GlyphPan = CharPointer_UTF8("\xc6\xa8");
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
    NewVUMeterObject,
    NumObjects,
    OtherObject
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
    { NewVUMeterObject, "vu" },
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
    OverlayActivationState,
    OverlayDirection,
    OverlayOrder
};

enum Align {
    Left = 0,
    Right,
    HCenter,
    HDistribute,
    Top,
    Bottom,
    VCenter,
    VDistribute
};
