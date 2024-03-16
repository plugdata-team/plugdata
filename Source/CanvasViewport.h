/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>
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

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport : public Viewport, public Timer, public NVGComponent
{
    // Attached to viewport so we can clean stuff up correctly
    struct ImageReleaseListener : public CachedComponentImage
    {
        ImageReleaseListener(CanvasViewport* parent) : viewport(parent)
        {
        }
        
        void paint(Graphics& g) override {
            if(viewport->stopRendering) {
                viewport->stopRendering = false; // If we get a paint event here, it means we should also be allowed to render from now on
                viewport->invalidArea = viewport->getLocalBounds().withTrimmedTop(-10);
            }
            
            viewport->cnv->pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();
        }
        
        bool invalidate (const Rectangle<int>& rect) override
        {
            viewport->invalidArea = rect.getUnion(viewport->invalidArea);
            return false;
        }
        
        bool invalidateAll() override
        {
            return false;
        }
        
        void releaseResources() override {
            viewport->stopRendering = true;
        };
        
        CanvasViewport* viewport;
    };
    
    // Attached to Canvas to listen to what areas it wants to see repainted
    struct InvalidationListener : public CachedComponentImage
    {
        InvalidationListener(CanvasViewport* parent) : viewport(parent)
        {
        }
        
    private:
        void paint(Graphics& g) override {};
        
        bool invalidate (const Rectangle<int>& rect) override
        {
            // Translate from canvas coords to viewport coords as float to prevent rounding errors
            auto invalidatedBounds = viewport->getLocalArea(viewport->cnv, rect.toFloat()).getSmallestIntegerContainer();
            viewport->invalidArea = invalidatedBounds.getUnion(viewport->invalidArea);
            return false;
        }
        
        bool invalidateAll() override
        {
            viewport->invalidArea = viewport->getLocalBounds().withTrimmedTop(-10);
            return false;
        }
        
        void releaseResources() override {};
        
        CanvasViewport* viewport;
    };

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
        // thus giving us a chance to attach the mouselistener on the middle-mouse click event
        void mouseDown(MouseEvent const& e) override
        {
            e.originalComponent->setMouseCursor(MouseCursor::DraggingHandCursor);
            downPosition = viewport->getViewPosition();
            downCanvasOrigin = viewport->cnv->canvasOrigin;
        }

        void mouseDrag(MouseEvent const& e) override
        {
            float scale = std::sqrt(std::abs(viewport->cnv->getTransform().getDeterminant()));

            auto infiniteCanvasOriginOffset = (viewport->cnv->canvasOrigin - downCanvasOrigin) * scale;
            viewport->setViewPosition(infiniteCanvasOriginOffset + downPosition - (scale * e.getOffsetFromDragStart().toFloat()).roundToInt());
        }

    private:
        CanvasViewport* viewport;
        Point<int> downPosition;
        Point<int> downCanvasOrigin;
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

        void setGrowAnimation(float newGrowValue)
        {
            growAnimation = newGrowValue;
            repaint();
        }

        void mouseDown(MouseEvent const& e) override
        {
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
            auto fullBounds = growingBounds.withX(2).withWidth(getWidth() - 4);

            if (isVertical) {
                growingBounds = thumbBounds.reduced(1).withLeft(thumbBounds.getX() + growPosition);
                roundedCorner = growingBounds.getWidth() * 0.5f;
                fullBounds = growingBounds.withY(2).withHeight(getHeight() - 4);
            }

            auto canvasColour = findColour(PlugDataColour::canvasBackgroundColourId);
            auto scrollbarColour = findColour(ScrollBar::ColourIds::thumbColourId);
            auto activeScrollbarColour = scrollbarColour.interpolatedWith(canvasColour.contrasting(0.6f), 0.7f);

            g.setColour(scrollbarColour.interpolatedWith(canvasColour, 0.7f).withAlpha(std::clamp(1.0f - growAnimation, 0.0f, 1.0f)));
            g.fillRoundedRectangle(fullBounds, roundedCorner);

            g.setColour(isMouseDragging ? activeScrollbarColour : scrollbarColour);
            g.fillRoundedRectangle(growingBounds, roundedCorner);
        }
        
        void render(NVGcontext* nvg)
        {
            auto growPosition = scrollBarThickness * 0.5f * growAnimation;
            auto growingBounds = thumbBounds.reduced(1).withTop(thumbBounds.getY() + growPosition);
            auto thumbCornerRadius = growingBounds.getHeight() * 0.5f;
            auto fullBounds = growingBounds.withX(2).withWidth(getWidth() - 4);
            
            auto canvasColour = findColour(PlugDataColour::canvasBackgroundColourId);
            auto scrollbarColour = findColour(ScrollBar::ColourIds::thumbColourId);
            auto activeScrollbarColour = scrollbarColour.interpolatedWith(canvasColour.contrasting(0.6f), 0.7f);
            auto fadeColour = scrollbarColour.interpolatedWith(canvasColour, 0.7f).withAlpha(std::clamp(1.0f - growAnimation, 0.0f, 1.0f));
            if (isVertical) {
                growingBounds = thumbBounds.reduced(1).withLeft(thumbBounds.getX() + growPosition);
                thumbCornerRadius = growingBounds.getWidth() * 0.5f;
                fullBounds = growingBounds.withY(2).withHeight(getHeight() - 4);
            }

            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, fullBounds.getX(), fullBounds.getY(), fullBounds.getWidth(), fullBounds.getHeight(), thumbCornerRadius);
            nvgFillColor(nvg, convertColour(fadeColour));
            nvgFill(nvg);
            
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, growingBounds.getX(), growingBounds.getY(), growingBounds.getWidth(), growingBounds.getHeight(), thumbCornerRadius);
            nvgFillColor(nvg, isMouseDragging ? convertColour(activeScrollbarColour) : convertColour(scrollbarColour));
            nvgFill(nvg);

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
        : NVGComponent(this)
        , editor(parent)
        , cnv(cnv)
    {
        glContext = editor->glContext.get();
        
        setScrollBarsShown(false, false);

        setPositioner(new ViewportPositioner(*this));

#if JUCE_IOS
        setScrollOnDragMode(ScrollOnDragMode::never);
#endif

        setScrollBarThickness(8);

        addAndMakeVisible(vbar);
        addAndMakeVisible(hbar);
            
        cnv->setCachedComponentImage(new InvalidationListener(this));
        setCachedComponentImage(new ImageReleaseListener(this));
    }
    
    ~CanvasViewport()
    {
        stopRendering = true;
    }
    
    void render(NVGcontext* nvg) override
    {
        if(stopRendering)
            return;
        
        frameTimer.addFrameTime();
        
        float pixelScale = cnv->pixelScale;
        int scaledWidth = getWidth() * pixelScale;
        int scaledHeight = getHeight() * pixelScale;
        
        auto splitPosition = glContext->getTargetComponent()->getLocalPoint(this, Point<int>(0, 0)).x * pixelScale;
        
        if(fbWidth != scaledWidth || fbHeight != scaledHeight || !framebuffer) {
            framebuffer = nvgluCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);
            fbWidth = scaledWidth;
            fbHeight = scaledHeight;
            invalidArea = getLocalBounds().withTrimmedTop(-10);
        }
        
        if(!invalidArea.isEmpty()) {
            auto invalidated = invalidArea.expanded(1);
            invalidArea = Rectangle<int>(0, 0, 0, 0);
            
            cnv->updateNVGFramebuffers(nvg, invalidated);

            nvgluBindFramebuffer(framebuffer);
            glViewport(0, 0, scaledWidth, scaledHeight); // TODO: it's more efficient if we only viewport the current invalidated area, but our rounded rect shader hates it

            nvgRect(nvg, invalidated.getX(), invalidated.getY(), invalidated.getWidth(), invalidated.getHeight());
            nvgFillColor(nvg, nvgRGBA(0, 0, 0, 0));
            nvgFill(nvg);
            
            renderFrame(nvg, invalidated);
            nvgluBindFramebuffer(nullptr);
            editor->needsBufferSwap = true;
        }
    }
    
    void blitToWindow(NVGcontext* nvg)
    {
        if(framebuffer) {
            float pixelScale = cnv->pixelScale;
            int scaledWidth = getWidth() * pixelScale;
            int scaledHeight = getHeight() * pixelScale;
            auto splitPosition = glContext->getTargetComponent()->getLocalPoint(this, Point<int>(0, 0)).x * pixelScale;
            
            glViewport(splitPosition, 0, scaledWidth, scaledHeight);
            nvgBeginFrame(nvg, getWidth(), getHeight(), pixelScale);
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, getWidth(), getHeight());
            nvgScissor(nvg, 0, 0, getWidth(), getHeight());
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, getWidth(), getHeight(), 0, framebuffer->image, 1));
            nvgFill(nvg);
            nvgEndFrame(nvg);

            //renderPerfMeter(nvg);
        }
    }
    
    void renderPerfMeter(NVGcontext* nvg)
    {
#if JUCE_DEBUG // draw fps meters
        nvgBeginFrame(nvg, getWidth(), getHeight(), cnv->pixelScale);
        frameTimer.render(nvg);
        nvgTranslate(nvg, 48, 0);
        realFrameTimer.render(nvg);
        nvgEndFrame(nvg);
#endif
    }
    
    void renderFrame(NVGcontext* nvg, Rectangle<int> const& invalidated)
    {
        float pixelScale = cnv->pixelScale;
        int width = getWidth();
        int height = getHeight();
        
        realFrameTimer.addFrameTime();
        
        nvgBeginFrame(nvg, width, height, pixelScale);
        nvgTranslate(nvg, 0, viewportOffsetY);
        nvgScissor (nvg, invalidated.getX(), invalidated.getY(), invalidated.getWidth(), invalidated.getHeight());

        cnv->performRender(nvg, invalidated);
        
        nvgSave(nvg);
        nvgTranslate(nvg, vbar.getX(), vbar.getY());
        vbar.render(nvg);
        nvgRestore(nvg);
        
        nvgSave(nvg);
        nvgTranslate(nvg, hbar.getX(), hbar.getY());
        hbar.render(nvg);
        nvgRestore(nvg);
        
#if ENABLE_CANVAS_FB_DEBUGGING
        static Random rng;
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGBA(rng.nextInt(255), rng.nextInt(255), rng.nextInt(255), 0x50));
        nvgRect(nvg, 0, 0, width, height);
        nvgFill(nvg);
