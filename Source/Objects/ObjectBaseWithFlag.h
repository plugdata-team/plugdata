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

    void drawTriangleFlag(NVGcontext* nvg, bool isHighlighted, Colour selected, Colour flag, bool topAndBottom = false)
    {
        const float flagSize = 9;

        const auto pixelScale = cnv->getRenderScale();
        const auto zoom = cnv->isScrolling ? 2.0f : getValue<float>(cnv->zoomScale);

        bool highlightUpdate = false;
        if (wasHighlighted != isHighlighted) {
            wasHighlighted = isHighlighted;
            highlightUpdate = true;
        }

        int const flagArea = flagSize * pixelScale * zoom;

        if (flagImage.needsUpdate(flagArea, flagArea) || highlightUpdate) {
            auto colour = isHighlighted ? selected : flag;
            flagImage = NVGImage(nvg, flagArea, flagArea, [pixelScale, zoom, colour, flagSize](Graphics &g) {
                g.addTransform(AffineTransform::scale(pixelScale * zoom, pixelScale * zoom));
                Path outerArea;
                outerArea.addRoundedRectangle(0, 0, flagSize, flagSize, Corners::objectCornerRadius, Corners::objectCornerRadius, 0, 1, 0, 0);
                outerArea.applyTransform(AffineTransform::translation(-0.5f, 0.5f));

                g.reduceClipRegion(outerArea);

                Path flagA;
                flagA.startNewSubPath(0, 0);
                flagA.lineTo(flagSize, 0);
                flagA.lineTo(flagSize, flagSize);
                flagA.closeSubPath();

                g.setColour(colour);
                g.fillPath(flagA);
            });
            //cnv->editor->nvgSurface.invalidateAll(); FIXME: do we need this here?
        }
        // draw triangle top right
        nvgFillPaint(nvg, nvgImagePattern(nvg, getWidth() - flagSize, 0, flagSize, flagSize, 0, flagImage.getImageId(), 1));
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
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, flagSize, flagSize, 0, flagImage.getImageId(), 1));
            nvgFill(nvg);
            nvgRestore(nvg);
        }
    }

private:
    NVGImage flagImage;

    bool wasHighlighted = false;
};