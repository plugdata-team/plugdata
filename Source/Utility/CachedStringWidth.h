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
        auto const stringHash = hash(singleLine);

        auto const cacheHit = stringWidthCache.find(stringHash);
        if (cacheHit != stringWidthCache.end())
            return cacheHit->second;

        auto const stringWidth = Font(FontSize).getStringWidth(singleLine);
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

    static void clearCache()
    {
        stringWidthCache.clear();
    }

    static inline UnorderedMap<hash32, int> stringWidthCache = UnorderedMap<hash32, int>();
};

struct CachedFontStringWidth final : public DeletedAtShutdown {
    ~CachedFontStringWidth() override
    {
        instance = nullptr;
    }

    float calculateSingleLineWidth(Font const& font, String const& singleLine)
    {
        auto stringHash = hash(singleLine);

        for (auto& [cachedFont, cache] : stringWidthCache) {
            if (cachedFont == font) {
                auto const cacheHit = cache.find(stringHash);
                if (cacheHit != cache.end())
                    return cacheHit->second;

                auto const stringWidth = font.getStringWidthFloat(singleLine);
                cache[stringHash] = stringWidth;

                return stringWidth;
            }
        }

        auto stringWidth = font.getStringWidth(singleLine);
        stringWidthCache.add({ font, { { stringHash, stringWidth } } });
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

    HeapArray<std::pair<Font, UnorderedMap<hash32, float>>> stringWidthCache = HeapArray<std::pair<Font, UnorderedMap<hash32, float>>>();

    static CachedFontStringWidth* get()
    {
        if (!instance)
            instance = new CachedFontStringWidth();

        return instance;
    }

    static inline CachedFontStringWidth* instance = nullptr;
};
