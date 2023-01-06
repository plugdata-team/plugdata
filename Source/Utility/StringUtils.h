/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>

struct StringUtils {

    inline static constexpr uint64_t num_items = 1ul << (sizeof(char) * 8ul);

    std::array<float, num_items> widths;
    
    StringUtils(Font font)
    {
        for (int i = 0; i < num_items; i++) {
            widths[i] = font.getStringWidth(String(std::string(1, (char)i)));
        }
    }

    float getStringWidth(String text) const
    {
        float totalWidth = 0.0f;

        auto* utf8 = text.toRawUTF8();
        auto numBytes = text.getNumBytesAsUTF8();

        for (int i = 0; i < numBytes; i++) {
            totalWidth += widths[(int)utf8[i]];
        }
        
        // In real text, letters are slightly closer together
        return totalWidth;
    }
    
    // used by console for a more optimised calculation
    static int getNumLines(int width, int stringWidth)
    {
        // On startup, width might be zero, this is an optimisation for that case
        if (width == 0)
            return 0;

        return std::max<int>(round(static_cast<float>(stringWidth) / (width - 38.0f)), 1);
    }

    // Used by text objects for estimating best text height for a set width
    static int getNumLines(String const& text, int width, Font font = Font(Font::getDefaultSansSerifFontName(), 13, 0))
    {
        int numLines = 1;

        Array<int> glyphs;
        Array<float> xOffsets;
        font.getGlyphPositions(text, glyphs, xOffsets);

        for (int i = 0; i < xOffsets.size(); i++) {
            if ((xOffsets[i] + 12) >= static_cast<float>(width) || text.getCharPointer()[i] == '\n') {
                for (int j = i + 1; j < xOffsets.size(); j++) {
                    xOffsets.getReference(j) -= xOffsets[i];
                }
                numLines++;
            }
        }

        return numLines;
    }
};
