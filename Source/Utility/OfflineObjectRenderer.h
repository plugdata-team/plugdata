#pragma once

#include <JuceHeader.h>
#include "Pd/Instance.h"

class ImageWithOffset {
public:
    ImageWithOffset(Image const& withImage = Image(), Point<int> withOffset = Point<int>())
        : image(withImage)
        , offset(withOffset) {};
    Image image;
    Point<int> offset;
};

class OfflineObjectRenderer {
public:
    OfflineObjectRenderer(pd::Instance* pd);
    virtual ~OfflineObjectRenderer();

    static OfflineObjectRenderer* findParentOfflineObjectRendererFor(Component* childComponent);

    ImageWithOffset patchToTempImage(String const& patch, float scale);

    bool checkIfPatchIsValid(String const& patch);
    
    std::pair<std::vector<bool>, std::vector<bool>> countIolets(String const& patch);
    
private:
    
    String stripConnections(String const& patch);
    
    Array<Rectangle<int>> objectRects;
    Rectangle<int> totalSize;
    t_glist* offlineCnv = nullptr;
    pd::Instance* pd;
};
