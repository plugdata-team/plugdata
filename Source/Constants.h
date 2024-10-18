/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Config.h"
#include <juce_data_structures/juce_data_structures.h>

struct Icons {
    inline static String const Open = "b";
    inline static String const Save = "c";
    inline static String const SaveAs = "d";
    inline static String const Undo = "e";
    inline static String const Redo = "f";
    inline static String const Add = "g";
    inline static String const AddObject = ";";
    inline static String const Settings = "h";
    inline static String const Sparkle = "i";
    inline static String const CPU = "j";
    inline static String const Clear = "k";
    inline static String const ClearText = "l";
    inline static String const Lock = "m";
    inline static String const Unlock = "n";
    inline static String const ConnectionStyle = "o";
    inline static String const Power = "p";
    inline static String const Audio = "q";
    inline static String const Search = "r";
    inline static String const Wand = "s";
    inline static String const Pencil = "t";
    inline static String const Grid = "u";
    inline static String const Pin = "v";
    inline static String const Keyboard = "w";
    inline static String const Folder = "x";
    inline static String const OpenedFolder = "y";
    inline static String const File = "z";
    inline static String const New = "z";
    inline static String const AutoScroll = "A";
    inline static String const Restore = "B";
    inline static String const Error = "C";
    inline static String const Message = "D";
    inline static String const Parameters = "E";
    inline static String const Presentation = "F";
    inline static String const Externals = "G";
    inline static String const Refresh = "H";
    inline static String const Up = "I";
    inline static String const Down = "J";
    inline static String const Edit = "K";
    inline static String const ThinDown = "L";
    inline static String const Sine = "M";
    inline static String const Documentation = "N";
    inline static String const AddCircled = "O";
    inline static String const Console = "P";
    inline static String const OpenLink = "Q";
    inline static String const Wrench = "R";
    inline static String const Back = "S";
    inline static String const Forward = "T";
    inline static String const Library = "U";
    inline static String const Menu = "V";
    inline static String const Info = "W";
    inline static String const Warning = "\"";
    inline static String const History = "X";
    inline static String const Protection = "Y";
    inline static String const DevTools = "{";
    inline static String const Help = "\\";
    inline static String const Checkmark = "_";

    inline static String const SavePatch = "Z";
    inline static String const ClosePatch = "[";
    inline static String const CloseAllPatches = "]";
    inline static String const Centre = "}";
    inline static String const FitAll = ">";
    inline static String const Eye = "|";
    inline static String const Magnet = "%";
    inline static String const SnapEdges = "#";
    inline static String const SnapCenters = "$";
    inline static String const ExportState = "^";
    inline static String const Trash = "~";
    inline static String const CanvasSettings = "&";
    inline static String const Eyedropper = "@";
    inline static String const HeartFilled = "?";
    inline static String const HeartStroked = ">";

    inline static String const Reset = "'";
    inline static String const More = ".";
    inline static String const MIDI = "`";
    inline static String const PluginMode = "=";
    inline static String const Compass = "+";

    inline static String const Reorder = "(";
    inline static String const Object = ":";

    inline static String const List = "!";
    inline static String const Graph = "<";

    inline static String const Heart = ",";
    inline static String const Download = "-";

    inline static String const Copy = "0";
    inline static String const Paste = "1";
    inline static String const Duplicate = "2";
    inline static String const Cut = "3";

    inline static String const AlignLeft = "4";
    inline static String const AlignRight = "5";
    inline static String const AlignHCentre = "6";
    inline static String const AlignHDistribute = "/";
    inline static String const AlignTop = "7";
    inline static String const AlignBottom = "8";
    inline static String const AlignVCentre = "9";
    inline static String const AlignVDistribute = "*";

    // ================== OBJECT ICONS ==================

    // generic
    inline static String const GlyphGenericSignal = CharPointer_UTF8("\xc3\x80");
    inline static String const GlyphGeneric = CharPointer_UTF8("\xc3\x81");

