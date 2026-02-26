/*
 // Copyright (c) 2021-2025 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_animation/juce_animation.h>
#include <juce_opengl/juce_opengl.h>
using namespace gl;

#include <nanovg.h>

#include <utility>

#include "Object.h"
#include "Objects/ObjectBase.h"
#include "Connection.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Utility/SettingsFile.h"

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport final : public Viewport
    , public NVGComponent
    , public Timer {
        
    class Minimap final : public Component
        , public Timer
        , public AsyncUpdater {
    public:
        explicit Minimap(Canvas* canvas)
            : cnv(canvas)
        {
        }

        void handleAsyncUpdate() override
        {
            auto const area = visibleArea / getValue<float>(cnv->zoomScale);
            bool renderMinimap = cnv->objects.not_empty();
            for (auto const* obj : cnv->objects) {
                if (obj->getBounds().intersects(area)) {
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
                minimapTargetAlpha = fadedIn;
                if (!isTimerRunning())
                    startTimer(11);
            } else if (!renderMinimap && minimapAlpha != fadedOut) {
                setVisible(showMinimap == 3);
                minimapTargetAlpha = fadedOut;
                if (!isTimerRunning())
                    startTimer(11);
            }
        }

        void updateMinimap(Rectangle<int> const area)
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
                Rectangle<int> fullBounds, viewBounds;
                int offsetX, offsetY;
                float scale;
            } b;

            auto const zoom = getValue<float>(cnv->zoomScale);
            b.viewBounds = cnv->viewport->getViewArea() / zoom;
            auto allObjectBounds = Rectangle<int>(cnv->canvasOrigin.x, cnv->canvasOrigin.y, b.viewBounds.getWidth(), b.viewBounds.getHeight());
            for (auto const* object : cnv->objects) {
                allObjectBounds = allObjectBounds.getUnion(object->getBounds());
            }

            b.fullBounds = isMouseDown ? boundsBeforeDrag : allObjectBounds.getUnion(b.viewBounds);

            b.offsetX = -std::min(0, b.fullBounds.getX() - cnv->canvasOrigin.x);
            b.offsetY = -std::min(0, b.fullBounds.getY() - cnv->canvasOrigin.y);
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

            float const x = cnv->viewport->getViewWidth() - (width + 10);
            float const y = cnv->viewport->getViewHeight() - (height + 10);

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
                cnv->viewport->setViewPosition(downPosition + e.getOffsetFromDragStart() / map.scale);
            }
        }

        void timerCallback() override
        {
            minimapAlpha = jmap<float>(0.2f, minimapAlpha, minimapTargetAlpha);
            if (approximatelyEqual(minimapAlpha, minimapTargetAlpha, Tolerance<float> {}.withAbsolute(0.01f))) {
                minimapAlpha = minimapTargetAlpha;
                stopTimer();
            }
            cnv->editor->nvgSurface.invalidateAll();
        }

    private:
        Canvas* cnv;
        float minimapAlpha = 0.0f;
        float minimapTargetAlpha = 0.0f;
        Rectangle<int> visibleArea;
        Point<int> downPosition;
        Rectangle<int> boundsBeforeDrag;
        bool isMouseDown : 1 = false;
        bool isMouseOver : 1 = false;
        static constexpr float width = 180;
        static constexpr float height = 130;
    };

    
    class MousePanner final : public MouseListener {
    public:
        explicit MousePanner(CanvasViewport* vp, Canvas* cnv)
            : viewport(vp), canvas(cnv)
        {
            canvas->addMouseListener(this, false);
        }
        
        ~MousePanner()
        {
            if(canvas) canvas->removeMouseListener(this);
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
            if(enableMousePanning) {
                viewport->zoomAnimator.complete();
                viewport->moveAnimator.complete();
                
                e.originalComponent->setMouseCursor(MouseCursor::DraggingHandCursor);
                downPosition = viewport->getViewPosition();
                downCanvasOrigin = viewport->cnv->canvasOrigin;
            }
        }
        
        bool doesMouseEventComponentBlockViewportDrag (const Component* eventComp)
        {
            for (auto c = eventComp; c != nullptr && c != viewport; c = c->getParentComponent())
                if (c->getViewportIgnoreDragFlag())
                    return true;

            return false;
        }

        void mouseDrag(MouseEvent const& e) override
        {
            if(enableMousePanning) {
                float const scale = std::sqrt(std::abs(viewport->cnv->getTransform().getDeterminant()));
                auto const infiniteCanvasOriginOffset = (viewport->cnv->canvasOrigin - downCanvasOrigin) * scale;
                viewport->setViewPosition(infiniteCanvasOriginOffset + downPosition - (scale * e.getOffsetFromDragStart().toFloat()).roundToInt());
            }
            
            auto index = e.source.getIndex();
            bool const touchMode = SettingsFile::getInstance()->getProperty<bool>("touch_mode");
            if(touchMode && e.source.isTouch() && index < 2)
            {
                if(doesMouseEventComponentBlockViewportDrag(e.eventComponent))
                    return;
                
                multiTouchOffset[index] = e.getOffsetFromDragStart().toFloat();
                multiTouchStartPosition[index] = e.getMouseDownPosition().toFloat();
                
                if(index == 1 && multiTouchOffset[0].getDistanceFromOrigin() > 2.0f && multiTouchOffset[1].getDistanceFromOrigin() > 2.0f)
                {
                    handleTouchGesture(e);
                }
            }
        }
        
        void mouseUp(MouseEvent const& e) override
        {
            multiTouchOffset[0] = {};
            multiTouchOffset[1] = {};
            multiTouchLastOffset = {};
            lastPinchScale = 1.0f;
            smoothedPinchScale = 1.0f;
            
            consumingTouchGesture = false;
        }
        
        void handleTouchGesture(MouseEvent const& e)
        {
            auto panOffset = (multiTouchOffset[0] + multiTouchOffset[1]) * 0.5f;
            
            auto centre = (multiTouchStartPosition[0] + multiTouchOffset[0] + multiTouchStartPosition[1] + multiTouchOffset[1]) * 0.5f;
            auto startDistance = multiTouchStartPosition[0].getDistanceFrom(multiTouchStartPosition[1]);
            auto currentDistance = (multiTouchStartPosition[0] + multiTouchOffset[0]).getDistanceFrom(multiTouchStartPosition[1] + multiTouchOffset[1]);
            float pinchScale = (startDistance > 0.0f) ? (currentDistance / startDistance) : 1.0f;
            bool isPan   = panOffset.getDistanceFromOrigin() > 8.0f;
            bool isPinch = std::abs(pinchScale - 1.0f) > 0.05f;
            
            if(isPan)
            {
                auto panDelta = (panOffset - multiTouchLastOffset) / 256.0f;
                auto canvasZoom = getValue<float>(canvas->zoomScale);
                
                multiTouchLastOffset = (multiTouchOffset[0] + multiTouchOffset[1]) * 0.5f;
                                
                juce::MouseWheelDetails details;
                details.deltaX = panDelta.x * canvasZoom;
                details.deltaY = panDelta.y * canvasZoom;
                details.isReversed = false;
                details.isSmooth = true;
                details.isInertial = false;
                
                auto event = MouseEvent (e.source, centre,
                                   e.mods, e.pressure, e.orientation, e.rotation, e.tiltX, e.tiltY,
                                   viewport, viewport, e.eventTime,
                                   e.mouseDownPosition, e.mouseDownTime, e.getNumberOfClicks(), true);
                
                viewport->mouseWheelMove(event, details);
                
                consumingTouchGesture = true;
            }
            if(isPinch)
            {
                smoothedPinchScale += (pinchScale - smoothedPinchScale) * 0.4f;
                float pinchScaleDelta = (smoothedPinchScale - lastPinchScale) + 1.0f;
                pinchScaleDelta = juce::jlimit(0.85f, 1.15f, pinchScaleDelta);
                lastPinchScale = smoothedPinchScale;
                
                viewport->mouseMagnify(e.withNewPosition(centre), pinchScaleDelta);
                consumingTouchGesture = true;
            }
        }
        

    private:
        CanvasViewport* viewport;
        Point<int> downPosition;
        Point<int> downCanvasOrigin;
        
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
        struct FadeTimer final : private ::Timer {
            std::function<bool()> callback;

            void start(int const interval, std::function<bool()> cb)
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

        struct FadeAnimator final : private ::Timer {
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
        ViewportScrollBar(bool const isVertical, CanvasViewport* viewport)
            : isVertical(isVertical)
            , viewport(viewport)
        {
            scrollBarThickness = viewport->getScrollBarThickness();
        }

        bool hitTest(int const x, int const y) override
        {
            Rectangle<float> fullBounds;
            if (isVertical)
                fullBounds = thumbBounds.withY(2).withHeight(getHeight() - 4);
            else
                fullBounds = thumbBounds.withX(2).withWidth(getWidth() - 4);

            if (fullBounds.contains(x, y))
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

        void setGrowAnimation(float const newGrowValue)
        {
            growAnimation = newGrowValue;
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

            scrollbarBgCol.a = (1.0f - growAnimation) * 150; // 0-150 opacity, not full opacity when active
            nvgDrawRoundedRect(nvg, fullBounds.getX(), fullBounds.getY(), fullBounds.getWidth(), fullBounds.getHeight(), scrollbarBgCol, scrollbarBgCol, thumbCornerRadius);

            auto const scrollBarThumbCol = isMouseDragging ? activeScrollbarCol : scrollbarCol;
            nvgDrawRoundedRect(nvg, growingBounds.getX(), growingBounds.getY(), growingBounds.getWidth(), growingBounds.getHeight(), scrollBarThumbCol, scrollBarThumbCol, thumbCornerRadius);
        }

        NVGcolor scrollbarCol;
        NVGcolor activeScrollbarCol;
        NVGcolor scrollbarBgCol;

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

    struct ViewportPositioner final : public Component::Positioner {
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
        : NVGComponent(this)
        , minimap(cnv)
        , editor(parent)
        , cnv(cnv)
        , panner(this, cnv)
    {
        setScrollBarsShown(false, false);

        setPositioner(new ViewportPositioner(*this));
        setScrollOnDragMode(ScrollOnDragMode::never);

        setScrollBarThickness(8);

        addChildComponent(minimap);
        addAndMakeVisible(vbar);
        addAndMakeVisible(hbar);
        
        updater.addAnimator(zoomAnimator);
        updater.addAnimator(moveAnimator);
        updater.addAnimator(bounceAnimator);
        
#if JUCE_IOS
        gestureCheck.startTimer(20);
#endif

        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this, [this]{
            return editor->getTabComponent().getVisibleCanvases().contains(this->cnv);
        }));

        lookAndFeelChanged();
    }

    ~CanvasViewport() override
    {
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
            // Cached geometry can look thicker/thinner at different zoom scales, so we reset all cached connections when zooming is done
            NVGCachedPath::resetAll();
        }

        scaleChanged = false;
        editor->nvgSurface.invalidateAll();
    }

    void setViewPositionAnimated(Point<int> const pos)
    {
        if (getViewPosition() != pos) {
            startPos = getViewPosition();
            targetPos = pos;
            moveAnimator.start();
        }
    }

    void lookAndFeelChanged() override
    {
        auto const scrollbarColour = hbar.findColour(ScrollBar::ColourIds::thumbColourId);
        auto const scrollbarCol = nvgColour(scrollbarColour);
        auto const canvasBgColour = PlugDataColours::canvasBackgroundColour;
        auto const activeScrollbarCol = nvgColour(scrollbarColour.interpolatedWith(canvasBgColour.contrasting(0.6f), 0.7f));
        auto const scrollbarBgCol = nvgColour(scrollbarColour.interpolatedWith(canvasBgColour, 0.7f));

        hbar.scrollbarCol = scrollbarCol;
        vbar.scrollbarCol = scrollbarCol;
        hbar.activeScrollbarCol = activeScrollbarCol;
        vbar.activeScrollbarCol = activeScrollbarCol;
        hbar.scrollbarBgCol = scrollbarBgCol;
        vbar.scrollbarBgCol = scrollbarBgCol;

        hbar.repaint();
        vbar.repaint();
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
        // Check event time to filter out duplicate events
        // This is a workaround for a bug in JUCE that can cause mouse events to be duplicated when an object has a MouseListener on its parent
        if (e.eventTime == lastScrollTime)
            return;
        
        moveAnimator.complete();
        zoomAnimator.complete();
                
        auto scrollFactor = 1.0f / (1.0f - wheel.deltaY);
        if (e.mods.isCommandDown()) {
            if (wheel.isSmooth || wheel.deltaY < 0.04) {
                zoomAnchorScreen = Desktop::getInstance().getMainMouseSource().getScreenPosition();
                applyScale(std::clamp(getValue<float>(cnv->zoomScale) * scrollFactor, 0.25f, 3.0f), false);
            }
            else {
                mouseMagnify(e, scrollFactor);
            }
        }

        Viewport::mouseWheelMove(e, wheel);
        lastScrollTime = e.eventTime;
    }

    void mouseMagnify(MouseEvent const& e, float const scrollFactor) override
    {
        if (e.eventTime == lastZoomTime || !cnv)
            return;

        moveAnimator.complete();
        zoomAnimator.complete();

        zoomAnchorScreen = Desktop::getInstance().getMainMouseSource().getScreenPosition();
        
        float const rawScale = logicalScale * scrollFactor;

#if JUCE_MAC || JUCE_IOS
        applyScale(rawScale, true);
#else
        applyScale(rawScale, e.source.isTouch());
#endif
        
        lastZoomTime = e.eventTime;
    }

    void magnify(float newScaleFactor)
    {
        newScaleFactor = std::clamp(newScaleFactor, 0.25f, 3.0f);
        if (approximatelyEqual(newScaleFactor, 0.0f)) newScaleFactor = 1.0f;
        if (newScaleFactor == animationTargetScale) return;

        zoomAnchorScreen = Desktop::getInstance().getMainMouseSource().getScreenPosition();
        animationStartScale = lastScaleFactor;
        animationTargetScale = newScaleFactor;
        scaleChanged = true;
        
        zoomAnimator.start();
    }
    
    void applyScale(float scale, bool allowOvershoot)
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
                scale = rubberBand(scale, 3.0f, 1.8f);
            else if (scale < 0.25f)
                scale = rubberBand(scale, 0.25f, 22.0f);
            
            if (!isPerformingGesture()) {
                animationStartScale = scale;
                animationTargetScale = clampedScale;
                bounceStartPosition = cnv->getLocalPoint(nullptr, zoomAnchorScreen);
                bounceAnimator.start();
                return;
            }
        }
        else
        {
            ignoreUnused(allowOvershoot);
            scale = clampedScale;
        }

        lastScaleFactor = scale;
        auto const oldPosition = cnv->getLocalPoint(nullptr, zoomAnchorScreen);
        cnv->setTransform(AffineTransform().scaled(scale));
        auto const newPosition = cnv->getLocalPoint(nullptr, zoomAnchorScreen);
        cnv->setTopLeftPosition(cnv->getPosition() + (newPosition - oldPosition).roundToInt());
        resized();
        cnv->zoomScale = scale;
    }

    void adjustScrollbarBounds()
    {
        if (getViewArea().isEmpty())
            return;

        auto const thickness = getScrollBarThickness();
        auto localArea = getLocalBounds().reduced(2);

        vbar.setBounds(localArea.removeFromRight(thickness).withTrimmedBottom(thickness).translated(-1, 0));
        hbar.setBounds(localArea.removeFromBottom(thickness).translated(0, -1));

        float const scale = 1.0f / std::sqrt(std::abs(cnv->getTransform().getDeterminant()));
        auto const contentArea = getViewArea() * scale;

        Rectangle<int> objectArea = contentArea.withPosition(cnv->canvasOrigin);
        for (auto const object : cnv->objects) {
            objectArea = objectArea.getUnion(object->getBounds());
        }

        auto const totalArea = contentArea.getUnion(objectArea);

        hbar.setRangeLimitsAndCurrentRange(totalArea.getX(), totalArea.getRight(), contentArea.getX(), contentArea.getRight());
        vbar.setRangeLimitsAndCurrentRange(totalArea.getY(), totalArea.getBottom(), contentArea.getY(), contentArea.getBottom());
    }

    void componentMovedOrResized(Component& c, bool const moved, bool const resized) override
    {
        if (editor->isInPluginMode())
            return;
        
        if(moved || resized)
            Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();
    }

    void visibleAreaChanged(Rectangle<int> const& r) override
    {
        if (scaleChanged) {
            cnv->isZooming = true;
            startTimer(150);
        }

        onScroll();
        adjustScrollbarBounds();
        minimap.updateMinimap(r);
        
        cnv->getParentComponent()->setSize(getWidth(), getHeight());
    }

    void resized() override
    {
        vbar.setVisible(isVerticalScrollBarShown());
        hbar.setVisible(isHorizontalScrollBarShown());
        minimap.setBounds(Rectangle<int>(getWidth() - 200, getHeight() - 150, 190, 140));

        if (editor->isInPluginMode())
            return;

        adjustScrollbarBounds();

        if (!SettingsFile::getInstance()->getProperty<bool>("centre_resized_canvas")) {
            Viewport::resized();
            return;
        }

        float scale = std::sqrt(std::abs(cnv->getTransform().getDeterminant()));

        // centre canvas when resizing viewport
        auto getCentre = [this, scale](Rectangle<int> const bounds) {
            if (scale > 1.0f) {
                auto const point = cnv->getLocalPoint(this, bounds.withZeroOrigin().getCentre());
                return point * scale;
            }
            return getViewArea().withZeroOrigin().getCentre();
        };

        auto const currentCentre = getCentre(previousBounds);
        previousBounds = getBounds();
        Viewport::resized();
        auto const newCentre = getCentre(getBounds());

        auto const offset = currentCentre - newCentre;
        setViewPosition(getViewPosition() + offset);
    }

    // Never respond to arrow keys, they have a different meaning
    bool keyPressed(KeyPress const& key) override
    {
        return false;
    }
        
    bool isConsumingTouchGesture()
    {
        return panner.isConsumingTouchGesture();
    }
        
        
    bool isPerformingGesture()
    {
#if JUCE_IOS || JUCE_MAC
        return OSUtils::ScrollTracker::isPerformingGesture();
#else
        return isConsumingTouchGesture();
#endif
    }

    std::function<void()> onScroll = [] { };

private:
    enum Timers { ResizeTimer,
        AnimationTimer };


    Minimap minimap;
    Time lastScrollTime;
    Time lastZoomTime;
    float lastScaleFactor = 1.0f;
    float logicalScale = 1.0f;
    PluginEditor* editor;
    Canvas* cnv;
    Rectangle<int> previousBounds;
    MousePanner panner;
    ViewportScrollBar vbar = ViewportScrollBar(true, this);
    ViewportScrollBar hbar = ViewportScrollBar(false, this);
    
    VBlankAnimatorUpdater updater { this };
        
    float animationStartScale = 1.0f;
    float animationTargetScale = 1.0f;
    Point<float> zoomAnchorScreen;
    Animator zoomAnimator = juce::ValueAnimatorBuilder{}
                               .withEasing(juce::Easings::createEaseInOutCubic())
                               .withDurationMs(180)
                               .withValueChangedCallback([this](float v) {
                                   float currentScale = makeAnimationLimits(animationStartScale, animationTargetScale).lerp(v);
                                   applyScale(currentScale, false);
                               }).build();
    Point<int> startPos;
    Point<int> targetPos;
    Animator moveAnimator = juce::ValueAnimatorBuilder{}
                               .withEasing(juce::Easings::createEaseInOutCubic())
                               .withDurationMs(300)
                               .withValueChangedCallback([this](float v) {
                                   auto const movedPos = makeAnimationLimits(startPos, targetPos).lerp(v);
                                   setViewPosition(movedPos.x, movedPos.y);
                               }).build();

    Point<float> bounceStartPosition;
    Animator bounceAnimator = juce::ValueAnimatorBuilder{}
        .withEasing(juce::Easings::createEaseOut())
        .withDurationMs(240)
        .withValueChangedCallback([this](float v) {
            float scale = makeAnimationLimits(animationStartScale, animationTargetScale).lerp(v);
            cnv->setTransform(AffineTransform().scaled(scale));
            auto const newPosition = cnv->getLocalPoint(nullptr, zoomAnchorScreen);
            cnv->setTopLeftPosition(cnv->getPosition() + (newPosition - bounceStartPosition).roundToInt());
            
            resized();
            cnv->zoomScale = scale;
            logicalScale = scale;
            
        }).build();
        
#if JUCE_IOS
    TimedCallback gestureCheck = TimedCallback([this](){
        auto scale = getValue<float>(cnv->zoomScale);
        if(!isPerformingGesture() && bounceAnimator.isComplete() && (scale < 0.25f || scale > 3.0f))
        {
            animationStartScale = scale;
            animationTargetScale = std::clamp(scale, 0.25f, 3.0f);
            bounceStartPosition = cnv->getLocalPoint(nullptr, zoomAnchorScreen);
            bounceAnimator.start();
        }
    });
#endif
    
    bool scaleChanged = false;
};
