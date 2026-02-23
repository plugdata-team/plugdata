/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Utility/Config.h"
#include <juce_data_structures/juce_data_structures.h>

struct Icons {
    static inline String const Open = "b";
    static inline String const Save = "c";
    static inline String const SaveAs = "d";
    static inline String const Undo = "e";
    static inline String const Redo = "f";
    static inline String const Add = "g";
    static inline String const AddObject = ";";
    static inline String const Settings = "h";
    static inline String const Sparkle = "i";
    static inline String const CPU = "j";
    static inline String const Clear = "k";
    static inline String const ClearText = "l";
    static inline String const Lock = "m";
    static inline String const Unlock = "n";
    static inline String const ConnectionStyle = "o";
    static inline String const Power = "p";
    static inline String const Audio = "q";
    static inline String const Search = "r";
    static inline String const Wand = "s";
    static inline String const Pencil = "t";
    static inline String const Grid = "u";
    static inline String const Pin = "v";
    static inline String const Keyboard = "w";
    static inline String const Folder = "x";
    static inline String const OpenedFolder = "y";
    static inline String const File = "z";
    static inline String const New = "z";
    static inline String const AutoScroll = "A";
    static inline String const Restore = "B";
    static inline String const Error = "C";
    static inline String const Message = "D";
    static inline String const Parameters = "E";
    static inline String const Presentation = "F";
    static inline String const Externals = "G";
    static inline String const Refresh = "H";
    static inline String const Up = "I";
    static inline String const Down = "J";
    static inline String const Edit = "K";
    static inline String const ThinDown = "L";
    static inline String const Sine = "M";
    static inline String const Documentation = "N";
    static inline String const AddCircled = "O";
    static inline String const Console = "P";
    static inline String const OpenLink = "Q";
    static inline String const Wrench = "R";
    static inline String const Back = "S";
    static inline String const Forward = "T";
    static inline String const Library = "U";
    static inline String const Menu = "V";
    static inline String const Info = "W";
    static inline String const Warning = "\"";
    static inline String const History = "X";
    static inline String const Protection = "Y";
    static inline String const DevTools = "{";
    static inline String const Help = "\\";
    static inline String const Checkmark = "_";

    static inline String const SavePatch = "Z";
    static inline String const ClosePatch = "[";
    static inline String const CloseAllPatches = "]";
    static inline String const Centre = "}";
    static inline String const FitAll = ">";
    static inline String const Eye = "|";
    static inline String const Magnet = "%";
    static inline String const SnapEdges = "#";
    static inline String const SnapCenters = "$";
    static inline String const ExportState = "^";
    static inline String const Trash = "~";
    static inline String const CanvasSettings = "&";
    static inline String const Eyedropper = "@";
    static inline String const HeartFilled = "?";
    static inline String const HeartStroked = ">";

    static inline String const Reset = "'";
    static inline String const More = ".";
    static inline String const MIDI = "`";
    static inline String const PluginMode = "=";
    static inline String const CommandInput = "+";

    static inline String const Reorder = "(";
    static inline String const Object = ":";
    static inline String const ObjectMulti = CharPointer_UTF8("\xc2\xb9");

    static inline String const List = "!";
    static inline String const Graph = "<";

    static inline String const Heart = ",";
    static inline String const Download = "-";

    static inline String const Copy = "0";
    static inline String const Paste = "1";
    static inline String const Duplicate = "2";
    static inline String const Cut = "3";

    static inline String const Storage = CharPointer_UTF8("\xc3\x90");
    static inline String const Money = CharPointer_UTF8("\xc3\x91");
    static inline String const Time = CharPointer_UTF8("\xc3\x92");
    static inline String const Store = CharPointer_UTF8("\xc3\x8f");
    static inline String const PanelExpand = CharPointer_UTF8("\xc3\x8d");
    static inline String const PanelContract = CharPointer_UTF8("\xc3\x8c");
    static inline String const ItemGrid = " ";

    static inline String const AlignLeft = "4";
    static inline String const AlignRight = "5";
    static inline String const AlignHCentre = "6";
    static inline String const AlignHDistribute = "/";
    static inline String const AlignTop = "7";
    static inline String const AlignBottom = "8";
    static inline String const AlignVCentre = "9";
    static inline String const AlignVDistribute = "*";

