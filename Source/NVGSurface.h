/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
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
#undef NANOVG_GL_IMPLEMENTATION
#include <nanovg_gl_utils.h>
#define NANOVG_GL_IMPLEMENTATION 1
#endif


#if NANOVG_METAL_IMPLEMENTATION
// With metal, all images and buffers get invalidated when the window becomes inactive
// so, we need to get a callback when that happens to inform plugdata that all images/framebuffers are now invalid
struct BackgroundProcessChecker
{
    bool checkIfWindowActivityChanged()
    {
        auto wasForeground = isForeground;
        isForeground = Process::isForegroundProcess();
        return wasForeground != isForeground;
    }

private:
    bool isForeground = false;
};
#endif



class NVGContextListener
{
public:
    virtual void nvgContextDeleted(NVGcontext* nvg) {};
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(NVGContextListener);
};

class FrameTimer;
class PluginEditor;
class NVGSurface : 
#if NANOVG_METAL_IMPLEMENTATION && JUCE_MAC
public NSViewComponent
#elif NANOVG_METAL_IMPLEMENTATION && JUCE_IOS
public UIViewComponent
#else
public Component, public Timer, public SettingsFileListener
#endif
{
public:
    NVGSurface(PluginEditor* editor);
    ~NVGSurface();
    void render();
    
    void triggerRepaint();
        
    void detachContext();
    bool makeContextActive();

#ifdef NANOVG_GL_IMPLEMENTATION
    void timerCallback() override;
#endif
    
    void propertyChanged(String const& name, var const& value) override;
    
    void sendContextDeleteMessage();
    void addNVGContextListener(NVGContextListener* listener);
    void removeNVGContextListener(NVGContextListener* listener);

    Rectangle<int> getInvalidArea() { return invalidArea; }
    
    float getRenderScale() const;

    void updateBounds(Rectangle<int> bounds);
    
    class InvalidationListener : public CachedComponentImage
    {
    public:
        InvalidationListener(NVGSurface& s, Component* origin, bool passRepaintEvents = false) : surface(s), originComponent(origin),  passEvents(passRepaintEvents)
        {
            
        }
        
        void paint(Graphics& g) override {};
        
        bool invalidate (const Rectangle<int>& rect) override
        {
            if(originComponent->isVisible()) {
                // Translate from canvas coords to viewport coords as float to prevent rounding errors
                auto invalidatedBounds = surface.getLocalArea(originComponent, rect.toFloat()).getSmallestIntegerContainer();
                surface.invalidateArea(invalidatedBounds);
            }
            return passEvents;
        }
        
        bool invalidateAll() override
        {   
            if(originComponent->isVisible()) {
                surface.invalidateArea(originComponent->getLocalBounds());
            }
            return passEvents;
        }
        
        void releaseResources() override {};
        
        
        NVGSurface& surface;
        Component* originComponent;
        bool passEvents;
    };
        
    void invalidateArea(Rectangle<int> area);
    void invalidateAll();
    
    NVGcontext* getRawContext() { return nvg; }

private:
    
    void renderArea(Rectangle<int> area);
    
    bool isAttached() const;
    
    void resized() override;
    
    void renderPerfMeter(NVGcontext* nvg);
    
    PluginEditor* editor;
    NVGcontext* nvg = nullptr;
    bool needsBufferSwap = false;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;
    
    Rectangle<int> invalidArea;
    NVGframebuffer* mainFBO = nullptr;
    NVGframebuffer* invalidFBO = nullptr;
    int fbWidth = 0, fbHeight = 0;
    
    bool hresize = false;
    bool resizing = false;
    Rectangle<int> newBounds;
    
    int framesToSkip = 0;
    
    Array<WeakReference<NVGContextListener>> nvgContextListeners;
    
#if NANOVG_GL_IMPLEMENTATION
    std::unique_ptr<OpenGLContext> glContext;
#endif
    
#if NANOVG_METAL_IMPLEMENTATION
    BackgroundProcessChecker metalActivityChecker;
#endif
    
    std::unique_ptr<FrameTimer> frameTimer;
};
