/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <JuceHeader.h>

struct FastStringWidth {

    inline static constexpr uint64_t num_items = 1ul << (sizeof(char) * 8ul);

    std::array<float, num_items> widths;

    FastStringWidth(Font font)
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
        return totalWidth * 0.80f;
    }

    /*
    // some tests
    const String testString = "The quick brown fox jumps over the lazy dog";
    void test(Font font) {
        std::cout << "accuracy:" << getStringWidth(testString) / font.getStringWidthFloat(testString) << std::endl;

    } */
};
