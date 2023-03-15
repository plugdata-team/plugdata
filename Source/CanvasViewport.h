/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Utility/GlobalMouseListener.h"

#include "Object.h"
#include "Connection.h"
#include "Canvas.h"

#include "Utility/SettingsFile.h"

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport : public Viewport
    , public AsyncUpdater {
    class MousePanner : public MouseListener {
    public:
        MousePanner(CanvasViewport* v)
            : viewport(v)
        {
        }

        void enablePanning(bool enabled)
        {
            if (auto* viewedComponent = viewport->getViewedComponent()) {
                if (enabled) {
                    viewedComponent->addMouseListener(this, true);
                } else {
                    viewedComponent->removeMouseListener(this);
                }
            }
        }

        // warning: this only works because Canvas::mouseDown gets called before the listener's mouse down
        // thus giving is a chance to attach the mouselistener on the middle-mouse click event
        void mouseDown(MouseEvent const& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::DraggingHandCursor);
            downPosition = viewport->getViewPosition();
            downCanvasOrigin = viewport->cnv->canvasOrigin;
        }

        void mouseDrag(MouseEvent const& e) override
        {
            beginDragAutoRepeat(10);

            float scale = 1.0f;
            if (auto* viewedComponent = viewport->getViewedComponent()) {
                scale = std::sqrt(std::abs(viewedComponent->getTransform().getDeterminant()));
            }

            auto infiniteCanvasOriginOffset = (viewport->cnv->canvasOrigin - downCanvasOrigin) * scale;
            viewport->setViewPosition(infiniteCanvasOriginOffset + downPosition - (scale * e.getOffsetFromDragStart().toFloat()).roundToInt());
        }

        void mouseUp(MouseEvent const& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
        }

    private:
        CanvasViewport* viewport;
        Point<int> downPosition;
        Point<int> downCanvasOrigin;
    };

    class FadingScrollbar : public ScrollBar
        , public ScrollBar::Listener {
        struct FadeTimer : private ::Timer {
            std::function<bool()> callback;

            void start(int interval, std::function<bool()> cb)
            {
                callback = cb;
                startTimer(interval);
            }

            void stop()
            {
                stopTimer();
            }

            void timerCallback() override
            {
                if (callback())
                    stopTimer();
            }
        };

        struct FadeAnimator : private ::Timer {
            FadeAnimator(Component* target)
                : targetComponent(target)
            {
            }

            void timerCallback()
            {
                auto alpha = targetComponent->getAlpha();
                if (alphaTarget > alpha) {
                    targetComponent->setAlpha(alpha + 0.2f);
                } else if (alphaTarget < alpha) {
                    float easedAlpha = pow(alpha, 1.0f / 2.0f);
                    easedAlpha -= 0.01f;
                    alpha = pow(easedAlpha, 2.0f);
                    if (alpha < 0.01f)
                        alpha = 0.0f;
                    targetComponent->setAlpha(alpha);
                } else {
                    stopTimer();
                }
            }

            void fadeIn()
            {
                alphaTarget = 1.0f;
                startTimerHz(60);
            }

            void fadeOut()
            {
                alphaTarget = 0.0f;
                startTimerHz(60);
            }

            Component* targetComponent;
            float alphaTarget = 0.0f;
        };

    public:
        FadingScrollbar(bool isVertical)
            : ScrollBar(isVertical)
        {
            ScrollBar::setVisible(true);
            addListener(this);
            setAutoHide(false);
            fadeOut();
        }

        void fadeIn(bool fadeOutAfterInterval)
        {
            setVisible(true);
            animator.fadeIn();

            if (fadeOutAfterInterval) {
                fadeTimer.start(800, [this]() {
                    fadeOut();
                    return true;
                });
            }
        }

        void fadeOut()
        {
            animator.fadeOut();
        }

    private:
        void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
        {
            fadeIn(true);
        }

        void mouseDown(MouseEvent const& e) override
        {
            auto currentRange = getCurrentRange();
            auto totalRange = getRangeLimit();
            auto thumbStart = jmap<int>(currentRange.getStart(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical() ? getHeight() : getWidth());
            auto thumbEnd = jmap<int>(currentRange.getEnd(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical() ? getHeight() : getWidth());

            isDraggingThumb = isVertical() ? (e.y >= thumbStart && e.y < thumbEnd) : (e.x >= thumbStart && e.x < thumbEnd);
            lastMousePos = isVertical() ? e.y : e.x;
            ScrollBar::mouseDown(e);
        }

        void mouseDrag(MouseEvent const& e) override
        {
            auto mousePos = isVertical() ? e.y : e.x;

            if (isDraggingThumb && lastMousePos != mousePos) {
                auto totalRange = getRangeLimit();
                auto deltaPixels = jmap<int>(mousePos - lastMousePos, 0, isVertical() ? getHeight() : getWidth(), totalRange.getStart(), totalRange.getEnd());

                setCurrentRangeStart(getCurrentRangeStart()
                    + deltaPixels * 2.5);
            }

            lastMousePos = mousePos;
        }

        void mouseEnter(MouseEvent const& e) override
        {
            fadeIn(false);
        }

        void mouseExit(MouseEvent const& e) override
        {
            fadeOut();
        }

        // Don't allow the viewport to manage scrollbar visibility!
        void setVisible(bool shouldBeVisible) override
        {
        }

        bool isDraggingThumb = false;
        int lastMousePos = 0;
        FadeAnimator animator = FadeAnimator(this);
        FadeTimer fadeTimer;
    };

    struct ViewportPositioner : public Component::Positioner {
        ViewportPositioner(Viewport& comp)
            : Component::Positioner(comp)
            , inset(comp.getScrollBarThickness())
        {
        }

        void applyNewBounds(Rectangle<int> const& newBounds) override
        {
            auto& component = getComponent();
            if (newBounds != component.getBounds()) {
                component.setBounds(newBounds.withTrimmedRight(-inset).withTrimmedBottom(-inset));
            }
        }

        int inset;
    };

public:
    CanvasViewport(PluginEditor* parent, Canvas* cnv)
        : editor(parent)
        , cnv(cnv)
    {
        recreateScrollbars();

        setPositioner(new ViewportPositioner(*this));
        adjustScrollbarBounds();
    }

    void lookAndFeelChanged() override
    {
        if (!vbar || !hbar)
            return;

        vbar->repaint();
        hbar->repaint();
    }

    void enableMousePanning(bool enablePanning)
    {
        panner.enablePanning(enablePanning);
    }

    void adjustScrollbarBounds()
    {
        if (!vbar || !hbar)
            return;

        auto thickness = getScrollBarThickness();

        auto contentArea = getLocalBounds().withTrimmedRight(thickness).withTrimmedBottom(thickness);

        vbar->setBounds(contentArea.removeFromRight(thickness).withTrimmedBottom(thickness));
        hbar->setBounds(contentArea.removeFromBottom(thickness));
    }

    void componentMovedOrResized(Component& c, bool moved, bool resized) override
    {
        Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();
    }

    void resized() override
    {
        adjustScrollbarBounds();

        // In case the window resizes, make sure we maintain the same origin point
        auto oldCanvasOrigin = cnv->canvasOrigin;
        Viewport::resized();
        handleUpdateNowIfNeeded();
        setCanvasOrigin(oldCanvasOrigin);
    }

    ScrollBar* createScrollBarComponent(bool isVertical) override
    {
        if (isVertical) {
            vbar = new FadingScrollbar(true);
            return vbar;
        } else {
            hbar = new FadingScrollbar(false);
            return hbar;
        }
    }

    void handleAsyncUpdate() override
    {
        if (cnv->updatingBounds)
            return;

        auto viewArea = getViewArea();

        cnv->updatingBounds = true;

        float scale = 1.0f / editor->getZoomScaleForCanvas(cnv);
        float smallerScale = std::max(1.0f, scale);

        auto newBounds = Rectangle<int>(cnv->canvasOrigin.x, cnv->canvasOrigin.y, (getWidth() - getScrollBarThickness()) * smallerScale, (getHeight() - getScrollBarThickness()) * smallerScale);

        if (SettingsFile::getInstance()->getProperty<int>("infinite_canvas")) {
            newBounds = newBounds.getUnion(viewArea.expanded(32) * scale);
        }
        for (auto* obj : cnv->objects) {
            newBounds = newBounds.getUnion(obj->getBoundsInParent().reduced(Object::margin));
        }
        for (auto* c : cnv->connections) {
            newBounds = newBounds.getUnion(c->getBoundsInParent());
        }

        cnv->setBounds(newBounds + cnv->getPosition());

        moveCanvasOrigin(-newBounds.getPosition());

        onScroll();

        cnv->updatingBounds = false;
    }

    void moveCanvasOrigin(Point<int> distance)
    {
        cnv->canvasOrigin += distance;

        cnv->updatingBounds = true;

        for (auto* obj : cnv->objects) {
            obj->setBounds(obj->getBounds() + distance);
        }
        for (auto* c : cnv->connections) {
            c->componentMovedOrResized(*this, true, false);
        }

        cnv->updatingBounds = false;
    }

    void setCanvasOrigin(Point<int> newOrigin)
    {
        moveCanvasOrigin(newOrigin - cnv->canvasOrigin);
    }

    void visibleAreaChanged(Rectangle<int> const& newVisibleArea) override
    {
        triggerAsyncUpdate();
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& d) override
    {
        triggerAsyncUpdate();
        Viewport::mouseWheelMove(e, d);
    }

    std::function<void()> onScroll = []() {};

private:
    PluginEditor* editor;
    Canvas* cnv;
    MousePanner panner = MousePanner(this);
    FadingScrollbar* vbar = nullptr;
    FadingScrollbar* hbar = nullptr;
};