    static inline String const Home = CharPointer_UTF8("\xc3\x8e");

    static inline String const ShowIndex = CharPointer_UTF8("\xc2\xbA");
    static inline String const ShowXY = CharPointer_UTF8("\xc2\xbb");

    // ================== OBJECT ICONS ==================

    // generic
    static inline String const GlyphGenericSignal = CharPointer_UTF8("\xc3\x80");
    static inline String const GlyphGeneric = CharPointer_UTF8("\xc3\x81");

    // default
    static inline String const GlyphEmptyObject = CharPointer_UTF8("\xc3\x82");
    static inline String const GlyphMessage = CharPointer_UTF8("\xc3\x84");
    static inline String const GlyphFloatBox = CharPointer_UTF8("\xc3\x83");
    static inline String const GlyphSymbolBox = CharPointer_UTF8("\xc3\x85");
    static inline String const GlyphListBox = CharPointer_UTF8("\xc3\x86");
    static inline String const GlyphComment = CharPointer_UTF8("\xc3\x87");

    // ui
    static inline String const GlyphBang = CharPointer_UTF8("\xc2\xa1");
    static inline String const GlyphToggle = CharPointer_UTF8("\xc2\xa2");
    static inline String const GlyphButton = CharPointer_UTF8("\xc2\xa3");
    static inline String const GlyphKnob = CharPointer_UTF8("\xc2\xa4");
    static inline String const GlyphNumber = CharPointer_UTF8("\xc2\xa5");
    static inline String const GlyphHSlider = CharPointer_UTF8("\xc2\xa8");
    static inline String const GlyphVSlider = CharPointer_UTF8("\xc2\xa9");
    static inline String const GlyphHRadio = CharPointer_UTF8("\xc2\xa6");
    static inline String const GlyphVRadio = CharPointer_UTF8("\xc2\xa7");
    static inline String const GlyphCanvas = CharPointer_UTF8("\xc2\xaa");
    static inline String const GlyphKeyboard = CharPointer_UTF8("\xc2\xab");
    static inline String const GlyphVUMeter = CharPointer_UTF8("\xc2\xac");
    static inline String const GlyphArray = CharPointer_UTF8("\xc2\xae");
    static inline String const GlyphGOP = CharPointer_UTF8("\xc2\xaf");
    static inline String const GlyphOscilloscope = CharPointer_UTF8("\xc2\xb0");
    static inline String const GlyphFunction = CharPointer_UTF8("\xc2\xb1");
    static inline String const GlyphMessbox = CharPointer_UTF8("\xc2\xb5");
    static inline String const GlyphBicoeff = CharPointer_UTF8("\xc2\xb3");

    // general
    static inline String const GlyphMetro = CharPointer_UTF8("\xc3\xa4");
    static inline String const GlyphCounter = CharPointer_UTF8("\xc3\xa6");
    static inline String const GlyphSelect = CharPointer_UTF8("\xc3\xa7");
    static inline String const GlyphRoute = CharPointer_UTF8("\xc3\xa8");
    static inline String const GlyphExpr = CharPointer_UTF8("\xc3\xb5");
    static inline String const GlyphLoadbang = CharPointer_UTF8("\xc3\xa9");
    static inline String const GlyphPack = CharPointer_UTF8("\xc3\xaa");
    static inline String const GlyphUnpack = CharPointer_UTF8("\xc3\xab");
    static inline String const GlyphPrint = CharPointer_UTF8("\xc3\xac");
    static inline String const GlyphNetsend = CharPointer_UTF8("\xc3\xae");
    static inline String const GlyphNetreceive = CharPointer_UTF8("\xc3\xad");
    static inline String const GlyphOSCsend = CharPointer_UTF8("\xc4\xb5");
    static inline String const GlyphOSCreceive = CharPointer_UTF8("\xc4\xb4");
    static inline String const GlyphTimer = CharPointer_UTF8("\xc3\xb6");
    static inline String const GlyphDelay = CharPointer_UTF8("\xc3\xb7");
    static inline String const GlyphTrigger = CharPointer_UTF8("\xc3\xb1");
    static inline String const GlyphMoses = CharPointer_UTF8("\xc3\xb2");
    static inline String const GlyphSpigot = CharPointer_UTF8("\xc3\xb3");
    static inline String const GlyphBondo = CharPointer_UTF8("\xc3\xb4");
    static inline String const GlyphSfz = CharPointer_UTF8("\xc3\xb8");

