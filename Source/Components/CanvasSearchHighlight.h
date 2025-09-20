/*
 // Copyright (c) 2024 Alexander Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Canvas.h"
#include "Object.h"

class CanvasSearchHighlight final : public Component
    , NVGComponent
    , Timer {
    SafePointer<Object> targetObj;
    Canvas* parentCnv;
    float opacity = 2.0f;

public:
    CanvasSearchHighlight(Canvas* cnv, Object* obj)
        : NVGComponent(this)
        , targetObj(obj)
        , parentCnv(cnv)
    {
        cnv->addAndMakeVisible(this);
        startTimerHz(60);

        setBounds(targetObj->getBounds());
    }

    void timerCallback() override
    {
        opacity -= 0.04f;
        repaint();

        if (opacity <= 0.0f) {
            stopTimer();
            parentCnv->removeCanvasSearchHighlight();
        }
    }

    void render(NVGcontext* nvg) override
    {
        if (!targetObj) {
            parentCnv->removeCanvasSearchHighlight();
            return;
        }

        auto const oB = targetObj->getBounds().reduced(Object::margin);

        NVGcolor oCol;
        NVGcolor iCol = oCol = parentCnv->selectedOutlineCol;
        iCol.a = opacity > 1.0f ? 150 : 150 * opacity;
        oCol.a = opacity > 1.0f ? 255 : 255 * opacity;
        nvgDrawRoundedRect(nvg, oB.getX(), oB.getY(), oB.getWidth(), oB.getHeight(), iCol, oCol, Corners::objectCornerRadius);
    }
};