    // default
    inline static String const GlyphEmptyObject = CharPointer_UTF8("\xc3\x82");
    inline static String const GlyphMessage = CharPointer_UTF8("\xc3\x84");
    inline static String const GlyphFloatBox = CharPointer_UTF8("\xc3\x83");
    inline static String const GlyphSymbolBox = CharPointer_UTF8("\xc3\x85");
    inline static String const GlyphListBox = CharPointer_UTF8("\xc3\x86");
    inline static String const GlyphComment = CharPointer_UTF8("\xc3\x87");

    // ui
    inline static String const GlyphBang = CharPointer_UTF8("\xc2\xa1");
    inline static String const GlyphToggle = CharPointer_UTF8("\xc2\xa2");
    inline static String const GlyphButton = CharPointer_UTF8("\xc2\xa3");
    inline static String const GlyphKnob = CharPointer_UTF8("\xc2\xa4");
    inline static String const GlyphNumber = CharPointer_UTF8("\xc2\xa5");
    inline static String const GlyphHSlider = CharPointer_UTF8("\xc2\xa8");
    inline static String const GlyphVSlider = CharPointer_UTF8("\xc2\xa9");
    inline static String const GlyphHRadio = CharPointer_UTF8("\xc2\xa6");
    inline static String const GlyphVRadio = CharPointer_UTF8("\xc2\xa7");
    inline static String const GlyphCanvas = CharPointer_UTF8("\xc2\xaa");
    inline static String const GlyphKeyboard = CharPointer_UTF8("\xc2\xab");
    inline static String const GlyphVUMeter = CharPointer_UTF8("\xc2\xac");
    inline static String const GlyphArray = CharPointer_UTF8("\xc2\xae");
    inline static String const GlyphGOP = CharPointer_UTF8("\xc2\xaf");
    inline static String const GlyphOscilloscope = CharPointer_UTF8("\xc2\xb0");
    inline static String const GlyphFunction = CharPointer_UTF8("\xc2\xb1");
    inline static String const GlyphMessbox = CharPointer_UTF8("\xc2\xb5");
    inline static String const GlyphBicoeff = CharPointer_UTF8("\xc2\xb3");

    // general
    inline static String const GlyphMetro = CharPointer_UTF8("\xc3\xa4");
    inline static String const GlyphCounter = CharPointer_UTF8("\xc3\xa6");
    inline static String const GlyphSelect = CharPointer_UTF8("\xc3\xa7");
    inline static String const GlyphRoute = CharPointer_UTF8("\xc3\xa8");
    inline static String const GlyphExpr = CharPointer_UTF8("\xc3\xb5");
    inline static String const GlyphLoadbang = CharPointer_UTF8("\xc3\xa9");
    inline static String const GlyphPack = CharPointer_UTF8("\xc3\xaa");
    inline static String const GlyphUnpack = CharPointer_UTF8("\xc3\xab");
    inline static String const GlyphPrint = CharPointer_UTF8("\xc3\xac");
    inline static String const GlyphNetsend = CharPointer_UTF8("\xc3\xae");
    inline static String const GlyphNetreceive = CharPointer_UTF8("\xc3\xad");
    inline static String const GlyphOSCsend = CharPointer_UTF8("\xc4\xb5");
    inline static String const GlyphOSCreceive = CharPointer_UTF8("\xc4\xb4");
    inline static String const GlyphTimer = CharPointer_UTF8("\xc3\xb6");
    inline static String const GlyphDelay = CharPointer_UTF8("\xc3\xb7");
    inline static String const GlyphTrigger = CharPointer_UTF8("\xc3\xb1");
    inline static String const GlyphMoses = CharPointer_UTF8("\xc3\xb2");
    inline static String const GlyphSpigot = CharPointer_UTF8("\xc3\xb3");
    inline static String const GlyphBondo = CharPointer_UTF8("\xc3\xb4");
    inline static String const GlyphSfz = CharPointer_UTF8("\xc3\xb8");

