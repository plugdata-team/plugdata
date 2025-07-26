/*
 // Copyright (c) 2021-2025 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl.h>
#    include <nanovg_gl_utils.h>
#endif

#include "NVGSurface.h"

#include "PluginEditor.h"
#include "PluginProcessor.h"

#define ENABLE_FPS_COUNT 0

class FrameTimer {
public:
    FrameTimer()
    {
        startTime = getNow();
        prevTime = startTime;
    }

    void render(NVGcontext* nvg, int const width, int const height, float const scale)
    {
        nvgBeginFrame(nvg, width, height, scale);

        nvgFillColor(nvg, nvgRGBA(40, 40, 40, 255));
        nvgFillRect(nvg, 0, 0, 40, 22);

        nvgFontSize(nvg, 20.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(nvg, nvgRGBA(240, 240, 240, 255));
        StackArray<char, 16> fpsBuf;
        snprintf(fpsBuf.data(), 16, "%d", static_cast<int>(round(1.0f / getAverageFrameTime())));
        nvgText(nvg, 7, 2, fpsBuf.data(), nullptr);

        nvgGlobalScissor(nvg, 0, 0, 40 * scale, 22 * scale);
        nvgEndFrame(nvg);
    }
    void addFrameTime()
    {
        auto const timeSeconds = getTime();
        auto const dt = timeSeconds - prevTime;
        perf_head = (perf_head + 1) % 32;
        frame_times[perf_head] = dt;
        prevTime = timeSeconds;
    }

    double getTime() const { return getNow() - startTime; }

private:
    static double getNow()
    {
        auto const ticks = Time::getHighResolutionTicks();
        return Time::highResolutionTicksToSeconds(ticks);
    }

    float getAverageFrameTime() const
    {
        float avg = 0;
        for (int i = 0; i < 32; i++) {
            avg += frame_times[i];
        }
        return avg / static_cast<float>(32);
    }

    float frame_times[32] = {};
    int perf_head = 0;
    double startTime = 0, prevTime = 0;
};

NVGSurface::NVGSurface(PluginEditor* e)
    : editor(e)
{
#ifdef NANOVG_GL_IMPLEMENTATION
    glContext = std::make_unique<OpenGLContext>();
    auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
    glContext->setPixelFormat(pixelFormat);
#    ifdef NANOVG_GL3_IMPLEMENTATION
    glContext->setOpenGLVersionRequired(OpenGLContext::OpenGLVersion::openGL3_2);
#    endif
    glContext->setSwapInterval(0);
#endif

#if ENABLE_FPS_COUNT
    frameTimer = std::make_unique<FrameTimer>();
#endif

    setInterceptsMouseClicks(false, false);
    setWantsKeyboardFocus(false);

    setSize(1, 1);

    editor->addChildComponent(backupImageComponent);

    // Start rendering asynchronously, so we are sure the window has been added to the desktop
    // kind of a hack, but works well enough
    MessageManager::callAsync([_this = SafePointer(this)] {
        if (_this) {
            _this->vBlankAttachment = std::make_unique<VBlankAttachment>(_this.getComponent(), std::bind(&NVGSurface::render, _this.getComponent()));
        }
    });
}

NVGSurface::~NVGSurface()
{
    detachContext();
}

void NVGSurface::initialise()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    auto* peer = getPeer()->getNativeHandle();
    auto* view = OSUtils::MTLCreateView(peer, 0, 0, getWidth(), getHeight());
    setView(view);
    setVisible(true);

    lastRenderScale = calculateRenderScale();
    nvg = nvgCreateContext(view, NVG_ANTIALIAS | NVG_TRIPLE_BUFFER, getWidth() * lastRenderScale, getHeight() * lastRenderScale);
#else
    setVisible(true);
    glContext->attachTo(*this);
    glContext->initialiseOnThread();
    glContext->makeActive();
    lastRenderScale = calculateRenderScale();
    nvg = nvgCreateContext(0);
#endif
    if (!nvg) {
        std::cerr << "could not initialise nvg" << std::endl;
        return;
    }

    updateWindowContextVisibility();

    surfaces[nvg] = this;
    nvgCreateFontMem(nvg, "Inter", (unsigned char*)BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize, 0);
    nvgCreateFontMem(nvg, "Inter-Regular", (unsigned char*)BinaryData::InterRegular_ttf, BinaryData::InterRegular_ttfSize, 0);
    nvgCreateFontMem(nvg, "Inter-Bold", (unsigned char*)BinaryData::InterBold_ttf, BinaryData::InterBold_ttfSize, 0);
    nvgCreateFontMem(nvg, "Inter-SemiBold", (unsigned char*)BinaryData::InterSemiBold_ttf, BinaryData::InterSemiBold_ttfSize, 0);
    nvgCreateFontMem(nvg, "Inter-Tabular", (unsigned char*)BinaryData::InterTabular_ttf, BinaryData::InterTabular_ttfSize, 0);
    nvgCreateFontMem(nvg, "icon_font-Regular", (unsigned char*)BinaryData::IconFont_ttf, BinaryData::IconFont_ttfSize, 0);
}

void NVGSurface::updateWindowContextVisibility()
{
#ifdef NANOVG_GL_IMPLEMENTATION
    if(glContext) glContext->setVisible(!renderThroughImage);
#else
    if(auto* view = getView()) {
        OSUtils::MTLSetVisible(view, !renderThroughImage);
    }
#endif
}

void NVGSurface::detachContext()
{
    if (makeContextActive()) {
        NVGFramebuffer::clearAll(nvg);
        NVGImage::clearAll(nvg);
        NVGCachedPath::clearAll(nvg);

        if (invalidFBO) {
            nvgDeleteFramebuffer(invalidFBO);
            invalidFBO = nullptr;
        }
        if (nvg) {
            nvgDeleteContext(nvg);
            nvg = nullptr;
            surfaces.erase(nvg);
        }

#ifdef NANOVG_METAL_IMPLEMENTATION
        if (auto* view = getView()) {
            OSUtils::MTLDeleteView(view);
            setView(nullptr);
        }
#else
        glContext->detach();
#endif
    }
}

void NVGSurface::updateBufferSize()
{
    float const pixelScale = getRenderScale();
    int const scaledWidth = std::max<int>(getWidth() * pixelScale, 1);
    int const scaledHeight = std::max<int>(getHeight() * pixelScale, 1);

    if (fbWidth != scaledWidth || fbHeight != scaledHeight || !invalidFBO) {
        if (invalidFBO)
            nvgDeleteFramebuffer(invalidFBO);
        invalidFBO = nvgCreateFramebuffer(nvg, scaledWidth, scaledHeight, NVG_IMAGE_PREMULTIPLIED);
        fbWidth = scaledWidth;
        fbHeight = scaledHeight;
        invalidArea = getLocalBounds();
    }
}

void NVGSurface::lookAndFeelChanged()
{
    if (makeContextActive()) {
        NVGFramebuffer::clearAll(nvg);
        NVGImage::clearAll(nvg);
        invalidateAll();
    }
}

bool NVGSurface::makeContextActive()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    // No need to make context active with Metal, so just check if we have initialised and return that
    return getView() != nullptr && nvg != nullptr && mnvgDevice(nvg) != nullptr;
#else
    if (glContext && glContext->makeActive()) {
        if (renderThroughImage)
            updateWindowContextVisibility();
        return true;
    }

    return false;
#endif
}

float NVGSurface::calculateRenderScale() const
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    return OSUtils::MTLGetPixelScale(getView()) * Desktop::getInstance().getGlobalScaleFactor();
#else
    return glContext->getRenderingScale();
#endif
}

float NVGSurface::getRenderScale() const
{
    return lastRenderScale;
}

void NVGSurface::updateBounds(Rectangle<int> bounds)
{
    setBounds(bounds);

#ifdef NANOVG_GL_IMPLEMENTATION
    updateWindowContextVisibility();
#endif
}

void NVGSurface::resized()
{
#ifdef NANOVG_METAL_IMPLEMENTATION
    if (auto* view = getView()) {
        auto const renderScale = getRenderScale();
        auto const* topLevel = getTopLevelComponent();
        auto const bounds = topLevel->getLocalArea(this, getLocalBounds()).toFloat() * renderScale;
        mnvgSetViewBounds(view, bounds.getWidth(), bounds.getHeight());
    }
#endif
    backupImageComponent.setBounds(editor->getLocalArea(this, getLocalBounds()));
    invalidateAll();
}

void NVGSurface::invalidateAll()
{
    invalidArea = invalidArea.getUnion(getLocalBounds());
}

void NVGSurface::invalidateArea(Rectangle<int> area)
{
    invalidArea = invalidArea.getUnion(area);
}

void NVGSurface::renderAll()
{
    invalidateAll();
    render();
}

void NVGSurface::render()
{
    if (renderThroughImage) {
        auto const startTime = Time::getMillisecondCounter();
        if (startTime - lastRenderTime < 32) {
            return; // When rendering through juce::image, limit framerate to 30 fps
        }
        lastRenderTime = startTime;
    }

    if (!getPeer()) {
        return;
    }

    if (!nvg) {
        initialise();
    }

    if (!makeContextActive()) {
        return;
    }

    auto pixelScale = calculateRenderScale();
    auto const desktopScale = Desktop::getInstance().getGlobalScaleFactor();
    auto const devicePixelScale = pixelScale / desktopScale;

    if (std::abs(lastRenderScale - pixelScale) > 0.1f) {
        detachContext();
        initialise();
        return; // Render on next frame
    }

#if NANOVG_METAL_IMPLEMENTATION
    if (pixelScale == 0) // This happens sometimes when an AUv3 plugin is hidden behind the parameter control view
    {
        return;
    }
    auto viewWidth = getWidth() * devicePixelScale;
    auto viewHeight = getHeight() * devicePixelScale;
#else
    auto viewWidth = getWidth() * pixelScale;
    auto viewHeight = getHeight() * pixelScale;
#endif

#if ENABLE_FPS_COUNT
    frameTimer->addFrameTime();
#endif

    updateBufferSize();

    invalidArea = invalidArea.getIntersection(getLocalBounds());

    for(auto bufferedObject : bufferedObjects)
    {
        if(bufferedObject)
            bufferedObject->updateFramebuffers(nvg);
    }
    
    if (!invalidArea.isEmpty()) {
        // Draw only the invalidated region on top of framebuffer
        nvgBindFramebuffer(invalidFBO);
        nvgViewport(0, 0, viewWidth, viewHeight);
#ifdef NANOVG_GL_IMPLEMENTATION
        glClear(GL_STENCIL_BUFFER_BIT);
#endif
        nvgBeginFrame(nvg, getWidth() * desktopScale, getHeight() * desktopScale, devicePixelScale);
        nvgScale(nvg, desktopScale, desktopScale);
        editor->renderArea(nvg, invalidArea);
        nvgGlobalScissor(nvg, invalidArea.getX() * pixelScale, invalidArea.getY() * pixelScale, invalidArea.getWidth() * pixelScale, invalidArea.getHeight() * pixelScale);
        nvgEndFrame(nvg);

#if ENABLE_FPS_COUNT
        frameTimer->render(nvg, getWidth(), getHeight(), pixelScale);
#endif

        if (renderThroughImage) {
            renderFrameToImage(backupRenderImage, invalidArea);
        } else {
            needsBufferSwap = true;
        }
        invalidArea = Rectangle<int>(0, 0, 0, 0);
    }

    if (needsBufferSwap) {
        nvgBindFramebuffer(nullptr);
        nvgBlitFramebuffer(nvg, invalidFBO, 0, 0, viewWidth, viewHeight);

#ifdef NANOVG_GL_IMPLEMENTATION
        glContext->swapBuffers();
#endif
        needsBufferSwap = false;
    }
}

void NVGSurface::renderFrameToImage(Image& image, Rectangle<int> area)
{
    nvgBindFramebuffer(nullptr);
    auto const bufferSize = fbHeight * fbWidth;
    if (bufferSize != backupPixelData.size())
        backupPixelData.resize(bufferSize);

    auto region = area.getIntersection(getLocalBounds()).toFloat() * getRenderScale();
    nvgReadPixels(nvg, invalidFBO, region.getX(), region.getY(), region.getWidth(), region.getHeight(), fbHeight, backupPixelData.data());

    if (!image.isValid() || image.getWidth() != fbWidth || image.getHeight() != fbHeight) {
        image = Image(Image::PixelFormat::ARGB, fbWidth, fbHeight, true);
    }

    Image::BitmapData imageData(image, Image::BitmapData::writeOnly);

    for (int y = 0; y < static_cast<int>(region.getHeight()); y++) {
        auto* scanLine = reinterpret_cast<uint32*>(imageData.getLinePointer(y + region.getY()));
        for (int x = 0; x < static_cast<int>(region.getWidth()); x++) {
#ifdef NANOVG_GL_IMPLEMENTATION
            // OpenGL images are upside down
            uint32 argb = backupPixelData[((int)region.getHeight() - (y + 1)) * (int)region.getWidth() + x];
#else
            uint32 argb = backupPixelData[y * static_cast<int>(region.getWidth()) + x];
#endif
            uint8 a = argb >> 24;
            uint8 r = argb >> 16;
            uint8 g = argb >> 8;
            uint8 b = argb;

            // order bytes as abgr
#ifdef NANOVG_GL_IMPLEMENTATION
            scanLine[x + (int)region.getX()] = (a << 24) | (b << 16) | (g << 8) | r;
#else
            scanLine[x + static_cast<int>(region.getX())] = a << 24 | r << 16 | g << 8 | b;
#endif
        }
    }

    backupImageComponent.setImage(Image()); // Need to set a dummy image first to force an update
    backupImageComponent.setImage(image);
}

void NVGSurface::setRenderThroughImage(bool const shouldRenderThroughImage)
{
    renderThroughImage = shouldRenderThroughImage;
    invalidateAll();
    updateWindowContextVisibility();
    backupImageComponent.setVisible(shouldRenderThroughImage);
}

NVGSurface* NVGSurface::getSurfaceForContext(NVGcontext* nvg)
{
    auto const nvgIter = surfaces.find(nvg);
    if (nvgIter != surfaces.end()) {
        return nvgIter->second;
    }

    return nullptr;
}

void NVGSurface::addBufferedObject(NVGComponent* component)
{
    bufferedObjects.insert(component);
}

void NVGSurface::removeBufferedObject(NVGComponent* component)
{
    bufferedObjects.erase(component);
}
