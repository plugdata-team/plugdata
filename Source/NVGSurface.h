/*
 // Copyright (c) 2021-2025 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
#    include <juce_gui_extra/juce_gui_extra.h>
#else
#    include <juce_opengl/juce_opengl.h>
using namespace juce::gl;
#endif

#include "Utility/Config.h"
#include "Utility/SettingsFile.h"

#include <atomic>
#include <nanovg_async.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

class FrameTimer;
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
        InvalidationChecker(std::function<void()> invalidate) : invalidateCache(invalidate)
        {
        }

        void paint(Graphics& g) override {};

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

private:
    float calculateRenderScale() const;
    void scheduleRender();
    void recordFrame();
    void createRenderContext();
    void destroyRenderContext();
    void renderBackendFrame();
    void presentFramebuffer(int viewWidth, int viewHeight);
    void requestBackendRender();

#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    void run() override;
#endif

#if NANOVG_GL_IMPLEMENTATION
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
#endif

    PluginEditor* editor;

    std::atomic<NVGcontext*> nvg { nullptr };      // async command-recording wrapper (used for all drawing)
    NVGcontext* baseNvg = nullptr;  // real backend context (framebuffers, blit, teardown)

    Rectangle<int> invalidArea;          // damage accumulated since the last recorded frame (message thread)
    Rectangle<int> inFlightDamage;       // damage of the last recorded frame, re-folded if it gets coalesced
    Rectangle<int> currentBounds;
    std::atomic<bool> frameReadyForReplay { false };
    std::atomic<int> recordedFramebufferWidth { 0 };
    std::atomic<int> recordedFramebufferHeight { 0 };
    std::atomic<bool> recordedFrameIsFullRepaint { false };

    void* mainFramebuffer = nullptr;        // real backend framebuffer (render thread only)
    int mainFramebufferWidth = 0;
    int mainFramebufferHeight = 0;

    static inline UnorderedMap<NVGcontext*, NVGSurface*> surfaces;

    UnorderedSegmentedSet<WeakReference<NVGComponent>> bufferedObjects;

    float lastRenderScale = 0.0f;

#if NANOVG_GL_IMPLEMENTATION
    std::unique_ptr<OpenGLContext> glContext;
#endif

#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    void* metalView = nullptr;
    std::atomic<bool> backendRenderRequested { false };
#endif

    std::unique_ptr<FrameTimer> frameTimer;
};
