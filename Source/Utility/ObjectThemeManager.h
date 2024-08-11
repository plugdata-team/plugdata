/*
 // Copyright (c) 2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

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

    void updateTheme()
    {
        auto& lnf = LookAndFeel::getDefaultLookAndFeel();
        bg = lnf.findColour(PlugDataColour::guiObjectBackgroundColourId);
        fg = lnf.findColour(PlugDataColour::canvasTextColourId);
        lbl = lnf.findColour(PlugDataColour::toolbarTextColourId);
        ln = lnf.findColour(PlugDataColour::guiObjectInternalOutlineColour);
    }

    String getCompleteFormat(String& name)
    {
        StringArray token;
        token.add(name);
        formatObject(token);
        return String("#X obj 0 0 " + token.joinIntoString(" "));
    }

    void formatObject(StringArray& tokens)
    {
        // See if we have preset parameters for this object
        // These parameters are designed to make the experience in plugdata better
        // Mostly larger GUI objects and a different colour scheme
        if (guiDefaults.contains(tokens[0])) {

            auto colourToHex = [](Colour colour) {
                return String("#" + colour.toDisplayString(false));
            };

            auto colourToIEM = [](Colour colour) {
                return String(String(colour.getRed()) + " " + String(colour.getGreen()) + " " + String(colour.getBlue()));
            };

            auto preset = guiDefaults.at(tokens[0]);

            preset = preset.replace("@bgColour_rgb", colourToIEM(bg));
            preset = preset.replace("@fgColour_rgb", colourToIEM(fg));
            preset = preset.replace("@lblColour_rgb", colourToIEM(lbl));
            preset = preset.replace("@lnColour_rgb", colourToIEM(ln));

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
    static inline std::map<String, String> const guiDefaults = {
        // UI OBJECTS:
        { "bng", "25 250 50 0 empty empty empty 17 7 0 10 @bgColour @fgColour @lblColour" },
        { "tgl", "25 0 empty empty empty 17 7 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "button", "25 25 @bgColour_rgb @fgColour_rgb" },
        { "knob", "50 0 127 0 0 empty empty @bgColour @lnColour @fgColour 1 0 0 0 1 270 0 0" },
        { "vsl", "17 128 0 127 0 0 empty empty empty 0 -9 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "hsl", "128 17 0 127 0 0 empty empty empty -2 -8 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "vslider", "17 128 0 127 0 0 empty empty empty 0 -9 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "hslider", "128 17 0 127 0 0 empty empty empty -2 -8 0 10 @bgColour @fgColour @lblColour 0 1" },
        { "vradio", "20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @lblColour 0" },
        { "hradio", "20 1 0 8 empty empty empty 0 -8 0 10 @bgColour @fgColour @lblColour 0" },
        { "nbx", "4 18 -1e+37 1e+37 0 0 empty empty empty 0 -8 0 10 @bgColour @lblColour @lblColour 0 256" },
        { "cnv", "15 100 60 empty empty empty 20 12 0 14 @lnColour @lblColour" },
        { "function", "200 100 empty empty 0 1 @bgColour_rgb @lblColour_rgb 0 0 0 0 0 1000 0" },
        { "scope~", "200 100 256 3 128 -1 1 0 0 0 0 @fgColour_rgb @bgColour_rgb @lnColour_rgb 0 empty" },
        { "messbox", "180 60 @bgColour_rgb @lblColour_rgb 0 12" },
        { "vu", "20 120 empty empty -1 -8 0 10 @bgColour @lblColour 1 0" },

        // ADDITIONAL UI OBJECTS:
        { "floatatom", "5 0 0 0 - - - 12" },
        { "symbolatom", "5 0 0 0 - - - 12" },
        { "listbox", "9 0 0 0 - - - 0" },
        { "numbox~", "4 15 100 @bgColour @fgColour 10 0 0 0" },
        { "note", "0 14 Inter empty 0 @lblColour_rgb 0 @bgColour_rgb 0 0 note" }
    };
};
