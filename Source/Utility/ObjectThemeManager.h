/*
 // Copyright (c) 2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"

/*
 * This is a static class that handles all theming & formatting for UI objects placed onto the canvas
 */

class ObjectThemeManager {
public:
    static inline ObjectThemeManager* instance = nullptr;

    static ObjectThemeManager* get()
    {
        if (!instance) {
            instance = new ObjectThemeManager();
        }

        return instance;
    }

    static unsigned int normalise(Colour const& colour)
    {
        auto hex = colour.toString().substring(2);
        int col = (int)strtol(hex.toRawUTF8(), 0, 16);
        return col & 0xFFFFFF;
    }

    void updateTheme(pd::Instance* instance)
    {
        bg = PlugDataColours::guiObjectBackgroundColour;
        fg = PlugDataColours::canvasTextColour;
        lbl = PlugDataColours::commentTextColour;
        ln = PlugDataColours::guiObjectInternalOutlineColour;

        instance->setThis();
        instance->lockAudioThread();
        auto* gui = libpd_this_instance()->pd_gui;
        gui->i_foregroundcolor = normalise(fg);
        gui->i_backgroundcolor = normalise(bg);
        gui->i_selectcolor = normalise(ln);
        gui->i_gopcolor = normalise(PlugDataColours::graphAreaColour);
        instance->unlockAudioThread();
    }

    String getCompleteFormat(String const& name) const
    {
        StringArray token;
        token.add(name);
        formatObject(token);
        return String("#X obj 0 0 " + token.joinIntoString(" "));
    }

    void formatObject(StringArray& tokens) const
    {
        // See if we have preset parameters for this object
        // These parameters are designed to make the experience in plugdata better
        // Mostly larger GUI objects and a different colour scheme
        if (guiDefaults.contains(tokens[0]) && tokens.size() == 1) {

            auto colourToHex = [](Colour const colour) {
                return String("#" + colour.toDisplayString(false));
            };

            auto colourToRGB = [](Colour const colour) {
                return String(String(colour.getRed()) + " " + String(colour.getGreen()) + " " + String(colour.getBlue()));
            };

            auto preset = guiDefaults.at(tokens[0]);

            preset = preset.replace("@bgColour_rgb", colourToRGB(bg));
            preset = preset.replace("@fgColour_rgb", colourToRGB(fg));
            preset = preset.replace("@lblColour_rgb", colourToRGB(lbl));
            preset = preset.replace("@lnColour_rgb", colourToRGB(ln));

            preset = preset.replace("@bgColour", colourToHex(bg));
            preset = preset.replace("@fgColour", colourToHex(fg));
            preset = preset.replace("@lblColour", colourToHex(lbl));
            preset = preset.replace("@lnColour", colourToHex(ln));

            tokens.addTokens(preset, false);
        }
    }

private:
    Colour bg = Colour();
    Colour fg = Colour();
    Colour lbl = Colour();
    Colour ln = Colour();

    // Initialisation parameters for GUI objects
    // Taken from pd save files, this will make sure that it directly initialises objects with the right parameters
    static inline UnorderedMap<String, String> const guiDefaults = {
        { "bng", "25 250 50 0 empty empty empty 0 -10 0 10 @bgColour @fgColour @lblColour" },
        { "tgl", "25 0 empty empty empty 0 -10 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "toggle", "25 0 empty empty empty 0 -10 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "button", "25 25 @bgColour @fgColour" },
        { "knob", "50 0 127 0 0 empty empty @bgColour @lnColour @fgColour 1 0 0 0 1 270 0 0 0 empty empty 0 12 6 -15 0 1 0 0" },
        { "vsl", "17 128 0 127 0 0 empty empty empty 0 -9 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "hsl", "128 17 0 127 0 0 empty empty empty -2 -8 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "vslider", "17 128 0 127 0 0 empty empty empty 0 -9 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "hslider", "128 17 0 127 0 0 empty empty empty -2 -8 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "vradio", "20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @lblColour 0" },
        { "hradio", "20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @lblColour 0" },
        { "nbx", "4 18 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 @bgColour @fgColour @lblColour 0 256" },
        { "cnv", "15 100 60 empty empty empty 20 12 0 14 @lnColour @lblColour" },
        { "function", "200 100 empty empty 0 1 @bgColour_rgb @lblColour_rgb 0 0 0 0 0 1000 0" },
        { "scope~", "200 100 256 3 128 -1 1 0 0 0 0 @fgColour_rgb @bgColour_rgb @lnColour_rgb 0 empty" },
        { "keyboard", "16 80 4 2 0 0 empty empty" },
        { "messbox", "180 60 @bgColour_rgb @lblColour_rgb 0 12" },
        { "vu", "20 120 empty empty -1 -8 0 10 #404040 @lblColour 1 0" },
        { "popmenu", "128 26 12 @bgColour @fgColour \\  empty empty empty empty 1 0 -1 1 0 1 0 0 0 0 0" },
        { "floatbox", "5 0 0 0 - - - 12" },
        { "symbolbox", "5 0 0 0 - - - 12" },
        { "listbox", "9 0 0 0 - - - 0" },
        { "numbox~", "4 15 100 @bgColour @fgColour 10 0 0 0" },
        { "note", "0 14 Inter empty 0 @lblColour_rgb 0 @bgColour_rgb 0 0 note" }
    };
};
