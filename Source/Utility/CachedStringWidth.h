/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Fonts.h";

template<int FontSize>
struct CachedStringWidth {
    static int calculateSingleLineWidth(const String& singleLine)
    {
        auto stringHash = hash(singleLine);

        FontStringKey key = { stringHash, Fonts::getCurrentFont().toString(), FontSize };

        if (stringWidthCache.find(key) != stringWidthCache.end())
            return stringWidthCache[key];
        
        auto const stringWidth = Fonts::getCurrentFont().withHeight(FontSize).getStringWidth(singleLine);
        stringWidthCache[key] = stringWidth;
        
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
    using FontStringKey = std::tuple<hash32, String, int>;
    static inline std::map<FontStringKey, int> stringWidthCache;
};

struct CachedFontStringWidth : public DeletedAtShutdown
{
    ~CachedFontStringWidth()
    {
        instance = nullptr;
    }
    
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
