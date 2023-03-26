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

/*
// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport : public Viewport
    , public AsyncUpdater {

    inline static int const infiniteCanvasMargin = 32;

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
            // We shouldn't need to do this, but there is something going wrong when you drag very quicly, then stop
            // Auto-repeating the drag makes it smoother, but it's not a perfect solution
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
            if (getWidth() <= 0 || getHeight() <= 0)
                return;

            scrollbarImage = Image(Image::ARGB, getWidth(), getHeight(), true);
            Graphics g(scrollbarImage);

            auto currentRange = getCurrentRange();
            auto totalRange = getRangeLimit();

            int margin = (infiniteCanvasMargin - 2);

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

        bool isAlreadyScrolling = false;

    public:
        std::function<void()> onScrollStart = []() {};
        std::function<void()> onScrollEnd = []() {};
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
        if (editor->isShowing())
            handleUpdateNowIfNeeded();
        //setCanvasOrigin(oldCanvasOrigin);
    }

    void updateBufferState()
    {
        // Uncomment this line to render canvas to image on scroll
        // This isn't very advantageous yet, because the dots need to be repainted when view area changes!
        // cnv->setBufferedToImage(isScrollingHorizontally || isScrollingVertically);
    }

    ScrollBar* createScrollBarComponent(bool isVertical) override
    {
        if (isVertical) {
            vbar = new FadingScrollbar(true);
            vbar->onScrollStart = [this]() {
                isScrollingHorizontally = true;
                updateBufferState();
            };

            vbar->onScrollEnd = [this]() {
                isScrollingVertically = false;
                updateBufferState();
            };
            return vbar;
        } else {
            hbar = new FadingScrollbar(false);
            hbar->onScrollStart = [this]() {
                isScrollingHorizontally = true;
                updateBufferState();
            };

            hbar->onScrollEnd = [this]() {
                isScrollingHorizontally = false;
                updateBufferState();
            };
            return hbar;
        }

        for (auto* scrollbar : std::vector<FadingScrollbar*> { hbar, vbar }) {
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
        
        auto oldBounds = cnv->getBounds();

        auto newBounds = Rectangle<int>(cnv->canvasOrigin.x, cnv->canvasOrigin.y, (getWidth() + 1 - getScrollBarThickness()) * smallerScale, (getHeight() + 1 -  getScrollBarThickness()) * smallerScale);

        if (SettingsFile::getInstance()->getProperty<int>("infinite_canvas")) {
            newBounds = newBounds.getUnion(viewArea.expanded(infiniteCanvasMargin) * scale);
        }
        for (auto* obj : cnv->objects) {
            newBounds = newBounds.getUnion(obj->getBoundsInParent().reduced(Object::margin));
        }
        for (auto* c : cnv->connections) {
            newBounds = newBounds.getUnion(c->getBoundsInParent());
        }

        
        std::cout << "old: " << oldBounds.getX() << " " << oldBounds.getY() << std::endl;
        
        std::cout << "new: " << (newBounds + cnv->getPosition()).getPosition().x << " " << (newBounds + cnv->getPosition()).getPosition().x << std::endl;
        
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

    bool isScrollingHorizontally = false;
    bool isScrollingVertically = false;
}; */

inline static int const infiniteCanvasMargin = 32;



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
        if (getWidth() <= 0 || getHeight() <= 0)
            return;

        scrollbarImage = Image(Image::ARGB, getWidth(), getHeight(), true);
        Graphics g(scrollbarImage);

        auto currentRange = getCurrentRange();
        auto totalRange = getRangeLimit();

        int margin = (infiniteCanvasMargin - 2);

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

    bool isAlreadyScrolling = false;

public:
    std::function<void()> onScrollStart = []() {};
    std::function<void()> onScrollEnd = []() {};
};


