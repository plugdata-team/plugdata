/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "NVGSurface.h"

#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg.h>
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>


#include "PluginEditor.h"
#include "Canvas.h"

NVGSurface::NVGSurface(PluginEditor* e) : editor(e)
{
    glContext = std::make_unique<OpenGLContext>();
    glContext->setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    glContext->setMultisamplingEnabled(false);
    glContext->setSwapInterval(0);
    
    setInterceptsMouseClicks(false, false);
    setWantsKeyboardFocus(false);
    
    // Start rendering asynchronously, so we are sure the window has been added to the desktop
    // kind of a hack, but works well enough
    MessageManager::callAsync([this](){
        // Render on vblank
        vBlankAttachment = std::make_unique<VBlankAttachment>(this, [this](){
            render();
        });
    });
}

NVGSurface::~NVGSurface()
{
    glContext->detach();
}


void NVGSurface::swapBuffers()
{
    needsBufferSwap = true;
}

OpenGLContext* NVGSurface::getGLContext()
{
    return glContext.get();
}


float NVGSurface::getRenderScale()
{
    return glContext->getRenderingScale();
}

void NVGSurface::render()
{
    Array<Canvas*> renderTargets;
    for(auto* split : editor->splitView.splits)
    {
        if(auto* cnv = split->getTabComponent()->getCurrentCanvas())
        {
            renderTargets.add(cnv);
        }
    }
    
    if(glContext->makeActive()) {
        for(auto* cnv : renderTargets)
        {
            cnv->render(nvg);
        }
    }
    
    if(!renderTargets.isEmpty() && !glContext->isAttached())
    {
        setVisible(true);
        setInterceptsMouseClicks(false, false);
        setWantsKeyboardFocus(false);
        glContext->attachTo(*this);
        glContext->setSwapInterval(0); // It's very important this happens after attachTo. Otherwise, it will be terribly slow on Windows/Linux
        
        if(nvg) nvgDeleteGL3(nvg);
        nvg = nvgCreateGL3(NVG_ANTIALIAS);
        if (!nvg) std::cerr << "could not initialise nvg" << std::endl;
        
        nvgCreateFontMem(nvg, "Inter", (unsigned char*)BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Regular", (unsigned char*)BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Bold", (unsigned char*)BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Semibold", (unsigned char*)BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Thin", (unsigned char*)BinaryData::InterThin_ttf, BinaryData::InterThin_ttfSize, 0);
        nvgCreateFontMem(nvg, "Icon", (unsigned char*)BinaryData::IconFont_ttf, BinaryData::IconFont_ttfSize, 0);
    }
    else if(renderTargets.isEmpty() && glContext->isAttached())
    {
        nvgDeleteGL3(nvg);
        nvg = nullptr;
        glContext->detach();
        openGLView.setVisible(false);
    }
    else if(needsBufferSwap && glContext->isAttached()) {
        for(auto* cnv : renderTargets)
        {
            cnv->finaliseRender(nvg);
        }
        glContext->swapBuffers();
        needsBufferSwap = false;
    }
    
    // We update frambuffers after we call swapBuffers to make sure the frame is on time
    for(auto* cnv : renderTargets)
    {
        cnv->updateFramebuffers(nvg);
    }
}


void NVGSurface::handleAsyncUpdate()
{
    
}
