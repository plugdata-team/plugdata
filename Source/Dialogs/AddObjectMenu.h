/*
 // Copyright (c) 2023 Alex Mitchell and Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include "Dialogs.h"

class ObjectItem : public Component, public SettableTooltipClient {
public:
    ObjectItem(PluginEditor* e, String const& text, String const& icon, String const& tooltip, String const& patch, ObjectIDs objectID, std::function<void(bool)> dismissCalloutBox)
        : iconText(icon)
        , titleText(text)
        , objectPatch(patch)
        , dismissMenu(dismissCalloutBox)
        , editor(e)
    {
        setTooltip(tooltip.replace("(@keypress) ", getKeyboardShortcutDescription(objectID)));
    }
    
    String getKeyboardShortcutDescription(ObjectIDs objectID)
    {
        auto keyPresses = editor->getKeyMappings()->getKeyPressesAssignedToCommand(objectID);
        if(keyPresses.size())
        {
            return "(" + keyPresses.getReference(0).getTextDescription() + ") ";
        }
        
        return String();
       
    }
    
    void paint(Graphics& g) override
    {
        auto highlight = findColour(PlugDataColour::popupMenuActiveBackgroundColourId);
        
        auto iconBounds = getLocalBounds().reduced(14).translated(0, -7);
        auto textBounds = getLocalBounds().removeFromBottom(14);
        
        if(isHovering)
        {
            g.setColour(highlight);
            PlugDataLook::fillSmoothedRectangle(g, iconBounds.toFloat(), Corners::defaultCornerRadius);
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

    void showCategory(String const& categoryToView)
    {
        objectButtons.clear();
        
        for (auto& [categoryName, objectCategory] : objectList) {
            
            if(categoryName != categoryToView) continue;
            
            for (auto& [icon, patch, tooltip, name, objectID] : objectCategory) {
                auto objectPatch = patch;
                if (objectPatch.isEmpty())
                    objectPatch = "#X obj 0 0 " + name;
                auto* button = objectButtons.add(new ObjectItem(editor, name, icon, tooltip, objectPatch, objectID, dismissMenu));
                addAndMakeVisible(button);
            }
        }
        
        resized();
    }
    
    OwnedArray<ObjectItem> objectButtons;
    

    static inline const std::vector<std::pair<String, std::vector<std::tuple<String, String, String, String, ObjectIDs>>>> objectList = {
        { "Default",
            {
                { Icons::GlyphEmpty, "#X obj 0 0", "(@keypress) Empty object", "Empty", NewObject},
                { Icons::GlyphMessage, "#X msg 0 0, f 9", "(@keypress) Message", "Message" , NewMessage},
                { Icons::GlyphFloatBox, "#X floatatom 0 0 9 0 0 0 - - - 12", "(@keypress) Float box", "Float" , NewFloatAtom},
                { Icons::GlyphSymbolBox, "#X symbolatom 0 0 9 0 0 0 - - - 12", "Symbol box", "Symbol" , NewSymbolAtom},
                { Icons::GlyphListBox, "#X listbox 0 0 9 0 0 0 - - - 12", "(@keypress) List box", "List" , NewListAtom},
                { Icons::GlyphComment, "#X text 0 0 comment", "(@keypress) Comment", "Comment" , NewComment},
                { Icons::GlyphArray, "#N canvas 0 0 450 250 (subpatch) 0;\n#X array array1 100 float 2;\n#X coords 0 1 100 -1 200 140 1;\n#X restore 0 0 graph;", "(@keypress) Array", "Array" , NewArray},
                { Icons::GlyphGOP, "#N canvas 0 0 450 250 (subpatch) 1;\n#X coords 0 1 100 -1 200 140 1 0 0;\n#X restore 226 1 graph;", "(@keypress) Graph on parent", "Graph" , NewGraphOnParent},
            }
        },
            { "UI",
                {
                    { Icons::GlyphNumber, "#X obj 0 0 nbx 4 18 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 @bgColour @fgColour @textColour 0 256", "(@keypress) Number box", "Number" , NewNumbox},
                    { Icons::GlyphBang, "#X obj 0 0 bng 25 250 50 0 empty empty empty 17 7 0 10 @bgColour @fgColour @labelColour", "(@keypress) Bang", "Bang" , NewBang},
                    { Icons::GlyphToggle, "#X obj 0 0 tgl 25 0 empty empty empty 17 7 0 10 @bgColour @fgColour @labelColour 0 1", "(@keypress) Toggle", "Toggle" , NewToggle},
                    { Icons::GlyphButton, "#X obj 0 0 button 25 25 @iemBgColour @iemFgColour 0", "Button", "Button" , OtherObject},
                    { Icons::GlyphKnob, "#X obj 0 0 knob 50 0 127 0 0 empty empty @bgColour @arcColour @fgColour 1 0 0 0 1 270 0 0;", "Knob", "Knob" , OtherObject},
                    { Icons::GlyphVSlider, "#X obj 0 0 vsl 17 128 0 127 0 0 empty empty empty 0 -9 0 10 @bgColour @fgColour @labelColour 0 1", "(@keypress) Vertical slider", "V. Slider" , NewVerticalSlider},
                    { Icons::GlyphHSlider, "#X obj 0 0 hsl 128 17 0 127 0 0 empty empty empty -2 -8 0 10 @bgColour @fgColour @labelColour 0 1", "(@keypress) Horizontal slider", "H. Slider" , NewHorizontalSlider},
                    { Icons::GlyphVRadio, "#X obj 0 0 vradio 20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @labelColour 0", "(@keypress) Vertical radio box", "V. Radio" , NewVerticalRadio},
                    { Icons::GlyphHRadio, "#X obj 0 0 hradio 20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @labelColour 0", "(@keypress) Horizontal radio box", "H. Radio" , NewHorizontalRadio},
                    { Icons::GlyphCanvas, "#X obj 0 0 cnv 15 100 60 empty empty empty 20 12 0 14 @canvasColour @labelColour 0", "(@keypress) Canvas", "Canvas" , NewCanvas},
                    { Icons::GlyphKeyboard, "#X obj 0 0 keyboard 16 80 4 2 0 0 empty empty", "Piano keyboard", "Keyboard" , OtherObject},
                    { Icons::GlyphVUMeter, "#X obj 0 0 vu 20 120 empty empty -1 -8 0 10 #191919 @labelColour 1 0", "(@keypress) VU meter", "VU Meter" , NewVUMeterObject},
                    { Icons::GlyphOscilloscope, "#X obj 0 0 oscope~ 130 130 256 3 128 -1 1 0 0 0 0 @iemFgColour @iemBgColour @iemGridColour 0 empty", "Oscilloscope", "Scope" , OtherObject},
                    { Icons::GlyphFunction, "#X obj 0 0 function 200 100 empty empty 0 1 @iemBgColour @iemFgColour 0 0 0 0 0 1000 0", "Function", "Function" , OtherObject},
                    { Icons::GlyphMessbox, "#X obj -0 0 messbox 180 60 @iemBgColour @iemFgColour 0 12", "ELSE Message box", "Messbox" , OtherObject},
                    { Icons::GlyphBicoeff, "#X obj 0 0 bicoeff 450 150 peaking", "Bicoeff generator", "Bicoeff" , OtherObject},
                } },
            { "General",
                {
                    { Icons::GlyphMetro, "#X obj 0 0 metro 500", "Metro", "Metro", OtherObject },
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
                    { Icons::GlyphGeneric, "#X obj 0 0 autotune", "Pitch quantizer", "Autotune", OtherObject },
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
                    { Icons::GlyphGeneric, "#X obj 0 0 s", "Send", "Send", OtherObject },
                    { Icons::GlyphGeneric, "#X obj 0 0 r", "Receive", "Receive", OtherObject },
                    { Icons::GlyphGeneric, "#X obj 0 0 s~", "Send~", "Send~", OtherObject },
                    { Icons::GlyphGeneric, "#X obj 0 0 r~", "Receive~", "Receive~", OtherObject },
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
                    { Icons::GlyphPlaits, "#X obj 0 0 plaits~ -model 0 440 0.25 0.5 0.1", "Plaits", "Plaits", OtherObject },
                } },
            { "FX~",
                {
                    { Icons::GlyphCrusher, "#X obj 0 0 crusher~ 0.1 0.1", "Crusher", "Crusher", OtherObject },
                    { Icons::GlyphDelayEffect, "#X obj 0 0 delay~ 22050 14700", "Delay", "Delay", OtherObject },
                    { Icons::GlyphDrive, "#X obj 0 0 drive~", "Drive", "Drive", OtherObject },
                    { Icons::GlyphFlanger, "#X obj 0 0 flanger~ 0.1 20 -0.6", "Flanger", "Flanger", OtherObject },
                    { Icons::GlyphReverb, "#X obj 0 0 free.rev~ 0.7 0.6 0.5 0.7", "Reverb", "Reverb", OtherObject },
                    { Icons::GlyphFreeze, "#X obj 0 0 freeze~", "Freeze", "Freeze", OtherObject },
                    { Icons::GlyphRingmod, "#X obj 0 0 rm~ 150", "Ringmod", "Ringmod", OtherObject },
                    { Icons::GlyphSVFilter, "#X obj 0 0 svfilter~ 1729 0.42", "State variable filter", "SVFilter", OtherObject },
                    { Icons::GlyphClip, "#X obj 0 0 clip~ -0.5 0.5", "Clip", "Clip", OtherObject },
                    { Icons::GlyphFold, "#X obj 0 0 fold~ -0.5 0.5", "Fold", "Fold", OtherObject },
                    { Icons::GlyphWrap, "#X obj 0 0 wrap2~ -0.5 0.5", "Wrap", "Wrap", OtherObject },
                }},
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

private:
    PluginEditor* editor;
    std::function<void(bool)> dismissMenu;
    int const itemSize = 64;
};


class ObjectCategoryView : public Component {
    
public:
    ObjectCategoryView(PluginEditor* e, std::function<void(bool)> dismissCalloutBox) : list(e, dismissCalloutBox)
    {
        addAndMakeVisible(list);
        list.showCategory("UI");

        for (auto const& [categoryName, category] : ObjectList::objectList) {
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
        auto buttonBounds = bounds.removeFromTop(48).reduced(6, 14);
        
        auto buttonWidth = buttonBounds.getWidth() / std::max(1, categories.size());
        int transX = 0;
        for(auto* category : categories)
        {
            category->setBounds(buttonBounds.removeFromLeft(buttonWidth).expanded(1, 0));
        }
        
        list.setBounds(bounds);
    }
    
private:
    ObjectList list;
    OwnedArray<TextButton> categories;
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
        auto b = getLocalBounds().reduced(4, 2);
        
        auto colour = findColour(PlugDataColour::popupMenuTextColourId);
        
        if (isMouseOver()) {
            g.setColour(findColour(PlugDataColour::popupMenuActiveBackgroundColourId));
            PlugDataLook::fillSmoothedRectangle(g, b.toFloat(), Corners::defaultCornerRadius);
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
        
        setSize(515, 300);
        
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
        /*
         This will draw a rounded rect background below the icons, I'm not entirely decided on this yet:
         On one hand, it adds hierarchy to the menu, which is nice. But at the same time, it also makes it busier
        */
        /*
        auto backgroundColour = findColour(PlugDataColour::popupMenuBackgroundColourId);
        auto foregroundBrightness = backgroundColour.getBrightness() + 0.03f;
        g.setColour(findColour(PlugDataColour::popupMenuBackgroundColourId).withBrightness(foregroundBrightness));
        
        auto objectListRectBounds = objectList.getBounds().reduced(4, 0).toFloat();
        auto categoriesListRectBounds = categoriesList.getBounds().withTrimmedTop(48).reduced(4, 0).withTrimmedBottom(4).toFloat();
        
        PlugDataLook::fillSmoothedRectangle(g, objectListRectBounds, Corners::largeCornerRadius);
        PlugDataLook::fillSmoothedRectangle(g, categoriesListRectBounds, Corners::largeCornerRadius);
        
        g.setColour(findColour(PlugDataColour::toolbarOutlineColourId));
        PlugDataLook::drawSmoothedRectangle(g, PathStrokeType(1.0f), objectListRectBounds, Corners::largeCornerRadius);
        PlugDataLook::drawSmoothedRectangle(g, PathStrokeType(1.0f), categoriesListRectBounds, Corners::largeCornerRadius); */
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
        if(currentCalloutBox) {
            // If the panel is pinned, only fade it out
            if(pinButton.toggleState) {
                animator.animateComponent(currentCalloutBox, currentCalloutBox->getBounds(), shouldHide ? 0.1f : 1.0f, 300, false, 0.0f, 0.0f);
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
        currentCalloutBox->setColour(PlugDataColour::popupMenuBackgroundColourId, currentCalloutBox->findColour(PlugDataColour::popupMenuBackgroundColourId));
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