struct CanvasViewport : public Component, public ComponentListener, public ScrollBar::Listener
{
    //==============================================================================
    CanvasViewport(Component* editor)
    {
        // content holder is used to clip the contents so they don't overlap the scrollbars
        addAndMakeVisible (contentHolder);
        contentHolder.setInterceptsMouseClicks (false, true);

        scrollBarThickness = getLookAndFeel().getDefaultScrollbarWidth();

        setInterceptsMouseClicks (false, true);
        setWantsKeyboardFocus (true);

        recreateScrollbars();
    }

    ~CanvasViewport()
    {
        deleteOrRemoveContentComp();
    }

    //==============================================================================
    void visibleAreaChanged (const Rectangle<int>&) {}
    void viewedComponentChanged (Component*) {}

    //==============================================================================
    void deleteOrRemoveContentComp()
    {
        if (contentComp != nullptr)
        {
            contentComp->removeComponentListener (this);

            if (deleteContent)
            {
                // This sets the content comp to a null pointer before deleting the old one, in case
                // anything tries to use the old one while it's in mid-deletion..
                std::unique_ptr<Component> oldCompDeleter (contentComp.get());
                contentComp = nullptr;
            }
            else
            {
                contentHolder.removeChildComponent (contentComp);
                contentComp = nullptr;
            }
        }
    }

    void setViewedComponent (Component* const newViewedComponent, const bool deleteComponentWhenNoLongerNeeded)
    {
        if (contentComp.get() != newViewedComponent)
        {
            deleteOrRemoveContentComp();
            contentComp = newViewedComponent;
            deleteContent = deleteComponentWhenNoLongerNeeded;

            if (contentComp != nullptr)
            {
                contentHolder.addAndMakeVisible (contentComp);
                setViewPosition (Point<int>());
                contentComp->addComponentListener (this);
            }

            viewedComponentChanged (contentComp);
            updateVisibleArea();
        }
    }

    void recreateScrollbars()
    {
        verticalScrollBar.reset();
        horizontalScrollBar.reset();

        verticalScrollBar  .reset (new FadingScrollbar (true));
        horizontalScrollBar.reset (new FadingScrollbar (false));

        addChildComponent (verticalScrollBar.get());
        addChildComponent (horizontalScrollBar.get());

        verticalScrollBar->addListener(this);
        horizontalScrollBar->addListener(this);
        verticalScrollBar->addMouseListener (this, true);
        horizontalScrollBar->addMouseListener (this, true);

        resized();
    }
    
    int getViewWidth() const noexcept                       { return lastVisibleArea.getWidth(); }
    int getViewHeight() const noexcept                      { return lastVisibleArea.getHeight(); }
    
    int getMaximumVisibleWidth() const            { return contentHolder.getWidth(); }
    int getMaximumVisibleHeight() const           { return contentHolder.getHeight(); }

    bool canScrollVertically() const noexcept     { return contentComp->getY() < 0 || contentComp->getBottom() > getHeight(); }
    bool canScrollHorizontally() const noexcept   { return contentComp->getX() < 0 || contentComp->getRight()  > getWidth(); }
    
    Point<int> getViewPosition() const noexcept             { return lastVisibleArea.getPosition(); }

    /** Returns the visible area of the child component, relative to its top-left */
    Rectangle<int> getViewArea() const noexcept             { return lastVisibleArea; }
    
    Component* getViewedComponent() const noexcept { return contentComp.get(); }

    Point<int> viewportPosToCompPos (Point<int> pos) const
    {
        jassert (contentComp != nullptr);

        auto contentBounds = contentHolder.getLocalArea (contentComp.get(), contentComp->getLocalBounds());

        Point<int> p (jmax (jmin (0, contentHolder.getWidth()  - contentBounds.getWidth()),  jmin (0, -(pos.x))),
                      jmax (jmin (0, contentHolder.getHeight() - contentBounds.getHeight()), jmin (0, -(pos.y))));

        return p.transformedBy (contentComp->getTransform().inverted());
    }

