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

#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
    frameTimeOverlay = std::make_unique<FrameTimeOverlay>();
    frameTimeVBlankAttachment = std::make_unique<VBlankAttachment>(this, [this](double timestampSec) {
        if (!frameTimeOverlay)
            return;

        frameTimeOverlay->addVBlank(timestampSec);
        requestBackendRender();
    });
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
    nvgluSetCornerRadius(12.0f * calculateRenderScale());
#    endif

    surfaces[asyncNvg] = this;
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
    glContext->setSwapInterval(0);
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

    serviceReadbackRequest();

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

    nanovg::setMainFramebuffer(asyncNvg, mainFramebuffer);
    nvgBindFramebuffer(reinterpret_cast<NVGframebuffer*>(mainFramebuffer));

    // Replay the recorded frame: redraws the damaged region into the persistent
    // framebuffer (the rest of it is preserved).
#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
    auto const renderStartMs = Time::getMillisecondCounterHiRes();
#endif
    bool const didRender = nanovg::performRender(asyncNvg);

    if (didRender) {
        frameReadyForReplay.store(false);
#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
        // The render-thread cost of actually replaying the frame -- a real estimate
        // of frame performance, unlike the (throttled) interval between repaints.
        if (frameTimeOverlay)
            frameTimeOverlay->addFrame(Time::getMillisecondCounterHiRes() - renderStartMs);
#endif
    }

#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
    drawFrameTimeOverlay(viewWidth, viewHeight, pixelScale);
#endif

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

void NVGSurface::handleAsyncUpdate()
{
    recordFrame();
}

void NVGSurface::scheduleRender()
{
    triggerAsyncUpdate();
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

#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
void NVGSurface::drawFrameTimeOverlay(int const viewWidth, int const viewHeight, float const scale)
{
    if (!frameTimeOverlay || !baseNvg || !mainFramebuffer)
        return;

    auto const size = FrameTimeOverlay::sizeFor(scale);
    auto const margin = 10.0f * scale;
    auto const x = static_cast<float>(viewWidth) - size.width - margin;
    auto const y = static_cast<float>(viewHeight) - size.height - margin;

    nvgViewport(0, 0, viewWidth, viewHeight);

    nvgBeginFrame(baseNvg, static_cast<float>(viewWidth), static_cast<float>(viewHeight), 1.0f);
    frameTimeOverlay->draw(baseNvg, x, y, scale);
    nvgGlobalScissor(baseNvg, 0, 0, viewWidth, viewHeight);
    nvgEndFrame(baseNvg);
}
#endif

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

#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
    auto const prepareStartMs = Time::getMillisecondCounterHiRes();
#endif

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

        if (isFullRepaint)
            nanovg::clear(asyncNvg);

        nanovg::nvgBeginFrame(asyncNvg, bounds.getWidth() * desktopScale, bounds.getHeight() * desktopScale, devicePixelScale);
        nanovg::nvgScale(asyncNvg, desktopScale, desktopScale);

        auto& llgc = editor->getOrCreateNanoLLGC(asyncNvg, lastRenderScale);
        llgc.resetClipRegion(AffineTransform::scale(desktopScale, desktopScale));
        Graphics g(llgc);
        g.reduceClipRegion(invalidArea);

        editor->paintEntireComponent(g, false);

#if PLUGDATA_NVG_REPAINT_DEBUG
        static Random rng;
        g.fillAll (Colour ((uint8) rng.nextInt (255),
                           (uint8) rng.nextInt (255),
                           (uint8) rng.nextInt (255),
                           (uint8) 0x50));
#endif

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

#if PLUGDATA_NVG_FRAME_TIME_OVERLAY
    if (frameTimeOverlay)
        frameTimeOverlay->setPrepareTime(Time::getMillisecondCounterHiRes() - prepareStartMs);
#endif

    invalidArea = {};
    frameReadyForReplay.store(true);
    requestBackendRender();
}

void NVGSurface::serviceReadbackRequest()
{
    // Render thread. Reads the requested region straight out of the persistent
    // main framebuffer and hands the raw BGRA pixels back to the waiting caller.
    if (!readbackPending.load())
        return;

    auto const area = readbackDeviceArea;
    bool succeeded = false;

    if (baseNvg && mainFramebuffer && !area.isEmpty()
        && area.getX() >= 0 && area.getY() >= 0
        && area.getRight() <= mainFramebufferWidth
        && area.getBottom() <= mainFramebufferHeight) {
        auto const w = area.getWidth();
        auto const h = area.getHeight();

        readbackData.malloc(static_cast<size_t>(w) * h * 4);
        nvgReadPixels(baseNvg, reinterpret_cast<NVGframebuffer*>(mainFramebuffer),
            area.getX(), area.getY(), w, h, mainFramebufferHeight, readbackData.getData());

        readbackWidth = w;
        readbackHeight = h;
        succeeded = true;
    }

    readbackSucceeded = succeeded;
    readbackPending.store(false);
    readbackReady.signal();
}

Image NVGSurface::renderToImage(Rectangle<int> logicalArea)
{
    JUCE_ASSERT_MESSAGE_THREAD;

    if (!makeContextActive())
        return {};

    logicalArea = logicalArea.getIntersection(getLocalBounds());
    if (logicalArea.isEmpty())
        return {};

    auto const scale = getRenderScale();
    Rectangle<int> const deviceBounds(0, 0,
        jmax(1, roundToInt(getWidth() * scale)),
        jmax(1, roundToInt(getHeight() * scale)));

    auto const deviceArea = Rectangle<int>(
        roundToInt(logicalArea.getX() * scale),
        roundToInt(logicalArea.getY() * scale),
        jmax(1, roundToInt(logicalArea.getWidth() * scale)),
        jmax(1, roundToInt(logicalArea.getHeight() * scale)))
                                .getIntersection(deviceBounds);

    if (deviceArea.isEmpty())
        return {};

    // One readback at a time; the eyedropper is the only caller.
    ScopedLock const sl(readbackLock);

    readbackDeviceArea = deviceArea;
    readbackSucceeded = false;
    readbackReady.reset();
    readbackPending.store(true);

    // Kick the render thread, which owns the framebuffer, to service the request.
    requestBackendRender();

    if (!readbackReady.wait(200) || !readbackSucceeded) {
        readbackPending.store(false);
        return {};
    }

    auto const w = readbackWidth;
    auto const h = readbackHeight;

    Image image(Image::ARGB, w, h, false);
    {
        Image::BitmapData bitmap(image, Image::BitmapData::writeOnly);
        auto const* src = readbackData.getData();
        for (int y = 0; y < h; ++y) {
            // nvgReadPixels returns top-left-origin BGRA for both GL and Metal.
            auto const* s = src + static_cast<size_t>(y) * w * 4;
            auto* d = reinterpret_cast<PixelARGB*>(bitmap.getLinePointer(y));
            for (int x = 0; x < w; ++x) {
                // Force opaque: the eyedropper wants the visible colour, not the
                // framebuffer's alpha.
                d[x].setARGB(255, s[2], s[1], s[0]);
                s += 4;
            }
        }
    }

    // Hand back a logical-resolution image so callers can work in logical coords.
    if (w != logicalArea.getWidth() || h != logicalArea.getHeight())
        return image.rescaled(logicalArea.getWidth(), logicalArea.getHeight(), Graphics::mediumResamplingQuality);

    return image;
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
