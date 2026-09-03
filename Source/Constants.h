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
    // TODO: placeholder (reuses the Lock glyph) until a dedicated play glyph is added to the icon font
    static inline String const Play = "m";
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
    static inline String const Palette = CharPointer_UTF8("\xc3\x8b");

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
    static inline String const PanelRight = CharPointer_UTF8("\xc3\x8c");
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

    static inline String const Record = CharPointer_UTF8("\xc3\x8a");
    static inline String const AudioSettings = CharPointer_UTF8("\xc3\x89");

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

    // Patch
    static inline String const GlyphSubpatch = CharPointer_UTF8("\xc7\x94");
    static inline String const GlyphInlet = CharPointer_UTF8("\xc7\x95");
    static inline String const GlyphOutlet = CharPointer_UTF8("\xc7\x96");
    static inline String const GlyphSignalInlet = CharPointer_UTF8("\xc7\x97");
    static inline String const GlyphSignalOutlet = CharPointer_UTF8("\xc7\x98");
    static inline String const GlyphClone = CharPointer_UTF8("\xc7\x99");
    static inline String const GlyphBlock = CharPointer_UTF8("\xc7\x9a");
    static inline String const GlyphSwitch = CharPointer_UTF8("\xc7\x9b");
    static inline String const GlyphDeclare = CharPointer_UTF8("\xc7\x9c");
    static inline String const GlyphSavestate = CharPointer_UTF8("\xc7\x9d");
    static inline String const GlyphPdcontrol = CharPointer_UTF8("\xc7\x9e");
    static inline String const GlyphArgs = CharPointer_UTF8("\xc7\x9f");
    static inline String const GlyphPresets = CharPointer_UTF8("\xc7\xa0");

    // User Interface
    static inline String const GlyphCircleSlider = CharPointer_UTF8("\xc7\xa1");
    static inline String const GlyphIncdec = CharPointer_UTF8("\xc7\xa2");
    static inline String const GlyphTabSelect = CharPointer_UTF8("\xc7\xa3");
    static inline String const GlyphGuiCanvas = CharPointer_UTF8("\xc7\xa4");
    static inline String const GlyphSlider2D = CharPointer_UTF8("\xc7\xa5");
    static inline String const GlyphMousePad = CharPointer_UTF8("\xc7\xa6");
    static inline String const GlyphMultiSlider = CharPointer_UTF8("\xc7\xa7");
    static inline String const GlyphRangeSlider = CharPointer_UTF8("\xc7\xa8");
    static inline String const GlyphMatrixCtl = CharPointer_UTF8("\xc7\xa9");
    static inline String const GlyphDrumSeq = CharPointer_UTF8("\xc7\xaa");
    static inline String const GlyphPopmenu = CharPointer_UTF8("\xc7\xab");
    static inline String const GlyphDisplay = CharPointer_UTF8("\xc7\xac");
    static inline String const GlyphTextNote = CharPointer_UTF8("\xc7\xad");
    static inline String const GlyphPic = CharPointer_UTF8("\xc7\xae");
    static inline String const GlyphOpenFile = CharPointer_UTF8("\xc7\xaf");
    static inline String const GlyphBiplot = CharPointer_UTF8("\xc7\xb0");
    static inline String const GlyphZBiplot = CharPointer_UTF8("\xc7\xb1");
    static inline String const GlyphSignalNumbox = CharPointer_UTF8("\xc7\xb2");
    static inline String const GlyphGainFader = CharPointer_UTF8("\xc7\xb3");
    static inline String const GlyphGainFader2 = CharPointer_UTF8("\xc7\xb4");
    static inline String const GlyphLevel = CharPointer_UTF8("\xc7\xb5");
    static inline String const GlyphMeterBar = CharPointer_UTF8("\xc7\xb6");
    static inline String const GlyphMeterBar2 = CharPointer_UTF8("\xc7\xb7");
    static inline String const GlyphSignalGraph = CharPointer_UTF8("\xc7\xb8");
    static inline String const GlyphSpectrum = CharPointer_UTF8("\xc7\xb9");
    static inline String const GlyphScope3D = CharPointer_UTF8("\xc7\xba");
    static inline String const GlyphPlaylist = CharPointer_UTF8("\xc7\xbb");

    // General
    static inline String const GlyphBangObject = CharPointer_UTF8("\xc7\xbc");
    static inline String const GlyphFloatObject = CharPointer_UTF8("\xc7\xbd");
    static inline String const GlyphIntObject = CharPointer_UTF8("\xc7\xbe");
    static inline String const GlyphSymbolObject = CharPointer_UTF8("\xc7\xbf");
    static inline String const GlyphValue = CharPointer_UTF8("\xc8\x80");
    static inline String const GlyphChange = CharPointer_UTF8("\xc8\x81");
    static inline String const GlyphSwap = CharPointer_UTF8("\xc8\x82");
    static inline String const GlyphUntil = CharPointer_UTF8("\xc8\x83");
    static inline String const GlyphChance = CharPointer_UTF8("\xc8\x84");
    static inline String const GlyphKeyInput = CharPointer_UTF8("\xc8\x85");

    // Lists & Text
    static inline String const GlyphListAppend = CharPointer_UTF8("\xc8\x86");
    static inline String const GlyphListPrepend = CharPointer_UTF8("\xc8\x87");
    static inline String const GlyphListStore = CharPointer_UTF8("\xc8\x88");
    static inline String const GlyphListSplit = CharPointer_UTF8("\xc8\x89");
    static inline String const GlyphListLength = CharPointer_UTF8("\xc8\x8a");
    static inline String const GlyphTextDefine = CharPointer_UTF8("\xc8\x8b");
    static inline String const GlyphTextGet = CharPointer_UTF8("\xc8\x8c");
    static inline String const GlyphTextSet = CharPointer_UTF8("\xc8\x8d");
    static inline String const GlyphTextSeq = CharPointer_UTF8("\xc8\x8e");
    static inline String const GlyphQlist = CharPointer_UTF8("\xc8\x8f");
    static inline String const GlyphTextfile = CharPointer_UTF8("\xc8\x90");
    static inline String const GlyphFormat = CharPointer_UTF8("\xc8\x91");
    static inline String const GlyphMakeFilename = CharPointer_UTF8("\xc8\x92");

    // Time & Sequencing
    static inline String const GlyphPipe = CharPointer_UTF8("\xc8\x93");
    static inline String const GlyphClockSync = CharPointer_UTF8("\xc8\x94");
    static inline String const GlyphMetronome = CharPointer_UTF8("\xc8\x95");
    static inline String const GlyphSpeed = CharPointer_UTF8("\xc8\x96");
    static inline String const GlyphTempo = CharPointer_UTF8("\xc8\x97");
    static inline String const GlyphScore = CharPointer_UTF8("\xc8\x98");
    static inline String const GlyphPattern = CharPointer_UTF8("\xc8\x99");
    static inline String const GlyphSequencer = CharPointer_UTF8("\xc8\x9a");
    static inline String const GlyphEuclid = CharPointer_UTF8("\xc8\x9b");
    static inline String const GlyphListSeq = CharPointer_UTF8("\xc8\x9c");
    static inline String const GlyphRecordTrack = CharPointer_UTF8("\xc8\x9d");

    // MIDI
    static inline String const GlyphBendIn = CharPointer_UTF8("\xc8\x9e");
    static inline String const GlyphBendOut = CharPointer_UTF8("\xc8\x9f");
    static inline String const GlyphTouchIn = CharPointer_UTF8("\xc8\xa0");
    static inline String const GlyphTouchOut = CharPointer_UTF8("\xc8\xa1");
    static inline String const GlyphPolyTouchIn = CharPointer_UTF8("\xc8\xa2");
    static inline String const GlyphPolyTouchOut = CharPointer_UTF8("\xc8\xa3");
    static inline String const GlyphMidiRealtime = CharPointer_UTF8("\xc8\xa4");
    static inline String const GlyphMakenote = CharPointer_UTF8("\xc8\xa5");
    static inline String const GlyphStripnote = CharPointer_UTF8("\xc8\xa6");
    static inline String const GlyphPolyVoices = CharPointer_UTF8("\xc8\xa7");
    static inline String const GlyphMidiLearn = CharPointer_UTF8("\xc8\xa8");
    static inline String const GlyphPanic = CharPointer_UTF8("\xc8\xa9");

    // Input & Output
    static inline String const GlyphOutMc = CharPointer_UTF8("\xc8\xaa");
    static inline String const GlyphSigConv = CharPointer_UTF8("\xc8\xab");
    static inline String const GlyphSnapshot = CharPointer_UTF8("\xc8\xac");
    static inline String const GlyphThrow = CharPointer_UTF8("\xc8\xad");
    static inline String const GlyphCatch = CharPointer_UTF8("\xc8\xae");
    static inline String const GlyphSignalPrint = CharPointer_UTF8("\xc8\xaf");
    static inline String const GlyphOscParse = CharPointer_UTF8("\xc8\xb0");
    static inline String const GlyphOscFormat = CharPointer_UTF8("\xc8\xb1");
    static inline String const GlyphPdlink = CharPointer_UTF8("\xc8\xb2");

    // Arrays & Files
    static inline String const GlyphTabread = CharPointer_UTF8("\xc8\xb3");
    static inline String const GlyphTabread4 = CharPointer_UTF8("\xc8\xb4");
    static inline String const GlyphTabwrite = CharPointer_UTF8("\xc8\xb5");
    static inline String const GlyphSoundfiler = CharPointer_UTF8("\xc8\xb6");
    static inline String const GlyphBuffer = CharPointer_UTF8("\xc8\xb7");
    static inline String const GlyphSfload = CharPointer_UTF8("\xc8\xb8");
    static inline String const GlyphTabosc = CharPointer_UTF8("\xc8\xb9");
    static inline String const GlyphTabplay = CharPointer_UTF8("\xc8\xba");
    static inline String const GlyphSignalTabread = CharPointer_UTF8("\xc8\xbb");
    static inline String const GlyphSignalTabread4 = CharPointer_UTF8("\xc8\xbc");
    static inline String const GlyphSignalTabwrite = CharPointer_UTF8("\xc8\xbd");
    static inline String const GlyphReadsf = CharPointer_UTF8("\xc8\xbe");
    static inline String const GlyphWritesf = CharPointer_UTF8("\xc8\xbf");
    static inline String const GlyphSamplePlayer = CharPointer_UTF8("\xc9\x80");

    // Oscillators
    static inline String const GlyphCosine = CharPointer_UTF8("\xc9\x81");
    static inline String const GlyphSine = CharPointer_UTF8("\xc9\x82");
    static inline String const GlyphPulseOsc = CharPointer_UTF8("\xc9\x83");
    static inline String const GlyphVSaw = CharPointer_UTF8("\xc9\x84");
    static inline String const GlyphBlip = CharPointer_UTF8("\xc9\x85");
    static inline String const GlyphFm = CharPointer_UTF8("\xc9\x86");
    static inline String const GlyphPm = CharPointer_UTF8("\xc9\x87");
    static inline String const GlyphWavetable2D = CharPointer_UTF8("\xc9\x88");
    static inline String const GlyphOscBank = CharPointer_UTF8("\xc9\x89");
    static inline String const GlyphLfo = CharPointer_UTF8("\xc9\x8a");

    // Noise & Random
    static inline String const GlyphNoise = CharPointer_UTF8("\xc9\x8b");
    static inline String const GlyphWhiteNoise = CharPointer_UTF8("\xc9\x8c");
    static inline String const GlyphPinkNoise = CharPointer_UTF8("\xc9\x8d");
    static inline String const GlyphBrownNoise = CharPointer_UTF8("\xc9\x8e");
    static inline String const GlyphGrayNoise = CharPointer_UTF8("\xc9\x8f");
    static inline String const GlyphVelvet = CharPointer_UTF8("\xc9\x90");
    static inline String const GlyphCrackle = CharPointer_UTF8("\xc9\x91");
    static inline String const GlyphPerlin = CharPointer_UTF8("\xc9\x92");
    static inline String const GlyphDust = CharPointer_UTF8("\xc9\x93");
    static inline String const GlyphLfNoise = CharPointer_UTF8("\xc9\x94");
    static inline String const GlyphStepNoise = CharPointer_UTF8("\xc9\x95");
    static inline String const GlyphRampNoise = CharPointer_UTF8("\xc9\x96");
    static inline String const GlyphRandPulse = CharPointer_UTF8("\xc9\x97");
    static inline String const GlyphRandom = CharPointer_UTF8("\xc9\x98");
    static inline String const GlyphRandFloat = CharPointer_UTF8("\xc9\x99");
    static inline String const GlyphRandInt = CharPointer_UTF8("\xc9\x9a");
    static inline String const GlyphDrunkard = CharPointer_UTF8("\xc9\x9b");
    static inline String const GlyphMarkov = CharPointer_UTF8("\xc9\x9c");

    // Envelopes
    static inline String const GlyphLineSignal = CharPointer_UTF8("\xc9\x9d");
    static inline String const GlyphVline = CharPointer_UTF8("\xc9\x9e");
    static inline String const GlyphLineCtl = CharPointer_UTF8("\xc9\x9f");
    static inline String const GlyphAdsr = CharPointer_UTF8("\xc9\xa0");
    static inline String const GlyphAsr = CharPointer_UTF8("\xc9\xa1");
    static inline String const GlyphDecayEnv = CharPointer_UTF8("\xc9\xa2");
    static inline String const GlyphEnvGen = CharPointer_UTF8("\xc9\xa3");
    static inline String const GlyphFuncGen = CharPointer_UTF8("\xc9\xa4");
    static inline String const GlyphEnvelopeShape = CharPointer_UTF8("\xc9\xa5");
    static inline String const GlyphSusLoop = CharPointer_UTF8("\xc9\xa6");
    static inline String const GlyphRampEnv = CharPointer_UTF8("\xc9\xa7");
    static inline String const GlyphGlide = CharPointer_UTF8("\xc9\xa8");
    static inline String const GlyphLag = CharPointer_UTF8("\xc9\xa9");
    static inline String const GlyphSlew = CharPointer_UTF8("\xc9\xaa");
    static inline String const GlyphSmooth = CharPointer_UTF8("\xc9\xab");

    // Filters
    static inline String const GlyphLop = CharPointer_UTF8("\xc9\xac");
    static inline String const GlyphHip = CharPointer_UTF8("\xc9\xad");
    static inline String const GlyphBpFilter = CharPointer_UTF8("\xc9\xae");
    static inline String const GlyphVcf = CharPointer_UTF8("\xc9\xaf");
    static inline String const GlyphBiquad = CharPointer_UTF8("\xc9\xb0");
    static inline String const GlyphSlop = CharPointer_UTF8("\xc9\xb1");
    static inline String const GlyphLowpassRes = CharPointer_UTF8("\xc9\xb2");
    static inline String const GlyphHighpassRes = CharPointer_UTF8("\xc9\xb3");
    static inline String const GlyphBandpassRes = CharPointer_UTF8("\xc9\xb4");
    static inline String const GlyphBandstopFilt = CharPointer_UTF8("\xc9\xb5");
    static inline String const GlyphLowshelfFilt = CharPointer_UTF8("\xc9\xb6");
    static inline String const GlyphHighshelfFilt = CharPointer_UTF8("\xc9\xb7");
    static inline String const GlyphParametricEq = CharPointer_UTF8("\xc9\xb8");
    static inline String const GlyphAllpassFilt = CharPointer_UTF8("\xc9\xb9");
    static inline String const GlyphCombFilt = CharPointer_UTF8("\xc9\xba");
    static inline String const GlyphResonantFilt = CharPointer_UTF8("\xc9\xbb");
    static inline String const GlyphCrossover = CharPointer_UTF8("\xc9\xbc");
    static inline String const GlyphMoog = CharPointer_UTF8("\xc9\xbd");

    // Effects
    static inline String const GlyphChorus = CharPointer_UTF8("\xc9\xbe");
    static inline String const GlyphPhaser = CharPointer_UTF8("\xc9\xbf");
    static inline String const GlyphTremolo = CharPointer_UTF8("\xca\x80");
    static inline String const GlyphVibrato = CharPointer_UTF8("\xca\x81");
    static inline String const GlyphVocoder = CharPointer_UTF8("\xca\x82");
    static inline String const GlyphWaveshaper = CharPointer_UTF8("\xca\x83");
    static inline String const GlyphDownsample = CharPointer_UTF8("\xca\x84");
    static inline String const GlyphPitchShift = CharPointer_UTF8("\xca\x85");
    static inline String const GlyphCompress = CharPointer_UTF8("\xca\x86");
    static inline String const GlyphExpand = CharPointer_UTF8("\xca\x87");
    static inline String const GlyphNoiseGate = CharPointer_UTF8("\xca\x88");
    static inline String const GlyphNormalize = CharPointer_UTF8("\xca\x89");
    static inline String const GlyphPlateReverb = CharPointer_UTF8("\xca\x8a");
    static inline String const GlyphEchoReverb = CharPointer_UTF8("\xca\x8b");
    static inline String const GlyphDelwrite = CharPointer_UTF8("\xca\x8c");
    static inline String const GlyphDelread = CharPointer_UTF8("\xca\x8d");

    // Analysis
    static inline String const GlyphEnvFollow = CharPointer_UTF8("\xca\x8e");
    static inline String const GlyphRms = CharPointer_UTF8("\xca\x8f");
    static inline String const GlyphMovRms = CharPointer_UTF8("\xca\x90");
    static inline String const GlyphPeakDetect = CharPointer_UTF8("\xca\x91");
    static inline String const GlyphVuDetect = CharPointer_UTF8("\xca\x92");
    static inline String const GlyphZerocross = CharPointer_UTF8("\xca\x93");
    static inline String const GlyphPeriodDetect = CharPointer_UTF8("\xca\x94");
    static inline String const GlyphBeatDetect = CharPointer_UTF8("\xca\x95");
    static inline String const GlyphChangedSignal = CharPointer_UTF8("\xca\x96");
    static inline String const GlyphSigmund = CharPointer_UTF8("\xca\x97");
    static inline String const GlyphBonk = CharPointer_UTF8("\xca\x98");
    static inline String const GlyphAboveThresh = CharPointer_UTF8("\xca\x99");
    static inline String const GlyphSchmitt = CharPointer_UTF8("\xca\x9a");
    static inline String const GlyphSampleHold = CharPointer_UTF8("\xca\x9b");

    // Multichannel
    static inline String const GlyphNumChans = CharPointer_UTF8("\xca\x9c");
    static inline String const GlyphMixChans = CharPointer_UTF8("\xca\x9d");
    static inline String const GlyphSumChans = CharPointer_UTF8("\xca\x9e");
    static inline String const GlyphSliceChans = CharPointer_UTF8("\xca\x9f");
    static inline String const GlyphRepeatChans = CharPointer_UTF8("\xca\xa0");
    static inline String const GlyphGroupChans = CharPointer_UTF8("\xca\xa1");
    static inline String const GlyphSelectChans = CharPointer_UTF8("\xca\xa2");
    static inline String const GlyphLaceChans = CharPointer_UTF8("\xca\xa3");
    static inline String const GlyphDelaceChans = CharPointer_UTF8("\xca\xa4");















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
    ToggleLeftSidebar,
    ToggleRightSidebar,
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
    TogglePresentationMode,
    TogglePluginMode,
    Compile,
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
