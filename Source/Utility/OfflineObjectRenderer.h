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
    ImageWithOffset(Image const& withImage = Image(), Point<int> withOffset = Point<int>())
        : image(withImage)
        , offset(withOffset)
    {
    }
    Image image;
    Point<int> offset;
};

class OfflineObjectRenderer {
public:
    
    static String patchToSVG(String const& patch);
    static ImageWithOffset patchToMaskedImage(String const& patch, float scale, bool makeInvalidImage = false);
    
    static std::pair<std::vector<bool>, std::vector<bool>> countIolets(String const& patch);
    static bool checkIfPatchIsValid(String const& patch);

private:
    
    static Array<Rectangle<int>> getObjectBoundsForPatch(String const& patch);
    static bool parseGraphSize(String const& objectText, Rectangle<int>& bounds);

    static ImageWithOffset patchToTempImage(String const& patch, float scale);
    
    enum PatchItemType
    {
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
