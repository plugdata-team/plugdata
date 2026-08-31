/*
 // Copyright (c) 2021-2025 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#if NANOVG_METAL_IMPLEMENTATION
#    include <juce_gui_extra/juce_gui_extra.h>
#else
#    include <juce_opengl/juce_opengl.h>
using namespace juce::gl;
#endif

#ifndef PLUGDATA_NVG_FRAME_TIME_OVERLAY
#    define PLUGDATA_NVG_FRAME_TIME_OVERLAY 0
#endif

#ifndef PLUGDATA_NVG_REPAINT_DEBUG
#    define PLUGDATA_NVG_REPAINT_DEBUG 0
#endif

#include "Utility/Config.h"
#include "Utility/SettingsFile.h"

#ifdef PLUGDATA_NVG_FRAME_TIME_OVERLAY
#include "Utility/FrameTimeOverlay.h"
#endif

#include <atomic>
#include <nanovg_async.h>

#if !NANOVG_METAL_IMPLEMENTATION
#define NANOVG_GL_IMPLEMENTATION 1
#endif

class PluginEditor;
class NVGComponent;

class NVGSurface final :
#if NANOVG_METAL_IMPLEMENTATION && JUCE_MAC
    public NSViewComponent, public Thread
#elif NANOVG_METAL_IMPLEMENTATION && JUCE_IOS
    public UIViewComponent, public Thread
#else
    public Component, public OpenGLRenderer
#endif
    , public AsyncUpdater
{
public:
    explicit NVGSurface(PluginEditor* editor);
    ~NVGSurface() override;

    void initialise();

    bool makeContextActive();

    void detachContext();

    void lookAndFeelChanged() override;

    void handleAsyncUpdate() override;

    Rectangle<int> getInvalidArea() const { return invalidArea; }

    float getRenderScale() const;

    void updateBounds(Rectangle<int> bounds);

    class InvalidationChecker final : public CachedComponentImage {
    public:
        InvalidationChecker(std::function<void()> invalidate, std::function<void(Graphics& g)> repaint = nullptr) : invalidateCache(invalidate), performPaint(repaint)
        {
        }

        void paint(Graphics& g) override {
            if(performPaint) performPaint(g);
        };

        bool invalidate(Rectangle<int> const& rect) override
        {
            invalidateCache();
            return true;
        }

        bool invalidateAll() override
        {
            invalidateCache();
            return true;
        }

        void releaseResources() override {}

        std::function<void()> invalidateCache;
        std::function<void(Graphics& g)> performPaint;
    };

    /** Caches all NanoVG commands produced while painting a JUCE component.

        Attach this to the component passed to the constructor using
        Component::setCachedComponentImage(). Any repaint invalidates the command
        buffer; the next paint records the component again and later paints replay
        the recorded commands.
    */
    class CommandBufferCache final : public CachedComponentImage {
    public:
        explicit CommandBufferCache(Component& componentToCache)
            : component(componentToCache)
        {
        }

        void paint(Graphics& g) override;

        bool invalidate(Rectangle<int> const&) override
        {
            dirty = true;
            return true;
        }

        bool invalidateAll() override
        {
            dirty = true;
            return true;
        }

        void releaseResources() override
        {
            commandBuffer.clear();
            recordingContext = nullptr;
            recordingScale = 0.0f;
            dirty = true;
        }

    private:
        Component& component;
        nanovg::CommandBuffer commandBuffer;
        NVGcontext* recordingContext = nullptr;
        float recordingScale = 0.0f;
        bool dirty = true;
    };

    class InvalidationListener final : public CachedComponentImage {
    public:
        InvalidationListener(NVGSurface& s, Component* origin, bool performNvgRepaint = false, std::function<bool()> canRepaintCheck = [] { return true; })
            : surface(s)
            , originComponent(origin)
            , canRepaint(canRepaintCheck)
            , nvgRepaint(performNvgRepaint)
        {
        }

        void paint(Graphics& g) override;

        bool invalidate(Rectangle<int> const& rect) override
        {
            // Translate from canvas coords to viewport coords as float to prevent rounding errors
            auto invalidatedBounds = surface.getLocalArea(originComponent, rect.toFloat()).getSmallestIntegerContainer();
            invalidatedBounds = invalidatedBounds.getIntersection(surface.getLocalBounds());

            if (originComponent->isVisible() && canRepaint() && !invalidatedBounds.isEmpty()) {
                surface.invalidateArea(invalidatedBounds);
            }

            return false;
        }

        bool invalidateAll() override
        {
            if (originComponent->isVisible() && canRepaint()) {
                auto invalidatedBounds = surface.getLocalArea(originComponent, originComponent->getLocalBounds());
                surface.invalidateArea(invalidatedBounds);
            }
            return false;
        }

        void releaseResources() override { }

        NVGSurface& surface;
        Component* originComponent;
        std::function<bool()> canRepaint;
        bool nvgRepaint = false;
    };

    void invalidateArea(Rectangle<int> area);
    void invalidateAll();

    static NVGSurface* getSurfaceForContext(NVGcontext*);

    void resized() override;

    void addBufferedObject(NVGComponent* component);
    void removeBufferedObject(NVGComponent* component);

    // Reads back a region of the rendered framebuffer into a JUCE image.
    // 'logicalArea' is in this component's (i.e. the editor's) logical coordinates.
    // The returned image is in logical resolution, opaque, top-left origin.
    // Used by the eyedropper, which can't snapshot the GPU-rendered UI the normal way.
    // Safe to call from the message thread; blocks briefly on the render thread.
    Image renderToImage(Rectangle<int> logicalArea);