#endif
        
        nvgEndFrame(nvg);
                    
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

        Rectangle<int> objectArea = contentArea.withPosition(cnv->canvasOrigin);
        for (auto object : cnv->objects) {
            objectArea = objectArea.getUnion(object->getBounds());
        }

        auto totalArea = contentArea.getUnion(objectArea);

        hbar.setRangeLimitsAndCurrentRange(totalArea.getX(), totalArea.getRight(), contentArea.getX(), contentArea.getRight());
        vbar.setRangeLimitsAndCurrentRange(totalArea.getY(), totalArea.getBottom(), contentArea.getY(), contentArea.getBottom());
    }

    void componentMovedOrResized(Component& c, bool moved, bool resized) override
    {
        if (cnv->pd->isInPluginMode())
            return;

        Viewport::componentMovedOrResized(c, moved, resized);
        adjustScrollbarBounds();
    }

    void visibleAreaChanged(Rectangle<int> const& r) override
    {
        cnv->isScrolling = true;
        startTimer(150);
        onScroll();
        adjustScrollbarBounds();
        invalidArea = getLocalBounds().withTrimmedTop(-10);
    }
    
    void timerCallback() override
    {
        stopTimer();
        cnv->isScrolling = false;
        invalidArea = getLocalBounds().withTrimmedTop(-10);
    }

    void resized() override
    {
        vbar.setVisible(isVerticalScrollBarShown());
        hbar.setVisible(isHorizontalScrollBarShown());

        if (editor->pd->isInPluginMode())
            return;

        adjustScrollbarBounds();

        if (!SettingsFile::getInstance()->getProperty<bool>("centre_resized_canvas")) {
            Viewport::resized();
            return;
        }

        float scale = std::sqrt(std::abs(cnv->getTransform().getDeterminant()));

        // centre canvas when resizing viewport
        auto getCentre = [this, scale](Rectangle<int> bounds) {
            if (scale > 1.0f) {
                auto point = cnv->getLocalPoint(this, bounds.withZeroOrigin().getCentre());
                return point * scale;
            }
            return getViewArea().withZeroOrigin().getCentre();
        };

        auto currentCentre = getCentre(previousBounds);
        previousBounds = getBounds();
        Viewport::resized();
        auto newCentre = getCentre(getBounds());

        auto offset = currentCentre - newCentre;
        setViewPosition(getViewPosition() + offset);
    }

    // Never respond to arrow keys, they have a different meaning
    bool keyPressed(KeyPress const& key) override
    {
        return false;
    }

    std::function<void()> onScroll = []() {};

