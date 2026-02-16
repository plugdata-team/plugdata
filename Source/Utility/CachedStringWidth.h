/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

template<int FontSize>
struct CachedStringWidth {

    static float calculateSingleLineWidth(String const& singleLine)
    {
        auto const stringHash = hash(singleLine);

        if (auto const cacheHit = stringWidthCache.find(stringHash); cacheHit != stringWidthCache.end())
            return cacheHit->second;

        auto const stringWidth = Fonts::getStringWidth(singleLine, FontSize);
        stringWidthCache[stringHash] = stringWidth;

        return stringWidth;
    }

    static float calculateStringWidth(String const& string)
    {
        float maximumLineWidth = 7;
        for (auto line : StringArray::fromLines(string)) {
            maximumLineWidth = std::max(calculateSingleLineWidth(line), maximumLineWidth);
        }

        return maximumLineWidth;
    }

    static void clearCache()
    {
        stringWidthCache.clear();
    }

    static inline auto stringWidthCache = UnorderedMap<hash32, float>();
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
                if (auto const cacheHit = cache.find(stringHash); cacheHit != cache.end())
                    return cacheHit->second;

                auto const stringWidth = Fonts::getStringWidth(singleLine, font);
                cache[stringHash] = stringWidth;

                return stringWidth;
            }
        }

        auto stringWidth = Fonts::getStringWidth(singleLine, font);
        stringWidthCache.add({ font, { { stringHash, stringWidth } } });
        return stringWidth;
    }

    float calculateStringWidth(Font const& font, String const& string)
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
