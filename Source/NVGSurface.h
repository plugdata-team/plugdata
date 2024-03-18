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

class PluginEditor;
class NVGSurface : public Component
{
public:
    NVGSurface(PluginEditor* editor);
    ~NVGSurface();
    void render();
    
    void triggerRepaint();
    
    OpenGLContext* getGLContext();
    float getRenderScale() const;
        
private:
    
    PluginEditor* editor;
    NVGcontext* nvg = nullptr;
    bool needsBufferSwap = false;
    Component openGLView;
    std::unique_ptr<OpenGLContext> glContext;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;
};