private:

    struct FrameTimer
    {
    public:
        FrameTimer()
        {
            startTime = getNow();
            prevTime = startTime;
        }
        
        void render(NVGcontext* nvg)
        {
            nvgBeginPath(nvg);
            nvgRect(nvg, 0, 0, 48, 32);
            nvgFillColor(nvg, nvgRGBA(40, 40, 40, 255));
            nvgFill(nvg);
            
            nvgFontSize(nvg, 24.0f);
            nvgTextAlign(nvg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
            nvgFillColor(nvg, nvgRGBA(240,240,240,255));
            char fpsBuf[16];
            snprintf(fpsBuf, 16, "%d", static_cast<int>(1.0f / getAverageFrameTime()));
            nvgText(nvg,8, 10, fpsBuf, nullptr);
        }
        void addFrameTime()
        {
            auto timeSeconds = getTime();
            auto dt = timeSeconds - prevTime;
            perf_head = (perf_head+1) % 32;
            frame_times[perf_head] = dt;
            prevTime = timeSeconds;
        }
        
        double getTime() { return getNow() - startTime; }
    private:
        double getNow()
        {
            auto ticks = Time::getHighResolutionTicks();
            return Time::highResolutionTicksToSeconds(ticks);
        }
        
        float getAverageFrameTime()
        {
            int i;
            float avg = 0;
            for (i = 0; i < 32; i++) {
                avg += frame_times[i];
            }
            return avg / (float)32;
        }

        
        float frame_times[32] = {};
        int perf_head = 0;
        double prevTime;
        double startTime = 0;
    };
    
    OpenGLContext* glContext = nullptr;
    //OpenGLFrameBuffer framebuffer = OpenGLFrameBuffer();
    Rectangle<int> invalidArea;
    bool stopRendering = false;
    NVGLUframebuffer* framebuffer;
    int fbWidth = 0, fbHeight = 0;
    
    Time lastScrollTime;
    PluginEditor* editor;
    Canvas* cnv;
    Rectangle<int> previousBounds;
    MousePanner panner = MousePanner(this);
    ViewportScrollBar vbar = ViewportScrollBar(true, this);
    ViewportScrollBar hbar = ViewportScrollBar(false, this);
    
    FrameTimer frameTimer;
    FrameTimer realFrameTimer;
    
#if JUCE_MAC
        int viewportOffsetY = 6;
#else
        int viewportOffsetY = 10;
#endif
};
