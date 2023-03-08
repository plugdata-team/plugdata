/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include <JuceHeader.h>

struct Icons {
    inline static const String Open = "b";
    inline static const String Save = "c";
    inline static const String SaveAs = "d";
    inline static const String Undo = "e";
    inline static const String Redo = "f";
    inline static const String Add = "g";
    inline static const String Settings = "h";
    inline static const String Hide = "i";
    inline static const String Show = "i";
    inline static const String Clear = "k";
    inline static const String ClearLarge = "l";
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
    inline static const String Direction = "{";
    
    inline static const String SavePatch = "Z";
    inline static const String ClosePatch = "[";
    inline static const String CloseAllPatches = "]";
};

enum PlugDataColour {
    toolbarBackgroundColourId,
    toolbarTextColourId,
    toolbarActiveColourId,

    tabBackgroundColourId,
    tabTextColourId,
    activeTabBackgroundColourId,
    activeTabTextColourId,

    canvasBackgroundColourId,
    canvasTextColourId,
    canvasDotsColourId,

    guiObjectBackgroundColourId,
    textObjectBackgroundColourId,

    objectOutlineColourId,
    objectSelectedOutlineColourId,
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
    levelMeterInactiveColourId,
    levelMeterTrackColourId,
    levelMeterThumbColourId,

    panelBackgroundColourId,
    panelTextColourId,
    panelActiveBackgroundColourId,
    panelActiveTextColourId,

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


enum FontStyle {
    Regular,
    Bold,
    Semibold,
    Thin,
    Monospace,
};

struct Fonts {
    Fonts()
    {
        Typeface::setTypefaceCacheSize(7);

        // jassert(!instance);

        // Our unicode font is too big, the compiler will run out of memory
        // To prevent this, we split the BinaryData into multiple files, and add them back together here
        std::vector<char> interUnicode;
        int i = 0;
        while (true) {
            int size;
            auto* resource = BinaryData::getNamedResource((String("InterUnicode_") + String(i) + "_ttf").toRawUTF8(), size);

            if (!resource) {
                break;
            }

            interUnicode.insert(interUnicode.end(), resource, resource + size);
            i++;
        }

        // Initialise typefaces
        defaultTypeface = Typeface::createSystemTypefaceFor(interUnicode.data(), interUnicode.size());
        currentTypeface = defaultTypeface;

        thinTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterThin_ttf, BinaryData::InterThin_ttfSize);

        boldTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize);

        semiBoldTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize);

        iconTypeface = Typeface::createSystemTypefaceFor(BinaryData::IconFont_ttf, BinaryData::IconFont_ttfSize);

        monoTypeface = Typeface::createSystemTypefaceFor(BinaryData::IBMPlexMono_ttf, BinaryData::IBMPlexMono_ttfSize);

        instance = this;
    }

    static Font getCurrentFont() { return Font(instance->currentTypeface); }
    static Font getDefaultFont() { return Font(instance->defaultTypeface); }
    static Font getBoldFont() { return Font(instance->boldTypeface); }
    static Font getSemiBoldFont() { return Font(instance->semiBoldTypeface); }
    static Font getThinFont() { return Font(instance->thinTypeface); }
    static Font getIconFont() { return Font(instance->iconTypeface); }
    static Font getMonospaceFont() { return Font(instance->monoTypeface); }

    static Font setCurrentFont(Font font) { return instance->currentTypeface = font.getTypefacePtr(); }
    
    // For drawing icons with icon font
    static void drawIcon(Graphics& g, String const& icon, Rectangle<int> bounds, Colour colour, int fontHeight = -1, bool centred = true)
    {
        if (fontHeight < 0)
            fontHeight = bounds.getHeight() / 1.2f;

        auto justification = centred ? Justification::centred : Justification::centredLeft;
        g.setFont(Fonts::getIconFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(icon, bounds, justification, false);
    }

    static void drawIcon(Graphics& g, String const& icon, int x, int y, int size, Colour colour, int fontHeight = -1, bool centred = true)
    {
        drawIcon(g, icon, { x, y, size, size }, colour, fontHeight, centred);
    }

    // For drawing bold, semibold or thin text
    static void drawStyledText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, FontStyle style, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        Font font;
        switch (style) {
        case Regular:
            font = Fonts::getCurrentFont();
            break;
        case Bold:
            font = Fonts::getBoldFont();
            break;
        case Semibold:
            font = Fonts::getSemiBoldFont();
            break;
        case Thin:
            font = Fonts::getThinFont();
            break;
        case Monospace:
            font = Fonts::getMonospaceFont();
            break;
        }

        g.setFont(font.withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    static void drawStyledText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour colour, FontStyle style, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawStyledText(g, textToDraw, { x, y, w, h }, colour, style, fontHeight, justification);
    }

    // For drawing regular text
    static void drawText(Graphics& g, String const& textToDraw, Rectangle<float> bounds, Colour colour, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    // For drawing regular text
    static void drawText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    static void drawText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour colour, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawText(g, textToDraw, Rectangle<int>(x, y, w, h), colour, fontHeight, justification);
    }

    static void drawFittedText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, int numLines = 1, float minimumHoriontalScale = 1.0f, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawFittedText(textToDraw, bounds, justification, numLines, minimumHoriontalScale);
    }

    static void drawFittedText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour const& colour, int numLines = 1, float minimumHoriontalScale = 1.0f, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawFittedText(g, textToDraw, { x, y, w, h }, colour, numLines, minimumHoriontalScale, fontHeight, justification);
    }

private:
    // This is effectively a singleton because it's loaded through SharedResourcePointer
    static inline Fonts* instance = nullptr;

    // Default typeface is Inter combined with Unicode symbols from GoNotoUniversal and emojis from NotoEmoji
    Typeface::Ptr defaultTypeface;

    Typeface::Ptr currentTypeface;

    Typeface::Ptr thinTypeface;
    Typeface::Ptr boldTypeface;
    Typeface::Ptr semiBoldTypeface;
    Typeface::Ptr iconTypeface;
    Typeface::Ptr monoTypeface;
};

struct Corners
{
    inline static float const windowCornerRadius = 7.5f;
    inline static float const defaultCornerRadius = 6.0f;
    inline static float const smallCornerRadius = 4.0f;
    inline static float objectCornerRadius = 2.75f;
};


enum ParameterType {
    tString,
    tInt,
    tFloat,
    tColour,
    tBool,
    tCombo,
    tRange
};

enum ParameterCategory {
    cGeneral,
    cAppearance,
    cLabel,
    cExtra
};



using ObjectParameter = std::tuple<String, ParameterType, ParameterCategory, Value*, std::vector<String>>; // name, type and pointer to value, list of items only for combobox and bool

using ObjectParameters = std::vector<ObjectParameter>; // List of elements and update function