private:
    void serviceReadbackRequest();

    float calculateRenderScale() const;
    void updateRenderScale();
    void snapshotEditorSize();
    void scheduleRender();
    void recordFrame();
    void createRenderContext();
    void destroyRenderContext();
    void renderBackendFrame();
    void presentFramebuffer(int viewWidth, int viewHeight);
    void requestBackendRender();

#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
    void drawFrameTimeOverlay(int viewWidth, int viewHeight, float scale);
#    if NANOVG_GL_IMPLEMENTATION
    void initialiseOpenGLGPUTimer();
    void shutdownOpenGLGPUTimer();
    void pollOpenGLGPUTimer();
    bool beginOpenGLGPUTimer();
    void endOpenGLGPUTimer();
#    endif
#endif

#if NANOVG_METAL_IMPLEMENTATION
    void run() override;
#endif

#if NANOVG_GL_IMPLEMENTATION
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
#endif

    PluginEditor* editor;

    AtomicValue<NVGcontext*> nvg { nullptr };      // async command-recording wrapper (used for all drawing)
    NVGcontext* baseNvg = nullptr;  // real backend context (framebuffers, blit, teardown)

    Rectangle<int> invalidArea;          // damage accumulated since the last recorded frame (message thread)
    Rectangle<int> inFlightDamage;       // damage of the last recorded frame, re-folded if it gets coalesced
    Rectangle<int> currentBounds;
    AtomicValue<bool> frameReadyForReplay { false };
    AtomicValue<int> recordedFramebufferWidth { 0 };
    AtomicValue<int> recordedFramebufferHeight { 0 };
    AtomicValue<bool> recordedFrameIsFullRepaint { false };

    NVGframebuffer* mainFramebuffer = nullptr;        // real backend framebuffer (render thread only)
    int mainFramebufferWidth = 0;
    int mainFramebufferHeight = 0;

    static inline UnorderedMap<NVGcontext*, NVGSurface*> surfaces;

    UnorderedSegmentedSet<WeakReference<NVGComponent>> bufferedObjects;

    float lastRenderScale = 0.0f;

    AtomicValue<float> cachedRenderScale { 0.0f };

    // Editor's logical (pre-scale) size, snapshotted on the message thread. The
    // render thread sizes the drawable/framebuffer from these instead of reading
    // the editor Component directly: Component bounds are message-thread-only, so
    // reading them off the render thread is a data race (see snapshotEditorSize).
    AtomicValue<int> editorWidth { 1 };
    AtomicValue<int> editorHeight { 1 };

    // Framebuffer readback (eyedropper). The request is filled on the message
    // thread and serviced on the render thread, which owns the framebuffer.
    CriticalSection readbackLock;                 // serialises readback requests
    WaitableEvent readbackReady;                  // signalled by the render thread
    AtomicValue<bool> readbackPending { false };
    Rectangle<int> readbackDeviceArea;            // requested region, in device pixels
    HeapBlock<uint8> readbackData;                // filled with BGRA pixels
    int readbackWidth = 0;
    int readbackHeight = 0;
    bool readbackSucceeded = false;

#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
    std::unique_ptr<FrameTimeOverlay> frameTimeOverlay;
    std::unique_ptr<VBlankAttachment> frameTimeVBlankAttachment;
#    if NANOVG_GL_IMPLEMENTATION
    static constexpr int gpuTimerQueryCount = 4;
    GLuint gpuTimerQueries[gpuTimerQueryCount]{};
    bool gpuTimerQueryPending[gpuTimerQueryCount]{};
    int gpuTimerWriteIndex = 0;
    int gpuTimerReadIndex = 0;
    int gpuTimerActiveQuery = -1;
    bool gpuTimerSupported = false;
    bool gpuTimerUsesExtResult = false;
#    endif
#endif

#if NANOVG_GL_IMPLEMENTATION
    std::unique_ptr<OpenGLContext> glContext;
#endif

#if NANOVG_METAL_IMPLEMENTATION
    void* metalView = nullptr;
    void* metalLayer = nullptr;   // the view's CAMetalLayer, cached on the message
                                  // thread so the render thread never touches the view
    AtomicValue<bool> backendRenderRequested { false };

    std::unique_ptr<VBlankAttachment> renderPacer;
#endif
};
