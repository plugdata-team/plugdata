#pragma once
#include <BinaryData.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "Utility/Config.h"

enum FontStyle {
    Regular,
    Bold,
    Semibold,
    Monospace,
    Tabular
};

struct Fonts {
    Fonts()
    {
        defaultTypeface = BinaryData::loadFont(BinaryData::InterVariable_ttf);
        monoTypeface = BinaryData::loadFont(BinaryData::RobotoMonoVariable_ttf);
        iconTypeface = BinaryData::loadFont(BinaryData::IconFont_ttf);

        currentTypeface = defaultTypeface;
        instance = this;

        // The monospace typeface never changes, so its weighted variant only needs preparing once
        monoBoldFont = getWeightedFont(monoTypeface, 700.0f);
        updateWeightedFonts();
    }


    static Font getCurrentFont() { return { FontOptions(instance->currentTypeface) }; }
    static Font getDefaultFont() { return { FontOptions(instance->defaultTypeface) }; }

    static Font getBoldFont() { return instance->boldFont; }
    static Font getSemiBoldFont() { return instance->semiBoldFont; }
    static Font getIconFont() { return { FontOptions(instance->iconTypeface) }; }
    static Font getMonospaceFont() { return { FontOptions(instance->monoTypeface) }; }
    static Font getMonospaceBoldFont() { return instance->monoBoldFont; }
    static Font getTabularNumbersFont() { return { FontOptions(instance->defaultTypeface).withFeatureEnabled(FontFeatureTag(1953396077u)) }; }

    static void setCurrentFont(Font const& font)
    {
        instance->currentTypeface = font.getTypefacePtr();
        instance->updateWeightedFonts();
    }

    static float getStringWidth(String text, Font font)
    {
        return GlyphArrangement().getStringWidth(font, text);
    }

    static float getStringWidth(String text, float fontSize)
    {
        return GlyphArrangement().getStringWidth(Font(FontOptions(fontSize)), text);
    }

    static float getStringWidth(String text)
    {
        return GlyphArrangement().getStringWidth(getDefaultFont(), text);
    }

    static int getStringWidthInt(String text, Font font)
    {
        return GlyphArrangement().getStringWidth(font, text);
    }

    static int getStringWidthInt(String text, float fontSize)
    {
        return GlyphArrangement().getStringWidth(Font(FontOptions(fontSize)), text);
    }

    static int getStringWidthInt(String text)
    {
        return GlyphArrangement().getStringWidth(getDefaultFont(), text);
    }

    static Array<File> getFontsInFolder(File const& patchFile)
    {
        return patchFile.findChildFiles(File::findFiles, false, "*.ttf;*.otf;");
    }

    template<typename T>
    static void drawText(T* llgc, String const& text, Rectangle<float> bounds, Font const& font, Colour colour, Justification just = Justification::centredLeft)
    {
        typename T::ScopedAnchoredDraw anchor(*llgc, Rectangle<float>());
        llgc->setFill(colour);
        llgc->setFont(font);
        llgc->drawText(text, bounds, just);
    }

    template<typename T>
    static void drawText(T* llgc, String const& text, Point<float> pos, Font const& font, Colour colour, Justification just = Justification::centredLeft)
    {
        typename T::ScopedAnchoredDraw anchor(*llgc, Rectangle<float>());
        llgc->setFill(colour);
        llgc->setFont(font);
        llgc->drawText(text, pos, just);
    }


    static std::optional<Font> findFont(File const& dirToSearch, String const& typefaceFileName)
    {
        Array<File> fontFiles = dirToSearch.getParentDirectory().findChildFiles(File::findFiles, false, "*.ttf;*.otf;");
        for (auto font : fontFiles) {
            if (font.getFileNameWithoutExtension() == typefaceFileName) {
                auto it = fontTable.find(font.getFullPathName());
                if (it != fontTable.end()) {
                    return it->second;
                }
                if (font.existsAsFile()) {
                    auto const fileStream = font.createInputStream();
                    if (fileStream == nullptr)
                        break;

                    MemoryBlock fontData;
                    fileStream->readIntoMemoryBlock(fontData);
                    auto typeface = Typeface::createSystemTypefaceFor(fontData.getData(), fontData.getSize());
                    fontTable[font.getFullPathName()] = Font(FontOptions(typeface));
                    return typeface;
                }
            }
        }

        return std::nullopt;
    }

    // For drawing icons with icon font
    static void drawIcon(Graphics& g, String const& icon, Rectangle<int> const bounds, Colour const colour, int fontHeight = -1, bool const centred = true)
    {
        if (fontHeight < 0)
            fontHeight = bounds.getHeight() / 1.2f;

        auto const justification = centred ? Justification::centred : Justification::centredLeft;
        auto const font = Fonts::getIconFont().withHeight(fontHeight);
        g.setFont(font);
        g.setColour(colour);
        g.drawText(icon, bounds, justification, false);
    }

    static void drawIcon(Graphics& g, String const& icon, int x, int y, int size, Colour const colour, int const fontHeight = -1, bool const centred = true)
    {
        drawIcon(g, icon, { x, y, size, size }, colour, fontHeight, centred);
    }