    // MIDI
    static inline String const GlyphMidiIn = CharPointer_UTF8("\xc4\x87");
    static inline String const GlyphMidiOut = CharPointer_UTF8("\xc4\x88");
    static inline String const GlyphNoteIn = CharPointer_UTF8("\xc4\x89");
    static inline String const GlyphNoteOut = CharPointer_UTF8("\xc4\x8a");
    static inline String const GlyphCtlIn = CharPointer_UTF8("\xc4\x8b");
    static inline String const GlyphCtlOut = CharPointer_UTF8("\xc4\x8c");
    static inline String const GlyphPgmIn = CharPointer_UTF8("\xc4\x8d");
    static inline String const GlyphPgmOut = CharPointer_UTF8("\xc4\x8e");
    static inline String const GlyphSysexIn = CharPointer_UTF8("\xc4\x8f");
    static inline String const GlyphSysexOut = CharPointer_UTF8("\xc4\x90");
    static inline String const GlyphMtof = CharPointer_UTF8("\xc4\x91");
    static inline String const GlyphFtom = CharPointer_UTF8("\xc4\x92");
    static inline String const GlyphAutotune = CharPointer_UTF8("\xc4\x93");

    // Multi~
    static inline String const GlyphMultiSnake = CharPointer_UTF8("\xc4\xbf");
    static inline String const GlyphMultiGet = CharPointer_UTF8("\xc5\x82");
    static inline String const GlyphMultiPick = CharPointer_UTF8("\xc5\x81");
    static inline String const GlyphMultiSig = CharPointer_UTF8("\xc5\x83");
    static inline String const GlyphMultiMerge = CharPointer_UTF8("\xc5\x84");
    static inline String const GlyphMultiUnmerge = CharPointer_UTF8("\xc5\x85");

    // IO~
    static inline String const GlyphAdc = CharPointer_UTF8("\xc4\xaa");
    static inline String const GlyphDac = CharPointer_UTF8("\xc4\xab");
    static inline String const GlyphOut = CharPointer_UTF8("\xc4\xac");
    static inline String const GlyphBlocksize = CharPointer_UTF8("\xc4\xad");
    static inline String const GlyphSamplerate = CharPointer_UTF8("\xc4\xae");
    static inline String const GlyphSetDsp = CharPointer_UTF8("\xc4\xaf");
    static inline String const GlyphSend = CharPointer_UTF8("\xc4\xb0");
    static inline String const GlyphReceive = CharPointer_UTF8("\xc4\xb1");
    static inline String const GlyphSignalSend = CharPointer_UTF8("\xc4\xb2");
    static inline String const GlyphSignalReceive = CharPointer_UTF8("\xc4\xb3");

    // OSC~
    static inline String const GlyphOsc = CharPointer_UTF8("\xc5\x8d");
    static inline String const GlyphPhasor = CharPointer_UTF8("\xc5\x8e");
    static inline String const GlyphSaw = CharPointer_UTF8("\xc5\x8f");
    static inline String const GlyphSaw2 = CharPointer_UTF8("\xc5\x90");
    static inline String const GlyphSquare = CharPointer_UTF8("\xc5\x91");
    static inline String const GlyphTriangle = CharPointer_UTF8("\xc5\x92");
    static inline String const GlyphImp = CharPointer_UTF8("\xc5\x93");
    static inline String const GlyphImp2 = CharPointer_UTF8("\xc5\x94");
    static inline String const GlyphWavetable = CharPointer_UTF8("\xc5\x95");
    static inline String const GlyphPlaits = CharPointer_UTF8("\xc5\x96");