    void setViewPosition (const int xPixelsOffset, const int yPixelsOffset)
    {
        setViewPosition ({ xPixelsOffset, yPixelsOffset });
    }

    void setViewPosition (Point<int> newPosition)
    {
        if (contentComp != nullptr)
            contentComp->setTopLeftPosition (viewportPosToCompPos (newPosition));
    }

    void setViewPositionProportionately (const double x, const double y)
    {
        if (contentComp != nullptr)
            setViewPosition (jmax (0, roundToInt (x * (contentComp->getWidth()  - getWidth()))),
                             jmax (0, roundToInt (y * (contentComp->getHeight() - getHeight()))));
    }

    bool autoScroll (const int mouseX, const int mouseY, const int activeBorderThickness, const int maximumSpeed)
    {
        if (contentComp != nullptr)
        {
            int dx = 0, dy = 0;

            if (horizontalScrollBar->isVisible() || canScrollHorizontally())
            {
                if (mouseX < activeBorderThickness)
                    dx = activeBorderThickness - mouseX;
                else if (mouseX >= contentHolder.getWidth() - activeBorderThickness)
                    dx = (contentHolder.getWidth() - activeBorderThickness) - mouseX;

                if (dx < 0)
                    dx = jmax (dx, -maximumSpeed, contentHolder.getWidth() - contentComp->getRight());
                else
                    dx = jmin (dx, maximumSpeed, -contentComp->getX());
            }

            if (verticalScrollBar->isVisible() || canScrollVertically())
            {
                if (mouseY < activeBorderThickness)
                    dy = activeBorderThickness - mouseY;
                else if (mouseY >= contentHolder.getHeight() - activeBorderThickness)
                    dy = (contentHolder.getHeight() - activeBorderThickness) - mouseY;

                if (dy < 0)
                    dy = jmax (dy, -maximumSpeed, contentHolder.getHeight() - contentComp->getBottom());
                else
                    dy = jmin (dy, maximumSpeed, -contentComp->getY());
            }

            if (dx != 0 || dy != 0)
            {
                contentComp->setTopLeftPosition (contentComp->getX() + dx,
                                                 contentComp->getY() + dy);

                return true;
            }
        }

        return false;
    }

    void componentMovedOrResized (Component&, bool, bool)
    {
        updateVisibleArea();
    }

    //==============================================================================
    void lookAndFeelChanged()
    {
        if (! customScrollBarThickness)
        {
            scrollBarThickness = getLookAndFeel().getDefaultScrollbarWidth();
            resized();
        }
    }

    void resized()
    {
        updateVisibleArea();
    }

