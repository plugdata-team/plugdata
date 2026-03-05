/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Containers.h"
#include "Utility/Hash.h"

using namespace juce;

class StackShadow final : public DeletedAtShutdown {
    struct RectShadowImage {
        Image corner, edge;
    };
    struct PathShadowImage {
        Image image;
        int x, y;
    };
public:
    
    static void drawShadowForRect(Graphics& g, Rectangle<int> bounds, int radius, float shadowCornerRadius = 5.0f, float opacity = 0.4f, int verticalOffset = 0);
    static void drawShadowForPath(Graphics& g, hash32 id, Path const& path, int radius, Colour colour, int verticalOffset = 0, int horizontalOffset = 0);

private:
    UnorderedMap<int64_t, RectShadowImage> shadowMap;
    UnorderedMap<uint64_t, PathShadowImage> pathShadowMap;

    static RectShadowImage generateShadowImages(int radius, float cornerRadius, float scale);
    static bool vImageStackBlur(Image& img, int radius);
    static void floatVectorStackBlur(Image& img, int radius);
    static Image generateBaseShadowImage(Path& p, int radius, int logicalW, int logicalH, float scale);

    JUCE_DECLARE_SINGLETON_INLINE(StackShadow, false)
};