    static inline String const GlyphOscBL = CharPointer_UTF8("\xc5\x97");
    static inline String const GlyphSawBL = CharPointer_UTF8("\xc5\x98");
    static inline String const GlyphSawBL2 = CharPointer_UTF8("\xc5\x99");
    static inline String const GlyphSquareBL = CharPointer_UTF8("\xc5\x9a");
    static inline String const GlyphTriBL = CharPointer_UTF8("\xc5\x9b");
    static inline String const GlyphImpBL = CharPointer_UTF8("\xc5\x9c");
    static inline String const GlyphImpBL2 = CharPointer_UTF8("\xc5\x9d");
    static inline String const GlyphWavetableBL = CharPointer_UTF8("\xc5\x9e");
    static inline String const GlyphLFORamp = CharPointer_UTF8("\xc5\x8f");
    static inline String const GlyphLFOSaw = CharPointer_UTF8("\xc5\x9f");
    static inline String const GlyphLFOSquare = CharPointer_UTF8("\xc5\xa0");
    static inline String const GlyphPulse = CharPointer_UTF8("\xc5\xa1");
    static inline String const GlyphPinknoise = CharPointer_UTF8("\xc5\xa2");

    // effects~
    static inline String const GlyphCrusher = CharPointer_UTF8("\xc6\x99");
    static inline String const GlyphDelayEffect = CharPointer_UTF8("\xc6\x9a");
    static inline String const GlyphDrive = CharPointer_UTF8("\xc6\x9b");
    static inline String const GlyphFlanger = CharPointer_UTF8("\xc6\x9c");
    static inline String const GlyphReverb = CharPointer_UTF8("\xc6\x9d");
    static inline String const GlyphFreeze = CharPointer_UTF8("\xc6\x9e");
    static inline String const GlyphRingmod = CharPointer_UTF8("\xc6\xa2");
    static inline String const GlyphSVFilter = CharPointer_UTF8("\xc6\xa6");
    static inline String const GlyphClip = CharPointer_UTF8("\xc6\xa3");
    static inline String const GlyphFold = CharPointer_UTF8("\xc6\xa4");
    static inline String const GlyphWrap = CharPointer_UTF8("\xc6\xa5");
    static inline String const GlyphCombRev = CharPointer_UTF8("\xc6\x9f");
    static inline String const GlyphComp = CharPointer_UTF8("\xc6\xa0");
    static inline String const GlyphBallance = CharPointer_UTF8("\xc6\xa7");
    static inline String const GlyphPan = CharPointer_UTF8("\xc6\xa8");

    // filters~
    static inline String const GlyphLowpass = CharPointer_UTF8("\xc7\x8b");
    static inline String const GlyphHighpass = CharPointer_UTF8("\xc7\x8c");
    static inline String const GlyphBandpass = CharPointer_UTF8("\xc7\x8d");
    static inline String const GlyphNotch = CharPointer_UTF8("\xc7\x8e");
    static inline String const GlyphRezLowpass = CharPointer_UTF8("\xc7\x8f");
    static inline String const GlyphRezHighpass = CharPointer_UTF8("\xc7\x90");
    static inline String const GlyphLowShelf = CharPointer_UTF8("\xc7\x91");
    static inline String const GlyphHighShelf = CharPointer_UTF8("\xc7\x92");
    static inline String const GlyphAllPass = CharPointer_UTF8("\xc7\x93");
    static inline String const GlyphFreqShift = CharPointer_UTF8("\xc6\x9a");

    // plugdata icon with three styles
    static inline String const PlugdataIconStandard = CharPointer_UTF8("\xc2\xbc");
    static inline String const PlugdataIconFilled = CharPointer_UTF8("\xc2\xbd");
    static inline String const PlugdataIconSilhouette = CharPointer_UTF8("\xc2\xbe");
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
    guiObjectInternalOutlineColourId,
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
    ShowCommandInput,
    NumItems // <-- the total number of items in this enum
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

UnorderedMap<ObjectIDs, String> const objectNames {
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
    { NewFloatAtom, "floatbox" },
    { NewSymbolAtom, "symbolbox" },
    { NewListAtom, "listbox" },
    { NewGraphOnParent, "graph" },
    { NewCanvas, "cnv" },
    { NewVUMeter, "vu" },
    { NewArray, "garray" },
};

struct Corners {
    static constexpr float windowCornerRadius = 12.0f;
    static constexpr float largeCornerRadius = 8.0f;
    static constexpr float defaultCornerRadius = 5.0f;
    static constexpr float resizeHanleCornerRadius = 2.75f;
    static inline float objectCornerRadius = 2.75f;
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

namespace PlatformStrings {
inline String getBrowserTip()
{
#if JUCE_MAC
    return "Reveal in Finder";
#elif JUCE_WINDOWS
    return "Reveal in Explorer";
#else
    return "Reveal in file browser";
#endif
}
}
