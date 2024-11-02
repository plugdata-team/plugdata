/*
 // Copyright (c) 2024 Alexander Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Canvas.h"
#include "Object.h"

class CanvasCrosshair : public Component
    , NVGComponent
    , Timer {
    SafePointer<Object> targetObj;
    Canvas* parentCnv;
    float opacity = 4.0f;

public:
    CanvasCrosshair(Canvas* cnv, Object* obj)
        : NVGComponent(this)
        , targetObj(obj)
        , parentCnv(cnv)
    {
        cnv->addAndMakeVisible(this);
        startTimerHz(60);
    }

    void timerCallback() override
    {
        // Fade in log space so it's smoother fade out
        auto log = std::pow(opacity, 1 / 2.2f);
        log -= 0.02f;
        opacity = std::pow(log, 2.2f);

        parentCnv->repaint();
        if (opacity <= 0.0f || !std::isfinite(opacity)) {
            stopTimer();
            parentCnv->removeCanvasCrosshair();
            opacity = 0.0f;
        }
    }

    void render(NVGcontext* nvg) override
    {
        if (targetObj) {
            auto oB = targetObj->getBounds();

            nvgBeginPath(nvg);

            // left line
            nvgMoveTo(nvg, 0, oB.getCentreY());
            nvgLineTo(nvg, oB.getPosition().x + Object::margin, oB.getCentreY());
            // right line
            nvgMoveTo(nvg, oB.getRight() - Object::margin, oB.getCentreY());
            nvgLineTo(nvg, Canvas::infiniteCanvasSize, oB.getCentreY());
            // top line
            nvgMoveTo(nvg, oB.getCentreX(), 0);
            nvgLineTo(nvg, oB.getCentreX(), oB.getPosition().y + Object::margin);
            // bottom line
            nvgMoveTo(nvg, oB.getCentreX(), oB.getBottom() - Object::margin);
            nvgLineTo(nvg, oB.getCentreX(), Canvas::infiniteCanvasSize);

            auto iCol = parentCnv->selectedOutlineCol;
            iCol.a = opacity > 1.0f ? 150 : 150 * opacity;

            // #define DoubleLine
#ifdef DoubleLine
            auto oCol = nvgLerpRGBA(parentCnv->selectedOutlineCol, nvgRGBA(0, 0, 0, 0), 0.5f);
            oCol.a = opacity > 1.0f ? 80 : 80 * opacity;
            nvgStrokeWidth(nvg, 6.0f);
            nvgStrokeColor(nvg, oCol);
            nvgStroke(nvg);
#endif
            nvgStrokeWidth(nvg, 4.0f);
            nvgStrokeColor(nvg, iCol);
            nvgStroke(nvg);
        }
    }
};
