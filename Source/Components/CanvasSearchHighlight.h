/*
 // Copyright (c) 2024 Alexander Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#include "Canvas.h"
#include "Object.h"

class CanvasSearchHighlight final : public Component
    , public NVGComponent {
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
        setBounds(targetObj->getBounds());
        updater.addAnimator(fade);
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

    void chainAnimation(Animator const& previousAnimation)
    {
        updater.addAnimator(previousAnimation, [this]() {
            fade.start();
        });
        previousAnimation.start();
    }

    VBlankAnimatorUpdater updater { this };
    Animator fade = ValueAnimatorBuilder {}
                        .withDurationMs(270)
                        .withEasing(Easings::createEaseOut())
                        .withValueChangedCallback([this](float v) {
                            opacity = 1.0f - v;
                            repaint();
                        })
                        .build();
};
