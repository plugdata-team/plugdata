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
    , public AsyncUpdater
    , public SettingsFileListener {

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
            //beginDragAutoRepeat(16);

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
            if (thumbBounds.contains(x,y))
                return true;
            return false;
        }

        void mouseDrag(MouseEvent const& e)
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
            if (e.mouseWasDraggedSinceMouseDown()){
                isMouseDragging = false;
                //animator.fadeOut();
            }
            repaint();
        }

        void mouseEnter(MouseEvent const& e) override
        {
            isMouseOver = true;
            //animator.fadeIn();
            repaint();
        }

        void mouseExit(MouseEvent const& e) override
        {
            isMouseOver = false;
            if (!isMouseDragging)
                //animator.fadeOut();
            repaint();
        }

        void setRangeLimitsAndCurrentRange(float minTotal, float maxTotal, float minCurrent, float maxCurrent)
        {
            totalRange = {minTotal, maxTotal};
            currentRange = {minCurrent, maxCurrent};
            updateThumbBounds();
        }

        void updateThumbBounds()
        {
            auto thumbStart = jmap<float>(currentRange.getStart(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical ? getHeight() : getWidth());
            auto thumbEnd = jmap<float>(currentRange.getEnd(), totalRange.getStart(), totalRange.getEnd(), 0, isVertical ? getHeight() : getWidth());

            if (isVertical)
                thumbBounds = { 0, thumbStart, getWidth(), thumbEnd - thumbStart };
            else
                thumbBounds = { thumbStart, 0, thumbEnd - thumbStart, getHeight() };
            repaint();
        }

        void paint(Graphics& g) override
        {
            g.setColour(findColour(ScrollBar::ColourIds::thumbColourId));
            g.fillRoundedRectangle(thumbBounds.reduced(1), 4.0f);
        }

    private:
        bool isVertical = false;
        bool isMouseOver = false;
        bool isMouseDragging = false;
        Range<float> totalRange = {0,0};
        Range<float> currentRange = {0,0};
        Rectangle<float> thumbBounds = {0,0};
        CanvasViewport* viewport;
        Point<int> viewPosition = {0,0};
        FadeAnimator animator = FadeAnimator(this);
        FadeTimer fadeTimer;
    };

