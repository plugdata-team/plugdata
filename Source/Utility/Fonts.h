#pragma once
#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "Utility/Config.h"

enum FontStyle {
    Regular,
    Bold,
    Semibold,
    Thin,
    Monospace,
    Variable,
    Tabular
};

struct Fonts {
    Fonts()
    {
        Typeface::setTypefaceCacheSize(7);

        // Our unicode font is too big, the compiler will run out of memory
        // To prevent this, we split the BinaryData into multiple files, and add them back together here
        std::vector<char> interUnicode;
        interUnicode.reserve(17 * 1024 * 1024); // Reserve 17mb to prevent more allocations
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
        monoTypeface = Typeface::createSystemTypefaceFor(BinaryData::RobotoMonoRegular_ttf, BinaryData::RobotoMonoRegular_ttfSize);
        variableTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterVariable_ttf, BinaryData::InterVariable_ttfSize);
        tabularTypeface = Typeface::createSystemTypefaceFor(BinaryData::InterTabular_ttf, BinaryData::InterTabular_ttfSize);

        instance = this;
    }

    static Font getCurrentFont() { return Font(instance->currentTypeface); }
    static Font getDefaultFont() { return Font(instance->defaultTypeface); }
    static Font getBoldFont() { return Font(instance->boldTypeface); }
    static Font getSemiBoldFont() { return Font(instance->semiBoldTypeface); }
    static Font getThinFont() { return Font(instance->thinTypeface); }
    static Font getIconFont() { return Font(instance->iconTypeface); }
    static Font getMonospaceFont() { return Font(instance->monoTypeface); }
    static Font getVariableFont() { return Font(instance->variableTypeface); }
    static Font getTabularNumbersFont() { return Font(instance->tabularTypeface); }

    static Font setCurrentFont(Font const& font) { return instance->currentTypeface = font.getTypefacePtr(); }

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

    static Font getFontFromStyle(FontStyle style)
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
        case Variable:
            font = Fonts::getVariableFont();
            break;
        case Tabular:
            font = Fonts::getTabularNumbersFont();
            break;
        }
        return font;
    }

    // For drawing bold, semibold or thin text
    static void drawStyledTextSetup(Graphics& g, Colour colour, FontStyle style, int fontHeight = 15)
    {
        g.setFont(getFontFromStyle(style).withHeight(fontHeight));
        g.setColour(colour);
    }

    // rectangle float version
    static void drawStyledText(Graphics& g, String const& textToDraw, Rectangle<float> bounds, Colour colour, FontStyle style, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawStyledTextSetup(g, colour, style, fontHeight);
        g.drawText(textToDraw, bounds, justification);
    }

    // rectangle int version
    static void drawStyledText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, FontStyle style, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawStyledTextSetup(g, colour, style, fontHeight);
        g.drawText(textToDraw, bounds, justification);
    }

    // int version
    static void drawStyledText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour colour, FontStyle style, int fontHeight = 15, Justification justification = Justification::centredLeft)
    {
        drawStyledTextSetup(g, colour, style, fontHeight);
        g.drawText(textToDraw, Rectangle<int>(x, y, w, h), justification);
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

    static void drawFittedText(Graphics& g, String const& textToDraw, Rectangle<int> bounds, Colour colour, int numLines = 1, float minimumHoriontalScale = 1.0f, float fontHeight = 15.0f, Justification justification = Justification::centredLeft, FontStyle style = FontStyle::Regular)
    {
        g.setFont(getFontFromStyle(style).withHeight(fontHeight));
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
    Typeface::Ptr variableTypeface;
    Typeface::Ptr tabularTypeface;
};
