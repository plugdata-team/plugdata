/*
 // Copyright (c) 2021-2025 Timothy Schoen and Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
#    include <juce_gui_extra/juce_gui_extra.h>
#else
#    include <juce_opengl/juce_opengl.h>
using namespace juce::gl;
#endif
#include <BinaryData.h>

#include <nanovg_async.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl.h>
#    include <nanovg_gl_utils.h>
#    if JUCE_LINUX || JUCE_BSD
void nvgluSetCornerRadius(float radius, bool left, bool right);
#    endif
#endif

#include "NVGSurface.h"
#include "Utility/NVGGraphicsContext.h"
#include "Utility/OSUtils.h"
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
        nanovg::nvgBeginFrame(nvg, width, height, scale);

        nanovg::nvgFillColor(nvg, nanovg::nvgRGBA(40, 40, 40, 255));
        nanovg::nvgFillRect(nvg, 0, 0, 40, 22);

        nanovg::nvgFontSize(nvg, 20.0f);
        nanovg::nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nanovg::nvgFillColor(nvg, nanovg::nvgRGBA(240, 240, 240, 255));
        StackArray<char, 16> fpsBuf;
        snprintf(fpsBuf.data(), 16, "%d", static_cast<int>(round(1.0f / getAverageFrameTime())));
        nanovg::nvgText(nvg, 7, 2, fpsBuf.data(), nullptr);

        nanovg::nvgGlobalScissor(nvg, 0, 0, 40 * scale, 22 * scale);
        nanovg::nvgEndFrame(nvg);
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

    float frame_times[32] = { };
    int perf_head = 0;
    double startTime = 0, prevTime = 0;
};

NVGSurface::NVGSurface(PluginEditor* e)
    :
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    Thread("NVGSurface Metal Renderer"),
#endif
    editor(e)
{
#ifdef NANOVG_GL_IMPLEMENTATION
    glContext = std::make_unique<OpenGLContext>();
    glContext->setRenderer(this);
    glContext->setComponentPaintingEnabled(false);
    auto pixelFormat = OpenGLPixelFormat(8, 8, 16, 8);
    glContext->setPixelFormat(pixelFormat);
#    ifdef NANOVG_GL3_IMPLEMENTATION
    glContext->setPreferredProfile(OpenGLProfile::core);
    glContext->setPreferredVersion(OpenGLVersion(3, 2));
#    endif
    glContext->setSwapInterval(0);
    glContext->setContinuousRepainting(false);
#endif

#if ENABLE_FPS_COUNT
    frameTimer = std::make_unique<FrameTimer>();
#endif

    setInterceptsMouseClicks(false, false);
    setWantsKeyboardFocus(false);
    setVisible(true);
    setSize(1, 1);

    MessageManager::callAsync([_this = SafePointer(this)] {
        if (_this) {
            _this->initialise();
            _this->invalidateAll();
        }
    });
}

NVGSurface::~NVGSurface()
{
    detachContext();
}

void NVGSurface::initialise()
{
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    if (!metalView) {
        auto* peer = getPeer();
        if (!peer)
            return;

        auto* nativePeer = peer->getNativeHandle();
        metalView = OSUtils::MTLCreateView(nativePeer, 0, 0, getWidth(), getHeight());
        setView(metalView);
        setVisible(true);
    }

    if (!isThreadRunning()) {
        backendRenderRequested.store(true);
        startThread(Thread::Priority::high);
        notify();
    }
#elif NANOVG_GL_IMPLEMENTATION
    if (glContext && !glContext->isAttached()) {
        glContext->attachTo(*this);
    }
#endif
}

void NVGSurface::createRenderContext()
{
    if (baseNvg)
        return;

#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    auto const pixelScale = calculateRenderScale();
    auto const viewWidth = jmax(1, roundToInt(editor->getWidth() * pixelScale));
    auto const viewHeight = jmax(1, roundToInt(editor->getHeight() * pixelScale));

    if (!metalView)
        return;

    //mnvgSetViewBounds(metalView, viewWidth, viewHeight);
    baseNvg = nvgCreateContext(metalView, 0, viewWidth, viewHeight);
#elif NANOVG_GL_IMPLEMENTATION
    // Runs on the render thread. Context create/destroy and the main framebuffer
    // are render-thread-owned; no contextMutex needed (see NVGSurface.h / recordFrame).
    baseNvg = nvgCreateContext(0);
#else
    return;
#endif

    if (!baseNvg) {
        MessageManager::callAsync([] {
            static bool isShowingMessageBox = false;
            if (!isShowingMessageBox) {
                isShowingMessageBox = true;
                std::cerr << "could not initialise nvg" << std::endl;
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
                auto const message = "Please check that Metal is available and working on this system.";
#else
                auto const message = "Please check that you have up-to-date graphics drivers installed. At least OpenGL 3.0 support is required to run plugdata.";
#endif
                AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,
                    "Could not initialize plugdata",
                    message,
                    "OK",
                    nullptr,
                    nullptr);
            }
        });
        return;
    }

    auto* asyncNvg = nanovg::create(baseNvg, false);
    nanovg::BackendFunctions backendFunctions;
    backendFunctions.createFramebuffer = [](NVGcontext* ctx, int width, int height, int imageFlags) -> void* {
        return nvgCreateFramebuffer(ctx, width, height, imageFlags);
    };
    backendFunctions.deleteFramebuffer = [](void* framebuffer) {
        nvgDeleteFramebuffer(reinterpret_cast<NVGframebuffer*>(framebuffer));
    };
    backendFunctions.bindFramebuffer = [](void* framebuffer) {
        nvgBindFramebuffer(reinterpret_cast<NVGframebuffer*>(framebuffer));
    };
    backendFunctions.framebufferImage = [](void* framebuffer) -> int {
        return framebuffer ? reinterpret_cast<NVGframebuffer*>(framebuffer)->image : 0;
    };
    backendFunctions.viewport = [](int x, int y, int width, int height) {
        nvgViewport(x, y, width, height);
    };
    backendFunctions.clear = [](NVGcontext* ctx) {
        nvgClear(ctx);
    };
    nanovg::setBackendFunctions(asyncNvg, backendFunctions);
    nvg.store(asyncNvg);

#    if JUCE_LINUX || JUCE_BSD
    nvgluSetCornerRadius(12.0f * calculateRenderScale(), roundedRight, roundedRight);
#    endif

    surfaces[asyncNvg] = this;

    nanovg::nvgAtlasTextThreshold(asyncNvg, 32.0f);
    nanovg::nvgCreateFontMem(asyncNvg, "Inter-Regular", BinaryData::getResourceCopy(BinaryData::InterRegular_ttf), BinaryData::getResourceSize(BinaryData::InterRegular_ttf), 0);
    nanovg::nvgCreateFontMem(asyncNvg, "Inter-Bold", BinaryData::getResourceCopy(BinaryData::InterBold_ttf), BinaryData::getResourceSize(BinaryData::InterBold_ttf), 0);
    nanovg::nvgCreateFontMem(asyncNvg, "Inter-Tabular", BinaryData::getResourceCopy(BinaryData::InterTabular_ttf), BinaryData::getResourceSize(BinaryData::InterTabular_ttf), 0);
    nanovg::nvgCreateFontMem(asyncNvg, "icon_font-Regular", BinaryData::getResourceCopy(BinaryData::IconFont_ttf), BinaryData::getResourceSize(BinaryData::IconFont_ttf), 0);

    MessageManager::callAsync([_this = SafePointer(this)] {
        if (_this)
            _this->invalidateAll();
    });
}

void NVGSurface::destroyRenderContext()
{
    // Runs on the render thread, while the message thread is blocked in detach(),
    // so recordFrame cannot be running concurrently during GL shutdown. Metal
    // calls this from its render thread after stopThread() has been requested.

    // Destroy the render-thread-owned main framebuffer while its backend context
    // is still current/valid (before nvgDeleteContext).
    if (mainFramebuffer) {
        nvgDeleteFramebuffer(reinterpret_cast<NVGframebuffer*>(mainFramebuffer));
        mainFramebuffer = nullptr;
    }
    mainFramebufferWidth = 0;
    mainFramebufferHeight = 0;

    if (auto* asyncNvg = nvg.exchange(nullptr)) {
        surfaces.erase(asyncNvg);
        nanovg::destroy(asyncNvg);   // frees the async layer's own backend framebuffers
    }

    if (baseNvg) {
        nvgDeleteContext(baseNvg);
        baseNvg = nullptr;
    }

    frameReadyForReplay.store(false);
    recordedFramebufferWidth.store(0);
    recordedFramebufferHeight.store(0);
    recordedFrameIsFullRepaint.store(false);
}

#if NANOVG_GL_IMPLEMENTATION
void NVGSurface::newOpenGLContextCreated()
{
    createRenderContext();
}

void NVGSurface::openGLContextClosing()
{
    destroyRenderContext();
}
#endif

#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
void NVGSurface::run()
{
    createRenderContext();

    while (!threadShouldExit()) {
        if (!backendRenderRequested.exchange(false))
            wait(-1);

        if (threadShouldExit())
            break;

        renderBackendFrame();
    }

    destroyRenderContext();
}
#endif

void NVGSurface::presentFramebuffer(int viewWidth, int viewHeight)
{
    nvgBindFramebuffer(nullptr);

#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    if (metalView)
        mnvgSetViewBounds(metalView, viewWidth, viewHeight);
#endif

    nvgViewport(0, 0, viewWidth, viewHeight);

    if (mainFramebuffer) {
        // Copy the FBO's colour buffer directly instead of sampling it as a
        // texture: on this macOS GL driver the render-to-texture write is not
        // visible to a subsequent texture fetch (even after glFinish), but the
        // colour buffer itself is correct (verified via glReadPixels). The Metal
        // backend's nvgBlitFramebuffer macro maps to the equivalent screen blit.
        nvgBlitFramebuffer(baseNvg, reinterpret_cast<NVGframebuffer*>(mainFramebuffer), 0, 0, viewWidth, viewHeight);
    } else {
        nvgClear(baseNvg);
    }
}

void NVGSurface::renderBackendFrame()
{
    auto const pixelScale = calculateRenderScale();
    auto const viewWidth = jmax(1, roundToInt(editor->getWidth() * pixelScale));
    auto const viewHeight = jmax(1, roundToInt(editor->getHeight() * pixelScale));

    if (!baseNvg)
        return;

    auto* asyncNvg = nvg.load();
    if (!asyncNvg)
        return;

    // The persistent main/damage framebuffer is owned entirely by this render
    // thread. When the window size changes, do not switch to a freshly-created
    // framebuffer until the message thread has published a full repaint for
    // this exact pixel size; otherwise a partial or missing frame would leave
    // empty pixels visible for one swap.
    if (!mainFramebuffer || mainFramebufferWidth != viewWidth || mainFramebufferHeight != viewHeight) {
        auto const hasMatchingFullFrame = frameReadyForReplay.load()
            && nanovg::hasPendingFrame(asyncNvg)
            && recordedFrameIsFullRepaint.load()
            && recordedFramebufferWidth.load() == viewWidth
            && recordedFramebufferHeight.load() == viewHeight;

        if (!hasMatchingFullFrame) {
            MessageManager::callAsync([_this = SafePointer(this)] {
                if (_this)
                    _this->invalidateAll();
            });

            presentFramebuffer(viewWidth, viewHeight);
            return;
        }

        auto* oldFramebuffer = reinterpret_cast<NVGframebuffer*>(mainFramebuffer);

        if (auto* newFramebuffer = nvgCreateFramebuffer(baseNvg, viewWidth, viewHeight, 0)) {
            mainFramebuffer = newFramebuffer;
            mainFramebufferWidth = viewWidth;
            mainFramebufferHeight = viewHeight;

            if (oldFramebuffer)
                nvgDeleteFramebuffer(oldFramebuffer);
        } else {
            presentFramebuffer(viewWidth, viewHeight);
            return;
        }
    }

    // Tell the async layer which real framebuffer the recorded
    // bindMainFramebuffer() ops resolve to, then bind it as the initial target.
    nanovg::setMainFramebuffer(asyncNvg, mainFramebuffer);
    nvgBindFramebuffer(reinterpret_cast<NVGframebuffer*>(mainFramebuffer));

    // Replay the recorded frame: redraws the damaged region into the persistent
    // framebuffer (the rest of it is preserved).
    bool const didRender = nanovg::performRender(asyncNvg);

    if (didRender) {
        frameReadyForReplay.store(false);
#    if ENABLE_FPS_COUNT
        if (frameTimer)
            frameTimer->addFrameTime();
#    endif
    }

    // Composite the persistent framebuffer onto the screen every pass, so the
    // window keeps showing the accumulated content after each buffer swap.
    presentFramebuffer(viewWidth, viewHeight);
}

#if NANOVG_GL_IMPLEMENTATION
void NVGSurface::renderOpenGL()
{
    renderBackendFrame();
}
#endif

void NVGSurface::detachContext()
{
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    if (isThreadRunning()) {
        backendRenderRequested.store(true);
        stopThread(5000);
    }

    backendRenderRequested.store(false);

    if (metalView) {
        setView(nullptr);
        OSUtils::MTLDeleteView(metalView);
        metalView = nullptr;
    }
#elif NANOVG_GL_IMPLEMENTATION
    if (glContext)
        glContext->detach();
#endif
}

void NVGSurface::updateBufferSize()
{
}

void NVGSurface::lookAndFeelChanged()
{
    // Message thread. Records cache-clear ops into the async buffer; no lock
    // needed (record<->replay is decoupled by the async layer's buffer swap).
    if (auto* asyncNvg = nvg.load()) {
        NVGCachedPath::clearAll(asyncNvg);
        NVGImage::clearAll(asyncNvg);
    }
    invalidateAll();
}

bool NVGSurface::makeContextActive()
{
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    return isThreadRunning() && nvg.load() != nullptr;
#elif NANOVG_GL_IMPLEMENTATION
    return glContext && glContext->isAttached();
#else
    return false;
#endif
}

float NVGSurface::calculateRenderScale() const
{
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    if (metalView) {
        auto const scale = OSUtils::MTLGetPixelScale(metalView);
        if (scale > 0.0f)
            return scale * Desktop::getInstance().getGlobalScaleFactor();
    }
#elif NANOVG_GL_IMPLEMENTATION
    if (glContext) {
        auto const scale = glContext->getRenderingScale();
        if (scale > 0.0f)
            return scale;
    }
#endif
    return Desktop::getInstance().getGlobalScaleFactor();
}

float NVGSurface::getRenderScale() const
{
    return lastRenderScale > 0.0f ? lastRenderScale : calculateRenderScale();
}

void NVGSurface::updateBounds(Rectangle<int>)
{
    currentBounds = editor->getLocalBounds();

    if (getBounds() != currentBounds)
        setBounds(currentBounds);

    initialise();
    invalidateAll();
}

void NVGSurface::resized()
{
    invalidateAll();

#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    if (metalView) {
        auto const pixelScale = calculateRenderScale();
        auto const viewWidth = jmax(1, roundToInt(editor->getWidth() * pixelScale));
        auto const viewHeight = jmax(1, roundToInt(editor->getHeight() * pixelScale));
        mnvgSetViewBounds(metalView, viewWidth, viewHeight);
    }
#endif
}

void NVGSurface::invalidateAll()
{
    invalidateArea(getLocalBounds());
}

void NVGSurface::invalidateArea(Rectangle<int> const area)
{
    invalidArea = invalidArea.getUnion(area);
    scheduleRender();
}

void NVGSurface::scheduleRender()
{
    if (renderScheduled.exchange(true))
        return;

    MessageManager::callAsync([_this = SafePointer(this)] {
        if (!_this)
            return;

        _this->renderScheduled.store(false);
        _this->recordFrame();
    });
}

void NVGSurface::renderAll()
{
    invalidateAll();
}

void NVGSurface::render()
{
    if (!MessageManager::getInstance()->isThisTheMessageThread()) {
        scheduleRender();
        return;
    }

    renderScheduled.store(false);
    recordFrame();
}

void NVGSurface::requestBackendRender()
{
#if NANOVG_METAL_IMPLEMENTATION && (JUCE_MAC || JUCE_IOS)
    backendRenderRequested.store(true);
    notify();
#elif NANOVG_GL_IMPLEMENTATION
    if (glContext)
        glContext->triggerRepaint();
#endif
}

void NVGSurface::recordFrame()
{
    JUCE_ASSERT_MESSAGE_THREAD;

    auto const bounds = editor->getLocalBounds();
    if (bounds.isEmpty())
        return;

    lastRenderScale = calculateRenderScale();
    auto const desktopScale = Desktop::getInstance().getGlobalScaleFactor();
    auto const devicePixelScale = lastRenderScale / desktopScale;

    int const fbWidth = jmax(1, roundToInt(bounds.getWidth() * lastRenderScale));
    int const fbHeight = jmax(1, roundToInt(bounds.getHeight() * lastRenderScale));

    // No contextMutex: recording only writes into the async layer's record buffer
    // (handed to the render thread via publish()'s tiny-lock swap) and reads the
    // `nvg` atomic. Context teardown runs on the render thread while the message
    // thread is blocked in detach(), so it can never overlap this.
    auto* asyncNvg = nvg.load();
    if (!asyncNvg) {
        initialise();
        return;
    }

    {
        // Damage tracking: draw only the region invalidated through the
        // InvalidationListener. If the previously recorded frame is still pending
        // it will be coalesced away by the publish below, so fold its damage back
        // in to make sure nothing is lost.
        if (nanovg::hasPendingFrame(asyncNvg))
            invalidArea = invalidArea.getUnion(inFlightDamage);

        auto const damage = invalidArea.getIntersection(bounds);
        if (damage.isEmpty())
            return;
        auto const isFullRepaint = damage.contains(bounds);

        nanovg::setCurrentPixelScale(asyncNvg, devicePixelScale);

        for (auto bufferedObject : bufferedObjects) {
            if (bufferedObject)
                bufferedObject->updateFramebuffers(asyncNvg);
        }

        // Render the damaged region into the persistent main framebuffer. The
        // render thread owns that framebuffer; we just record "bind the main target"
        // here. It must come AFTER updateFramebuffers, because buffered objects
        // bind (and unbind to the default target) their own framebuffers while
        // updating.
        nanovg::bindMainFramebuffer(asyncNvg);
        nanovg::viewport(asyncNvg, 0, 0, fbWidth, fbHeight);
        nanovg::nvgBeginFrame(asyncNvg, bounds.getWidth() * desktopScale, bounds.getHeight() * desktopScale, devicePixelScale);
        nanovg::nvgScale(asyncNvg, desktopScale, desktopScale);

        auto& llgc = editor->getOrCreateNanoLLGC(asyncNvg, lastRenderScale);
        llgc.resetClipRegion();
        Graphics g(llgc);
        g.reduceClipRegion(invalidArea);

        editor->paintEntireComponent(g, false);

        // Restrict the backend flush to the damaged region so the rest of the
        // framebuffer keeps its previous contents (persistence). Coordinates are
        // in device pixels.
        nanovg::nvgGlobalScissor(asyncNvg,
            roundToInt(damage.getX() * lastRenderScale),
            roundToInt(damage.getY() * lastRenderScale),
            roundToInt(damage.getWidth() * lastRenderScale),
            roundToInt(damage.getHeight() * lastRenderScale));

        nanovg::nvgEndFrame(asyncNvg);

        inFlightDamage = damage;
        recordedFramebufferWidth.store(fbWidth);
        recordedFramebufferHeight.store(fbHeight);
        recordedFrameIsFullRepaint.store(isFullRepaint);
    }

    invalidArea = {};
    frameReadyForReplay.store(true);
    requestBackendRender();
}

#if JUCE_LINUX || JUCE_BSD
void NVGSurface::setRoundedBottomCorners(bool left, bool right)
{
    roundedLeft = left;
    roundedRight = right;
#    ifdef NANOVG_GL_IMPLEMENTATION
    nvgluSetCornerRadius(12.0f * getRenderScale(), roundedLeft, roundedRight);
#    endif
}
#endif

void NVGSurface::setRenderThroughImage(bool const shouldRenderThroughImage)
{
    renderThroughImage = shouldRenderThroughImage;
    invalidateAll();
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

// TODO: juce9, I think we can remove this?
void NVGSurface::handleCommandMessage(int)
{
    scheduleRender();
}

void NVGSurface::InvalidationListener::paint(Graphics& g) {
    if(nvgRepaint) {
        if (auto* llgc = dynamic_cast<NVGGraphicsContext*>(&g.getInternalContext())) {
            if (auto* nvgComponent = dynamic_cast<NVGComponent*>(originComponent)) {
                nvgComponent->render(llgc->getContext());
                return;
            }
        }
    }
}
