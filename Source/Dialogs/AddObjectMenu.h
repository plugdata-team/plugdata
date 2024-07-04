/*
 // Copyright (c) 2023 Alex Mitchell and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Dialogs.h"
#include "Components/ObjectDragAndDrop.h"

#define DEBUG_PRINT_OBJECT_LIST 0

class ObjectItem : public ObjectDragAndDrop
    , public SettableTooltipClient {
public:
    ObjectItem(PluginEditor* e, String const& text, String const& icon, String const& tooltip, String const& patch, ObjectIDs objectID, std::function<void(bool)> dismissCalloutBox)
        : ObjectDragAndDrop(e)
        , titleText(text)
        , iconText(icon)
        , objectPatch(patch)
        , dismissMenu(dismissCalloutBox)
        , editor(e)
    {
        setTooltip(tooltip.replace("(@keypress) ", getKeyboardShortcutDescription(objectID)));
    }

    void dismiss(bool withAnimation) override
    {
        dismissMenu(withAnimation);
    }

    String getKeyboardShortcutDescription(ObjectIDs objectID)
    {
        auto keyPresses = editor->commandManager.getKeyMappings()->getKeyPressesAssignedToCommand(objectID);
        if (keyPresses.size()) {
            return "(" + keyPresses.getReference(0).getTextDescription() + ") ";
        }

        return String();
    }

    void paint(Graphics& g) override
    {
        auto highlight = findColour(PlugDataColour::popupMenuActiveBackgroundColourId);

        auto iconBounds = getLocalBounds().reduced(14).translated(0, -7);
        auto textBounds = getLocalBounds().removeFromBottom(14);

        if (isHovering) {
            g.setColour(highlight);
            g.fillRoundedRectangle(iconBounds.toFloat(), Corners::defaultCornerRadius);
        }

        Fonts::drawText(g, titleText, textBounds, findColour(PlugDataColour::popupMenuTextColourId), 13.0f, Justification::centred);
        Fonts::drawIcon(g, iconText, iconBounds.reduced(2), findColour(PlugDataColour::popupMenuTextColourId), 30);
    }

    bool hitTest(int x, int y) override
    {
        return getLocalBounds().reduced(16).translated(0, -7).contains(x, y);
    }

    void mouseEnter(MouseEvent const& e) override
    {
        isHovering = true;
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        isHovering = false;
        repaint();
    }

    String getObjectString() override
    {
        // If this is an array, replace @arrName with unused name
        auto patchString = objectPatch;
        if (patchString.contains("@arrName")) {
            editor->pd->setThis();
            patchString = patchString.replace("@arrName", String::fromUTF8(pd::Interface::getUnusedArrayName()->s_name));
        }
        return patchString;
    }

    String getPatchStringName() override
    {
        return titleText + String(" object");
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown()) {
            dismissMenu(false);
        } else {
#if !JUCE_IOS
            ObjectClickAndDrop::attachToMouse(this);
            dismissMenu(false);
#endif
        }
    }

private:
    String titleText;
    String iconText;
    String objectPatch;
    bool isHovering = false;
    std::function<void(bool)> dismissMenu;
    PluginEditor* editor;
};

class ObjectList : public Component {
public:
    ObjectList(PluginEditor* e, std::function<void(bool)> dismissCalloutBox)
        : editor(e)
        , dismissMenu(dismissCalloutBox)
    {
#if DEBUG_PRINT_OBJECT_LIST == 1
        printAllObjects();
#endif
    }

    void resized() override
    {
        auto width = getWidth();

        int column = 0;
        int maxColumns = width / itemSize;
        int offset = 0;

        for (auto button : objectButtons) {
            button->setBounds(column * itemSize, offset, itemSize, itemSize);
            column++;
            if (column >= maxColumns) {
                column = 0;
                offset += itemSize;
            }
        }
    }

    void showCategory(String const& categoryToView)
    {
        objectButtons.clear();

        auto objectsToShow = getValue<bool>(SettingsFile::getInstance()->getPropertyAsValue("hvcc_mode")) ? heavyObjectList : defaultObjectList;

        for (auto& [categoryName, objectCategory] : objectsToShow) {

            if (categoryName != categoryToView)
                continue;

            for (auto& [icon, patch, tooltip, name, objectID] : objectCategory) {
                auto objectPatch = patch;
                if (objectPatch.isEmpty())
                    objectPatch = "#X obj 0 0 " + name;
                else if (!objectPatch.startsWith("#")) {
                    objectPatch = ObjectThemeManager::get()->getCompleteFormat(objectPatch);
                }
                auto* button = objectButtons.add(new ObjectItem(editor, name, icon, tooltip, objectPatch, objectID, dismissMenu));
                addAndMakeVisible(button);
            }
        }

        resized();
    }

    static void printAllObjects()
    {
        static bool hasRun = false;
        if (hasRun)
            return;

        hasRun = true;

        std::cout << "==== object icon list in CSV format ====" << std::endl;

        String cat;
        for (auto& [categoryName, objectCategory] : defaultObjectList) {
            cat = categoryName;
            for (auto& [icon, patch, tooltip, name, objectID] : objectCategory) {
                std::cout << cat << ", " << name << ", " << icon << std::endl;
            }
        }

        std::cout << "==== end of list ====" << std::endl;
    }

    OwnedArray<ObjectItem> objectButtons;

    static inline std::vector<std::pair<String, std::vector<std::tuple<String, String, String, String, ObjectIDs>>>> const defaultObjectList = {
        { "Default",
            {
                { Icons::GlyphEmptyObject, "#X obj 0 0", "(@keypress) Empty object", "Object", NewObject },
                { Icons::GlyphMessage, "#X msg 0 0", "(@keypress) Message", "Message", NewMessage },
                { Icons::GlyphFloatBox, "#X floatatom 0 0 5 0 0 0 - - - 0", "(@keypress) Float box", "Float", NewFloatAtom },
                { Icons::GlyphSymbolBox, "#X symbolatom 0 0 10 0 0 0 - - - 0", "Symbol box", "Symbol", NewSymbolAtom },
                { Icons::GlyphListBox, "#X listbox 0 0 20 0 0 0 - - - 0", "(@keypress) List box", "List", NewListAtom },
                { Icons::GlyphComment, "#X text 0 0 comment", "(@keypress) Comment", "Comment", NewComment },
                { Icons::GlyphArray, "#N canvas 0 0 450 250 (subpatch) 0;\n#X array @arrName 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 0 0 graph;", "(@keypress) Array", "Array", NewArray },
                { Icons::GlyphGOP, "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore 0 0 graph;", "(@keypress) Graph on parent", "Graph", NewGraphOnParent },
            } },
        { "UI",
            {
                // GUI object default settings are in ObjectManager.h
                { Icons::GlyphBang, "bng", "(@keypress) Bang", "Bang", NewBang },
                { Icons::GlyphToggle, "tgl", "(@keypress) Toggle", "Toggle", NewToggle },
                { Icons::GlyphButton, "button", "Button", "Button", OtherObject },
                { Icons::GlyphKnob, "knob", "Knob", "Knob", OtherObject },
                { Icons::GlyphVSlider, "vsl", "(@keypress) Vertical slider", "V. Slider", NewVerticalSlider },
                { Icons::GlyphHSlider, "hsl", "(@keypress) Horizontal slider", "H. Slider", NewHorizontalSlider },
                { Icons::GlyphVRadio, "vradio", "(@keypress) Vertical radio box", "V. Radio", NewVerticalRadio },
                { Icons::GlyphHRadio, "hradio", "(@keypress) Horizontal radio box", "H. Radio", NewHorizontalRadio },
                { Icons::GlyphNumber, "nbx", "(@keypress) Number box", "Number", NewNumbox },
                { Icons::GlyphCanvas, "cnv", "(@keypress) Canvas", "Canvas", NewCanvas },
                { Icons::GlyphFunction, "function", "Function", "Function", OtherObject },
                { Icons::GlyphOscilloscope, "scope~", "Oscilloscope", "Scope", OtherObject },
                { Icons::GlyphKeyboard, "#X obj 0 0 keyboard 16 80 4 2 0 0 empty empty", "Piano keyboard", "Keyboard", OtherObject },
                { Icons::GlyphMessbox, "messbox", "ELSE Message box", "Messbox", OtherObject },
                { Icons::GlyphBicoeff, "#X obj 0 0 bicoeff 450 150 peaking", "Bicoeff generator", "Bicoeff", OtherObject },
                { Icons::GlyphVUMeter, "vu", "(@keypress) VU meter", "VU Meter", NewVUMeter },
            } },
        { "General",
            {
                { Icons::GlyphMetro, "#X obj 0 0 metro 1 120 permin", "Metro", "Metro", OtherObject },
                { Icons::GlyphCounter, "#X obj 0 0 count 5", "Count", "Count", OtherObject },
                { Icons::GlyphTrigger, "#X obj 0 0 trigger", "Trigger", "Trigger", OtherObject },
                { Icons::GlyphMoses, "#X obj 0 0 moses", "Moses", "Moses", OtherObject },
                { Icons::GlyphSpigot, "#X obj 0 0 spigot", "Spigot", "Spigot", OtherObject },
                { Icons::GlyphBondo, "#X obj 0 0 bondo", "Bondo", "Bondo", OtherObject },
                { Icons::GlyphSelect, "#X obj 0 0 select", "Select", "Select", OtherObject },
                { Icons::GlyphRoute, "#X obj 0 0 route", "Route", "Route", OtherObject },
                { Icons::GlyphExpr, "#X obj 0 0 expr", "Expr", "Expr", OtherObject },
                { Icons::GlyphLoadbang, "#X obj 0 0 loadbang", "Loadbang", "Loadbang", OtherObject },
                { Icons::GlyphPack, "#X obj 0 0 pack", "Pack", "Pack", OtherObject },
                { Icons::GlyphUnpack, "#X obj 0 0 unpack", "Unpack", "Unpack", OtherObject },
                { Icons::GlyphPrint, "#X obj 0 0 print", "Print", "Print", OtherObject },
                { Icons::GlyphTimer, "#X obj 0 0 timer", "Timer", "Timer", OtherObject },
                { Icons::GlyphDelay, "#X obj 0 0 delay 1 60 permin", "Delay", "Delay", OtherObject },
                { Icons::GlyphSfz, "#X obj 0 0 sfz~", "Sfz sample player using sfizz", "Sfz", OtherObject },
            } },
        { "MIDI",
            {
                { Icons::GlyphMidiIn, "#X obj 0 0 midiin", "MIDI in", "MIDI in", OtherObject },
                { Icons::GlyphMidiOut, "#X obj 0 0 midiout", "MIDI out", "MIDI out", OtherObject },
                { Icons::GlyphNoteIn, "#X obj 0 0 notein", "Note in", "Note in", OtherObject },
                { Icons::GlyphNoteOut, "#X obj 0 0 noteout", "Note out", "Note out", OtherObject },
                { Icons::GlyphCtlIn, "#X obj 0 0 ctlin", "Control in", "Ctl in", OtherObject },
                { Icons::GlyphCtlOut, "#X obj 0 0 ctlout", "Control out", "Ctl out", OtherObject },
                { Icons::GlyphPgmIn, "#X obj 0 0 pgmin", "Program in", "Pgm in", OtherObject },
                { Icons::GlyphPgmOut, "#X obj 0 0 pgmout", "Program out", "Pgm out", OtherObject },
                { Icons::GlyphSysexIn, "#X obj 0 0 sysexin", "Sysex in", "Sysex in", OtherObject },
                { Icons::GlyphMtof, "#X obj 0 0 mtof", "MIDI to frequency", "mtof", OtherObject },
                { Icons::GlyphFtom, "#X obj 0 0 ftom", "Frequency to MIDI", "ftom", OtherObject },
                { Icons::GlyphAutotune, "#X obj 0 0 autotune", "Pitch quantizer", "Autotune", OtherObject },
            } },
        { "IO",
            {
                { Icons::GlyphAdc, "#X obj 0 0 adc~", "Adc", "Adc", OtherObject },
                { Icons::GlyphDac, "#X obj 0 0 dac~", "Dac", "Dac", OtherObject },
                { Icons::GlyphOut, "#X obj 0 0 out~", "Out", "Out", OtherObject },
                { Icons::GlyphBlocksize, "#X obj 0 0 blocksize~", "Blocksize", "Blocksize", OtherObject },
                { Icons::GlyphSamplerate, "#X obj 0 0 samplerate~", "Samplerate", "Samplerate", OtherObject },
                { Icons::GlyphSetDsp, "#X obj 0 0 setdsp~", "Setdsp", "Setdsp", OtherObject },
                { Icons::GlyphNetreceive, "#X obj 0 0 netreceive", "Netreceive", "Netreceive", OtherObject },
                { Icons::GlyphNetsend, "#X obj 0 0 netsend", "Netsend", "Netsend", OtherObject },
                { Icons::GlyphOSCsend, "#X obj 0 0 osc.send", "OSC send", "OSC send", OtherObject },
                { Icons::GlyphOSCreceive, "#X obj 0 0 osc.receive", "OSC receive", "OSC receive", OtherObject },
                { Icons::GlyphSend, "#X obj 0 0 s", "Send", "Send", OtherObject },
                { Icons::GlyphReceive, "#X obj 0 0 r", "Receive", "Receive", OtherObject },
                { Icons::GlyphSignalSend, "#X obj 0 0 s~", "Send~", "Send~", OtherObject },
                { Icons::GlyphSignalReceive, "#X obj 0 0 r~", "Receive~", "Receive~", OtherObject },
            } },
        { "Osc~",
            {
                { Icons::GlyphPhasor, "#X obj 0 0 phasor~", "Phasor", "Phasor", OtherObject },
                { Icons::GlyphOsc, "#X obj 0 0 osc~ 440", "Osc", "Osc", OtherObject },
                { Icons::GlyphOscBL, "#X obj 0 0 bl.osc~ 440", "Osc band limited", "Bl. Osc", OtherObject },
                { Icons::GlyphTriangle, "#X obj 0 0 tri~ 440", "Triangle", "Triangle", OtherObject },
                { Icons::GlyphTriBL, "#X obj 0 0 bl.tri~ 100", "Triangle band limited", "Bl. Tri", OtherObject },
                { Icons::GlyphSquare, "#X obj 0 0 square~", "Square", "Square", OtherObject },
                { Icons::GlyphSquareBL, "#X obj 0 0 bl.tri~ 440", "Square band limited", "Bl. Square", OtherObject },
                { Icons::GlyphSaw, "#X obj 0 0 saw~ 440", "Saw", "Saw", OtherObject },
                { Icons::GlyphSawBL, "#X obj 0 0 bl.saw~ 440", "Saw band limited", "Bl. Saw", OtherObject },
                { Icons::GlyphImp, "#X obj 0 0 imp~ 100", "Impulse", "Impulse", OtherObject },
                { Icons::GlyphImpBL, "#X obj 0 0 bl.imp~ 100", "Impulse band limited", "Bl. Imp", OtherObject },
                { Icons::GlyphWavetable, "#X obj 0 0 wavetable~", "Wavetable", "Wavetab", OtherObject },
                { Icons::GlyphWavetableBL, "#X obj 0 0 bl.wavetable~", "Wavetable band limited", "Bl. Wavetab", OtherObject },
                { Icons::GlyphPlaits, "#X obj 0 0 plaits~", "Plaits", "Plaits", OtherObject },
            } },
        { "FX~",
            {
                { Icons::GlyphCrusher, "#X obj 0 0 crusher~ 0.1 0.1", "Crusher", "Crusher", OtherObject },
                { Icons::GlyphDelayEffect, "#X obj 0 0 delay~ 22050 14700", "Delay", "Delay", OtherObject },
                { Icons::GlyphDrive, "#X obj 0 0 drive~", "Drive", "Drive", OtherObject },
                { Icons::GlyphFlanger, "#X obj 0 0 flanger~ 0.1 20 -0.6", "Flanger", "Flanger", OtherObject },
                { Icons::GlyphCombRev, "#X obj 0 0 comb.rev~ 500 1 0.99 0.99", "Comb reverberator", "Comb. Rev", OtherObject },
                { Icons::GlyphDuck, "#X obj 0 0 duck~", "Sidechain compressor", "Duck", OtherObject },
                { Icons::GlyphBallance, "#X obj 0 0 balance~", "Balance", "Balance", OtherObject },
                { Icons::GlyphPan, "#X obj 0 0 pan2~", "Pan", "Pan", OtherObject },
                { Icons::GlyphReverb, "#X obj 0 0 free.rev~ 0.7 0.6 0.5 0.7", "Reverb", "Reverb", OtherObject },
                { Icons::GlyphFreeze, "#X obj 0 0 freeze~", "Freeze", "Freeze", OtherObject },
                { Icons::GlyphRingmod, "#X obj 0 0 rm~ 150", "Ringmod", "Ringmod", OtherObject },
                { Icons::GlyphSVFilter, "#X obj 0 0 svfilter~ 1729 0.42", "State variable filter", "SVFilter", OtherObject },
                { Icons::GlyphClip, "#X obj 0 0 clip~ -0.5 0.5", "Clip", "Clip", OtherObject },
                { Icons::GlyphFold, "#X obj 0 0 fold~ -0.5 0.5", "Fold", "Fold", OtherObject },
                { Icons::GlyphWrap, "#X obj 0 0 wrap2~ -0.5 0.5", "Wrap", "Wrap", OtherObject },
            } },
        { "Multi~",
            {
                { Icons::GlyphMultiSnake, "#X obj 0 0 snake~ 2", "Multichannel snake", "Snake", OtherObject },
                { Icons::GlyphMultiGet, "#X obj 0 0 get~", "Multichannel get", "Get", OtherObject },
                { Icons::GlyphMultiPick, "#X obj 0 0 pick~", "Multichannel pick", "Pick", OtherObject },
                { Icons::GlyphMultiSig, "#X obj 0 0 sigs~", "Multichannel value signal", "Sigs", OtherObject },
                { Icons::GlyphMultiMerge, "#X obj 0 0 merge~", "Multichannel merge", "Merge", OtherObject },
                { Icons::GlyphMultiUnmerge, "#X obj 0 0 unmerge~", "Multichannel unmerge", "Unmerge", OtherObject },
            } },
        { "Math",
            {
                { Icons::GlyphGeneric, "", "Add", "+", OtherObject },
                { Icons::GlyphGeneric, "", "Subtract", "-", OtherObject },
                { Icons::GlyphGeneric, "", "Multiply", "*", OtherObject },
                { Icons::GlyphGeneric, "", "Divide", "/", OtherObject },
                { Icons::GlyphGeneric, "", "Remainder", "%", OtherObject },
                { Icons::GlyphGeneric, "", "Reversed inlet subtraction", "!-", OtherObject },
                { Icons::GlyphGeneric, "", "Reversed inlet division", "!/", OtherObject },
                { Icons::GlyphGeneric, "", "Greater than", ">", OtherObject },
                { Icons::GlyphGeneric, "", "Less than", "<", OtherObject },
                { Icons::GlyphGeneric, "", "Greater or equal", ">=", OtherObject },
                { Icons::GlyphGeneric, "", "Less or equal", "<=", OtherObject },
                { Icons::GlyphGeneric, "", "Equality", "==", OtherObject },
                { Icons::GlyphGeneric, "", "Not equal", "!=", OtherObject },
                { Icons::GlyphGeneric, "", "Minimum", "min", OtherObject },
                { Icons::GlyphGeneric, "", "Maximum", "max", OtherObject },
            } },
        { "Math~",
            {
                { Icons::GlyphGenericSignal, "", "(signal) Add", "+~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Subtract", "-~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Multiply", "*~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Divide", "/~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Remainder", "%~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Reversed inlet subtraction", "!-~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Reversed inlet division", "!/~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Greater than", ">~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Less than", "<~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Greater or equal", ">=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Less or equal", "<=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Equality", "==~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Not equal", "!=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Minimum", "min~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Maximum", "max~", OtherObject },
            } },
    };

    static inline std::vector<std::pair<String, std::vector<std::tuple<String, String, String, String, ObjectIDs>>>> const heavyObjectList = {
        { "Default",
            {
                { Icons::GlyphEmptyObject, "#X obj 0 0", "(@keypress) Empty object", "Object", NewObject },
                { Icons::GlyphMessage, "#X msg 0 0", "(@keypress) Message", "Message", NewMessage },
                { Icons::GlyphFloatBox, "#X floatatom 0 0 5 0 0 0 - - - 0", "(@keypress) Float box", "Float", NewFloatAtom },
                { Icons::GlyphSymbolBox, "#X symbolatom 0 0 10 0 0 0 - - - 0", "Symbol box", "Symbol", NewSymbolAtom },
                { Icons::GlyphComment, "#X text 0 0 comment", "(@keypress) Comment", "Comment", NewComment },
                { Icons::GlyphArray, "#N canvas 0 0 450 250 (subpatch) 0;\n#X array @arrName 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 0 0 graph;", "(@keypress) Array", "Array", NewArray },
                { Icons::GlyphGOP, "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore 0 0 graph;", "(@keypress) Graph on parent", "Graph", NewGraphOnParent },
            } },
        { "UI",
            {
                // GUI object default settings are in OjbectManager.h
                { Icons::GlyphBang, "bng", "(@keypress) Bang", "Bang", NewBang },
                { Icons::GlyphToggle, "tgl", "(@keypress) Toggle", "Toggle", NewToggle },
                { Icons::GlyphVSlider, "vsl", "(@keypress) Vertical slider", "V. Slider", NewVerticalSlider },
                { Icons::GlyphHSlider, "hsl", "(@keypress) Horizontal slider", "H. Slider", NewHorizontalSlider },
                { Icons::GlyphVRadio, "vradio", "(@keypress) Vertical radio box", "V. Radio", NewVerticalRadio },
                { Icons::GlyphHRadio, "hradio", "(@keypress) Horizontal radio box", "H. Radio", NewHorizontalRadio },
                { Icons::GlyphNumber, "nbx", "(@keypress) Number box", "Number", NewNumbox },
                { Icons::GlyphCanvas, "cnv", "(@keypress) Canvas", "Canvas", NewCanvas },
            } },
        { "General",
            {
                { Icons::GlyphMetro, "#X obj 0 0 metro 1 120 permin", "Metro", "Metro", OtherObject },
                { Icons::GlyphTrigger, "#X obj 0 0 trigger", "Trigger", "Trigger", OtherObject },
                { Icons::GlyphMoses, "#X obj 0 0 moses", "Moses", "Moses", OtherObject },
                { Icons::GlyphSpigot, "#X obj 0 0 spigot", "Spigot", "Spigot", OtherObject },
                { Icons::GlyphSelect, "#X obj 0 0 select", "Select", "Select", OtherObject },
                { Icons::GlyphRoute, "#X obj 0 0 route", "Route", "Route", OtherObject },
                { Icons::GlyphLoadbang, "#X obj 0 0 loadbang", "Loadbang", "Loadbang", OtherObject },
                { Icons::GlyphPack, "#X obj 0 0 pack", "Pack", "Pack", OtherObject },
                { Icons::GlyphUnpack, "#X obj 0 0 unpack", "Unpack", "Unpack", OtherObject },
                { Icons::GlyphPrint, "#X obj 0 0 print", "Print", "Print", OtherObject },
                { Icons::GlyphTimer, "#X obj 0 0 timer", "Timer", "Timer", OtherObject },
                { Icons::GlyphDelay, "#X obj 0 0 delay 1 60 permin", "Delay", "Delay", OtherObject },
            } },
        { "MIDI",
            {
                { Icons::GlyphMidiIn, "#X obj 0 0 midiin", "MIDI in", "MIDI in", OtherObject },
                { Icons::GlyphMidiOut, "#X obj 0 0 midiout", "MIDI out", "MIDI out", OtherObject },
                { Icons::GlyphNoteIn, "#X obj 0 0 notein", "Note in", "Note in", OtherObject },
                { Icons::GlyphNoteOut, "#X obj 0 0 noteout", "Note out", "Note out", OtherObject },
                { Icons::GlyphCtlIn, "#X obj 0 0 ctlin", "Control in", "Ctl in", OtherObject },
                { Icons::GlyphCtlOut, "#X obj 0 0 ctlout", "Control out", "Ctl out", OtherObject },
                { Icons::GlyphPgmIn, "#X obj 0 0 pgmin", "Program in", "Pgm in", OtherObject },
                { Icons::GlyphPgmOut, "#X obj 0 0 pgmout", "Program out", "Pgm out", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 touchin", "Touch in", "Tch in", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 touchout", "Touch out", "Tch in", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 polytouchin", "Poly Touch in", "Ptch in", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 polytouchout", "Poly Touch in", "Ptch out", OtherObject },
                { Icons::GlyphMtof, "#X obj 0 0 mtof", "MIDI to frequency", "mtof", OtherObject },
                { Icons::GlyphFtom, "#X obj 0 0 ftom", "Frequency to MIDI", "ftom", OtherObject },
            } },
        { "IO",
            {
                { Icons::GlyphAdc, "#X obj 0 0 adc~", "Adc", "Adc", OtherObject },
                { Icons::GlyphDac, "#X obj 0 0 dac~", "Dac", "Dac", OtherObject },
                { Icons::GlyphSend, "#X obj 0 0 s", "Send", "Send", OtherObject },
                { Icons::GlyphReceive, "#X obj 0 0 r", "Receive", "Receive", OtherObject },
                { Icons::GlyphSignalSend, "#X obj 0 0 s~", "Send~", "Send~", OtherObject },
                { Icons::GlyphSignalReceive, "#X obj 0 0 r~", "Receive~", "Receive~", OtherObject },
            } },
        { "Osc~",
            {
                { Icons::GlyphPhasor, "#X obj 0 0 phasor~", "Phasor", "Phasor", OtherObject },
                { Icons::GlyphOsc, "#X obj 0 0 osc~ 440", "Osc", "Osc", OtherObject },
                { Icons::GlyphOscBL, "#X obj 0 0 hv.osc~ sine", "Sine band limited", "Hv Sine", OtherObject },
                { Icons::GlyphSquareBL, "#X obj 0 0 hv.osc~ square", "Square band limited", "Hv Square", OtherObject },
                { Icons::GlyphSawBL, "#X obj 0 0 hv.osc~ saw", "Saw band limited", "Hv Saw", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.pinknoise~", "Pink Noise", "Pink Noise", OtherObject },
                { Icons::GlyphOsc, "#X obj 0 0 hv.lfo sine", "Sine LFO", "Sine LFO", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.lfo ramp", "Ramp LFO", "Ramp LFO", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.lfo saw", "Saw LFO", "Saw LFO", OtherObject },
                { Icons::GlyphTriangle, "#X obj 0 0 hv.lfo triangle", "Triangle LFO", "Tri LFO", OtherObject },
                { Icons::GlyphSquare, "#X obj 0 0 hv.lfo square", "Square LFO", "Sq LFO", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.lfo pulse", "Pulse LFO", "Pulse LFO", OtherObject },
            } },
        { "FX~",
            {
                { Icons::GlyphGeneric, "#X obj 0 0 hv.compressor~", "Compressor", "Compress", OtherObject },
                { Icons::GlyphFlanger, "#X obj 0 0 hv.flanger~", "Flanger", "Flanger", OtherObject },
                { Icons::GlyphCombRev, "#X obj 0 0 hv.comb~", "Comb filter", "Comb. Filt", OtherObject },
                { Icons::GlyphReverb, "#X obj 0 0 hv.reverb~", "Reverb", "Reverb", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.filter~ lowpass", "Lowpass Filter", "Lp Filter", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.filter~ bandpass1", "Bandpass Filter", "Bp Filter", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.filter~ highpass", "Highpass Filter", "Hp Filter", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.filter~ allpass", "Allpass Filter", "Ap Filter", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.hip~", "One-pole Highpass", "Hp Filter", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.lop~", "One-pole Lowpass", "Lp Filter", OtherObject },
                { Icons::GlyphGeneric, "#X obj 0 0 hv.freqshift~", "Frequency Shifter", "Freq Shift", OtherObject },
            } },
        { "Math",
            {
                { Icons::GlyphGeneric, "", "Add", "+", OtherObject },
                { Icons::GlyphGeneric, "", "Subtract", "-", OtherObject },
                { Icons::GlyphGeneric, "", "Multiply", "*", OtherObject },
                { Icons::GlyphGeneric, "", "Divide", "/", OtherObject },
                { Icons::GlyphGeneric, "", "Remainder", "%", OtherObject },
                { Icons::GlyphGeneric, "", "Greater than", ">", OtherObject },
                { Icons::GlyphGeneric, "", "Less than", "<", OtherObject },
                { Icons::GlyphGeneric, "", "Greater or equal", ">=", OtherObject },
                { Icons::GlyphGeneric, "", "Less or equal", "<=", OtherObject },
                { Icons::GlyphGeneric, "", "Equality", "==", OtherObject },
                { Icons::GlyphGeneric, "", "Not equal", "!=", OtherObject },
                { Icons::GlyphGeneric, "", "Minimum", "min", OtherObject },
                { Icons::GlyphGeneric, "", "Maximum", "max", OtherObject },
            } },
        { "Math~",
            {
                { Icons::GlyphGenericSignal, "", "(signal) Add", "+~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Subtract", "-~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Multiply", "*~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Divide", "/~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.gt~", "(signal) Greater than", ">~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.lt~", "(signal) Less than", "<~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.gte~", "(signal) Greater or equal", ">=~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.lte~", "(signal) Less or equal", "<=~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.eq~", "(signal) Equality", "==~", OtherObject },
                { Icons::GlyphGenericSignal, "#X obj 0 0 hv.neq~", "(signal) Not equal", "!=~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Minimum", "min~", OtherObject },
                { Icons::GlyphGenericSignal, "", "(signal) Maximum", "max~", OtherObject },
            } },
    };

private:
    PluginEditor* editor;
    std::function<void(bool)> dismissMenu;
    int const itemSize = 64;
};

class ObjectCategoryView : public Component {

public:
    ObjectCategoryView(PluginEditor* e, std::function<void(bool)> dismissCalloutBox)
        : list(e, dismissCalloutBox)
    {
        addAndMakeVisible(list);

        auto objectsToShow = getValue<bool>(SettingsFile::getInstance()->getPropertyAsValue("hvcc_mode")) ? ObjectList::heavyObjectList : ObjectList::defaultObjectList;

        // make the 2nd category active (which will be after the default category if it exists)
        if (objectsToShow.size() > 1) {
            // this should always be populated, but just incase we are testing a new blank object list etc
            list.showCategory(objectsToShow[1].first);
        }
        for (auto const& [categoryName, category] : objectsToShow) {
            if (categoryName == "Default")
                continue;

            auto* button = categories.add(new TextButton(categoryName));
            button->setConnectedEdges(12);
            button->onClick = [this, cName = categoryName]() {
                list.showCategory(cName);
                resized();
            };
            button->setClickingTogglesState(true);
            button->setRadioGroupId(hash("add_menu_category"));
            button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.035f));
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuBackgroundColourId).contrasting(0.075f));
            button->setColour(ComboBox::outlineColourId, Colours::transparentBlack);
            addAndMakeVisible(button);
        }

        if (categories.size() > 0) {
            categories.getFirst()->setConnectedEdges(Button::ConnectedOnRight);
            categories.getFirst()->setToggleState(true, dontSendNotification);
            categories.getLast()->setConnectedEdges(Button::ConnectedOnLeft);
        }
        resized();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto buttonBounds = bounds.removeFromTop(48).reduced(6, 14).translated(4, 0);

        auto buttonWidth = buttonBounds.getWidth() / std::max(1, categories.size());
        for (auto* category : categories) {
            category->setBounds(buttonBounds.removeFromLeft(buttonWidth).expanded(1, 0));
        }

        list.setBounds(bounds);
    }

private:
    ObjectList list;
    OwnedArray<TextButton> categories;
};

class AddObjectMenuButton : public Component {
    String const icon;
    String const text;

public:
    bool toggleState = false;
    bool clickingTogglesState = false;
    std::function<void(void)> onClick = []() {};

    explicit AddObjectMenuButton(String const& iconStr, String const& textStr = String())
        : icon(iconStr)
        , text(textStr)
    {
        setInterceptsMouseClicks(true, false);
        setAlwaysOnTop(true);
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().reduced(4, 2);

        auto colour = findColour(PlugDataColour::popupMenuTextColourId);

        if (isMouseOver()) {
            g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
            g.fillRoundedRectangle(b.toFloat(), Corners::defaultCornerRadius);
        }

        if (toggleState) {
            colour = findColour(PlugDataColour::toolbarActiveColourId);
        }

        auto iconArea = b.removeFromLeft(24).withSizeKeepingCentre(24, 24);

        if (text.isNotEmpty()) {
            Fonts::drawIcon(g, icon, iconArea.translated(3.0f, 0.0f), colour, 14.0f, true);
            b.removeFromLeft(4);
            b.removeFromRight(3);

            Fonts::drawFittedText(g, text, b, colour, 14.0f);
        } else {
            Fonts::drawIcon(g, icon, iconArea, colour, 14.0f, true);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if (clickingTogglesState) {
            toggleState = !toggleState;
        }

        onClick();
        repaint();
    }

    void mouseEnter(MouseEvent const& e) override
    {
        repaint();
    }

    void mouseExit(MouseEvent const& e) override
    {
        repaint();
    }
};

class AddObjectMenu : public Component {

public:
    explicit AddObjectMenu(PluginEditor* e)
        : objectBrowserButton(Icons::Object, "Show Object Browser")
        , pinButton(Icons::Pin)
        , editor(e)
        , objectList(e, [this](bool shouldFade) { dismiss(shouldFade); })
        , categoriesList(e, [this](bool shouldFade) { dismiss(shouldFade); })
    {
        categoriesList.setVisible(true);

        addAndMakeVisible(categoriesList);
        addAndMakeVisible(objectList);
        addAndMakeVisible(objectBrowserButton);
        addAndMakeVisible(pinButton);

        setSize(515, 300);

        objectList.showCategory("Default");
        objectBrowserButton.onClick = [this]() {
            if (currentCalloutBox)
                currentCalloutBox->dismiss();
            Dialogs::showObjectBrowserDialog(&editor->openedDialog, editor);
        };

        pinButton.toggleState = SettingsFile::getInstance()->getProperty<bool>("add_object_menu_pinned");
        pinButton.clickingTogglesState = true;

        pinButton.onClick = [this]() {
            SettingsFile::getInstance()->setProperty("add_object_menu_pinned", pinButton.toggleState);
        };
        pinButton.repaint();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();

        auto buttonsBounds = bounds.removeFromTop(26);
        pinButton.setBounds(buttonsBounds.removeFromRight(28));
        objectBrowserButton.setBounds(buttonsBounds);
        bounds.removeFromTop(6);
        objectList.setBounds(bounds.removeFromTop(75));
        categoriesList.setBounds(bounds);
    }

    void dismiss(bool shouldHide)
    {
        if (currentCalloutBox) {
            // If the panel is pinned, only fade it out
            if (pinButton.toggleState) {
                animator.animateComponent(currentCalloutBox, currentCalloutBox->getBounds(), shouldHide ? 0.1f : 1.0f, 300, false, 0.0f, 0.0f);
            }
            // Otherwise, fade the panel on drag start: calling dismiss or setVisible will lead to the drag event getting lost, so we just set alpha instead
            // Ditto for calling animator.fadeOut because that will also call setVisible(false)
            else if (shouldHide) {
                animator.animateComponent(currentCalloutBox, currentCalloutBox->getBounds(), 0.0f, 300, false, 0.0f, 0.0f);
            }
            // and destroy the panel on mouse-up
            else {
                currentCalloutBox->dismiss();
            }
        }
    }

    static void show(PluginEditor* editor, Rectangle<int> bounds)
    {
        auto addObjectMenu = std::make_unique<AddObjectMenu>(editor);
        currentCalloutBox = &editor->showCalloutBox(std::move(addObjectMenu), bounds);
    }

private:
    AddObjectMenuButton objectBrowserButton;
    AddObjectMenuButton pinButton;
    static inline SafePointer<CallOutBox> currentCalloutBox = nullptr;
    PluginEditor* editor;
    ObjectList objectList;
    ObjectCategoryView categoriesList;
    ComponentAnimator animator;
};
