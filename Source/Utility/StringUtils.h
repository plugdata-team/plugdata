/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

struct StringUtils {

    inline static constexpr uint64_t num_items = 1ul << (sizeof(char) * 8ul);

    std::array<float, num_items> widths {};

    explicit StringUtils(Font const& font)
    {
        for (int i = 0; i < num_items; i++) {
            widths[i] = font.getStringWidth(String(std::string(1, (char)i)));
        }
    }

    float getStringWidth(String const& text) const
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

    static float getPreciseStringWidth(String const& text, Font const& font)
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

template<int FontSize>
struct CachedStringWidth {
    
    static int calculateSingleLineWidth(const String& singleLine)
    {
        auto stringHash = hash(singleLine);
        
        auto cacheHit = stringWidthCache.find(stringHash);
        if(cacheHit != stringWidthCache.end()) return cacheHit->second;
        
        auto stringWidth = Font(FontSize).getStringWidth(singleLine);
        stringWidthCache[stringHash] = stringWidth;
        
        return stringWidth;
    }
    
    static int calculateStringWidth(const String& string)
    {
        int maximumLineWidth = 7;
        for(auto line : StringArray::fromLines(string))
        {
            maximumLineWidth = std::max(calculateSingleLineWidth(line), maximumLineWidth);
        }
        
        return maximumLineWidth;
    }


    static inline std::unordered_map<hash32, int> stringWidthCache = std::unordered_map<hash32, int>();
};

struct CachedFontStringWidth : public DeletedAtShutdown
{
    float calculateSingleLineWidth(Font const& font, const String& singleLine)
    {
        auto stringHash = hash(singleLine);
        
        for(auto& [cachedFont, cache] : stringWidthCache)
        {
            if(cachedFont == font)
            {
                auto cacheHit = cache.find(stringHash);
                if(cacheHit != cache.end()) 
                    return cacheHit->second;
                
                auto stringWidth = font.getStringWidthFloat(singleLine);
                cache[stringHash] = stringWidth;
                
                return stringWidth;
            }
        }
        
        auto stringWidth = font.getStringWidth(singleLine);
        stringWidthCache.push_back({font, {{stringHash, stringWidth}}});
        return stringWidth;
    }
    
    int calculateStringWidth(Font const& font, const String& string)
    {
        float maximumLineWidth = 7;
        for(auto line : StringArray::fromLines(string))
        {
            maximumLineWidth = std::max(calculateSingleLineWidth(font, line), maximumLineWidth);
        }
        
        return maximumLineWidth;
    }
    
    std::vector<std::pair<Font, std::unordered_map<hash32, float>>> stringWidthCache = std::vector<std::pair<Font, std::unordered_map<hash32, float>>>();
    
    static CachedFontStringWidth* get()
    {
        if(!instance) instance = new CachedFontStringWidth();
        
        return instance;
    }
    
    static inline CachedFontStringWidth* instance = nullptr;
};
