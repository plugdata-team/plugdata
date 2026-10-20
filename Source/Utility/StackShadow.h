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
    // How high an element floats above what's behind it. Each level is a fixed pair of shadows, a tight contact
    // shadow plus a wider ambient one, both cast downward, so the same kind of element looks the same everywhere.
    // Pass the element's real bounds, and leave room around it for the shadow's reach, or its tail gets clipped
    enum Elevation {
        Control, // Controls lifted off their track, like the active tab or a colour picker knob. Reaches ~3px
        Card,    // Cards resting on a panel. Reaches ~8px below, ~5px to the sides
        Popup,   // Menus, tooltips and drag images floating over content. Reaches ~20px
        Window   // Dialogs and windows. Darker than Popup, with the same ~20px reach
    };

    // Intensity scales the shadow's opacity, for elements that need more or less separation than their level gives
    static void drawShadowForRect(Graphics& g, Rectangle<int> bounds, float cornerRadius, Elevation elevation, float intensity = 1.0f);
    static void drawShadowForPath(Graphics& g, hash32 id, Path const& path, Elevation elevation, float intensity = 1.0f);

private:
    struct Layer {
        int spread, offset, radius;
        float opacity;
    };

    ~StackShadow();

    static std::array<Layer, 2> getLayers(Elevation elevation);
    static void drawRectLayer(Graphics& g, Rectangle<int> bounds, float cornerRadius, Layer const& layer, float intensity);
    static void drawPathLayer(Graphics& g, uint64_t cacheKey, Path const& path, Layer const& layer, float intensity);

    UnorderedMap<int64_t, RectShadowImage> shadowMap;
    UnorderedMap<uint64_t, PathShadowImage> pathShadowMap;

    static RectShadowImage generateShadowImages(int radius, float cornerRadius, float scale);
    static bool vImageStackBlur(Image& img, int radius);
    static void floatVectorStackBlur(Image& img, int radius);
    static Image generateBaseShadowImage(Path const& p, int radius, int logicalW, int logicalH, float scale);

    JUCE_DECLARE_SINGLETON_INLINE(StackShadow, false)
};
