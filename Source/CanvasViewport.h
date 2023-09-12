/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>

#include "Utility/GlobalMouseListener.h"

#include "Object.h"
#include "Connection.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Utility/SettingsFile.h"

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport : public Viewport {

    class MousePanner : public MouseListener {
    public:
        explicit MousePanner(CanvasViewport* v)
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
            mouseDelta = {0, 0};
            previousMousePos = e.getScreenPosition();

            for (auto* object : viewport->cnv->objects)
                object->setBufferedToImage(true);

            withInfiniteDrag = SettingsFile::getInstance()->getProperty<bool>("infinite_drag");
        }

        void mouseDrag(MouseEvent const& e) override
        {
            if (rateReducer.tooFast())
                return;

            auto pos = e.getScreenPosition();

            auto newPos = pos;
            mouseDelta += newPos - previousMousePos;
            previousMousePos = newPos;

            float scale = std::sqrt(std::abs(viewport->cnv->getTransform().getDeterminant()));
            auto infiniteCanvasOriginOffset = (viewport->cnv->canvasOrigin - downCanvasOrigin) * scale;

            auto newCanvasPos = infiniteCanvasOriginOffset + downPosition - mouseDelta;
            viewport->setViewPosition(newCanvasPos);

            auto vPos = viewport->getScreenPosition();
            auto width = viewport->getWidth();
            auto height = viewport->getHeight();
            Point<int> wrappedPos;

            // we need to calculate this last, otherwise the canvas flickers when jumping to new position
            if (withInfiniteDrag && (pos.getX() < vPos.getX() || pos.getY() < vPos.getY() || pos.getX() > vPos.getX() + width || pos.getY() > vPos.getY() + height)) {
                wrappedPos.x = (pos.getX() - vPos.getX() + width) % width;
                wrappedPos.y = (pos.getY() - vPos.getY() + height) % height;

                auto newCursorPos = vPos + wrappedPos;
                auto mouseSource = e.source;
                mouseSource.setScreenPosition(newCursorPos.toFloat());
                previousMousePos = newCursorPos;
            }
        }

        void mouseUp(MouseEvent const& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::NormalCursor);
            for (auto* object : viewport->cnv->objects){
                object->setBufferedToImage(false);
            }
        }

    private:
        CanvasViewport* viewport;
        Point<int> downPosition;
        Point<int> downCanvasOrigin;
        Point<int> mouseDelta;
        Point<int> previousMousePos;

        bool withInfiniteDrag = false;

        RateReducer rateReducer = RateReducer(200);
    };

    class ViewportScrollBar : public Component {
        struct FadeTimer : private ::Timer {
            std::function<bool()> callback;

            void start(int interval, std::function<bool()> cb)
            {
                callback = std::move(cb);
                startTimer(interval);
            }

            void timerCallback() override
            {
                if (callback())
                    stopTimer();
            }
        };

        struct FadeAnimator : private ::Timer {
            explicit FadeAnimator(ViewportScrollBar* target)
                : targetComponent(target)
            {
            }

            void timerCallback() override
            {
                auto growth = targetComponent->growAnimation;
                if (growth < growthTarget) {
                    growth += 0.1f;
                    if (growth >= growthTarget) {
                        stopTimer();
                    }
                    targetComponent->setGrowAnimation(growth);
                } else if (growth > growthTarget) {
                    growth -= 0.1f;
                    if (growth <= growthTarget) {
                        growth = growthTarget;
                        stopTimer();
                    }
                    targetComponent->setGrowAnimation(growth);
                } else {
                    stopTimer();
                }
            }

            void grow()
            {
                growthTarget = 0.0f;
                startTimerHz(60);
            }

            void shrink()
            {
                growthTarget = 1.0f;
                startTimerHz(60);
            }

            ViewportScrollBar* targetComponent;
            float growthTarget = 0.0f;
        };

    public:
        ViewportScrollBar(bool isVertical, CanvasViewport* viewport)
            : isVertical(isVertical)
            , viewport(viewport)
        {
            scrollBarThickness = viewport->getScrollBarThickness();
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
            if (isVertical) {
                delta = Point<float>(0, e.getDistanceFromDragStartY());
            } else {
                delta = Point<float>(e.getDistanceFromDragStartX(), 0);
            }
            viewport->setViewPosition(viewPosition + (delta * 4).toInt());
            repaint();
        }

        void setGrowAnimation(float newGrowValue)
        {
            growAnimation = newGrowValue;
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
                if (!isMouseOver)
                    animator.shrink();
            }
            repaint();
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isMouseOver = true;
            animator.grow();
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isMouseOver = false;
            if (!isMouseDragging)
                animator.shrink();
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
                thumbBounds = Rectangle<float>(0, thumbStart, getWidth(), thumbEnd - thumbStart);
            else
                thumbBounds = Rectangle<float>(thumbStart, 0, thumbEnd - thumbStart, getHeight());
            repaint();
        }

        void paint(Graphics& g) override
        {
            auto growPosition = scrollBarThickness * 0.5f * growAnimation;

            auto growingBounds = thumbBounds.reduced(1).withTop(thumbBounds.getY() + growPosition);
            auto roundedCorner = growingBounds.getHeight() * 0.5f;
            if (isVertical) {
                growingBounds = thumbBounds.reduced(1).withLeft(thumbBounds.getX() + growPosition);
                roundedCorner = growingBounds.getWidth() * 0.5f;
            }

            g.setColour(findColour(ScrollBar::ColourIds::thumbColourId));
            g.fillRoundedRectangle(growingBounds, roundedCorner);
        }

    private:
        bool isVertical = false;
        bool isMouseOver = false;
        bool isMouseDragging = false;

        float growAnimation = 1.0f;

        int scrollBarThickness = 0;

        Range<float> totalRange = { 0, 0 };
        Range<float> currentRange = { 0, 0 };
        Rectangle<float> thumbBounds = { 0, 0 };
        CanvasViewport* viewport;
        Point<int> viewPosition = { 0, 0 };
        FadeAnimator animator = FadeAnimator(this);
        FadeTimer fadeTimer;
    };

    struct ViewportPositioner : public Component::Positioner {
        explicit ViewportPositioner(Viewport& comp)
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

        setScrollBarThickness(8);

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
        // Check event time to filter out duplicate events
        // This is a workaround for a bug in JUCE that can cause mouse events to be duplicated when an object has a MouseListener on its parent
        if (e.eventTime == lastScrollTime)
            return;

        if (e.mods.isCommandDown() && !editor->pd->isInPluginMode()) {
            mouseMagnify(e, 1.0f / (1.0f - wheel.deltaY));
        }

        Viewport::mouseWheelMove(e, wheel);
        lastScrollTime = e.eventTime;
    }

    void mouseMagnify(MouseEvent const& e, float scrollFactor) override
    {
        if (!cnv)
            return;

        auto& scale = cnv->zoomScale;

        auto value = getValue<float>(scale);

        // Apply and limit zoom
        value = std::clamp(value * scrollFactor, 0.2f, 3.0f);

        scale = value;
    }

    void adjustScrollbarBounds()
    {
        if (getViewArea().isEmpty())
            return;

        auto thickness = getScrollBarThickness();
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

    void visibleAreaChanged(Rectangle<int> const& r) override
    {
        onScroll();
        adjustScrollbarBounds();
    }

    void resized() override
    {
        vbar.setVisible(isVerticalScrollBarShown());
        hbar.setVisible(isHorizontalScrollBarShown());

        if (editor->pd->isInPluginMode())
            return;

        adjustScrollbarBounds();

        float scale = std::sqrt(std::abs(cnv->getTransform().getDeterminant()));

        // centre canvas when resizing viewport
        auto getCentre = [this, scale](Rectangle<int> bounds) {
            if (scale > 1.0f) {
                auto point = cnv->getLocalPoint(this, bounds.withZeroOrigin().getCentre());
                return point * scale;
            }
            return getViewArea().withZeroOrigin().getCentre();
        };

        auto currentCenter = getCentre(previousBounds);
        previousBounds = getBounds();
        Viewport::resized();
        auto newCenter = getCentre(getBounds());

        auto offset = currentCenter - newCenter;
        setViewPosition(getViewPosition() + offset);
    }

    // Never respond to arrow keys, they have a different meaning
    bool keyPressed(KeyPress const& key) override
    {
        return false;
    }

    std::function<void()> onScroll = []() {};

private:
    Time lastScrollTime;
    PluginEditor* editor;
    Canvas* cnv;
    Rectangle<int> previousBounds;
    MousePanner panner = MousePanner(this);
    ViewportScrollBar vbar = ViewportScrollBar(true, this);
    ViewportScrollBar hbar = ViewportScrollBar(false, this);
};
