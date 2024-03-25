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

#include <nanovg.h>

class FrameTimer;
class PluginEditor;
class NVGSurface : 
#if NANOVG_METAL_IMPLEMENTATION && JUCE_MAC
public NSViewComponent
#elif NANOVG_METAL_IMPLEMENTATION && JUCE_IOS
public UIViewComponent
#else
#define LIN_OR_WIN
public Component, public Timer
#endif
{
public:
    NVGSurface(PluginEditor* editor);
    ~NVGSurface();
    void render();
    
    void triggerRepaint();
        
    void detachContext();
    bool makeContextActive();

#ifdef LIN_OR_WIN
    void timerCallback() override;
#endif

    
    float getRenderScale() const;

    void updateBounds(Rectangle<int> bounds);
        
private:
    
    bool isAttached() const;
    
    void resized() override;
    
    void renderPerfMeter(NVGcontext* nvg);
    
    PluginEditor* editor;
    NVGcontext* nvg = nullptr;
    bool needsBufferSwap = false;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;

    bool hresize = false;
    bool resizing = false;
    Rectangle<int> newBounds;
    
#if NANOVG_GL_IMPLEMENTATION
    std::unique_ptr<OpenGLContext> glContext;
#endif
    
    std::unique_ptr<FrameTimer> frameTimer;
};
