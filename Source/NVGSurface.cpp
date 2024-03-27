/*
 // Copyright (c) 2021-2022 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#include <nanovg_gl.h>
#include <nanovg_gl_utils.h>
#endif

#include "NVGSurface.h"

#include "PluginEditor.h"
#include "PluginMode.h"
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
        snprintf(fpsBuf, 16, "%d", static_cast<int>(round(1.0f / getAverageFrameTime())));
        nvgText(nvg, 7, 2, fpsBuf, nullptr);
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
#endif
    
    setInterceptsMouseClicks(false, false);
    setWantsKeyboardFocus(false);

    setSize(1,1);
    
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

#ifdef LIN_OR_WIN
void NVGSurface::timerCallback()
{
    updateBounds(newBounds);
    if (getBounds() == newBounds)
        stopTimer();
}
#endif


void NVGSurface::triggerRepaint()
{
    needsBufferSwap = true;
}

bool NVGSurface::makeContextActive()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    // No need to make context active with Metal, so just check if we have initialised and return that
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
    auto desktopScale = Desktop::getInstance().getGlobalScaleFactor();
#ifdef NANOVG_METAL_IMPLEMENTATION
    if(!isAttached()) return 2.0f * desktopScale;
    return OSUtils::MTLGetPixelScale(getView()) * desktopScale;
#else
    if(!isAttached()) return desktopScale;
    return glContext->getRenderingScale() * desktopScale;
#endif
}

void NVGSurface::updateBounds(Rectangle<int> bounds)
{
#ifdef LIN_OR_WIN
    newBounds = bounds;
    if (hresize)
        setBounds(bounds.withHeight(getHeight()));
    else
        setBounds(bounds.withWidth(getWidth()));

    resizing = true;
#else
    setBounds(bounds);
#endif
}

void NVGSurface::resized()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    if(auto* view = getView()) {
        auto desktopScale = Desktop::getInstance().getGlobalScaleFactor();
        auto renderScale = OSUtils::MTLGetPixelScale(view);
        auto* topLevel = getTopLevelComponent();
        auto bounds = topLevel->getLocalArea(this, getLocalBounds()) * desktopScale;
#if JUCE_IOS
        OSUtils::MTLResizeView(view, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
#endif
        mnvgSetViewBounds(view, (renderScale * bounds.getWidth()) - 4, (renderScale * bounds.getHeight()) - 4);
    }
#endif
}

bool NVGSurface::isAttached() const
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    return getView() != nullptr;
#else
    return glContext->isAttached();
#endif
}

void NVGSurface::invalidateArea(Rectangle<int> area)
{
    invalidArea = invalidArea.getUnion(area);
}

void NVGSurface::renderArea(Rectangle<int> area)
{
    if(editor->pluginMode) {
        editor->pluginMode->render(nvg);
    }
    else {
        for(auto* split : editor->splitView.splits)
        {
            if(auto* cnv = split->getTabComponent()->getCurrentCanvas())
            {
                nvgSave(nvg);
                nvgTranslate(nvg, split->getX(), 0);
                nvgScissor(nvg, 0, 0, split->getWidth(), split->getHeight());
                cnv->performRender(nvg, area.translated(-split->getX(), 0));
                nvgRestore(nvg);
            }
        }
    }
}

void NVGSurface::render()
{
    bool hasCanvas = editor->pluginMode != nullptr;
    for(auto* split : editor->splitView.splits)
    {
        if(auto* cnv = split->getTabComponent()->getCurrentCanvas())
        {
            hasCanvas = true;
            break;
        }
    }
    
    if(makeContextActive()) {
        float pixelScale = getRenderScale();
        int scaledWidth = getWidth() * pixelScale;
        int scaledHeight = getHeight() * pixelScale;
        
        if(fbWidth != scaledWidth || fbHeight != scaledHeight || !mainFBO) {
            mainFBO = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);
            invalidFBO = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, 0);
            fbWidth = scaledWidth;
            fbHeight = scaledHeight;
            invalidArea = getLocalBounds();
        }
        
        if(!invalidArea.isEmpty()) {
            auto invalidated = invalidArea.expanded(1);
            invalidArea = Rectangle<int>(0, 0, 0, 0);
            
            // First, draw only the invalidated region to a separate framebuffer
            // I've found that nvgScissor doesn't always clip everything, meaning that there will be graphical glitches if we don't do this
            
            nvgBindFramebuffer(invalidFBO);
            nvgViewport(0, 0, getWidth() * pixelScale, getHeight() * pixelScale);
            nvgClear(nvg);
            
            nvgBeginFrame(nvg, getWidth(), getHeight(), pixelScale);
            nvgScissor (nvg, invalidated.getX(), invalidated.getY(), invalidated.getWidth(), invalidated.getHeight());
            nvgGlobalCompositeOperation(nvg, NVG_SOURCE_OVER);
            
            renderArea(invalidated);
            nvgEndFrame(nvg);
            
            nvgBindFramebuffer(mainFBO);
            nvgViewport(0, 0, getWidth() * pixelScale, getHeight() * pixelScale);
            nvgBeginFrame(nvg, getWidth(), getHeight(), pixelScale);
            nvgBeginPath(nvg);
            nvgRect(nvg, invalidated.getX(), invalidated.getY(), invalidated.getWidth(), invalidated.getHeight());
            nvgScissor(nvg, invalidated.getX(), invalidated.getY(), invalidated.getWidth(), invalidated.getHeight());
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, getWidth(), getHeight(), 0, invalidFBO->image, 1));
            nvgFill(nvg);
        
#if ENABLE_FB_DEBUGGING
            static Random rng;
            nvgBeginPath(nvg);
            nvgFillColor(nvg, nvgRGBA(rng.nextInt(255), rng.nextInt(255), rng.nextInt(255), 0x50));
            nvgRect(nvg, 0, 0, getWidth(), getHeight());
            nvgFill(nvg);
#endif
        
            nvgEndFrame(nvg);
            
            nvgBindFramebuffer(nullptr);
            needsBufferSwap = true;
        }
        
#if ENABLE_FPS_COUNT
        frameTimer->addFrameTime();
#endif
    }
    
    if(hasCanvas && !isAttached())
    {
        setVisible(true);
        setInterceptsMouseClicks(false, false);
        setWantsKeyboardFocus(false);

        if(nvg) nvgDeleteContext(nvg);
        
#ifdef NANOVG_METAL_IMPLEMENTATION
        auto* peer = getPeer()->getNativeHandle();
        auto renderScale = Desktop::getInstance().getGlobalScaleFactor() * OSUtils::MTLGetPixelScale(peer);
        auto* view = OSUtils::MTLCreateView(peer, 0, 0, getWidth(), getHeight());
        setView(view);
        nvg = nvgCreateContext(view, NVG_ANTIALIAS | NVG_TRIPLE_BUFFER, getWidth() * renderScale, getHeight() * renderScale);
        setVisible(true);
#if JUCE_IOS
        resized();
#endif
#else
        glContext->attachTo(*this);
        glContext->initialiseOnThread();
        glContext->setSwapInterval(0); // It's very important this happens after attachTo. Otherwise, it will be terribly slow on Windows and Linux
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
    else if(!hasCanvas && isAttached())
    {
        if(invalidFBO) nvgDeleteFramebuffer(invalidFBO);
        if(mainFBO) nvgDeleteFramebuffer(mainFBO);
        invalidFBO = nullptr;
        mainFBO = nullptr;
        
        if(nvg) nvgDeleteContext(nvg);
        nvg = nullptr;
        detachContext();
    }
    else if(needsBufferSwap && isAttached() && mainFBO) {
        float pixelScale = getRenderScale();
        nvgViewport(0, 0, getWidth() * pixelScale, getHeight() * pixelScale);
        
        nvgBeginFrame(nvg, getWidth(), getHeight(), pixelScale);
        
        nvgBeginPath(nvg);
        nvgSave(nvg);
        nvgRect(nvg, 0, 0, getWidth(), getHeight());
        nvgScissor(nvg, 0, 0, getWidth(), getHeight());
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, getWidth(), getHeight(), 0, mainFBO->image, 1));
        nvgFill(nvg);
        nvgRestore(nvg);
        
        editor->splitView.render(nvg); // Render split view outlines and tab dnd areas
        if(auto* zoomLabel = reinterpret_cast<Component*>(editor->zoomLabel.get())) // If we don't cast through the first inherited class, this is UB
        {
            auto zoomLabelPos = getLocalPoint(zoomLabel, Point<int>(0, 0));
            nvgSave(nvg);
            nvgTranslate(nvg, zoomLabelPos.x, zoomLabelPos.y);
            dynamic_cast<NVGComponent*>(zoomLabel)->render(nvg); // Render zoom notifier at the bottom left
            nvgRestore(nvg);
        }
        
#if ENABLE_FPS_COUNT
        nvgSave(nvg);
        frameTimer->render(nvg);
        nvgRestore(nvg);
#endif
        
        nvgEndFrame(nvg);

#ifdef NANOVG_GL_IMPLEMENTATION
        glContext->swapBuffers();
        if (resizing) {
            hresize = !hresize;
            resizing = false;
        }
        if (getBounds() != newBounds)
            startTimerHz(60);
#endif
        needsBufferSwap = false;
    }
    
    // We update frambuffers after we call swapBuffers to make sure the frame is on time
    if(isAttached()) {
        for(auto* split : editor->splitView.splits)
        {
            if(auto* cnv = split->getTabComponent()->getCurrentCanvas())
            {
                cnv->updateFramebuffers(nvg, cnv->getLocalBounds(), 8);
            }
        }
    }
}
