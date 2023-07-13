#pragma once

#include <JuceHeader.h>

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
    OfflineObjectRenderer();
    virtual ~OfflineObjectRenderer();

    static OfflineObjectRenderer* findParentOfflineObjectRendererFor(Component* childComponent);

    ImageWithOffset patchToTempImage(String const& patch);

    bool checkIfPatchIsValid(String const& patch);

private:
    class OfflineObjectRendererComponent;
    std::unique_ptr<OfflineObjectRendererComponent> offlineObjectRendererComponent;
};
