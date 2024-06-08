/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

template<int FontSize>
struct CachedStringWidth {

    static int calculateSingleLineWidth(String const& singleLine)
    {
        auto stringHash = hash(singleLine);

        auto cacheHit = stringWidthCache.find(stringHash);
        if (cacheHit != stringWidthCache.end())
            return cacheHit->second;

        auto stringWidth = Font(FontSize).getStringWidth(singleLine);
        stringWidthCache[stringHash] = stringWidth;

        return stringWidth;
    }

    static int calculateStringWidth(String const& string)
    {
        int maximumLineWidth = 7;
        for (auto line : StringArray::fromLines(string)) {
            maximumLineWidth = std::max(calculateSingleLineWidth(line), maximumLineWidth);
        }

        return maximumLineWidth;
    }

    static inline std::unordered_map<hash32, int> stringWidthCache = std::unordered_map<hash32, int>();
};

struct CachedFontStringWidth : public DeletedAtShutdown {
    ~CachedFontStringWidth()
    {
        instance = nullptr;
    }

    float calculateSingleLineWidth(Font const& font, String const& singleLine)
    {
        auto stringHash = hash(singleLine);

        for (auto& [cachedFont, cache] : stringWidthCache) {
            if (cachedFont == font) {
                auto cacheHit = cache.find(stringHash);
                if (cacheHit != cache.end())
                    return cacheHit->second;

                auto stringWidth = font.getStringWidthFloat(singleLine);
                cache[stringHash] = stringWidth;

                return stringWidth;
            }
        }

        auto stringWidth = font.getStringWidth(singleLine);
        stringWidthCache.push_back({ font, { { stringHash, stringWidth } } });
        return stringWidth;
    }

    int calculateStringWidth(Font const& font, String const& string)
    {
        float maximumLineWidth = 7;
        for (auto line : StringArray::fromLines(string)) {
            maximumLineWidth = std::max(calculateSingleLineWidth(font, line), maximumLineWidth);
        }

        return maximumLineWidth;
    }

    std::vector<std::pair<Font, std::unordered_map<hash32, float>>> stringWidthCache = std::vector<std::pair<Font, std::unordered_map<hash32, float>>>();

    static CachedFontStringWidth* get()
    {
        if (!instance)
            instance = new CachedFontStringWidth();

        return instance;
    }

    static inline CachedFontStringWidth* instance = nullptr;
};