/*
    class FadingScrollbar : public ScrollBar
        , public ScrollBar::Listener {
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
        FadingScrollbar(bool isVertical, bool infinite)
            : isInfinite(infinite)
            , ScrollBar(isVertical)
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

            if (!isAlreadyScrolling) {
                onScrollStart();
                isAlreadyScrolling = true;
            }

            if (fadeOutAfterInterval) {
                fadeTimer.start(800, [this]() {
                    fadeOut();
                    return true;
                });
            }
        }

        void prepareImage()
        {
            auto currentRange = getCurrentRange();
            auto totalRange = getRangeLimit();

            if (getWidth() <= 0 || getHeight() <= 0 || totalRange.isEmpty() || currentRange.isEmpty())
                return;

            scrollbarImage = Image(Image::ARGB, getWidth(), getHeight(), true);
            Graphics g(scrollbarImage);

            int margin = isInfinite ? (infiniteCanvasMargin - 2) : 0;

            auto thumbStart = jmap<int>(currentRange.getStart(), totalRange.getStart() + margin, totalRange.getEnd() - margin, 0, isVertical() ? getHeight() : getWidth());
            auto thumbEnd = jmap<int>(currentRange.getEnd(), totalRange.getStart() + margin, totalRange.getEnd() - margin, 0, isVertical() ? getHeight() : getWidth());

            auto thumbBounds = Rectangle<int>();

            if (isVertical())
                thumbBounds = { 0, thumbStart, getWidth(), thumbEnd - thumbStart };
            else
                thumbBounds = { thumbStart, 0, thumbEnd - thumbStart, getHeight() };

            auto c = findColour(ScrollBar::ColourIds::thumbColourId);
            g.setColour(isMouseOver() ? c.brighter(0.25f) : c);
            g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), 4.0f);
        }

        void paint(Graphics& g) override
        {
            if (scrollbarImage.isValid()) {
                g.drawImageAt(scrollbarImage, 0, 0);
            }
        }

        void fadeOut()
        {
            isAlreadyScrolling = false;
            onScrollEnd();
            animator.fadeOut();
        }

    private:
        void resized() override
        {
            prepareImage();
            ScrollBar::resized();
        }

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

        bool isDraggingThumb = false;
        int lastMousePos = 0;
        FadeAnimator animator = FadeAnimator(this);
        FadeTimer fadeTimer;

        Image scrollbarImage;

        bool isInfinite;
        bool isAlreadyScrolling = false;

    public:
        std::function<void()> onScrollStart = []() {};
        std::function<void()> onScrollEnd = []() {};
    };
*/

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
        //adjustScrollbarBounds();
        addAndMakeVisible(vbar);
        addAndMakeVisible(hbar);
    }

    Rectangle<int> getTotalArea()
    {
        Rectangle<int> visibleObjectUnion;
        for (auto object : cnv->objects) {
            visibleObjectUnion = visibleObjectUnion.getUnion(object->getBounds());
        }
        return getViewArea().getUnion(visibleObjectUnion);
    }

    void lookAndFeelChanged() override
    {
        //if (!vbar || !hbar)
        //    return;

        //vbar->repaint();
        //hbar->repaint();
    }

    void enableMousePanning(bool enablePanning)
    {
        panner.enablePanning(enablePanning);
    }

    void adjustScrollbarBounds()
    {
        if (getViewArea().isEmpty())
            return;

        //auto thickness = getScrollBarThickness();
        auto thickness = 8;

        auto contentArea = getLocalBounds().withTrimmedRight(thickness).withTrimmedBottom(thickness);

        vbar.setBounds(contentArea.removeFromRight(thickness).withTrimmedBottom(thickness).translated(-1, 0));
        hbar.setBounds(contentArea.removeFromBottom(thickness));

        Rectangle<int> objectArea;
        for (auto object : cnv->objects) {
                objectArea = objectArea.getUnion(object->getBounds() * cnv->editor->getZoomScaleForCanvas(cnv));
            }

        auto totalArea = getViewArea().getUnion(objectArea);

        hbar.setRangeLimitsAndCurrentRange(totalArea.getX(), totalArea.getRight(), getViewArea().getX(), getViewArea().getRight());
        vbar.setRangeLimitsAndCurrentRange(totalArea.getY(), totalArea.getBottom(), getViewArea().getY(), getViewArea().getBottom());
    }

    void componentMovedOrResized(Component& c, bool moved, bool resized) override
    {
        if (isUpdatingBounds || editor->pd->pluginMode != var(false))
            return;

        isUpdatingBounds = true;

        Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();

        isUpdatingBounds = false;
    }

    void resized() override
    {
        if (editor->pd->pluginMode != var(false))
            return;

        adjustScrollbarBounds();

        // In case the window resizes, make sure we maintain the same origin point
        auto oldCanvasOrigin = cnv->canvasOrigin;
        Viewport::resized();
        if (editor->isShowing())
            handleUpdateNowIfNeeded();
        setCanvasOrigin(oldCanvasOrigin);
    }

    void updateBufferState()
    {
        // Uncomment this line to render canvas to image on scroll
        // This isn't very advantageous yet, because the dots need to be repainted when view area changes!
        // cnv->setBufferedToImage(isScrollingHorizontally || isScrollingVertically);
    }

    void handleAsyncUpdate() override
    {
        if (cnv->updatingBounds || !getViewedComponent())
            return;

        auto viewArea = getViewArea();

        cnv->updatingBounds = true;

        float scale = 1.0f / editor->getZoomScaleForCanvas(cnv);
        float smallerScale = std::max(1.0f, scale);

        auto newBounds = Rectangle<int>(cnv->canvasOrigin.x, cnv->canvasOrigin.y, (getWidth() + 1 - getScrollBarThickness()) * smallerScale, (getHeight() + 1 - getScrollBarThickness()) * smallerScale);

        if (SettingsFile::getInstance()->getProperty<int>("infinite_canvas") && !cnv->isPalette) {
            newBounds = newBounds.getUnion(viewArea.expanded(infiniteCanvasMargin) * scale);
        }
        //for (auto* obj : cnv->objects) {
        //    newBounds = newBounds.getUnion(obj->getBoundsInParent().reduced(Object::margin));
        //}
        //for (auto* c : cnv->connections) {
        //    newBounds = newBounds.getUnion(c->getBoundsInParent());
        //}

        //cnv->setBounds(newBounds + cnv->getPosition());

        //moveCanvasOrigin(-newBounds.getPosition());

        //setViewPosition(-newBounds.getPosition())

        onScroll();
    }

    void moveCanvasOrigin(Point<int> distance)
    {
        cnv->canvasOrigin += distance;
    }

    void propertyChanged(String name, var value) override
    {
        if (name == "infinite_canvas") {
            //recreateScrollbars();
        }
    }

    void setCanvasOrigin(Point<int> newOrigin)
    {
        moveCanvasOrigin(newOrigin - cnv->canvasOrigin);
    }

    void visibleAreaChanged(Rectangle<int> const& newVisibleArea) override
    {
        //triggerAsyncUpdate();
    }

    // Never respond to arrow keys, they have a different meaning
    bool keyPressed(KeyPress const& key) override
    {
        return false;
    }

    void mouseWheelMove(MouseEvent const& e, MouseWheelDetails const& d) override
    {
        //triggerAsyncUpdate();
        Viewport::mouseWheelMove(e, d);
    }

    std::function<void()> onScroll = []() {};

private:
    PluginEditor* editor;
    Canvas* cnv;
    MousePanner panner = MousePanner(this);
    //FadingScrollbar* vbar = nullptr;
    //FadingScrollbar* hbar = nullptr;
    NewScrollBar vbar = NewScrollBar(true, this);
    NewScrollBar hbar = NewScrollBar(false, this);

    bool isScrollingHorizontally = false;
    bool isScrollingVertically = false;

    bool isUpdatingBounds = false;
};
