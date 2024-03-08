/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <JuceHeader.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <nanovg.h>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>

#include <utility>

#include "Object.h"
#include "Connection.h"
#include "Canvas.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include "Utility/SettingsFile.h"

// Special viewport that shows scrollbars on top of content instead of next to it
class CanvasViewport : public Viewport, public OpenGLRenderer
{

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
    
    struct InvalidationListener : public CachedComponentImage
    {
        InvalidationListener(CanvasViewport* parent) : viewport(parent)
        {
        }
    private:
        void paint(Graphics& g) override {};
        
        bool invalidate (const juce::Rectangle<int>& rect) override
        {
            ScopedLock lock(viewport->invalidAreaLock);
            // Translate from canvas coords to viewport coords, expand by one to fix zoom rounding errors
            viewport->invalidArea.add(viewport->getLocalArea(viewport->cnv, rect).expanded(1));
            return true;
        }
        
        bool invalidateAll() override
        {
            ScopedLock lock(viewport->invalidAreaLock);
            viewport->invalidArea = viewport->getLocalBounds().withTrimmedTop(-10);
            return true;
        }
        
        void releaseResources() override {};
        
        CanvasViewport* viewport;
    };

public:
    CanvasViewport(PluginEditor* parent, Canvas* cnv)
        : editor(parent)
        , cnv(cnv)
    {

        glContext.setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
        glContext.setSwapInterval(0);
        glContext.setMultisamplingEnabled(false);
        glContext.setComponentPaintingEnabled(false);
        glContext.setContinuousRepainting(true);
        
        // TODO: do this in a better place
        MessageManager::callAsync([this, cnv](){
            if(auto* holder = cnv->getParentComponent())
            {
                glContext.setRenderer(this);
                cnv->setCachedComponentImage(new InvalidationListener(this));
                glContext.attachTo(*holder);

            }
        });
       
        setScrollBarsShown(false, false);

        setPositioner(new ViewportPositioner(*this));

#if JUCE_IOS
        setScrollOnDragMode(ScrollOnDragMode::never);
#endif

        setScrollBarThickness(8);

        addAndMakeVisible(vbar);
        addAndMakeVisible(hbar);
    }
    
    ~CanvasViewport()
    {
        glContext.detach();
    }
    
    void newOpenGLContextCreated() override
    {
        nvg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
        if (!nvg)
            std::cout << "could not init nvg" << std::endl;
        
        
        nvgCreateFontMem(nvg, "Inter", (unsigned char*)BinaryData::InterVariable_ttf, BinaryData::InterVariable_ttfSize, 0);

        // swap interval needs to be set after the context has been created (here)
        // if the GPU is nvidia, and gsync is active, this setting will be ignored, and swap interval of 1 will be used instead
        // this should be fine if gsync is controlling the swap however, as the mouse will be synced to gsync also.
        glContext.setSwapInterval(0);
        contextChanged = true;
    }
    
    void openGLContextClosing() override
    {
        
    }
    
#define ENABLE_PARIAL_REPAINT 1
    
    void renderOpenGL() override
    {
        Rectangle<int> invalidated;
        {
            ScopedLock lock(invalidAreaLock);
            invalidated = invalidArea.getBounds();
            invalidArea = RectangleList<int>();
        }

#if JUCE_LINUX          
        invalidated = invalidated.translated(0, 10);
#endif
        
        int width = getWidth();
        int height = getHeight();
        int scaledWidth = getWidth() * pixelScale;
        int scaledHeight = getHeight() * pixelScale;
        
#if !ENABLE_PARIAL_REPAINT
        glViewport(0, 0, scaledWidth, scaledHeight);
        OpenGLHelpers::clear(Colours::black);
        
        nvgBeginFrame(nvg, width, height, pixelScale);
        {
            const MessageManagerLock mmLock;
            cnv->renderNVG(nvg, invalidated);
        }
        nvgEndFrame(nvg);
        return;
#endif
        if(!invalidated.isEmpty()) {
            if(contextChanged || framebuffer.getWidth() != scaledWidth || framebuffer.getHeight() != scaledHeight) {
                framebuffer.initialise(glContext, scaledWidth, scaledHeight);
                contextChanged = false;
            }
            
            framebuffer.makeCurrentRenderingTarget();

            glViewport(0, 0, scaledWidth, scaledHeight);

            nvgBeginFrame(nvg, width, height, pixelScale);
            nvgScissor (nvg, invalidated.getX(), invalidated.getY(), invalidated.getWidth(), invalidated.getHeight());
            {
                const MessageManagerLock mmLock;
                if(mmLock.lockWasGained()) {
                    // If the lock was not gained, don't clear the canvas background, just let it render the old frame again
                    nvgBeginPath(nvg);
                    nvgFillColor(nvg, nvgRGB(0, 0, 0));
                    nvgRect(nvg, 0, 0, width, height);
                    nvgFill(nvg);
#if JUCE_LINUX          
                    nvgTranslate(nvg, 0, 10);
#endif
                    
                    cnv->renderNVG(nvg, invalidated);
                }
                else {
                    std::cout << "Lock not gained!" << std::endl;
                }
            }
#if 0
            static juce::Random rng;
            nvgBeginPath(nvg);
            nvgFillColor(nvg, nvgRGBA(rng.nextInt(255), rng.nextInt(255), rng.nextInt(255), 0x50));
            nvgRect(nvg, 0, 0, width, height);
            nvgFill(nvg);
#endif
            nvgEndFrame(nvg);
            
            framebuffer.releaseAsRenderingTarget();
        }
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer.getFrameBufferID());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, scaledWidth, scaledHeight, 0, 0, scaledWidth, scaledHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void paint(Graphics& g) override
    {
        pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();
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
        {
            ScopedLock lock(invalidAreaLock);
            invalidArea = getLocalBounds().withTrimmedTop(-10);
        }
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
    NVGcontext* nvg;
    OpenGLContext glContext;
    OpenGLFrameBuffer framebuffer;
    RectangleList<int> invalidArea;
    CriticalSection invalidAreaLock;
    
    bool contextChanged = false;
    float pixelScale = 1.0f;
    Time lastScrollTime;
    PluginEditor* editor;
    Canvas* cnv;
    Rectangle<int> previousBounds;
    MousePanner panner = MousePanner(this);
    ViewportScrollBar vbar = ViewportScrollBar(true, this);
    ViewportScrollBar hbar = ViewportScrollBar(false, this);
};
