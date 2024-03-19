/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "NVGSurface.h"

#ifdef NANOVG_GL_IMPLEMENTATION
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>
#endif

#include "PluginEditor.h"
#include "Canvas.h"

#define ENABLE_FPS_COUNT 1

class FrameTimer
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
        nvgRect(nvg, 0, 0, 40, 22);
        nvgFillColor(nvg, nvgRGBA(40, 40, 40, 255));
        nvgFill(nvg);
        
        nvgFontSize(nvg, 20.0f);
        nvgTextAlign(nvg,NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
        nvgFillColor(nvg, nvgRGBA(240,240,240,255));
        char fpsBuf[16];
        snprintf(fpsBuf, 16, "%d", static_cast<int>(1.0f / getAverageFrameTime()));
        nvgText(nvg, 8, 2, fpsBuf, nullptr);
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
        float avg = 0;
        for (int i = 0; i < 32; i++) {
            avg += frame_times[i];
        }
        return avg / (float)32;
    }

    float frame_times[32] = {};
    int perf_head = 0;
    double startTime = 0, prevTime = 0;
};


NVGSurface::NVGSurface(PluginEditor* e) : editor(e)
{
#ifdef NANOVG_GL_IMPLEMENTATION
    glContext = std::make_unique<OpenGLContext>();
    glContext->setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
    glContext->setMultisamplingEnabled(false);
    glContext->setSwapInterval(0);
#endif
    
#if ENABLE_FPS_COUNT
    frameTimer = std::make_unique<FrameTimer>();
    realFrameTimer = std::make_unique<FrameTimer>();
#endif
    
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
    detachContext();
}


void NVGSurface::triggerRepaint()
{
    needsBufferSwap = true;
}

bool NVGSurface::makeContextActive()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    return getView() != nullptr && nvg != nullptr;
#else
    if(glContext) return glContext->makeActive();
#endif
    
    return false;
}

void NVGSurface::detachContext()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    if(auto* view = getView()) {
        OSUtils::MTLDeleteView(view);
        setView(nullptr);
    }
#else
    if(glContext) glContext->detach();
#endif
}

float NVGSurface::getRenderScale() const
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    return 2.0f; // TODO: fix this!
#else
    return glContext->getRenderingScale();
#endif
}

void NVGSurface::resized()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
      mnvgSetViewBounds(getView(), getRenderScale() * getWidth(), getRenderScale() * getHeight());
#endif
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
    
    if(makeContextActive()) {
#if ENABLE_FPS_COUNT
        frameTimer->addFrameTime();
#endif
        
        for(auto* cnv : renderTargets)
        {
            cnv->render(nvg);
        }
    }
    
#ifdef NANOVG_METAL_IMPLEMENTATION
    bool isAttached = getView() != nullptr;
#else
    bool isAttached = glContext->isAttached();
#endif
    
    if(!renderTargets.isEmpty() && !isAttached)
    {
        setVisible(true);
        setInterceptsMouseClicks(false, false);
        setWantsKeyboardFocus(false);

        if(nvg) nvgDeleteContext(nvg);
        
#ifdef NANOVG_METAL_IMPLEMENTATION
        setView(OSUtils::MTLCreateView(getPeer()->getNativeHandle(), 20, 20, getRenderScale() * getWidth(), getRenderScale() * getHeight()));
        nvg = nvgCreateContext(getView(), NVG_ANTIALIAS | NVG_TRIPLE_BUFFER, getRenderScale() * getWidth(), getRenderScale() * getHeight());
        setVisible(true);
#else
        glContext->attachTo(*this);
        glContext->setSwapInterval(0); // It's very important this happens after attachTo. Otherwise, it will be terribly slow on Windows/Linux
        nvg = nvgCreateContext(NVG_ANTIALIAS);
#endif
        
        if (!nvg) std::cerr << "could not initialise nvg" << std::endl;
        nvgCreateFontMem(nvg, "Inter", (unsigned char*)BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Regular", (unsigned char*)BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Bold", (unsigned char*)BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Semibold", (unsigned char*)BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize, 0);
        nvgCreateFontMem(nvg, "Inter-Thin", (unsigned char*)BinaryData::InterThin_ttf, BinaryData::InterThin_ttfSize, 0);
        nvgCreateFontMem(nvg, "Icon", (unsigned char*)BinaryData::IconFont_ttf, BinaryData::IconFont_ttfSize, 0);
    }
    else if(renderTargets.isEmpty() && isAttached)
    {
        if(nvg) nvgDeleteContext(nvg);
        nvg = nullptr;
        detachContext();
    }
    else if(needsBufferSwap && isAttached) {
#if ENABLE_FPS_COUNT
        realFrameTimer->addFrameTime();
#endif
        
        float pixelScale = getRenderScale();
        nvgViewport(0, 0, getWidth() * pixelScale, getHeight() * pixelScale);
        
        nvgBeginFrame(nvg, getWidth(), getHeight(), pixelScale);
        
        for(auto* cnv : renderTargets)
        {
            cnv->finaliseRender(nvg);
        }
        
        editor->splitView.render(nvg); // Render split view outlines and tab dnd areas
        if(auto* zoomLabel = reinterpret_cast<Component*>(editor->zoomLabel.get())) // If we don't cast through the first inherited class, this is UB
        {
            auto zoomLabelPos = getLocalPoint(zoomLabel, Point<int>(0, 0));
            nvgSave(nvg);
            nvgTranslate(nvg, zoomLabelPos.x, zoomLabelPos.y);
            dynamic_cast<NVGComponent*>(zoomLabel)->render(nvg); // Render zoom notifier at the bottom left
            nvgRestore(nvg);
        }
        
        renderPerfMeter(nvg);
        
        nvgEndFrame(nvg);

#ifdef NANOVG_GL_IMPLEMENTATION
        glContext->swapBuffers();
#endif
        needsBufferSwap = false;
    }
    
    // We update frambuffers after we call swapBuffers to make sure the frame is on time
    for(auto* cnv : renderTargets)
    {
        cnv->updateFramebuffers(nvg);
    }
}


void NVGSurface::renderPerfMeter(NVGcontext* nvg)
{
#if ENABLE_FPS_COUNT
    nvgSave(nvg);
#ifdef JUCE_MAC || JUCE_IOS
    nvgTranslate(nvg, 0, 0);
#else
    nvgTranslate(nvg, 0, 8);
#endif
    
    frameTimer->render(nvg);
    nvgTranslate(nvg, 40, 0);
    realFrameTimer->render(nvg);
    nvgRestore(nvg);
#endif
}
