/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

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
        return totalWidth * 0.95f;
    }

    static float getPreciseStringWidth(String text, Font font)
    {
        float maxLineLength = 0;
        for (auto& line : StringArray::fromLines(text)) {
            maxLineLength = std::max(maxLineLength, font.getStringWidthFloat(line));
        }

        return maxLineLength;
    }

    // used by console for a more optimised calculation
    static int getNumLines(int width, int stringWidth)
    {
        // On startup, width might be zero, this is an optimisation for that case
        if (width == 0)
            return 0;

        return std::max<int>(round(static_cast<float>(stringWidth) / (width - 38.0f)), 1);
    }
};