    //==============================================================================
    void updateVisibleArea()
    {
        auto scrollbarWidth = getScrollBarThickness();
        const bool canShowAnyBars = getWidth() > scrollbarWidth && getHeight() > scrollbarWidth;
        const bool canShowHBar = showHScrollbar && canShowAnyBars;
        const bool canShowVBar = showVScrollbar && canShowAnyBars;

        bool hBarVisible = false, vBarVisible = false;
        Rectangle<int> contentArea = getLocalBounds();
        Rectangle<int> contentBounds;

        contentHolder.setBounds (contentArea);
        if (auto cc = contentComp.get())
            contentBounds = contentHolder.getLocalArea (cc, cc->getLocalBounds());

        auto visibleOrigin = -contentBounds.getPosition();

        auto& hbar = *verticalScrollBar;
        auto& vbar = *horizontalScrollBar;

        hbar.setBounds (contentArea.getX(), hScrollbarBottom ? contentArea.getHeight() : 0, contentArea.getWidth(), scrollbarWidth);
        hbar.setRangeLimits (0.0, contentBounds.getWidth());
        hbar.setCurrentRange (visibleOrigin.x, contentArea.getWidth());
        hbar.setSingleStepSize (singleStepX);

        if (canShowHBar && ! hBarVisible)
            visibleOrigin.setX (0);

        vbar.setBounds (vScrollbarRight ? contentArea.getWidth() : 0, contentArea.getY(), scrollbarWidth, contentArea.getHeight());
        vbar.setRangeLimits (0.0, contentBounds.getHeight());
        vbar.setCurrentRange (visibleOrigin.y, contentArea.getHeight());
        vbar.setSingleStepSize (singleStepY);

        if (canShowVBar && ! vBarVisible)
            visibleOrigin.setY (0);

        // Force the visibility *after* setting the ranges to avoid flicker caused by edge conditions in the numbers.
        hbar.setVisible (hBarVisible);
        vbar.setVisible (vBarVisible);

        if (contentComp != nullptr)
        {
            auto newContentCompPos = viewportPosToCompPos (visibleOrigin);

            if (contentComp->getBounds().getPosition() != newContentCompPos)
            {
                contentComp->setTopLeftPosition (newContentCompPos);  // (this will re-entrantly call updateVisibleArea again)
                return;
            }
        }

        const Rectangle<int> visibleArea (visibleOrigin.x, visibleOrigin.y,
                                          jmin (contentBounds.getWidth()  - visibleOrigin.x, contentArea.getWidth()),
                                          jmin (contentBounds.getHeight() - visibleOrigin.y, contentArea.getHeight()));

        if (lastVisibleArea != visibleArea)
        {
            lastVisibleArea = visibleArea;
            visibleAreaChanged (visibleArea);
        }

        hbar.handleUpdateNowIfNeeded();
        vbar.handleUpdateNowIfNeeded();
    }

    void setScrollBarsShown (const bool showVerticalScrollbarIfNeeded,
                                       const bool showHorizontalScrollbarIfNeeded,
                                       const bool allowVerticalScrollingWithoutScrollbar,
                                       const bool allowHorizontalScrollingWithoutScrollbar)
    {
        allowScrollingWithoutScrollbarV = allowVerticalScrollingWithoutScrollbar;
        allowScrollingWithoutScrollbarH = allowHorizontalScrollingWithoutScrollbar;

        if (showVScrollbar != showVerticalScrollbarIfNeeded
             || showHScrollbar != showHorizontalScrollbarIfNeeded)
        {
            showVScrollbar = showVerticalScrollbarIfNeeded;
            showHScrollbar = showHorizontalScrollbarIfNeeded;
            updateVisibleArea();
        }
    }

    void setScrollBarThickness (const int thickness)
    {
        int newThickness;

        // To stay compatible with the previous code: use the
        // default thickness if thickness parameter is zero
        // or negative
        if (thickness <= 0)
        {
            customScrollBarThickness = false;
            newThickness = getLookAndFeel().getDefaultScrollbarWidth();
        }
        else
        {
            customScrollBarThickness = true;
            newThickness = thickness;
        }

        if (scrollBarThickness != newThickness)
        {
            scrollBarThickness = newThickness;
            updateVisibleArea();
        }
    }

    int getScrollBarThickness() const
    {
        return scrollBarThickness;
    }

    void scrollBarMoved (ScrollBar* scrollBarThatHasMoved, double newRangeStart)
    {
        auto newRangeStartInt = roundToInt (newRangeStart);

        if (scrollBarThatHasMoved == horizontalScrollBar.get())
        {
            setViewPosition (newRangeStartInt, lastVisibleArea.getY());
        }
        else if (scrollBarThatHasMoved == verticalScrollBar.get())
        {
            setViewPosition (lastVisibleArea.getX(), newRangeStartInt);
        }
    }

    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
    {
        if (e.eventComponent == this)
            if (! useMouseWheelMoveIfNeeded (e, wheel))
                Component::mouseWheelMove (e, wheel);
    }

