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

private:
    class OfflineObjectRendererComponent;
    std::unique_ptr<OfflineObjectRendererComponent> offlineObjectRendererComponent;
};