    // MIDI
    inline static String const GlyphMidiIn = CharPointer_UTF8("\xc4\x87");
    inline static String const GlyphMidiOut = CharPointer_UTF8("\xc4\x88");
    inline static String const GlyphNoteIn = CharPointer_UTF8("\xc4\x89");
    inline static String const GlyphNoteOut = CharPointer_UTF8("\xc4\x8a");
    inline static String const GlyphCtlIn = CharPointer_UTF8("\xc4\x8b");
    inline static String const GlyphCtlOut = CharPointer_UTF8("\xc4\x8c");
    inline static String const GlyphPgmIn = CharPointer_UTF8("\xc4\x8d");
    inline static String const GlyphPgmOut = CharPointer_UTF8("\xc4\x8e");
    inline static String const GlyphSysexIn = CharPointer_UTF8("\xc4\x8f");
    inline static String const GlyphSysexOut = CharPointer_UTF8("\xc4\x90");
    inline static String const GlyphMtof = CharPointer_UTF8("\xc4\x91");
    inline static String const GlyphFtom = CharPointer_UTF8("\xc4\x92");
    inline static String const GlyphAutotune = CharPointer_UTF8("\xc4\x93");

    // Multi~
    inline static String const GlyphMultiSnake = CharPointer_UTF8("\xc4\xbf");
    inline static String const GlyphMultiGet = CharPointer_UTF8("\xc5\x82");
    inline static String const GlyphMultiPick = CharPointer_UTF8("\xc5\x81");
    inline static String const GlyphMultiSig = CharPointer_UTF8("\xc5\x83");
    inline static String const GlyphMultiMerge = CharPointer_UTF8("\xc5\x84");
    inline static String const GlyphMultiUnmerge = CharPointer_UTF8("\xc5\x85");

    // IO~
    inline static String const GlyphAdc = CharPointer_UTF8("\xc4\xaa");
    inline static String const GlyphDac = CharPointer_UTF8("\xc4\xab");
    inline static String const GlyphOut = CharPointer_UTF8("\xc4\xac");
    inline static String const GlyphBlocksize = CharPointer_UTF8("\xc4\xad");
    inline static String const GlyphSamplerate = CharPointer_UTF8("\xc4\xae");
    inline static String const GlyphSetDsp = CharPointer_UTF8("\xc4\xaf");
    inline static String const GlyphSend = CharPointer_UTF8("\xc4\xb0");
    inline static String const GlyphReceive = CharPointer_UTF8("\xc4\xb1");
    inline static String const GlyphSignalSend = CharPointer_UTF8("\xc4\xb2");
    inline static String const GlyphSignalReceive = CharPointer_UTF8("\xc4\xb3");

    // OSC~
    inline static String const GlyphOsc = CharPointer_UTF8("\xc5\x8d");
    inline static String const GlyphPhasor = CharPointer_UTF8("\xc5\x8e");
    inline static String const GlyphSaw = CharPointer_UTF8("\xc5\x8f");
    inline static String const GlyphSaw2 = CharPointer_UTF8("\xc5\x90");
    inline static String const GlyphSquare = CharPointer_UTF8("\xc5\x91");
    inline static String const GlyphTriangle = CharPointer_UTF8("\xc5\x92");
    inline static String const GlyphImp = CharPointer_UTF8("\xc5\x93");
    inline static String const GlyphImp2 = CharPointer_UTF8("\xc5\x94");
    inline static String const GlyphWavetable = CharPointer_UTF8("\xc5\x95");
    inline static String const GlyphPlaits = CharPointer_UTF8("\xc5\x96");

    inline static String const GlyphOscBL = CharPointer_UTF8("\xc5\x97");
    inline static String const GlyphSawBL = CharPointer_UTF8("\xc5\x98");
    inline static String const GlyphSawBL2 = CharPointer_UTF8("\xc5\x99");
    inline static String const GlyphSquareBL = CharPointer_UTF8("\xc5\x9a");
    inline static String const GlyphTriBL = CharPointer_UTF8("\xc5\x9b");
    inline static String const GlyphImpBL = CharPointer_UTF8("\xc5\x9c");
    inline static String const GlyphImpBL2 = CharPointer_UTF8("\xc5\x9d");
    inline static String const GlyphWavetableBL = CharPointer_UTF8("\xc5\x9e");

