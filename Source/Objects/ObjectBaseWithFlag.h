/*
 // Copyright (c) 2024 Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "ObjectBase.h"

class ObjectBaseWithFlag : public ObjectBase
{
public:
    ObjectBaseWithFlag(pd::WeakReference obj, Object* parent)
        : ObjectBase(obj, parent)
    {
    }

    void drawTriangleFlag(NVGcontext* nvg, bool isHighlighted, bool topAndBottom = false)
    {
        auto flagSize = Object::flagSize;

        // If this object is inside a subpatch then it's canvas won't update framebuffers
        // We need to find the base canvas it's in (which will have the same zoom) and use
        // that canvases triangle image
        auto getRootCanvas = [this]() -> Canvas* {
            Canvas* parentCanvas = findParentComponentOfClass<Canvas>();
            while (Canvas* parent = parentCanvas->findParentComponentOfClass<Canvas>()) {
                parentCanvas = parent;
            }
            return parentCanvas;
        };

        auto* rootCnv = getRootCanvas();
        auto objectFlagId = isHighlighted ? rootCnv->objectFlagSelected.getImageId() : rootCnv->objectFlag.getImageId();

        // draw triangle top right
        nvgFillPaint(nvg, nvgImagePattern(nvg, getWidth() - flagSize, 0, flagSize, flagSize, 0, objectFlagId, 1));
        nvgFillRect(nvg, getWidth() - flagSize, 0, flagSize, flagSize);

        if (topAndBottom) {
            // draw same triangle flipped bottom right
            nvgSave(nvg);
            // Rotate around centre
            auto halfFlagSize = flagSize * 0.5f;
            nvgTranslate(nvg, getWidth() - halfFlagSize, getHeight() - halfFlagSize);
            nvgRotate(nvg, degreesToRadians<float>(90));
            nvgTranslate(nvg, -halfFlagSize, -halfFlagSize);

            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, flagSize, flagSize);
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, flagSize, flagSize, 0, objectFlagId, 1));
            nvgFill(nvg);
            nvgRestore(nvg);
        }
    }
};