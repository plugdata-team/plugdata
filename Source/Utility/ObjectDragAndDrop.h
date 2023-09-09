#pragma once

#include <JuceHeader.h>
#include "ZoomableDragAndDropContainer.h"
#include "OfflineObjectRenderer.h"

class ObjectDragAndDrop : public Component
{
public:
    ObjectDragAndDrop(){};

    virtual String getObjectString() = 0;

    virtual void dismiss(bool withAnimation){};

    void lookAndFeelChanged() override
    {
        resetDragAndDropImage();
    }

    void resetDragAndDropImage()
    {
        dragImage.image = Image();
    }

    void setIsReordering(bool isReordering)
    {
        reordering = isReordering;
    }

    void mouseDrag(MouseEvent const& e) override final
    {
        if (reordering || e.getDistanceFromDragStart() < 5)
            return;

        auto* dragContainer = ZoomableDragAndDropContainer::findParentDragContainerFor(this);

        if (dragContainer->isDragAndDropActive())
            return;

        auto scale = 2.0f;
        if (dragImage.image.isNull()) {
            auto offlineObjectRenderer = OfflineObjectRenderer::findParentOfflineObjectRendererFor(this);
            dragImage = offlineObjectRenderer->patchToTempImage(getObjectString(), scale);
        }

        dismiss(true);

        Array<var> palettePatchWithOffset;
        palettePatchWithOffset.add(var(dragImage.offset.getX()));
        palettePatchWithOffset.add(var(dragImage.offset.getY()));
        palettePatchWithOffset.add(var(getObjectString()));
        dragContainer->startDragging(palettePatchWithOffset, this, ScaledImage(dragImage.image, scale), true, nullptr, nullptr, true);
    }

private:
    bool reordering = false;
    ImageWithOffset dragImage;
};