    // effects~
    inline static String const GlyphCrusher = CharPointer_UTF8("\xc6\x99");
    inline static String const GlyphDelayEffect = CharPointer_UTF8("\xc6\x9a");
    inline static String const GlyphDrive = CharPointer_UTF8("\xc6\x9b");
    inline static String const GlyphFlanger = CharPointer_UTF8("\xc6\x9c");
    inline static String const GlyphReverb = CharPointer_UTF8("\xc6\x9d");
    inline static String const GlyphFreeze = CharPointer_UTF8("\xc6\x9e");
    inline static String const GlyphRingmod = CharPointer_UTF8("\xc6\xa2");
    inline static String const GlyphSVFilter = CharPointer_UTF8("\xc6\xa6");
    inline static String const GlyphClip = CharPointer_UTF8("\xc6\xa3");
    inline static String const GlyphFold = CharPointer_UTF8("\xc6\xa4");
    inline static String const GlyphWrap = CharPointer_UTF8("\xc6\xa5");
    inline static String const GlyphCombRev = CharPointer_UTF8("\xc6\x9f");
    inline static String const GlyphDuck = CharPointer_UTF8("\xc6\xa0");
    inline static String const GlyphBallance = CharPointer_UTF8("\xc6\xa7");
    inline static String const GlyphPan = CharPointer_UTF8("\xc6\xa8");
};

enum PlugDataColour {
    toolbarBackgroundColourId,
    toolbarTextColourId,
    toolbarActiveColourId,
    toolbarHoverColourId,
    toolbarOutlineColourId,
    activeTabBackgroundColourId,

    canvasBackgroundColourId,
    canvasTextColourId,
    canvasDotsColourId,

    presentationBackgroundColourId,

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
    gemColourId,

    dialogBackgroundColourId,

    sidebarBackgroundColourId,
    sidebarTextColourId,
    sidebarActiveBackgroundColourId,

    levelMeterActiveColourId,
    levelMeterBackgroundColourId,
    levelMeterThumbColourId,

    panelBackgroundColourId,
    panelForegroundColourId,
    panelTextColourId,
    panelActiveBackgroundColourId,

    popupMenuBackgroundColourId,
    popupMenuActiveBackgroundColourId,
    popupMenuTextColourId,

    scrollbarThumbColourId,
    graphAreaColourId,
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
    PanDragKey,
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
    Triggerize,
    Tidy,
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
    ShowHelp,
    OpenObjectBrowser,
    ToggleDSP,
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
    NewVUMeter,
    NumObjects,
    OtherObject
};

std::map<ObjectIDs, String> const objectNames {
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
    { NewGraphOnParent, "graph" },
    { NewCanvas, "cnv" },
    { NewVUMeter, "vu" },
    { NewArray, "garray" },
};

struct Corners {
    inline static float const windowCornerRadius = 12.0f;
    inline static float const largeCornerRadius = 8.0f;
    inline static float const defaultCornerRadius = 5.0f;
    inline static float const resizeHanleCornerRadius = 2.75f;
    inline static float objectCornerRadius = 2.75f;
};

enum Overlay {
    None = 0,
    Origin = 1 << 0,
    Border = 1 << 1,
    Index = 1 << 2,
    Coordinate = 1 << 3,
    ActivationState = 1 << 4,
    ConnectionActivity = 1 << 5,
    Order = 1 << 6,
    Direction = 1 << 7,
    Behind = 1 << 8
};

enum Align {
    Left = 0,
    Right,
    HCentre,
    HDistribute,
    Top,
    Bottom,
    VCentre,
    VDistribute
};
