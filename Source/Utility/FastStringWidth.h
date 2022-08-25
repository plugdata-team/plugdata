#pragma once
#include <JuceHeader.h>

struct FastStringWidth {
    
    inline static constexpr uint64_t num_items = 1ul << (sizeof(char) * 8ul);
    
    std::array<float, num_items> widths;
    
    FastStringWidth(Font font)
    {
        for(char i = 0; i < num_items; i++)
        {
            widths[i] = font.getStringWidth(String::fromUTF8(&i, 1));
        }
    }
    
    float getStringWidth(String text) const {
        float totalWidth = 0.0f;
        
        auto* utf8 = text.toRawUTF8();
        auto numBytes = text.getNumBytesAsUTF8();
        
        for(int i = 0; i < numBytes; i++)
        {
            totalWidth += widths[(int)utf8[i]];
        }
        
        return totalWidth;
    }
};
