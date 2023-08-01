/*
 // Copyright (c) 2023 Alex Mitchell and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Dialogs.h"

class ObjectItem : public Component, public SettableTooltipClient {
public:
    ObjectItem(PluginEditor* e, String const& text, String const& icon, String const& tooltip, String const& patch, std::function<void(bool)> dismissCalloutBox)
        :
        iconText(icon)
        , titleText(text)
        , objectPatch(patch)
        , dismissMenu(dismissCalloutBox)
        , editor(e)
    {
        setTooltip(tooltip);

        // glyph reflection

        switch(hash(iconText)){
        // default objects
        case hash("GlyphGeneric"):
            glyph = Icons::GlyphGeneric;
            break;
        case hash("GlyphGenericSignal"):
            glyph = Icons::GlyphGenericSignal;
            break;
        case hash("GlyphEmpty"):
            glyph = Icons::GlyphEmpty;
            break;
        case hash("GlyphMessage"):
            glyph = Icons::GlyphMessage;
            break;
        case hash("GlyphFloatBox"):
            glyph = Icons::GlyphFloatBox;
            break;
        case hash("GlyphSymbolBox"):
            glyph = Icons::GlyphSymbolBox;
            break;
        case hash("GlyphListBox"):
            glyph = Icons::GlyphListBox;
            break;
        case hash("GlyphComment"):
            glyph = Icons::GlyphComment;
            break;
        // UI objects
        case hash("GlyphBang"):
            glyph = Icons::GlyphBang;
            break;
        case hash("GlyphToggle"):
            glyph = Icons::GlyphToggle;
            break;
        case hash("GlyphButton"):
            glyph = Icons::GlyphButton;
            break;
        case hash("GlyphKnob"):
            glyph = Icons::GlyphKnob;
            break;
        case hash("GlyphNumber"):
            glyph = Icons::GlyphNumber;
            break;
        case hash("GlyphHSlider"):
            glyph = Icons::GlyphHSlider;
            break;
        case hash("GlyphVSlider"):
            glyph = Icons::GlyphVSlider;
            break;
        case hash("GlyphHRadio"):
            glyph = Icons::GlyphHRadio;
            break;
        case hash("GlyphVRadio"):
            glyph = Icons::GlyphVRadio;
            break;
        case hash("GlyphCanvas"):
            glyph = Icons::GlyphCanvas;
            break;
        case hash("GlyphKeyboard"):
            glyph = Icons::GlyphKeyboard;
            break;
        case hash("GlyphVUMeter"):
            glyph = Icons::GlyphVUMeter;
            break;
        case hash("GlyphArray"):
            glyph = Icons::GlyphArray;
            break;
        case hash("GlyphGOP"):
            glyph = Icons::GlyphGOP;
            break;
        case hash("GlyphOscilloscope"):
            glyph = Icons::GlyphOscilloscope;
            break;
        case hash("GlyphFunction"):
            glyph = Icons::GlyphFunction;
            break;
        case hash("GlyphMessbox"):
            glyph = Icons::GlyphMessbox;
            break;
        case hash("GlyphBicoeff"):
            glyph = Icons::GlyphBicoeff;
            break;
        // general
        case hash("GlyphMetro"):
            glyph = Icons::GlyphMetro;
            break;
        case hash("GlyphCounter"):
            glyph = Icons::GlyphCounter;
            break;
        case hash("GlyphSelect"):
            glyph = Icons::GlyphSelect;
            break;
        case hash("GlyphRoute"):
            glyph = Icons::GlyphRoute;
            break;
        case hash("GlyphExpr"):
            glyph = Icons::GlyphExpr;
            break;
        case hash("GlyphLoadbang"):
            glyph = Icons::GlyphLoadbang;
            break;
        case hash("GlyphPack"):
            glyph = Icons::GlyphPack;
            break;
        case hash("GlyphUnpack"):
            glyph = Icons::GlyphUnpack;
            break;
        case hash("GlyphPrint"):
            glyph = Icons::GlyphPrint;
            break;
        case hash("GlyphNetsend"):
            glyph = Icons::GlyphNetsend;
            break;
        case hash("GlyphNetreceive"):
            glyph = Icons::GlyphNetreceive;
            break;
        case hash("GlyphTimer"):
            glyph = Icons::GlyphTimer;
            break;
        case hash("GlyphDelay"):
            glyph = Icons::GlyphDelay;
            break;

        // MIDI:
        case hash("GlyphMidiIn"):
            glyph = Icons::GlyphMidiIn;
            break;
        case hash("GlyphMidiOut"):
            glyph = Icons::GlyphMidiOut;
            break;
        case hash("GlyphNoteIn"):
            glyph = Icons::GlyphNoteIn;
            break;
        case hash("GlyphNoteOut"):
            glyph = Icons::GlyphNoteOut;
            break;
        case hash("GlyphCtlIn"):
            glyph = Icons::GlyphCtlIn;
            break;
        case hash("GlyphCtlOut"):
            glyph = Icons::GlyphCtlOut;
            break;
        case hash("GlyphPgmIn"):
            glyph = Icons::GlyphPgmIn;
            break;
        case hash("GlyphPgmOut"):
            glyph = Icons::GlyphPgmOut;
            break;
        case hash("GlyphSysexIn"):
            glyph = Icons::GlyphSysexIn;
            break;
        case hash("GlyphSysexOut"):
            glyph = Icons::GlyphSysexOut;
            break;
        case hash("GlyphMtof"):
            glyph = Icons::GlyphMtof;
            break;
        case hash("GlyphFtom"):
            glyph = Icons::GlyphFtom;
            break;
        // IO~
        case hash("GlyphAdc"):
            glyph = Icons::GlyphAdc;
            break;
        case hash("GlyphDac"):
            glyph = Icons::GlyphDac;
            break;
        case hash("GlyphOut"):
            glyph = Icons::GlyphOut;
            break;
        case hash("GlyphBlocksize"):
            glyph = Icons::GlyphBlocksize;
            break;
        case hash("GlyphSamplerate"):
            glyph = Icons::GlyphSamplerate;
            break;
        case hash("GlyphSetDsp"):
            glyph = Icons::GlyphSetDsp;
            break;
        // OSC~
        case hash("GlyphOsc"):
            glyph = Icons::GlyphOsc;
            break;
        case hash("GlyphPhasor"):
            glyph = Icons::GlyphPhasor;
            break;
        case hash("GlyphSaw"):
            glyph = Icons::GlyphSaw;
            break;
        case hash("GlyphSaw2"):
            glyph = Icons::GlyphSaw2;
            break;
        case hash("GlyphSquare"):
            glyph = Icons::GlyphSquare;
            break;
        case hash("GlyphTriangle"):
            glyph = Icons::GlyphTriangle;
            break;
        case hash("GlyphImp"):
            glyph = Icons::GlyphImp;
            break;
        case hash("GlyphImp2"):
            glyph = Icons::GlyphImp2;
            break;
        case hash("GlyphWavetable"):
            glyph = Icons::GlyphWavetable;
            break;
        case hash("GlyphPlaits"):
            glyph = Icons::GlyphPlaits;
            break;
        case hash("GlyphOscBL"):
            glyph = Icons::GlyphOscBL;
            break;
        case hash("GlyphSawBL"):
            glyph = Icons::GlyphSawBL;
            break;
        case hash("GlyphSawBL2"):
            glyph = Icons::GlyphSawBL2;
            break;
        case hash("GlyphSquareBL"):
            glyph = Icons::GlyphSquareBL;
            break;
        case hash("GlyphTriBL"):
            glyph = Icons::GlyphTriBL;
            break;
        case hash("GlyphImpBL"):
            glyph = Icons::GlyphImpBL;
            break;
        case hash("GlyphImpBL2"):
            glyph = Icons::GlyphImpBL2;
            break;
        case hash("GlyphWavetableBL"):
            glyph = Icons::GlyphWavetableBL;
            break;
        // effects
        case hash("GlyphCrusher"):
            glyph = Icons::GlyphCrusher;
            break;
        case hash("GlyphDelayEffect"):
            glyph = Icons::GlyphDelayEffect;
            break;
        case hash("GlyphDrive"):
            glyph = Icons::GlyphDrive;
            break;
        case hash("GlyphFlanger"):
            glyph = Icons::GlyphFlanger;
            break;
        case hash("GlyphReverb"):
            glyph = Icons::GlyphReverb;
            break;
        case hash("GlyphFreeze"):
            glyph = Icons::GlyphFreeze;
            break;
        case hash("GlyphRingmod"):
            glyph = Icons::GlyphRingmod;
            break;

        default:
            glyph = String();
        }
    }

    void paint(Graphics& g) override
    {
        auto standardColour = findColour(PlugDataColour::textObjectBackgroundColourId);
        auto highlight = findColour(PlugDataColour::toolbarHoverColourId);
        
        auto iconBounds = getLocalBounds().reduced(16).translated(0, -7);
        auto textBounds = getLocalBounds().removeFromBottom(14);
        
        g.setColour(isHovering ? highlight : standardColour);
        g.fillRoundedRectangle(iconBounds.toFloat(), Corners::defaultCornerRadius);

        Fonts::drawText(g, titleText, textBounds, findColour(PlugDataColour::popupMenuTextColourId), 13.0f, Justification::centred);
        
        if (glyph.isEmpty()){
            Fonts::drawStyledText(g, iconText, iconBounds.reduced(2), findColour(PlugDataColour::popupMenuTextColourId), FontStyle::Regular, 18, Justification::centred);
        }
        else {
            Fonts::drawIcon(g, glyph, iconBounds.reduced(2), findColour(PlugDataColour::popupMenuTextColourId), 30);
        }
    }

    bool hitTest(int x, int y) override
    {
        return getLocalBounds().reduced(4).contains(x, y);
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

    void mouseDrag(MouseEvent const& e) override
    {
        auto* dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);
        
        if (dragContainer->isDragAndDropActive())
            return;
        
        auto patchWithTheme = substituteThemeColours(objectPatch);

        if (dragImage.image.isNull()) {
            auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
            dragImage = offlineObjectRenderer->patchToTempImage(patchWithTheme);
        }

        dismissMenu(true);
        
        Array<var> palettePatchWithOffset;
        palettePatchWithOffset.add(var(dragImage.offset.getX()));
        palettePatchWithOffset.add(var(dragImage.offset.getY()));
        palettePatchWithOffset.add(var(patchWithTheme));
        dragContainer->startDragging(palettePatchWithOffset, this, dragImage.image, true, nullptr, nullptr, true);
    }
    
    void mouseUp(MouseEvent const& e) override
    {
        dismissMenu(false);
    }

    void lookAndFeelChanged() override
    {
        dragImage.image = Image();
    }

    String substituteThemeColours(String patch)
    {
        auto colourToHex = [this](PlugDataColour colourEnum){
            auto colour = findColour(colourEnum);
            return String("#" + colour.toDisplayString(false));
        };

        auto colourToIEM = [this](PlugDataColour colourEnum){
            auto colour = findColour(colourEnum);
            return String(String(colour.getRed()) + " " + String(colour.getGreen()) + " " + String(colour.getBlue()));
        };

        String colouredObjects = patch;

        colouredObjects = colouredObjects.replace("@bgColour", colourToHex(PlugDataColour::guiObjectBackgroundColourId));
        colouredObjects = colouredObjects.replace("@fgColour", colourToHex(PlugDataColour::canvasTextColourId));
        // TODO: these both are the same, but should be different?
        colouredObjects = colouredObjects.replace("@arcColour", colourToHex(PlugDataColour::guiObjectInternalOutlineColour));
        colouredObjects = colouredObjects.replace("@canvasColour", colourToHex(PlugDataColour::guiObjectInternalOutlineColour));
        // TODO: these both are the same, but should be different?
        colouredObjects = colouredObjects.replace("@textColour", colourToHex(PlugDataColour::toolbarTextColourId));
        colouredObjects = colouredObjects.replace("@labelColour", colourToHex(PlugDataColour::toolbarTextColourId));

        colouredObjects = colouredObjects.replace("@iemBgColour", colourToIEM(PlugDataColour::guiObjectBackgroundColourId));
        colouredObjects = colouredObjects.replace("@iemFgColour", colourToIEM(PlugDataColour::canvasTextColourId));
        colouredObjects = colouredObjects.replace("@iemGridColour", colourToIEM(PlugDataColour::guiObjectInternalOutlineColour));

        return colouredObjects;
    }

    String getTitleText()
    {
        return titleText;
    }

private:
    String titleText;
    String iconText;
    String objectPatch;
    String glyph;
    ImageWithOffset dragImage;
    bool isHovering = false;
    std::function<void(bool)> dismissMenu;
    PluginEditor* editor;
};

class ObjectList : public Component {
public:
    ObjectList(PluginEditor* e, std::function<void(bool)> dismissCalloutBox)
        : editor(e), dismissMenu(dismissCalloutBox)
    {
    }
    
    void resized() override
    {
        auto width = getWidth();

        int column = 0;
        int row = 0;
        int maxColumns = width / itemSize;
        int offset = 0;
        
        for (int i = 0; i < objectButtons.size(); i++) {
            auto* button = objectButtons[i];
            button->setBounds(column * itemSize, offset, itemSize, itemSize);
            column++;
            if (column >= maxColumns) {
                column = 0;
                offset += itemSize;
            }
        }
    }

    int getContentHeight()
    {
        int maxColumns = std::max(6, getWidth() / itemSize);
        return (objectButtons.size() / maxColumns) * itemSize + (objectButtons.size() % maxColumns != 0) * itemSize;
    }

    void showCategory(const String& categoryToView)
    {
        objectButtons.clear();
        
        for (auto& [categoryName, objectCategory] : objectList) {
            
            if(categoryName != categoryToView) continue;
            
            for (auto& [icon, patch, tooltip, name] : objectCategory) {
                auto objectPatch = patch;
                if (objectPatch.isEmpty())
                    objectPatch = "#X obj 0 0 " + name;
                auto* button = objectButtons.add(new ObjectItem(editor, name, icon, tooltip, objectPatch, dismissMenu));
                addAndMakeVisible(button);
            }
        }
        
        resized();
    }

    OwnedArray<ObjectItem> objectButtons;

    static inline const std::vector<std::pair<String, std::vector<std::tuple<String, String, String, String>>>> objectList = {
        { "Default",
            {
                { "GlyphEmpty", "#X obj 0 0", "(ctrl+1) Empty object", "Empty" },
                { "GlyphMessage", "#X msg 0 0", "(ctrl+2) Message", "Message" },
                { "GlyphFloatBox", "#X floatatom 0 0", "(ctrl+3) Float box", "Float" },
                { "GlyphSymbolBox", "#X symbolatom 0 0", "Symbol box", "Symbol" },
                { "GlyphListBox", "#X listbox 0 0", "(ctrl+4) List box", "List" },
                { "GlyphComment", "#X text 0 0 comment", "(ctrl+5) Comment", "Comment" },
                { "GlyphArray", "#N canvas 0 0 450 250 (subpatch) 0;\n#X array array1 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 0 0 graph;", "(ctrl+shift+A) Array", "Array" },
                { "GlyphGOP", "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore 226 1 graph;", "(ctrl+shift+G) Graph on parent", "Graph" },
            }},
        { "UI",
            {
                { "GlyphNumber", "#X obj 0 0 nbx 4 18 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 @bgColour @fgColour @textColour 0 256", "(ctrl+shift+N) Number box", "Number" },
                { "GlyphBang", "#X obj 0 0 bng 25 250 50 0 empty empty empty 17 7 0 10 @bgColour @fgColour @labelColour", "(ctrl+shift+B) Bang", "Bang" },
                { "GlyphToggle", "#X obj 0 0 tgl 25 0 empty empty empty 17 7 0 10 @bgColour @fgColour @labelColour 0 1", "(ctrl+shift+T) Toggle", "Toggle" },
                { "GlyphButton", "#X obj 0 0 button 25 25 @iemBgColour @iemFgColour 0", "Button", "Button" },
                { "GlyphKnob", "#X obj 0 0 knob 50 0 127 0 0 empty empty @bgColour @arcColour @fgColour 1 0 0 0 1 270 0 0;", "Knob", "Knob" },
                { "GlyphVSlider", "#X obj 0 0 vsl 17 128 0 127 0 0 empty empty empty 0 -9 0 10 @bgColour @fgColour @labelColour 0 1", "(ctrl+shift+V) Vertical slider", "V. Slider" },
                { "GlyphHSlider", "#X obj 0 0 hsl 128 17 0 127 0 0 empty empty empty -2 -8 0 10 @bgColour @fgColour @labelColour 0 1", "(ctrl+shift+J) Horizontal slider", "H. Slider" },
                { "GlyphVRadio", "#X obj 0 0 vradio 20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @labelColour 0", "(ctrl+shift+D) Vertical radio box", "V. Radio" },
                { "GlyphHRadio", "#X obj 0 0 hradio 20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @labelColour 0", "(ctrl+shift+I) Horizontal radio box", "H. Radio" },
                { "GlyphCanvas", "#X obj 0 0 cnv 15 100 60 empty empty empty 20 12 0 14 @canvasColour @labelColour 0", "(ctrl+shift+C) Canvas", "Canvas" },
                { "GlyphKeyboard", "#X obj 0 0 keyboard 16 80 4 2 0 0 empty empty", "Piano keyboard", "Keyboard" },
                { "GlyphVUMeter", "#X obj 0 0 vu 20 120 empty empty -1 -8 0 10 #191919 @labelColour 1 0", "(ctrl+shift+U) VU meter", "VU Meter" },
                { "GlyphOscilloscope", "#X obj 0 0 oscope~ 130 130 256 3 128 -1 1 0 0 0 0 @iemFgColour @iemBgColour @iemGridColour 0 empty", "Oscilloscope", "Oscilloscope" },
                { "GlyphFunction", "#X obj 0 0 function 200 100 empty empty 0 1 @iemBgColour @iemFgColour 0 0 0 0 0 1000 0", "Function", "Function" },
                { "GlyphMessbox", "#X obj -0 0 messbox 180 60 @iemBgColour @iemFgColour 0 12", "ELSE Message box", "Messbox" },
                { "GlyphBicoeff", "#X obj 0 0 bicoeff 450 150 peaking", "Bicoeff generator", "Bicoeff" },
            }},
        { "General",
            {
                { "GlyphMetro", "#X obj 0 0 metro 500", "Metro", "Metro" },
                { "GlyphCounter", "#X obj 0 0 counter 0 5", "Counter", "Counter" },
                { "GlyphSelect", "#X obj 0 0 select", "Select", "Select" },
                { "GlyphRoute", "#X obj 0 0 route", "Route", "Route" },
                { "GlyphExpr", "#X obj 0 0 expr", "Expr", "Expr" },
                { "GlyphLoadbang", "#X obj 0 0 loadbang", "Loadbang", "Loadbang" },
                { "GlyphPack", "#X obj 0 0 pack", "Pack", "Pack" },
                { "GlyphUnpack", "#X obj 0 0 unpack", "Unpack", "Unpack" },
                { "GlyphPrint", "#X obj 0 0 print", "Print", "Print" },
                { "GlyphNetreceive", "#X obj 0 0 netreceive", "Netreceive", "Netreceive" },
                { "GlyphNetsend", "#X obj 0 0 netsend", "Netsend", "Netsend" },
                { "GlyphTimer", "#X obj 0 0 timer", "Timer", "Timer" },
                { "GlyphDelay", "#X obj 0 0 delay 1 60 permin", "Delay", "Delay" },
            }},
        { "MIDI",
            {
                { "GlyphMidiIn", "#X obj 0 0 midiin", "MIDI in", "MIDI in" },
                { "GlyphMidiOut", "#X obj 0 0 midiout", "MIDI out", "MIDI out" },
                { "GlyphNoteIn", "#X obj 0 0 notein", "Note in", "Note in" },
                { "GlyphNoteOut", "#X obj 0 0 noteout", "Note out", "Note out" },
                { "GlyphCtlIn", "#X obj 0 0 ctlin", "Control in", "Ctl in" },
                { "GlyphCtlOut", "#X obj 0 0 ctlout", "Control out", "Ctl out" },
                { "GlyphPgmIn", "#X obj 0 0 pgmin", "Program in", "Pgm in" },
                { "GlyphPgmOut", "#X obj 0 0 pgmout", "Program out", "Pgm out" },
                { "GlyphSysexIn", "#X obj 0 0 sysexin", "Sysex in", "Sysex in" },
                { "GlyphSysexOut", "#X obj 0 0 sysexout", "Sysex out", "Sysex out" },
                { "GlyphMtof", "#X obj 0 0 mtof", "MIDI to frequency", "MIDI to Freq." },
                { "GlyphFtom", "#X obj 0 0 ftom", "Frequency to MIDI", "Freq. to MIDI" },
            }},
        { "IO~",
            {
                { "GlyphAdc", "#X obj 0 0 adc~", "Adc", "Adc" },
                { "GlyphDac", "#X obj 0 0 dac~", "Dac", "Dac" },
                { "GlyphOut", "#X obj 0 0 out~", "Out", "Out" },
                { "GlyphBlocksize", "#X obj 0 0 blocksize~", "Blocksize", "Blocksize" },
                { "GlyphSamplerate", "#X obj 0 0 samplerate~", "Samplerate", "Samplerate" },
                { "GlyphSetDsp", "#X obj 0 0 setdsp~", "Setdsp", "Setdsp" },
            }},
        { "Osc~",
            {
                { "GlyphOsc", "#X obj 0 0 osc~ 440", "Osc", "Osc" },
                { "GlyphPhasor", "#X obj 0 0 phasor~", "Phasor", "Phasor" },
                { "GlyphSaw", "#X obj 0 0 saw~ 440", "Saw", "Saw" },
                { "GlyphSaw2", "#X obj 0 0 saw2~ 440", "Saw 2", "Saw 2" },
                { "GlyphSquare", "#X obj 0 0 square~", "Square", "Square" },
                { "GlyphTriangle", "#X obj 0 0 tri~ 440", "Triangle", "Triangle" },
                { "GlyphImp", "#X obj 0 0 imp~ 100", "Impulse", "Impulse" },
                { "GlyphImp2", "#X obj 0 0 imp2~ 100", "Impulse 2", "Impulse 2" },
                { "GlyphWavetable", "#X obj 0 0 wavetable~", "Wavetable", "Wavetable" },
                { "GlyphPlaits", "#X obj 0 0 plaits~ -model 0 440 0.25 0.5 0.1", "Plaits", "Plaits" },

                // band limited
                { "GlyphOscBL", "#X obj 0 0 bl.osc~ 440", "Osc band limited", "Bl. Osc" },
                { "GlyphSawBL", "#X obj 0 0 bl.saw~ 440", "Saw band limited", "Bl. Saw" },
                { "GlyphSawBL2", "#X obj 0 0 bl.saw2~", "Saw band limited 2", "Bl. Saw 2" },
                { "GlyphSquareBL", "#X obj 0 0 bl.tri~ 440", "Square band limited", "Bl. Square" },
                { "GlyphTriBL", "#X obj 0 0 bl.tri~ 100", "Triangle band limited", "Bl. Triangle" },
                { "GlyphImpBL", "#X obj 0 0 bl.imp~ 100", "Impulse band limited", "Bl. Impulse" },
                { "GlyphImpBL2", "#X obj 0 0 bl.imp2~", "Impulse band limited 2", "Bl. Impulse 2" },
                { "GlyphWavetableBL", "#X obj 0 0 bl.wavetable~", "Wavetable band limited", "Bl. Wavetab." },
            }},
        { "FX~",
            {
                { "GlyphCrusher", "#X obj 0 0 crusher~ 0.1 0.1", "Crusher", "Crusher" },
                { "GlyphDelayEffect", "#X obj 0 0 delay~ 22050 14700", "Delay", "Delay" },
                { "GlyphDrive", "#X obj 0 0 drive~", "Drive", "Drive" },
                { "GlyphFlanger", "#X obj 0 0 flanger~ 0.1 20 -0.6", "Flanger", "Flanger" },
                { "GlyphReverb", "#X obj 0 0 free.rev~ 0.7 0.6 0.5 0.7", "Reverb", "Reverb" },
                { "GlyphFreeze", "#X obj 0 0 freeze~", "Freeze", "Freeze" },
                { "GlyphRingmod", "#X obj 0 0 rm~ 150", "Ringmod", "Ringmod" },
            }},
        { "Math",
            {
                { "GlyphGeneric", "",  "Add", "+" },
                { "GlyphGeneric", "",  "Subtract", "-" },
                { "GlyphGeneric", "",  "Multiply", "*" },
                { "GlyphGeneric", "",  "Divide", "/" },
                { "GlyphGeneric", "",  "Remainder", "%" },
                { "GlyphGeneric", "", "Reversed inlet subtraction", "!-" },
                { "GlyphGeneric", "", "Reversed inlet division", "!/" },
                { "GlyphGeneric", "",  "Greater than", ">" },
                { "GlyphGeneric", "",  "Less than", "<" },
                { "GlyphGeneric", "", "Greater or equal", ">=" },
                { "GlyphGeneric", "", "Less or equal", "<=" },
                { "GlyphGeneric", "", "Equality", "==" },
                { "GlyphGeneric", "", "Not equal", "!=" },
            }},
        { "Math~",
            {
                { "GlyphGenericSignal", "",  "(signal) Add", "+~" },
                { "GlyphGenericSignal", "",  "(signal) Subtract", "-~" },
                { "GlyphGenericSignal", "",  "(signal) Multiply", "*~" },
                { "GlyphGenericSignal", "",  "(signal) Divide", "/~" },
                { "GlyphGenericSignal", "",  "(signal) Remainder", "%~" },
                { "GlyphGenericSignal", "", "(signal) Reversed inlet subtraction", "!-~" },
                { "GlyphGenericSignal", "", "(signal) Reversed inlet division", "!/~" },
                { "GlyphGenericSignal", "",  "(signal) Greater than", ">~" },
                { "GlyphGenericSignal", "",  "(signal) Less than", "<~" },
                { "GlyphGenericSignal", "", "(signal) Greater or equal", ">=~" },
                { "GlyphGenericSignal", "", "(signal) Less or equal", "<=~" },
                { "GlyphGenericSignal", "", "(signal) Equality", "==~" },
                { "GlyphGenericSignal", "", "(signal) Not equal", "!=~" },
            }},

    };
    
private:
    PluginEditor* editor;
    std::function<void(bool)> dismissMenu;
    const int itemSize = 68;
};


class ObjectCategoryView : public Component {
    
public:
    ObjectCategoryView(PluginEditor* e, std::function<void(bool)> dismissCalloutBox) : list(e, dismissCalloutBox)
    {
        viewport.setViewedComponent(&list, false);
        viewport.setScrollBarsShown(true, false, false, false);
        addAndMakeVisible(viewport);
        
        list.setVisible(true);
        list.showCategory("UI");
        
        for(const auto& [categoryName , category] : ObjectList::objectList)
        {
            if(categoryName == "Default") continue;
            
            auto* button = categories.add(new TextButton(categoryName));
            button->setConnectedEdges(12);
            button->onClick = [this, cName = categoryName](){
                list.showCategory(cName);
                resized();
            };
            button->setClickingTogglesState(true);
            button->setRadioGroupId(hash("add_menu_category"));
            button->setColour(TextButton::textColourOffId, findColour(PlugDataColour::popupMenuTextColourId));
            button->setColour(TextButton::textColourOnId, findColour(PlugDataColour::popupMenuActiveTextColourId));
                              
            button->setColour(TextButton::buttonOnColourId, findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
            button->setColour(TextButton::buttonColourId, findColour(PlugDataColour::popupMenuBackgroundColourId));
            addAndMakeVisible(button);
        }
        
        categories.getFirst()->setConnectedEdges(Button::ConnectedOnRight);
        categories.getFirst()->setToggleState(true, dontSendNotification);
        categories.getLast()->setConnectedEdges(Button::ConnectedOnLeft);
        resized();
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        auto buttonBounds = bounds.removeFromTop(48).reduced(8, 14);
        
        auto buttonWidth = buttonBounds.getWidth() / std::max(1, categories.size());
        int transX = 0;
        for(auto* category : categories)
        {
            category->setBounds(buttonBounds.removeFromLeft(buttonWidth).translated(transX--, 0));
        }
        
        viewport.setBounds(bounds);
        list.setBounds(bounds.withHeight(list.getContentHeight()));
    }
    
private:
    ObjectList list;
    OwnedArray<TextButton> categories;
    BouncingViewport viewport;
};

class AddObjectMenuButton : public Component {
    const String icon;
    const String text;


public:
    bool toggleState = false;
    bool clickingTogglesState = false;
    std::function<void(void)> onClick = []() {};

    AddObjectMenuButton(String iconStr, String textStr = String()) : icon(iconStr), text(textStr)
    {
        setInterceptsMouseClicks(true, false);
        setAlwaysOnTop(true);
    }

    void paint(Graphics& g) override
    {
        auto b = getLocalBounds();
        
        auto colour = findColour(PlugDataColour::popupMenuTextColourId);
        
        if (isMouseOver()) {
            g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
            PlugDataLook::fillSmoothedRectangle(g, Rectangle<float>(1, 1, getWidth() - 2, getHeight() - 2), Corners::defaultCornerRadius);
            colour = findColour(PlugDataColour::popupMenuActiveTextColourId);
        }
        
        if(toggleState)
        {
            colour = findColour(PlugDataColour::toolbarActiveColourId);
        }

        auto iconArea = b.removeFromLeft(24).withSizeKeepingCentre(24, 24);
        
        if(text.isNotEmpty()) {
            Fonts::drawIcon(g, icon, iconArea.translated(3.0f, 0.0f), colour, 14.0f, true);
            b.removeFromLeft(4);
            b.removeFromRight(3);
            
            Fonts::drawFittedText(g, text, b, colour, 14.0f);
        }
        else {
            Fonts::drawIcon(g, icon, iconArea, colour, 14.0f, true);
        }
    }

    void mouseUp(MouseEvent const& e) override
    {
        if(clickingTogglesState)
        {
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
    AddObjectMenu(PluginEditor* e)
    : editor(e),
    objectList(e, [this](bool shouldFade){ dismiss(shouldFade); }),
    categoriesList(e, [this](bool shouldFade){ dismiss(shouldFade); }),
    objectBrowserButton(Icons::Object, "Show Object Browser"),
    pinButton(Icons::Pin)
    {
        categoriesList.setVisible(true);
        
        addAndMakeVisible(categoriesList);
        addAndMakeVisible(objectList);
        addAndMakeVisible(objectBrowserButton);
        addAndMakeVisible(pinButton);
        
        setSize(450, 375);
        

        objectList.showCategory("Default");
        objectBrowserButton.onClick = [this](){
            if(currentCalloutBox) currentCalloutBox->dismiss();
            Dialogs::showObjectBrowserDialog(&editor->openedDialog, editor);
        };
        
        
        pinButton.toggleState = SettingsFile::getInstance()->getProperty<bool>("add_object_menu_pinned");
        pinButton.clickingTogglesState = true;
        
        pinButton.onClick = [this](){
            SettingsFile::getInstance()->setProperty("add_object_menu_pinned", pinButton.toggleState);
        };
        pinButton.repaint();
    }

    void paint(Graphics& g) override
    {
        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId).brighter(0.6f));
        PlugDataLook::fillSmoothedRectangle(g, objectList.getBounds().toFloat(), Corners::largeCornerRadius);
        PlugDataLook::fillSmoothedRectangle(g, categoriesList.getBounds().withTrimmedTop(48).toFloat(), Corners::largeCornerRadius);
    }
    
    void resized() override
    {
        auto bounds = getLocalBounds();
        
        auto buttonsBounds = bounds.removeFromTop(24);
        pinButton.setBounds(buttonsBounds.removeFromRight(24));
        objectBrowserButton.setBounds(buttonsBounds);
        bounds.removeFromTop(6);
        objectList.setBounds(bounds.removeFromTop(150));
        categoriesList.setBounds(bounds);
    }
    
    void dismiss(bool shouldHide)
    {
        if(currentCalloutBox) {
            // If the panel is pinned, only fade it out
            if(pinButton.toggleState)
            {
                animator.animateComponent(currentCalloutBox, currentCalloutBox->getBounds(), shouldHide ? 0.25f : 1.0f, 300, false, 0.0f, 0.0f);
            }
            // Otherwise, fade the panel on drag start: calling dismiss or setVisible will lead the the drag event getting lost, so we just set alpha instead
            // Ditto for calling animator.fadeOut because that will also call setVisible(false)
            else if(shouldHide) {
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
        currentCalloutBox = &CallOutBox::launchAsynchronously(std::move(addObjectMenu), bounds, editor);
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
