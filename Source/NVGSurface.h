/*
 // Copyright (c) 2021-2025 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include "Utility/Config.h"
#include "Utility/SettingsFile.h"

#include <nanovg.h>
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
    public NSViewComponent
#elif NANOVG_METAL_IMPLEMENTATION && JUCE_IOS
    public UIViewComponent
#else
    public Component
#endif
{
public:
    NVGSurface(PluginEditor* editor);
    ~NVGSurface() override;

    void initialise();
    void updateBufferSize();

    void renderAll();
    void render();

    bool makeContextActive();

    void detachContext();

    void lookAndFeelChanged() override;

    Rectangle<int> getInvalidArea() const { return invalidArea; }

    float getRenderScale() const;

    void updateBounds(Rectangle<int> bounds);

    class InvalidationListener final : public CachedComponentImage {
    public:
        InvalidationListener(NVGSurface& s, Component* origin,  std::function<bool()> canRepaintCheck = [](){ return true; })
            : surface(s)
            , originComponent(origin)
            , canRepaint(canRepaintCheck)
        {
        }

        void paint(Graphics& g) override { }

        bool invalidate(Rectangle<int> const& rect) override
        {
            // Translate from canvas coords to viewport coords as float to prevent rounding errors
            auto invalidatedBounds = surface.getLocalArea(originComponent, rect.expanded(2).toFloat()).getSmallestIntegerContainer();
            invalidatedBounds = invalidatedBounds.getIntersection(surface.getLocalBounds());

            if (originComponent->isVisible() && canRepaint() && !invalidatedBounds.isEmpty()) {
                surface.invalidateArea(invalidatedBounds);
            }

            return surface.renderThroughImage;
        }

        bool invalidateAll() override
        {
            if (originComponent->isVisible() && canRepaint()) {
                surface.invalidateArea(originComponent->getLocalBounds());
            }
            return surface.renderThroughImage;
        }

        void releaseResources() override { }

        NVGSurface& surface;
        Component* originComponent;
        std::function<bool()> canRepaint;
    };

    void invalidateArea(Rectangle<int> area);
    void invalidateAll();

    void setRenderThroughImage(bool renderThroughImage);

    static NVGSurface* getSurfaceForContext(NVGcontext*);

    void renderFrameToImage(Image& image, Rectangle<int> area);

    void resized() override;
    
    void addBufferedObject(NVGComponent* component);
    void removeBufferedObject(NVGComponent* component);
    

private:
    float calculateRenderScale() const;

    // Sets the surface context to render through floating window, or inside editor as image
    void updateWindowContextVisibility();

    PluginEditor* editor;
    NVGcontext* nvg = nullptr;
    bool needsBufferSwap = false;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;

    Rectangle<int> invalidArea;
    NVGframebuffer* invalidFBO = nullptr;
    int fbWidth = 0, fbHeight = 0;

    static inline UnorderedMap<NVGcontext*, NVGSurface*> surfaces;

    juce::Image backupRenderImage;
    bool renderThroughImage = false;
    ImageComponent backupImageComponent;
    HeapArray<uint32> backupPixelData;
    
    UnorderedSegmentedSet<WeakReference<NVGComponent>> bufferedObjects;

    float lastRenderScale = 0.0f;
    uint32 lastRenderTime;

#if NANOVG_GL_IMPLEMENTATION
    std::unique_ptr<OpenGLContext> glContext;
#endif

    std::unique_ptr<FrameTimer> frameTimer;
};