    static int rescaleMouseWheelDistance (float distance, int singleStepSize) noexcept
    {
        if (distance == 0.0f)
            return 0;

        distance *= 14.0f * (float) singleStepSize;

        return roundToInt (distance < 0 ? jmin (distance, -1.0f)
                                        : jmax (distance,  1.0f));
    }
    
    bool useMouseWheelMoveIfNeeded (const MouseEvent& e, const MouseWheelDetails& wheel)
    {
        if (! (e.mods.isAltDown() || e.mods.isCtrlDown() || e.mods.isCommandDown()))
        {
            const bool canScrollVert = (allowScrollingWithoutScrollbarV || verticalScrollBar->isVisible());
            const bool canScrollHorz = (allowScrollingWithoutScrollbarH || horizontalScrollBar->isVisible());

            if (canScrollHorz || canScrollVert)
            {
                auto deltaX = rescaleMouseWheelDistance (wheel.deltaX, singleStepX);
                auto deltaY = rescaleMouseWheelDistance (wheel.deltaY, singleStepY);

                auto pos = getViewPosition();

                if (deltaX != 0 && deltaY != 0 && canScrollHorz && canScrollVert)
                {
                    pos.x -= deltaX;
                    pos.y -= deltaY;
                }
                else if (canScrollHorz && (deltaX != 0 || e.mods.isShiftDown() || ! canScrollVert))
                {
                    pos.x -= deltaX != 0 ? deltaX : deltaY;
                }
                else if (canScrollVert && deltaY != 0)
                {
                    pos.y -= deltaY;
                }

                if (pos != getViewPosition())
                {
                    setViewPosition (pos);
                    return true;
                }
            }
        }

        return false;
    }
    
    void enableMousePanning(bool enablePanning)
    {
        panner.enablePanning(enablePanning);
    }

    void setScrollBarPosition (bool verticalScrollbarOnRight,
                                         bool horizontalScrollbarAtBottom)
    {
        vScrollbarRight  = verticalScrollbarOnRight;
        hScrollbarBottom = horizontalScrollbarAtBottom;

        resized();
    }
    
    class AccessibilityIgnoredComponent : public Component
    {
        std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override
        {
            return createIgnoredAccessibilityHandler (*this);
        }
    };
    
    std::unique_ptr<FadingScrollbar> verticalScrollBar, horizontalScrollBar;
    AccessibilityIgnoredComponent contentHolder;
    WeakReference<Component> contentComp;
    Rectangle<int> lastVisibleArea;
    int scrollBarThickness = 0;
    int singleStepX = 16, singleStepY = 16;
    bool showHScrollbar = true, showVScrollbar = true, deleteContent = true;
    bool customScrollBarThickness = false;
    bool allowScrollingWithoutScrollbarV = false, allowScrollingWithoutScrollbarH = false;
    bool vScrollbarRight = true, hScrollbarBottom = true;
    
    std::function<void()> onScroll;
    std::function<Rectangle<int>()> getContentBounds;
    
    
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
            // TODO: fix this!
            //downCanvasOrigin = dynamic_cast<Canvas*>(viewport->getViewedComponent())->canvasOrigin;
        }

        void mouseDrag(MouseEvent const& e) override
        {
            // We shouldn't need to do this, but there is something going wrong when you drag very quicly, then stop
            // Auto-repeating the drag makes it smoother, but it's not a perfect solution
            Component::beginDragAutoRepeat(10);

            float scale = 1.0f;
            /* TODO: fix this!
            if (auto* cnv = dynamic_cast<Canvas*>(viewport->getViewedComponent())) {
                scale = std::sqrt(std::abs(cnv->getTransform().getDeterminant()));
                auto infiniteCanvasOriginOffset = (cnv->canvasOrigin - downCanvasOrigin) * scale;
                viewport->setViewPosition(infiniteCanvasOriginOffset + downPosition - (scale * e.getOffsetFromDragStart().toFloat()).roundToInt());
            } */
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
    
    MousePanner panner = MousePanner(this);
};
