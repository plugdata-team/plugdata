/*
 // Copyright (c) 2021-2025 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>

#include <nanovg.h>
#include <utility>

#include "Object.h"
#include "Objects/ObjectBase.h"
#include "Connection.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Utility/SettingsFile.h"

class CanvasViewport : public Component
    , public NVGComponent
    , public Timer {
    class Minimap final : public Component
        , public AsyncUpdater {
    public:
        explicit Minimap(Canvas* canvas)
            : cnv(canvas)
        {
            updater.addAnimator(alphaAnimator);
            alphaAnimator.complete();
        }

        void handleAsyncUpdate() override
        {
            auto const area = visibleArea;
            bool renderMinimap = cnv->objects.not_empty();
            for (auto const* obj : cnv->objects) {
                if (obj->getBounds().toFloat().intersects(area)) {
                    renderMinimap = false;
                    break;
                }
            }

            auto const showMinimap = SettingsFile::getInstance()->getProperty<int>("show_minimap");
            float fadedIn = 0.0f;
            float fadedOut = 0.0f;
            if (showMinimap == 1) {
                fadedIn = 0.0f;
                fadedOut = 0.0f;
            } else if (showMinimap == 2) {
                fadedIn = 1.0f;
                fadedOut = 0.0f;
            } else if (showMinimap == 3) {
                fadedIn = 1.0f;
                fadedOut = 0.5f;
                if (isMouseOver)
                    renderMinimap = true;
            }

            if (renderMinimap && minimapAlpha != fadedIn) {
                setVisible(showMinimap != 1);
                animateAlphaTo(fadedIn);
            } else if (!renderMinimap && minimapAlpha != fadedOut) {
                setVisible(showMinimap == 3);
                animateAlphaTo(fadedOut);
            }
        }

        void updateMinimap(Rectangle<float> const area)
        {
            if (isMouseDown || area.isEmpty())
                return;

            visibleArea = area;
            triggerAsyncUpdate();
        }

        auto getMapBounds()
        {
            struct
            {
                Rectangle<float> fullBounds, viewBounds;
                int offsetX, offsetY;
                float scale;
            } b;

            b.viewBounds = cnv->viewport->getViewArea();
            auto allObjectBounds = Rectangle<float>(cnv->canvasOrigin.x, cnv->canvasOrigin.y, b.viewBounds.getWidth(), b.viewBounds.getHeight());
            for (auto const* object : cnv->objects) {
                allObjectBounds = allObjectBounds.getUnion(object->getBounds().toFloat());
            }

            b.fullBounds = isMouseDown ? boundsBeforeDrag : allObjectBounds.getUnion(b.viewBounds);

            b.offsetX = -std::min(0.f, b.fullBounds.getX() - cnv->canvasOrigin.x);
            b.offsetY = -std::min(0.f, b.fullBounds.getY() - cnv->canvasOrigin.y);
            b.scale = std::min<float>(width / (b.fullBounds.getWidth() + b.offsetX), height / (b.fullBounds.getHeight() + b.offsetY));
            boundsBeforeDrag = b.fullBounds;

            return b;
        }

        void render(NVGcontext* nvg)
        {
            if (approximatelyEqual(minimapAlpha, 0.0f))
                return;

            nvgGlobalAlpha(nvg, minimapAlpha);

            auto map = getMapBounds();

            float const x = cnv->viewport->getWidth() - (width + 10);
            float const y = cnv->viewport->getHeight() - (height + 10);

            auto const canvasBackground = PlugDataColours::canvasBackgroundColour;
            auto const mapBackground = canvasBackground.contrasting(0.5f);

            // draw background
            nvgFillColor(nvg, nvgColour(mapBackground.withAlpha(0.4f)));
            nvgFillRoundedRect(nvg, x - 4, y - 4, width + 8, height + 8, Corners::largeCornerRadius);

            nvgFillColor(nvg, nvgColour(mapBackground.withAlpha(0.8f)));

            // draw objects
            for (auto const* object : cnv->objects) {
                auto b = (object->getBounds().reduced(Object::margin).translated(map.offsetX, map.offsetY) - cnv->canvasOrigin).toFloat() * map.scale;
                nvgFillRoundedRect(nvg, x + b.getX(), y + b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius * map.scale);
            }

            // draw visible area
            nvgDrawRoundedRect(nvg, x + (map.offsetX + map.viewBounds.getX() - cnv->canvasOrigin.x) * map.scale, y + (map.offsetY + map.viewBounds.getY() - cnv->canvasOrigin.y) * map.scale, map.viewBounds.getWidth() * map.scale, map.viewBounds.getHeight() * map.scale, nvgColour(canvasBackground.withAlpha(0.6f)), nvgColour(canvasBackground.contrasting(0.4f)), 0.0f);
            nvgGlobalAlpha(nvg, 1.0f);
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isMouseOver = true;
            triggerAsyncUpdate();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isMouseOver = false;
            triggerAsyncUpdate();
        }

        void mouseDown(MouseEvent const& e) override
        {
            downPosition = cnv->viewport->getViewPosition();

            auto map = getMapBounds();
            auto const realViewBounds = Rectangle<int>((map.offsetX + map.viewBounds.getX() - cnv->canvasOrigin.x) * map.scale, (map.offsetY + map.viewBounds.getY() - cnv->canvasOrigin.y) * map.scale, map.viewBounds.getWidth() * map.scale, map.viewBounds.getHeight() * map.scale);
            isMouseDown = realViewBounds.contains(e.getMouseDownPosition());
        }

        void mouseUp(MouseEvent const& e) override
        {
            auto const viewBounds = cnv->viewport->getViewArea();
            downPosition = viewBounds.getPosition();
            isMouseDown = false;
            updateMinimap(viewBounds);
            cnv->editor->nvgSurface.invalidateAll();
        }

        void mouseDrag(MouseEvent const& e) override
        {
            auto map = getMapBounds();

            if (isMouseDown) {
                cnv->viewport->setViewPosition(downPosition + e.getOffsetFromDragStart().toFloat() / map.scale);
            }
        }

    private:
        void animateAlphaTo(float const target)
        {
            alphaAnimationStart = minimapAlpha;
            alphaAnimationTarget = target;
            if (alphaAnimator.isComplete()) {
                alphaAnimator.start();
            }
        }

        Canvas* cnv;
        Rectangle<float> visibleArea;
        Point<float> downPosition;
        Rectangle<float> boundsBeforeDrag;
        bool isMouseDown : 1 = false;
        bool isMouseOver : 1 = false;
        static constexpr float width = 180;
        static constexpr float height = 130;

        VBlankAnimatorUpdater updater { this };

        float minimapAlpha = 0.0f;
        float alphaAnimationStart, alphaAnimationTarget;
        Animator alphaAnimator = ValueAnimatorBuilder {}
                                     .withDurationMs(220)
                                     .withEasing(Easings::createEaseInOutCubic())
                                     .withValueChangedCallback([this](float v) {
                                         minimapAlpha = makeAnimationLimits(alphaAnimationStart, alphaAnimationTarget).lerp(v);
                                         cnv->editor->nvgSurface.invalidateAll();
                                     })
                                     .build();
    };

    class MousePanner final : public MouseListener {
    public:
        explicit MousePanner(CanvasViewport* vp, Canvas* cnv)
            : viewport(vp)
            , canvas(cnv)
        {
            canvas->addMouseListener(this, false);
        }

        ~MousePanner()
        {
            if (canvas)
                canvas->removeMouseListener(this);
        }

        void enablePanning(bool const enabled)
        {
            enableMousePanning = enabled;
        }

        bool isConsumingTouchGesture()
        {
            return consumingTouchGesture;
        }

        // warning: this only works because Canvas::mouseDown gets called before the listener's mouse down
        // thus giving us a chance to attach the mouselistener on the middle-mouse click event
        // Specifically, we use the hitTest in on-canvas objects to check if panning mod is down
        // Because the hitTest can decided where the mouseEvent goes.
        void mouseDown(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown() && !e.mods.isMiddleButtonDown())
                return;

            // Cancel the animation timer for the search panel
            if (enableMousePanning) {
                viewport->zoomAnimator.complete();
                viewport->moveAnimator.complete();

                e.originalComponent->setMouseCursor(MouseCursor::DraggingHandCursor);
                downPosition = viewport->getViewPosition();
                downCanvasOrigin = viewport->cnv->canvasOrigin.toFloat();
            }
        }

        bool doesMouseEventComponentBlockViewportDrag(Component const* eventComp)
        {
            for (auto c = eventComp; c != nullptr && c != viewport; c = c->getParentComponent())
                if (c->getViewportIgnoreDragFlag())
                    return true;

            return false;
        }

        void mouseDrag(MouseEvent const& e) override
        {
            if (enableMousePanning) {
                float const scale = viewport->getViewScale();
                auto const infiniteCanvasOriginOffset = (viewport->cnv->canvasOrigin.toFloat() - downCanvasOrigin) * scale;
                viewport->setViewPosition(infiniteCanvasOriginOffset + downPosition - (scale * e.getOffsetFromDragStart().toFloat()));
            }

            auto index = e.source.getIndex();
            bool const touchMode = SettingsFile::getInstance()->getProperty<bool>("touch_mode");
            if (touchMode && e.source.isTouch() && index < 2) {
                if (doesMouseEventComponentBlockViewportDrag(e.eventComponent))
                    return;

                multiTouchOffset[index] = e.getOffsetFromDragStart().toFloat();
                multiTouchStartPosition[index] = e.getMouseDownPosition().toFloat();

                if (index == 1 && multiTouchOffset[0].getDistanceFromOrigin() > 2.0f && multiTouchOffset[1].getDistanceFromOrigin() > 2.0f) {
                    handleTouchGesture(e);
                }
            }
        }

        void mouseUp(MouseEvent const& e) override
        {
            if(consumingTouchGesture) {
                consumingTouchGesture = false;
                auto centre = (multiTouchStartPosition[0] + multiTouchOffset[0] + multiTouchStartPosition[1] + multiTouchOffset[1]) * 0.5f;
                viewport->applyScale(viewport->getViewScale(), viewport->getLocalPoint(e.eventComponent, centre), true);
            }

            multiTouchOffset[0] = {};
            multiTouchOffset[1] = {};
            multiTouchLastOffset = {};
            lastPinchScale = 1.0f;
            smoothedPinchScale = 1.0f;
        }

        void handleTouchGesture(MouseEvent const& e)
        {
            auto panOffset = (multiTouchOffset[0] + multiTouchOffset[1]) * 0.5f;

            auto centre = (multiTouchStartPosition[0] + multiTouchOffset[0] + multiTouchStartPosition[1] + multiTouchOffset[1]) * 0.5f;
            auto startDistance = multiTouchStartPosition[0].getDistanceFrom(multiTouchStartPosition[1]);
            auto currentDistance = (multiTouchStartPosition[0] + multiTouchOffset[0]).getDistanceFrom(multiTouchStartPosition[1] + multiTouchOffset[1]);
            float pinchScale = (startDistance > 0.0f) ? (currentDistance / startDistance) : 1.0f;
            bool isPan = panOffset.getDistanceFromOrigin() > 8.0f;
            bool isPinch = std::abs(pinchScale - 1.0f) > 0.07f;

            if (isPan) {
                auto panDelta = (panOffset - multiTouchLastOffset) / 256.0f;
                auto canvasZoom = getValue<float>(canvas->zoomScale);

                multiTouchLastOffset = (multiTouchOffset[0] + multiTouchOffset[1]) * 0.5f;

                MouseWheelDetails details;
                details.deltaX = panDelta.x * canvasZoom;
                details.deltaY = panDelta.y * canvasZoom;
                details.isReversed = false;
                details.isSmooth = true;
                details.isInertial = false;

                auto event = MouseEvent(e.source, viewport->getLocalPoint(e.eventComponent, centre),
                    e.mods, e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY,
                    viewport, viewport, e.eventTime,
                    e.mouseDownPosition, e.mouseDownTime, e.getNumberOfClicks(), true);

                consumingTouchGesture = true;
                viewport->mouseWheelMove(event, details);
            }
            if (isPinch) {
                if (!consumingTouchGesture) {
                    logicalScale = getViewScale();
                }

                smoothedPinchScale += (pinchScale - smoothedPinchScale) * 0.4f;
                float pinchScaleDelta = (smoothedPinchScale - lastPinchScale) + 1.0f;
                pinchScaleDelta = jlimit(0.85f, 1.15f, pinchScaleDelta);
                lastPinchScale = smoothedPinchScale;
                consumingTouchGesture = true;
                viewport->mouseMagnify(e.withNewPosition(centre).getEventRelativeTo(viewport), pinchScaleDelta);
            }
        }

        Point<float> getPointerCentre()
        {
            if(SettingsFile::getInstance()->getProperty<bool>("touch_mode"))
            {
                auto centre = (multiTouchStartPosition[0] + multiTouchOffset[0] + multiTouchStartPosition[1] + multiTouchOffset[1]) * 0.5f;
                return viewport->getLocalPoint(canvas.get(), centre);
            }
            else {
                return viewport->getMouseXYRelative().toFloat();
            }
        };

    private:
        CanvasViewport* viewport;
        Point<float> downPosition;
        Point<float> downCanvasOrigin;

        Point<float> multiTouchOffset[2];
        Point<float> multiTouchStartPosition[2];
        Point<float> multiTouchLastOffset;
        float lastPinchScale = 1.0f;
        float smoothedPinchScale = 1.0f;

        bool enableMousePanning = false;
        bool consumingTouchGesture = false;
        SafePointer<Canvas> canvas;
    };

    class ViewportScrollBar final : public Component {
    public:
        ViewportScrollBar(bool const isVertical, CanvasViewport* viewport)
            : viewport(viewport)
            , isVertical(isVertical)
        {
            updater.addAnimator(growAnimator);
            lookAndFeelChanged();
        }

        void lookAndFeelChanged() override
        {
            auto const scrollbarColour = findColour(ScrollBar::ColourIds::thumbColourId);
            scrollbarCol = nvgColour(scrollbarColour);
            activeScrollbarCol = nvgColour(scrollbarColour.interpolatedWith(PlugDataColours::canvasBackgroundColour.contrasting(0.6f), 0.7f));
            scrollbarBgCol = nvgColour(scrollbarColour.interpolatedWith(PlugDataColours::canvasBackgroundColour, 0.7f));
            ;
            repaint();
        }

        bool hitTest(int const x, int const y) override
        {
            if (isVertical)
                return thumbBounds.withY(2).withHeight(getHeight() - 4).contains(x, y);
            else
                return thumbBounds.withX(2).withWidth(getWidth() - 4).contains(x, y);
        }

        void mouseDrag(MouseEvent const& e) override
        {
            auto delta = isVertical ? Point<float>(0, e.getDistanceFromDragStartY()) : Point<float>(e.getDistanceFromDragStartX(), 0);
            viewport->setViewPosition(viewPosition + (delta * 4));
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown())
                return;

            isMouseDragging = true;
            viewPosition = viewport->getViewPosition();
            repaint();
        }

        void mouseUp(MouseEvent const& e) override
        {
            isMouseDragging = false;
            if (e.mouseWasDraggedSinceMouseDown()) {
                if (!isMouseOver)
                    animateGrowTo(1.0f); // shrink
            }
            repaint();
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isMouseOver = true;
            animateGrowTo(0.0f); // grow
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isMouseOver = false;
            if (!isMouseDragging)
                animateGrowTo(1.0f); // shrink
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
            auto const thumbStart = jmap<int>(currentRange.getStart(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical ? getHeight() : getWidth());
            auto const thumbEnd = jmap<int>(currentRange.getEnd(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical ? getHeight() : getWidth());

            if (isVertical)
                thumbBounds = Rectangle<float>(0, thumbStart, getWidth(), thumbEnd - thumbStart);
            else
                thumbBounds = Rectangle<float>(thumbStart, 0, thumbEnd - thumbStart, getHeight());

            repaint();
        }

        void render(NVGcontext* nvg)
        {
            auto const growPosition = scrollBarThickness * 0.5f * growAnimation;
            auto growingBounds = thumbBounds.reduced(1).withTop(thumbBounds.getY() + growPosition);
            auto thumbCornerRadius = growingBounds.getHeight();
            auto fullBounds = growingBounds.withX(2).withWidth(getWidth() - 4);

            if (isVertical) {
                growingBounds = thumbBounds.reduced(1).withLeft(thumbBounds.getX() + growPosition);
                thumbCornerRadius = growingBounds.getWidth();
                fullBounds = growingBounds.withY(2).withHeight(getHeight() - 4);
            }

            scrollbarBgCol.a = (1.0f - growAnimation) * 150;
            nvgDrawRoundedRect(nvg, fullBounds.getX(), fullBounds.getY(), fullBounds.getWidth(), fullBounds.getHeight(), scrollbarBgCol, scrollbarBgCol, thumbCornerRadius);

            auto const scrollBarThumbCol = isMouseDragging ? activeScrollbarCol : scrollbarCol;
            nvgDrawRoundedRect(nvg, growingBounds.getX(), growingBounds.getY(), growingBounds.getWidth(), growingBounds.getHeight(), scrollBarThumbCol, scrollBarThumbCol, thumbCornerRadius);
        }

        NVGcolor scrollbarCol;
        NVGcolor activeScrollbarCol;
        NVGcolor scrollbarBgCol;

    private:
        void animateGrowTo(float const target)
        {
            animationStart = growAnimation;
            animationTarget = target;
            growAnimator.start();
        }

        static constexpr float scrollBarThickness = 10.f;

        Range<float> totalRange = { 0, 0 };
        Range<float> currentRange = { 0, 0 };
        Rectangle<float> thumbBounds = { 0, 0 };

        CanvasViewport* viewport;
        Point<float> viewPosition = { 0, 0 };

        VBlankAnimatorUpdater updater { this };

        float growAnimation = 1.0f;
        float animationStart, animationTarget;
        Animator growAnimator = ValueAnimatorBuilder {}
                                    .withDurationMs(220)
                                    .withEasing(Easings::createEaseInOut())
                                    .withValueChangedCallback([this](float v) {
                                        growAnimation = makeAnimationLimits(animationStart, animationTarget).lerp(v);
                                        repaint();
                                    })
                                    .build();

        bool isVertical : 1 = false;
        bool isMouseOver : 1 = false;
        bool isMouseDragging : 1 = false;
    };

public:
    CanvasViewport(PluginEditor* parent, Canvas* cnv)
        : NVGComponent(this)
        , panner(this, cnv)
        , minimap(cnv)
        , editor(parent)
        , cnv(cnv)
    {
        addAndMakeVisible(cnv);

        addChildComponent(minimap);
        addAndMakeVisible(vbar);
        addAndMakeVisible(hbar);

        updater.addAnimator(zoomAnimator);
        updater.addAnimator(moveAnimator);
        updater.addAnimator(bounceAnimator);

#if JUCE_IOS
        gestureCheck.startTimer(20);
#endif

        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this, [this] {
            return editor->getTabComponent().getVisibleCanvases().contains(this->cnv);
        }));
    }

    void render(NVGcontext* nvg, Rectangle<int> const area)
    {
        minimap.render(nvg);

        if (area.intersects(vbar.getBounds())) {
            NVGScopedState scopedState(nvg);
            nvgTranslate(nvg, vbar.getX(), vbar.getY());
            vbar.render(nvg);
        }

        if (area.intersects(hbar.getBounds())) {
            NVGScopedState scopedState(nvg);
            nvgTranslate(nvg, hbar.getX(), hbar.getY());
            hbar.render(nvg);
        }
    }

    void timerCallback() override
    {
        stopTimer();
        cnv->isZooming = false;

        // Cached geometry can look thicker/thinner at different zoom scales, so we update all cached connections when zooming is done
        if (scaleChanged) {
            NVGCachedPath::resetAll();
        }

        scaleChanged = false;
        editor->nvgSurface.invalidateAll();
    }

    void setViewPositionAnimated(Point<float> const pos, float targetScale = -1.0f)
    {
        if (targetScale <= 0.0f)
            targetScale = getViewScale();

        animationStartScale = getViewScale();
        animationTargetScale = targetScale;

        animationStartPos = getViewPosition();
        animationEndPos = pos;
        moveAnimator.start();
    }

    void enableMousePanning(bool const enablePanning)
    {
        panner.enablePanning(enablePanning);
    }

    bool hitTest(int x, int y) override
    {
        // needed so that mouseWheel event is registered in presentation mode
        return true;
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& wheel) override
    {
        moveAnimator.complete();
        zoomAnimator.complete();

        auto scrollFactor = 1.0f / (1.0f - wheel.deltaY);
        if (e.mods.isCommandDown()) {
            if (wheel.isSmooth || std::abs(wheel.deltaY) < 0.01) {
                applyScale(std::clamp(getValue<float>(cnv->zoomScale) * scrollFactor, 0.25f, 3.0f), e.position, false);
            } else {
                mouseMagnify(e, scrollFactor);
            }
        }

        if (e.eventComponent == this)
            if (!useMouseWheelMoveIfNeeded(e, wheel))
                Component::mouseWheelMove(e, wheel);
    }

    bool useMouseWheelMoveIfNeeded(MouseEvent const& e, MouseWheelDetails const& wheel)
    {
        if (!(e.mods.isAltDown() || e.mods.isCtrlDown() || e.mods.isCommandDown())) {
            auto delta = Point<float>(wheel.deltaX, wheel.deltaY) * 224.0f;
            auto pos = getViewPosition();

            // Smooth out to prevent zigzagging effect (especially noticable with inertia on macOS)
            smoothedDelta = (smoothingFactor * delta) + ((1.0f - smoothingFactor) * smoothedDelta);
            if (e.mods.isShiftDown())
                pos.x -= (delta.x != 0 ? delta.x : delta.y);
            else
                pos -= (wheel.isSmooth && !e.source.isTouch()) ? smoothedDelta : delta;

            if (pos != getViewPosition()) {
                setViewPosition(pos);
                return true;
            }
        }

        return false;
    }

    void mouseMagnify(MouseEvent const& e, float const scrollFactor) override
    {
        moveAnimator.complete();
        zoomAnimator.complete();

        logicalScale *= scrollFactor;
        logicalScale = std::clamp(logicalScale, 0.12f, 3.6f);


#if JUCE_MAC || JUCE_IOS
        applyScale(logicalScale, e.position, true);
#else
        applyScale(logicalScale, e.position, e.source.isTouch());
#endif
    }

    void magnify(float newScale)
    {
        auto pos = getMouseXYRelative();
        if (getLocalBounds().contains(pos)) {
            startMagnification(newScale, pos);
        } else {
            startMagnification(newScale, getLocalBounds().getCentre());
        }
    }

    void magnifyCentred(float newScale)
    {
        startMagnification(newScale, getLocalBounds().getCentre());
    }

    void startMagnification(float newScaleFactor, Point<int> centre)
    {
        auto lastScale = getViewScale();
        newScaleFactor = std::clamp(newScaleFactor, 0.25f, 3.0f);
        if (approximatelyEqual(newScaleFactor, 0.0f))
            newScaleFactor = 1.0f;

        animationStartScale = lastScale;
        animationTargetScale = newScaleFactor;
        animationCentre = centre.toFloat();

        scaleChanged = true;
        zoomAnimator.start();
    }

    void applyScale(float scale, Point<float> centre, bool allowOvershoot)
    {
        logicalScale = scale;
        float const clampedScale = std::clamp(scale, 0.25f, 3.0f);
        if (allowOvershoot && !approximatelyEqual(scale, clampedScale)) {
            auto rubberBand = [](float x, float limit, float stiffness) {
                float const excess = std::abs(x - limit);
                float const displacement = excess / (1.0f + (excess * stiffness));
                return (x < limit) ? limit - displacement : limit + displacement;
            };

            if (scale > 3.0f)
                scale = rubberBand(scale, 3.0f, 1.6f);
            else if (scale < 0.25f)
                scale = rubberBand(scale, 0.25f, 20.0f);

            if (!isPerformingGesture()) {
                animationStartScale = scale;
                animationTargetScale = clampedScale;
                bounceAnimator.start();
                return;
            }
        } else {
            scale = clampedScale;
        }

        float const oldScale = getViewScale();
        cnv->zoomScale = scale;

        if (oldScale > 0) {
            float const ratio = getViewScale() / oldScale;
            Point<float> const contentPointUnderMouse = (viewPosition + centre);
            viewPosition = (contentPointUnderMouse * ratio) - centre;
        }

        updateCanvasTransform();
    }

    void updateCanvasTransform()
    {
        auto const transform = AffineTransform::scale(getViewScale()).translated(-viewPosition.x, -viewPosition.y);
        cnv->setTransform(transform);
        updateScrollbars();

        if (onScroll)
            onScroll();
        repaint();

        minimap.updateMinimap(getViewArea());
    }

    bool isPerformingGesture()
    {
#if JUCE_IOS || JUCE_MAC
        return OSUtils::ScrollTracker::isPerformingGesture();
#else
        return panner.isConsumingTouchGesture();
#endif
    }

    // Returns a one-shot move animation so you can cascade it with another animation
    Animator const& getMoveAnimation(Point<float> const pos)
    {
        static Animator moveAnimation = ValueAnimatorBuilder {}.build();

        moveAnimation.complete();
        moveAnimation = ValueAnimatorBuilder {}
                            .withEasing(Easings::createEaseInOutCubic())
                            .withDurationMs(300)
                            .withValueChangedCallback([this](float v) {
                                auto const currentScale = jmap(v, 0.0f, 1.0f, animationStartScale, animationTargetScale);
                                cnv->zoomScale = currentScale;
                                logicalScale = currentScale;
                                auto const currentPos = makeAnimationLimits(animationStartPos, animationEndPos).lerp(v);
                                setViewPosition(currentPos);
                            })
                            .build();

        animationStartScale = animationTargetScale = getViewScale();
        animationStartPos = getViewPosition();
        animationEndPos = pos;
        return moveAnimation;
    }

    void setViewPosition(Point<float> newPos)
    {
        auto scale = getViewScale();
        auto const contentWidth = cnv->getWidth() * scale;
        auto const contentHeight = cnv->getHeight() * scale;

        auto const maxX = std::max(0.0f, contentWidth - getWidth());
        auto const maxY = std::max(0.0f, contentHeight - getHeight());

        viewPosition.x = std::clamp<float>(newPos.x, 0.0f, maxX);
        viewPosition.y = std::clamp<float>(newPos.y, 0.0f, maxY);

        updateCanvasTransform();
    }

    Point<float> getViewPosition()
    {
        return viewPosition;
    }

    Point<float> getViewPositionUnscaled()
    {
        return viewPosition / getViewScale();
    }

    Rectangle<float> getViewArea()
    {
        auto scale = getViewScale();
        return { viewPosition.x / scale, viewPosition.y / scale, getWidth() / scale, getHeight() / scale };
    }

    float getViewPositionX()
    {
        return viewPosition.x;
    }

    float getViewPositionY()
    {
        return viewPosition.y;
    }

    float getViewScale()
    {
        return getValue<float>(cnv->zoomScale);
    }

    void updateScrollbars()
    {
        if (getViewArea().isEmpty())
            return;

        auto const thickness = 9.f;
        auto localArea = getLocalBounds().reduced(2);

        vbar.setBounds(localArea.removeFromRight(thickness).withTrimmedBottom(thickness).translated(-1, 0));
        hbar.setBounds(localArea.removeFromBottom(thickness).translated(0, -1));

        auto const scale = getViewScale();
        auto const contentArea = Rectangle<float>(viewPosition.x / scale, viewPosition.y / scale, getWidth(), getHeight());

        Rectangle<float> objectArea = contentArea.withPosition(cnv->canvasOrigin.toFloat());
        for (auto const object : cnv->objects) {
            objectArea = objectArea.getUnion(object->getBounds().toFloat());
        }

        auto const totalArea = contentArea.getUnion(objectArea);

        hbar.setRangeLimitsAndCurrentRange(totalArea.getX(), totalArea.getRight(), contentArea.getX(), contentArea.getRight());
        vbar.setRangeLimitsAndCurrentRange(totalArea.getY(), totalArea.getBottom(), contentArea.getY(), contentArea.getBottom());
    }

    void resized() override
    {
        auto const currentBounds = getLocalBounds();

        if (SettingsFile::getInstance()->getProperty<bool>("centre_resized_canvas")) {
            if (!previousBounds.isEmpty()) {
                auto const deltaW = (float)(currentBounds.getWidth() - previousBounds.getWidth());
                auto const deltaH = (float)(currentBounds.getHeight() - previousBounds.getHeight());

                auto newPos = viewPosition;
                newPos.x -= deltaW * 0.5f;
                newPos.y -= deltaH * 0.5f;

                setViewPosition(newPos);
            }
        }

        previousBounds = currentBounds;

        updateScrollbars();
        updateCanvasTransform();

        minimap.setBounds(Rectangle<int>(getWidth() - 200, getHeight() - 150, 190, 140));
    }

    std::function<void()> onScroll = [] { };

private:
    Point<float> viewPosition;

    MousePanner panner;
    ViewportScrollBar vbar = ViewportScrollBar(true, this);
    ViewportScrollBar hbar = ViewportScrollBar(false, this);
    Minimap minimap;

    PluginEditor* editor;
    Canvas* cnv;

    float logicalScale = 1.0f;
    Rectangle<int> previousBounds;
    bool scaleChanged = false;

    Point<float> smoothedDelta;
    const float smoothingFactor = 0.3f;

    VBlankAnimatorUpdater updater { this };
    float animationStartScale, animationTargetScale;
    Point<float> animationStartPos, animationEndPos, animationCentre;
    Animator zoomAnimator = ValueAnimatorBuilder {}
                                .withEasing(Easings::createEaseInOutCubic())
                                .withDurationMs(220)
                                .withValueChangedCallback([this](float v) {
                                    float currentScale = makeAnimationLimits(animationStartScale, animationTargetScale).lerp(v);
                                    applyScale(currentScale, animationCentre, false);
                                })
                                .build();

    Animator moveAnimator = ValueAnimatorBuilder {}
                                .withEasing(Easings::createEaseInOutCubic())
                                .withDurationMs(300)
                                .withValueChangedCallback([this](float v) {
                                    auto const currentScale = jmap(v, 0.0f, 1.0f, animationStartScale, animationTargetScale);
                                    cnv->zoomScale = currentScale;
                                    logicalScale = currentScale;
                                    auto const currentPos = makeAnimationLimits(animationStartPos, animationEndPos).lerp(v);
                                    setViewPosition(currentPos);
                                })
                                .build();

    Animator bounceAnimator = ValueAnimatorBuilder {}
                                  .withEasing(Easings::createEaseOut())
                                  .withDurationMs(220)
                                  .withValueChangedCallback([this](float v) {
                                      float const oldScale = getViewScale();
                                      cnv->zoomScale = makeAnimationLimits(animationStartScale, animationTargetScale).lerp(v);

                                      auto const mousePos = panner.getPointerCentre();
                                      if (oldScale > 0) {
                                          float const ratio = getViewScale() / oldScale;
                                          Point<float> const contentPointUnderMouse = (viewPosition + mousePos);
                                          viewPosition = (contentPointUnderMouse * ratio) - mousePos;
                                      }
                                      logicalScale = getViewScale();
                                      updateCanvasTransform();
                                  })
                                  .build();

#if JUCE_IOS
    TimedCallback gestureCheck = TimedCallback([this]() {
        auto scale = getValue<float>(cnv->zoomScale);
        if (!isPerformingGesture() && bounceAnimator.isComplete() && (scale < 0.25f || scale > 3.0f)) {
            magnify(std::clamp(scale, 0.25f, 3.0f));
        }
    });
#endif
};
