/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace gl;

#include <nanovg.h>

#include <utility>

#include "Object.h"
#include "Connection.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Utility/SettingsFile.h"

class Minimap : public Component
    , public Timer
    , public AsyncUpdater {
public:
    Minimap(Canvas* canvas)
        : cnv(canvas)
    {
    }

    void handleAsyncUpdate() override
    {
        auto area = visibleArea / getValue<float>(cnv->zoomScale);
        bool renderMinimap = cnv->objects.not_empty();
        for (auto* obj : cnv->objects) {
            if (obj->getBounds().intersects(area)) {
                renderMinimap = false;
                break;
            }
        }
        
        auto showMinimap = SettingsFile::getInstance()->getProperty<int>("show_minimap");
        float fadedIn;
        float fadedOut;
        if(showMinimap == 1)
        {
            fadedIn = 0.0f;
            fadedOut = 0.0f;
        }
        else if(showMinimap == 2)
        {
            fadedIn = 1.0f;
            fadedOut = 0.0f;
        }
        else if(showMinimap == 3)
        {
            fadedIn = 1.0f;
            fadedOut = 0.5f;
            if(isMouseOver) renderMinimap = true;
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

    void updateMinimap(Rectangle<int> area)
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
        Rectangle<int> allObjectBounds = Rectangle<int>(cnv->canvasOrigin.x, cnv->canvasOrigin.y, b.viewBounds.getWidth(), b.viewBounds.getHeight());
        for (auto* object : cnv->objects) {
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

        auto canvasBackground = cnv->findColour(PlugDataColour::canvasBackgroundColourId);
        auto mapBackground = canvasBackground.contrasting(0.5f);

        // draw background
        nvgFillColor(nvg, NVGComponent::convertColour(mapBackground.withAlpha(0.4f)));
        nvgFillRoundedRect(nvg, x - 4, y - 4, width + 8, height + 8, Corners::largeCornerRadius);

        nvgFillColor(nvg, NVGComponent::convertColour(mapBackground.withAlpha(0.8f)));

        // draw objects
        for (auto* object : cnv->objects) {
            auto b = (object->getBounds().reduced(Object::margin).translated(map.offsetX, map.offsetY) - cnv->canvasOrigin).toFloat() * map.scale;
            nvgFillRoundedRect(nvg, x + b.getX(), y + b.getY(), b.getWidth(), b.getHeight(), Corners::objectCornerRadius * map.scale);
        }

        // draw visible area
        nvgDrawRoundedRect(nvg, x + (map.offsetX + map.viewBounds.getX() - cnv->canvasOrigin.x) * map.scale, y + (map.offsetY + map.viewBounds.getY() - cnv->canvasOrigin.y) * map.scale, map.viewBounds.getWidth() * map.scale, map.viewBounds.getHeight() * map.scale, NVGComponent::convertColour(canvasBackground.withAlpha(0.6f)), NVGComponent::convertColour(canvasBackground.contrasting(0.4f)), 0.0f);
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
        auto realViewBounds = Rectangle<int>((map.offsetX + map.viewBounds.getX() - cnv->canvasOrigin.x) * map.scale, (map.offsetY + map.viewBounds.getY() - cnv->canvasOrigin.y) * map.scale, map.viewBounds.getWidth() * map.scale, map.viewBounds.getHeight() * map.scale);
        isMouseDown = realViewBounds.contains(e.getMouseDownPosition());
    }

    void mouseUp(MouseEvent const& e) override
    {
        auto viewBounds = cnv->viewport->getViewArea();
        downPosition = viewBounds.getPosition();
        isMouseDown = false;
        updateMinimap(viewBounds);
        cnv->editor->nvgSurface.invalidateAll();
    }

    void mouseDrag(MouseEvent const& e) override
    {
        auto map = getMapBounds();

        if (isMouseDown) {
            cnv->viewport->setViewPosition(downPosition + (e.getOffsetFromDragStart() / map.scale));
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
    bool isMouseDown:1 = false;
    bool isMouseOver:1 = false;
    static constexpr float width = 180;
    static constexpr float height = 130;
};

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport final : public Viewport
    , public NVGComponent
    , public MultiTimer {
    class MousePanner final : public MouseListener {
    public:
        explicit MousePanner(CanvasViewport* vp)
            : viewport(vp)
        {
        }

        void enablePanning(bool const enabled)
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
        // thus giving us a chance to attach the mouselistener on the middle-mouse click event
        // Specifically, we use the hitTest in on-canvas objects to check if panning mod is down
        // Because the hitTest can decided where the mouseEvent goes.
        void mouseDown(MouseEvent const& e) override
        {
            if (!e.mods.isLeftButtonDown() && !e.mods.isMiddleButtonDown())
                return;

            // Cancel the animation timer for the search panel
            viewport->stopTimer(Timers::AnimationTimer);

            e.originalComponent->setMouseCursor(MouseCursor::DraggingHandCursor);
            downPosition = viewport->getViewPosition();
            downCanvasOrigin = viewport->cnv->canvasOrigin;
        }

        void mouseDrag(MouseEvent const& e) override
        {
            float const scale = std::sqrt(std::abs(viewport->cnv->getTransform().getDeterminant()));

            auto const infiniteCanvasOriginOffset = (viewport->cnv->canvasOrigin - downCanvasOrigin) * scale;
            viewport->setViewPosition(infiniteCanvasOriginOffset + downPosition - (scale * e.getOffsetFromDragStart().toFloat()).roundToInt());
        }

    private:
        CanvasViewport* viewport;
        Point<int> downPosition;
        Point<int> downCanvasOrigin;
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
    {
        setScrollBarsShown(false, false);

        setPositioner(new ViewportPositioner(*this));

#if JUCE_IOS
        setScrollOnDragMode(ScrollOnDragMode::never);
#endif

        setScrollBarThickness(8);

        addChildComponent(minimap);
        addAndMakeVisible(vbar);
        addAndMakeVisible(hbar);

        setCachedComponentImage(new NVGSurface::InvalidationListener(editor->nvgSurface, this));

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

    void timerCallback(int const ID) override
    {
        switch (ID) {
        case Timers::ResizeTimer: {
            stopTimer(Timers::ResizeTimer);
            cnv->isZooming = false;

            // Cached geometry can look thicker/thinner at different zoom scales, so we update all cached connections when zooming is done
            if (scaleChanged) {
                // Cached geometry can look thicker/thinner at different zoom scales, so we reset all cached connections when zooming is done
                NVGCachedPath::resetAll();
            }

            scaleChanged = false;
            editor->nvgSurface.invalidateAll();
        } break;
        case Timers::AnimationTimer: {
            auto lerp = [](Point<int> const start, Point<int> const end, float const t) {
                return start.toFloat() + (end.toFloat() - start.toFloat()) * t;
            };
            auto const movedPos = lerp(startPos, targetPos, lerpAnimation);
            setViewPosition(movedPos.x, movedPos.y);

            if (lerpAnimation >= 1.0f) {
                stopTimer(Timers::AnimationTimer);
                lerpAnimation = 0.0f;
            }

            lerpAnimation += animationSpeed;
            break;
        }
        }
    }

    void setViewPositionAnimated(Point<int> const pos)
    {
        if (getViewPosition() != pos) {
            startPos = getViewPosition();
            targetPos = pos;
            lerpAnimation = 0.0f;
            auto const distance = startPos.getDistanceFrom(pos) * getValue<float>(cnv->zoomScale);
            // speed up animation if we are traveling a shorter distance (hardcoded for now)
            animationSpeed = distance < 10.0f ? 0.1f : 0.02f;

            startTimer(Timers::AnimationTimer, 1000 / 90);
        }
    }

    void lookAndFeelChanged() override
    {
        auto const scrollbarColour = hbar.findColour(ScrollBar::ColourIds::thumbColourId);
        auto const scrollbarCol = convertColour(scrollbarColour);
        auto const canvasBgColour = findColour(PlugDataColour::canvasBackgroundColourId);
        auto const activeScrollbarCol = convertColour(scrollbarColour.interpolatedWith(canvasBgColour.contrasting(0.6f), 0.7f));
        auto const scrollbarBgCol = convertColour(scrollbarColour.interpolatedWith(canvasBgColour, 0.7f));

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

        // Cancel the animation timer for the search panel
        stopTimer(Timers::AnimationTimer);

        if (e.mods.isCommandDown()) {
            mouseMagnify(e, 1.0f / (1.0f - wheel.deltaY));
        }

        Viewport::mouseWheelMove(e, wheel);
        lastScrollTime = e.eventTime;
    }

    void mouseMagnify(MouseEvent const& e, float const scrollFactor) override
    {
        // Check event time to filter out duplicate events
        // This is a workaround for a bug in JUCE that can cause mouse events to be duplicated when an object has a MouseListener on its parent
        if (e.eventTime == lastZoomTime || !cnv)
            return;

        // Apply and limit zoom
        magnify(std::clamp(getValue<float>(cnv->zoomScale) * scrollFactor, 0.25f, 3.0f));
        lastZoomTime = e.eventTime;
    }

    void magnify(float newScaleFactor)
    {
        if (approximatelyEqual(newScaleFactor, 0.0f)) {
            newScaleFactor = 1.0f;
        }

        if (newScaleFactor == lastScaleFactor) // float comparison ok here as it's set by the same value
            return;

        lastScaleFactor = newScaleFactor;

        scaleChanged = true;

        // Get floating point mouse position relative to screen
        auto const mousePosition = Desktop::getInstance().getMainMouseSource().getScreenPosition();
        // Get mouse position relative to canvas
        auto const oldPosition = cnv->getLocalPoint(nullptr, mousePosition);
        // Apply transform and make sure viewport bounds get updated
        cnv->setTransform(AffineTransform().scaled(newScaleFactor));
        // After zooming, get mouse position relative to canvas again
        auto const newPosition = cnv->getLocalPoint(nullptr, mousePosition);
        // Calculate offset to keep our mouse position the same as before this zoom action
        auto const offset = newPosition - oldPosition;
        cnv->setTopLeftPosition(cnv->getPosition() + offset.roundToInt());

        // This is needed to make sure the viewport the current canvas bounds to the lastVisibleArea variable
        // Without this, future calls to getViewPosition() will give wrong results
        resized();
        cnv->zoomScale = newScaleFactor;
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

        Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();
    }

    void visibleAreaChanged(Rectangle<int> const& r) override
    {
        if (scaleChanged) {
            cnv->isZooming = true;
            startTimer(Timers::ResizeTimer, 150);
        }

        onScroll();
        adjustScrollbarBounds();
        minimap.updateMinimap(r);
        editor->nvgSurface.invalidateAll();
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

    std::function<void()> onScroll = [] { };

private:
    enum Timers { ResizeTimer,
        AnimationTimer };
    Point<int> startPos;
    Point<int> targetPos;
    float lerpAnimation;
    float animationSpeed;

    Minimap minimap;
    Time lastScrollTime;
    Time lastZoomTime;
    float lastScaleFactor = -1.0f;
    PluginEditor* editor;
    Canvas* cnv;
    Rectangle<int> previousBounds;
    MousePanner panner = MousePanner(this);
    ViewportScrollBar vbar = ViewportScrollBar(true, this);
    ViewportScrollBar hbar = ViewportScrollBar(false, this);
    bool scaleChanged = false;
};
