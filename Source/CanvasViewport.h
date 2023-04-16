/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Utility/GlobalMouseListener.h"

#include "Object.h"
#include "Connection.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Utility/SettingsFile.h"

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport : public Viewport {

    inline static int const infiniteCanvasMargin = 48;

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
                    viewedComponent->addMouseListener(this, false);
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
            // We shouldn't need to do this, but there is something going wrong when you drag very quicly, then stop
            // Auto-repeating the drag makes it smoother, but it's not a perfect solution
            // beginDragAutoRepeat(16);

            float scale = std::sqrt(std::abs(viewport->cnv->getTransform().getDeterminant()));

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

    class NewScrollBar : public Component {
        struct FadeTimer : private ::Timer {
            std::function<bool()> callback;

            void start(int interval, std::function<bool()> cb)
            {
                callback = cb;
                startTimer(interval);
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

                    targetComponent->setAlpha(alpha + 0.3f);
                } else if (alphaTarget < alpha) {
                    float easedAlpha = pow(alpha, 0.5f);
                    easedAlpha -= 0.015f;
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
                startTimerHz(45);
            }

            void fadeOut()
            {
                alphaTarget = 0.0f;
                startTimerHz(45);
            }

            Component* targetComponent;
            float alphaTarget = 0.0f;
        };

    public:
        NewScrollBar(bool isVertical, CanvasViewport* viewport)
            : isVertical(isVertical)
            , viewport(viewport)
        {
        }
        ~NewScrollBar()
        {
        }

        bool hitTest(int x, int y) override
        {
            if (thumbBounds.contains(x, y))
                return true;
            return false;
        }

        void mouseDrag(MouseEvent const& e) override
        {
            Point<float> delta;
            float scale = 1.0f;
            if (auto* viewedComponent = viewport->getViewedComponent()) {
                scale = std::sqrt(std::abs(viewedComponent->getTransform().getDeterminant()));
            }
            if (isVertical) {
                delta = Point<float>(0, e.getDistanceFromDragStartY());
            } else {
                delta = Point<float>(e.getDistanceFromDragStartX(), 0);
            }
            viewport->setViewPosition(viewPosition + (delta * 4).toInt());
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            isMouseDragging = true;
            viewPosition = viewport->getViewPosition();
        }

        void mouseUp(MouseEvent const& e) override
        {
            if (e.mouseWasDraggedSinceMouseDown()) {
                isMouseDragging = false;
                // animator.fadeOut();
            }
            repaint();
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isMouseOver = true;
            // animator.fadeIn();
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isMouseOver = false;
            if (!isMouseDragging)
                // animator.fadeOut();
                repaint();
        }

        void setRangeLimitsAndCurrentRange(float minTotal, float maxTotal, float minCurrent, float maxCurrent)
        {
            totalRange = { minTotal, maxTotal };
            currentRange = { minCurrent, maxCurrent };
            updateThumbBounds();
        }

        void updateThumbBounds()
        {
            auto thumbStart = jmap<int>(currentRange.getStart(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical ? getHeight() : getWidth());
            auto thumbEnd = jmap<int>(currentRange.getEnd(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical ? getHeight() : getWidth());

            if (isVertical)
                thumbBounds = { 0, thumbStart, getWidth(), thumbEnd - thumbStart };
            else
                thumbBounds = { thumbStart, 0, thumbEnd - thumbStart, getHeight() };
            repaint();
        }

        void paint(Graphics& g) override
        {
            float roundedCorner;
            if (isVertical)
                roundedCorner = getWidth() * 0.5f;
            else
                roundedCorner = getHeight() * 0.5f;

            g.setColour(findColour(ScrollBar::ColourIds::thumbColourId));
            g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), 4.0f);
        }

    private:
        bool isVertical = false;
        bool isMouseOver = false;
        bool isMouseDragging = false;
        Range<float> totalRange = { 0, 0 };
        Range<float> currentRange = { 0, 0 };
        Rectangle<int> thumbBounds = { 0, 0 };
        CanvasViewport* viewport;
        Point<int> viewPosition = { 0, 0 };
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
        setScrollBarsShown(false, false);

        setPositioner(new ViewportPositioner(*this));

        addAndMakeVisible(vbar);
        addAndMakeVisible(hbar);
    }

    void lookAndFeelChanged() override
    {
        hbar.repaint();
        vbar.repaint();
    }

    void enableMousePanning(bool enablePanning)
    {
        panner.enablePanning(enablePanning);
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        if (e.mods.isCommandDown() && !(editor->pd->isInPluginMode() || cnv->isPalette)) {
            mouseMagnify(e, 1.0f / (1.0f - wheel.deltaY));
        }
        Viewport::mouseWheelMove(e, wheel);
    }

    void mouseMagnify(MouseEvent const& e, float scrollFactor)
    {
        if (!cnv)
            return;

        auto& scale = cnv->zoomScale;

        float value = getValue<float>(scale);

        // Apply and limit zoom
        value = std::clamp(value * scrollFactor, 0.2f, 3.0f);

        scale = value;
    }

    void adjustScrollbarBounds()
    {
        if (getViewArea().isEmpty())
            return;

        // auto thickness = getScrollBarThickness();
        auto thickness = 8;
        auto localArea = getLocalBounds().reduced(8);

        vbar.setBounds(localArea.removeFromRight(thickness).withTrimmedBottom(thickness).translated(-1, 0));
        hbar.setBounds(localArea.removeFromBottom(thickness));

        float scale = 1.0f / std::sqrt(std::abs(cnv->getTransform().getDeterminant()));
        auto contentArea = getViewArea() * scale;

        Rectangle<int> objectArea;
        for (auto object : cnv->objects) {
            objectArea = objectArea.getUnion(object->getBounds());
        }

        auto totalArea = contentArea.getUnion(objectArea);

        hbar.setRangeLimitsAndCurrentRange(totalArea.getX(), totalArea.getRight(), contentArea.getX(), contentArea.getRight());
        vbar.setRangeLimitsAndCurrentRange(totalArea.getY(), totalArea.getBottom(), contentArea.getY(), contentArea.getBottom());
    }

    void componentMovedOrResized(Component& c, bool moved, bool resized) override
    {
        if (editor->pd->isInPluginMode())
            return;

        Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();
    }

    // TODO: do we need this?
    void visibleAreaChanged(Rectangle<int> const& r) override
    {
        adjustScrollbarBounds();
    }

    void resized() override
    {
        vbar.setVisible(isVerticalScrollBarShown());
        hbar.setVisible(isHorizontalScrollBarShown());

        if (editor->pd->isInPluginMode())
            return;

        adjustScrollbarBounds();

        Viewport::resized();
    }

    // Never respond to arrow keys, they have a different meaning
    bool keyPressed(KeyPress const& key) override
    {
        return false;
    }

    std::function<void()> onScroll = []() {};

private:
    PluginEditor* editor;
    Canvas* cnv;
    MousePanner panner = MousePanner(this);
    NewScrollBar vbar = NewScrollBar(true, this);
    NewScrollBar hbar = NewScrollBar(false, this);

    bool isScrollingHorizontally = false;
    bool isScrollingVertically = false;
};