    static Font getFontFromStyle(FontStyle const style)
    {
        Font font = Font(FontOptions());
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
        case Monospace:
            font = Fonts::getMonospaceFont();
            break;
        case Tabular:
            font = Fonts::getTabularNumbersFont();
            break;
        }
        return font;
    }

    // For drawing bold, semibold or thin text
    static void drawStyledTextSetup(Graphics& g, Colour const colour, FontStyle const style, int const fontHeight = 15)
    {
        g.setFont(getFontFromStyle(style).withHeight(fontHeight));
        g.setColour(colour);
    }

    // rectangle float version
    static void drawStyledText(Graphics& g, String const& textToDraw, Rectangle<float> const bounds, Colour const colour, FontStyle const style, int const fontHeight = 15, Justification const justification = Justification::centredLeft)
    {
        drawStyledTextSetup(g, colour, style, fontHeight);
        g.drawText(textToDraw, bounds, justification);
    }

    // rectangle int version
    static void drawStyledText(Graphics& g, String const& textToDraw, Rectangle<int> const bounds, Colour const colour, FontStyle const style, int const fontHeight = 15, Justification const justification = Justification::centredLeft)
    {
        drawStyledTextSetup(g, colour, style, fontHeight);
        g.drawText(textToDraw, bounds, justification);
    }

    // int version
    static void drawStyledText(Graphics& g, String const& textToDraw, int const x, int const y, int const w, int const h, Colour const colour, FontStyle const style, int const fontHeight = 15, Justification const justification = Justification::centredLeft)
    {
        drawStyledTextSetup(g, colour, style, fontHeight);
        g.drawText(textToDraw, Rectangle<int>(x, y, w, h), justification);
    }

    // For drawing regular text
    static void drawText(Graphics& g, String const& textToDraw, Rectangle<float> const bounds, Colour const colour, int const fontHeight = 15, Justification const justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    // For drawing regular text
    static void drawText(Graphics& g, String const& textToDraw, Rectangle<int> const bounds, Colour const colour, int const fontHeight = 15, Justification const justification = Justification::centredLeft)
    {
        g.setFont(Fonts::getCurrentFont().withHeight(fontHeight));
        g.setColour(colour);
        g.drawText(textToDraw, bounds, justification);
    }

    static void drawText(Graphics& g, String const& textToDraw, int const x, int const y, int const w, int const h, Colour const colour, int const fontHeight = 15, Justification const justification = Justification::centredLeft)
    {
        drawText(g, textToDraw, Rectangle<int>(x, y, w, h), colour, fontHeight, justification);
    }

    static void drawFittedText(Graphics& g, String const& textToDraw, Rectangle<int> const bounds, Colour const colour, int const numLines = 1, float const minimumHoriontalScale = 1.0f, float const fontHeight = 15.0f, Justification const justification = Justification::centredLeft, FontStyle const style = FontStyle::Regular)
    {
        g.setFont(getFontFromStyle(style).withHeight(fontHeight));
        g.setColour(colour);
        g.drawFittedText(textToDraw, bounds, justification, numLines, minimumHoriontalScale);
    }

    static void drawFittedText(Graphics& g, String const& textToDraw, int x, int y, int w, int h, Colour const& colour, int const numLines = 1, float const minimumHoriontalScale = 1.0f, int const fontHeight = 15, Justification const justification = Justification::centredLeft)
    {
        drawFittedText(g, textToDraw, { x, y, w, h }, colour, numLines, minimumHoriontalScale, fontHeight, justification);
    }

private:
    static bool supportsWeightVariable(Typeface::Ptr const& typeface)
    {
        if (typeface == nullptr)
            return false;

        constexpr auto weightTag = FontFeatureTag("wght");

        for (auto variable : typeface->getSupportedVariables()) {
            if (variable == weightTag)
                return true;
        }

        return false;
    }

    // Re-prepare the weighted variants of the current typeface, so we don't have to clone on every request
    void updateWeightedFonts()
    {
        boldFont = getWeightedFont(currentTypeface, 700.0f);
        semiBoldFont = getWeightedFont(currentTypeface, 600.0f);
    }

    static Font getWeightedFont(Typeface::Ptr const& typeface, float const weight)
    {
        if (typeface == nullptr)
            return Font(FontOptions()).boldened();

        Font font { FontOptions(typeface) };

        if (!supportsWeightVariable(typeface))
            return font.boldened();

        FontVariableSetting const settings[] { { "wght", weight } };

        if (auto weightedTypeface = typeface->cloneWithVariableSettings(settings); weightedTypeface != nullptr)
            return { FontOptions(weightedTypeface) };

        return font.boldened();
    }

    // This is effectively a singleton because it's loaded through SharedResourcePointer
    static inline Fonts* instance = nullptr;

    // Default typeface is Inter combined with Unicode symbols from GoNotoUniversal and emojis from NotoEmoji
    Typeface::Ptr defaultTypeface;
    Typeface::Ptr currentTypeface;
    Typeface::Ptr iconTypeface;
    Typeface::Ptr monoTypeface;

    // Weighted fonts prepared in advance, refreshed whenever the current font changes
    Font boldFont { FontOptions() };
    Font semiBoldFont { FontOptions() };
    Font monoBoldFont { FontOptions() };

    static inline auto fontTable = UnorderedMap<String, Font>();
};
