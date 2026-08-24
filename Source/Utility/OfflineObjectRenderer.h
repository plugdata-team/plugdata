/*
 // Copyright (c) 2021-2023 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"
#include "Pd/Instance.h"

class ImageWithOffset {
public:
    explicit ImageWithOffset(Image const& withImage = Image(), Point<int> const withOffset = Point<int>())
        : image(withImage)
        , offset(withOffset)
    {
    }
    Image image;
    Point<int> offset;
};

struct PlugDataLook;

class OfflineObjectRenderer {
public:
    static ImageWithOffset patchToMaskedImage(PlugDataLook const& look, String const& patch, float scale, bool makeInvalidImage = false);
    static SmallArray<Rectangle<int>> getObjectBoundsForPatch(String const& patch);

    static std::pair<SmallArray<bool>, SmallArray<bool>> countIolets(String const& patch);
    static bool checkIfPatchIsValid(String const& patch);

private:
    static bool parseGraphSize(String const& objectText, Rectangle<int>& bounds);

    static ImageWithOffset patchToTempImage(String const& patch, float scale);

    enum PatchItemType {
        Object,
        Comment,
        Message,
        Connection,
        CanvasStart,
        CanvasEnd,
        GraphCoords
    };

    static void parsePatch(String const& patch, std::function<void(PatchItemType, int, String const&)> callback);
};
