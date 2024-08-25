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
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

class FrameTimer;
class PluginEditor;
class NVGSurface :
#if NANOVG_METAL_IMPLEMENTATION && JUCE_MAC
    public NSViewComponent
#elif NANOVG_METAL_IMPLEMENTATION && JUCE_IOS
    public UIViewComponent
#else
    public Component
    , public Timer
#endif
{
public:
    NVGSurface(PluginEditor* editor);
    ~NVGSurface();

    void initialise();
    void updateBufferSize();

    void render();

    void triggerRepaint();
    bool makeContextActive();

    void detachContext();

#ifdef NANOVG_GL_IMPLEMENTATION
    void timerCallback() override;
#endif

    void lookAndFeelChanged() override;

    Rectangle<int> getInvalidArea() { return invalidArea; }

    float getRenderScale() const;

    void updateBounds(Rectangle<int> bounds);

    class InvalidationListener : public CachedComponentImage {
    public:
        InvalidationListener(NVGSurface& s, Component* origin, bool passRepaintEvents = false)
            : surface(s)
            , originComponent(origin)
            , passEvents(passRepaintEvents)
        {
        }

        void paint(Graphics& g) override {};

        bool invalidate(Rectangle<int> const& rect) override
        {
            if (originComponent->isVisible()) {
                // Translate from canvas coords to viewport coords as float to prevent rounding errors
                auto invalidatedBounds = surface.getLocalArea(originComponent, rect.expanded(2).toFloat()).getSmallestIntegerContainer();
                surface.invalidateArea(invalidatedBounds);
            }
            return passEvents;
        }

        bool invalidateAll() override
        {
            if (originComponent->isVisible()) {
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
    
    void setRenderThroughImage(bool renderThroughImage);
    
    NVGcontext* getRawContext() { return nvg; }

    static NVGSurface* getSurfaceForContext(NVGcontext*);

private:
    
    void renderFrameToImage(NVGframebuffer* fb, Rectangle<int> area);
    
    float calculateRenderScale() const;
    
    void resized() override;

    PluginEditor* editor;
    NVGcontext* nvg = nullptr;
    bool needsBufferSwap = false;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;

    Rectangle<int> invalidArea;
    NVGframebuffer* invalidFBO = nullptr;
    int fbWidth = 0, fbHeight = 0;

    static inline std::map<NVGcontext*, NVGSurface*> surfaces;
    
    juce::Image backupRenderImage;
    bool renderThroughImage = false;
    ImageComponent backupImageComponent;
    std::vector<uint32> backupPixelData;

    float lastRenderScale = 0.0f;
    uint32 lastRenderTime;

    Colour cnvColJuce;
    NVGcolor cnvCol;
    
#if NANOVG_GL_IMPLEMENTATION
    bool hresize = false;
    bool resizing = false;
    Rectangle<int> newBounds;
    std::unique_ptr<OpenGLContext> glContext;
#endif

    std::unique_ptr<FrameTimer> frameTimer;
};

class NVGComponent {
public:
    NVGComponent(Component* comp)
        : component(*comp)
    {
    }

    static NVGcolor convertColour(Colour c)
    {
        return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
    }

    NVGcolor findNVGColour(int colourId)
    {
        return convertColour(component.findColour(colourId));
    }

    static void setJUCEPath(NVGcontext* nvg, Path const& p)
    {
        Path::Iterator i(p);

        nvgBeginPath(nvg);

        while (i.next()) {
            switch (i.elementType) {
            case Path::Iterator::startNewSubPath:
                nvgMoveTo(nvg, i.x1, i.y1);
                break;
            case Path::Iterator::lineTo:
                nvgLineTo(nvg, i.x1, i.y1);
                break;
            case Path::Iterator::quadraticTo:
                nvgQuadTo(nvg, i.x1, i.y1, i.x2, i.y2);
                break;
            case Path::Iterator::cubicTo:
                nvgBezierTo(nvg, i.x1, i.y1, i.x2, i.y2, i.x3, i.y3);
                break;
            case Path::Iterator::closePath:
                nvgClosePath(nvg);
                break;
            default:
                break;
            }
        }
    }

    virtual void render(NVGcontext*) {};

private:
    Component& component;

    JUCE_DECLARE_WEAK_REFERENCEABLE(NVGComponent)
};

class NVGImage {
public:
    enum NVGImageFlags {
        RepeatImage = 1 << 0,
        DontClear = 1 << 1,
        AlphaImage = 1 << 2
    };

    NVGImage(NVGcontext* nvg, int width, int height, std::function<void(Graphics&)> renderCall, int imageFlags = 0)
    {
        bool clearImage = !(imageFlags & NVGImageFlags::DontClear);
        bool repeatImage = imageFlags & NVGImageFlags::RepeatImage;

        // When JUCE image format is SingleChannel the graphics context will render only the alpha component
        // into the image data, it is not a greyscale image of the graphics context.
        auto imageFormat = imageFlags & NVGImageFlags::AlphaImage ? Image::SingleChannel : Image::ARGB;

        Image image = Image(imageFormat, width, height, clearImage);
        Graphics g(image); // Render resize handles with JUCE, since rounded rect exclusion is hard with nanovg
        renderCall(g);
        loadJUCEImage(nvg, image, repeatImage);
        allImages.insert(this);
    }

    NVGImage()
    {
        allImages.insert(this);
    }

    NVGImage(NVGImage& other)
    {
        // Check for self-assignment
        if (this != &other) {
            nvg = other.nvg;
            imageId = other.imageId;
            imageWidth = other.imageWidth;
            imageHeight = other.imageHeight;
            onImageInvalidate = other.onImageInvalidate;
            
            other.imageId = 0;
            allImages.insert(this);
        }
    }

    NVGImage& operator=(NVGImage&& other) noexcept
    {
        // Check for self-assignment
        if (this != &other) {
            // Delete current image
            if(imageId && nvg)
            {
                if (auto* surface = NVGSurface::getSurfaceForContext(nvg)) {
                    surface->makeContextActive();
                }
                nvgDeleteImage(nvg, imageId);
            }
            
            nvg = other.nvg;
            imageId = other.imageId;
            imageWidth = other.imageWidth;
            imageHeight = other.imageHeight;
            onImageInvalidate = other.onImageInvalidate;
            
            other.imageId = 0; // Important, makes sure the old buffer can't delete this buffer
            allImages.insert(this);
        }

        return *this;
    }

    ~NVGImage()
    {
        if (imageId && nvg) {
            if (auto* surface = NVGSurface::getSurfaceForContext(nvg)) {
                surface->makeContextActive();
            }

            nvgDeleteImage(nvg, imageId);
        }
        allImages.erase(this);
    }

    static void clearAll(NVGcontext* nvg)
    {
        for (auto* image : allImages) {
            if (image->isValid() && image->nvg == nvg) {
                nvgDeleteImage(image->nvg, image->imageId);
                image->imageId = 0;
                if (image->onImageInvalidate)
                    image->onImageInvalidate();
            }
        }
    }

    bool isValid()
    {
        return imageId != 0;
    }

    void renderJUCEComponent(NVGcontext* nvg, Component& component, float scale)
    {
        Image componentImage = component.createComponentSnapshot(Rectangle<int>(0, 0, component.getWidth(), component.getHeight()), false, scale);
        if (componentImage.isNull())
            return;

        loadJUCEImage(nvg, componentImage);

        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, component.getWidth(), component.getHeight(), 0, imageId, 1.0f));
        nvgFillRect(nvg, 0, 0, component.getWidth(), component.getHeight());
    }

    void loadJUCEImage(NVGcontext* context, Image& image, int repeatImage = false)
    {
        Image::BitmapData imageData(image, Image::BitmapData::readOnly);

        int width = imageData.width;
        int height = imageData.height;

        if (imageId && imageWidth == width && imageHeight == height && nvg == context) {
            nvgUpdateImage(nvg, imageId, imageData.data);
        } else {
            nvg = context;
            auto flags = repeatImage ? NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY : 0;

            if (image.isARGB())
                imageId = nvgCreateImageARGB(nvg, width, height, flags | NVG_IMAGE_PREMULTIPLIED, imageData.data);
            else if (image.isSingleChannel())
                imageId = nvgCreateImageAlpha(nvg, width, height, flags, imageData.data);

            imageWidth = width;
            imageHeight = height;
        }
    }

    void render(NVGcontext* nvg, Rectangle<int> b)
    {
        if (imageId) {
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, b.getWidth(), b.getHeight(), 0, imageId, 1));
            nvgFillRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
        }
    }

    bool needsUpdate(int width, int height)
    {
        return imageId == 0 || width != imageWidth || height != imageHeight || isDirty;
    }

    int getImageId()
    {
        return imageId;
    }

    void setDirty()
    {
        isDirty = true;
    }

    NVGcontext* nvg = nullptr;
    int imageId = 0;
    int imageWidth = 0, imageHeight = 0;
    bool isDirty = false;

    std::function<void()> onImageInvalidate = nullptr;

    static inline std::set<NVGImage*> allImages = std::set<NVGImage*>();
};

class NVGFramebuffer {
public:
    NVGFramebuffer()
    {
        allFramebuffers.insert(this);
    }

    ~NVGFramebuffer()
    {
        if (fb) {
            if (auto* surface = NVGSurface::getSurfaceForContext(nvg)) {
                surface->makeContextActive();
            }

            nvgDeleteFramebuffer(fb);
            fb = nullptr;
        }
        allFramebuffers.erase(this);
    }

    static void clearAll(NVGcontext* nvg)
    {
        for (auto* buffer : allFramebuffers) {
            if (buffer->nvg == nvg && buffer->fb) {
                nvgDeleteFramebuffer(buffer->fb);
                buffer->fb = nullptr;
            }
        }
    }

    bool needsUpdate(int width, int height)
    {
        return fb == nullptr || width != fbWidth || height != fbHeight || fbDirty;
    }

    bool isValid()
    {
        return fb != nullptr;
    }

    void setDirty()
    {
        fbDirty = true;
    }

    void bind(NVGcontext* ctx, int width, int height)
    {
        if (!fb || fbWidth != width || fbHeight != height) {
            nvg = ctx;
            if (fb)
                nvgDeleteFramebuffer(fb);
            fb = nvgCreateFramebuffer(nvg, width, height, NVG_IMAGE_PREMULTIPLIED);
            fbWidth = width;
            fbHeight = height;
        }

        nvgBindFramebuffer(fb);
    }

    void unbind()
    {
        nvgBindFramebuffer(nullptr);
    }

    void renderToFramebuffer(NVGcontext* nvg, int width, int height, std::function<void(NVGcontext*)> renderCallback)
    {
        bind(nvg, width, height);
        renderCallback(nvg);
        unbind();
        fbDirty = false;
    }

    void render(NVGcontext* nvg, Rectangle<int> b)
    {
        if (fb) {
            nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, b.getWidth(), b.getHeight(), 0, fb->image, 1));
            nvgFillRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
        }
    }

    int getImage()
    {
        if (!fb)
            return -1;

        return fb->image;
    }

private:
    static inline std::set<NVGFramebuffer*> allFramebuffers;

    NVGcontext* nvg;
    NVGframebuffer* fb = nullptr;
    int fbWidth, fbHeight;
    bool fbDirty = false;
};

class NVGCachedPath {
public:
    NVGCachedPath()
    {
        allCachedPaths.insert(this);
    }

    ~NVGCachedPath()
    {
        if (cacheId != -1) {
            nvgDeletePath(nvg, cacheId);
            cacheId = -1;
        }
        allCachedPaths.erase(this);
    }

    static void clearAll(NVGcontext* nvg)
    {
        for (auto* buffer : allCachedPaths) {
            if (buffer->nvg == nvg) {
                buffer->clear();
            }
        }
    }
    
    void clear()
    {
        if(cacheId != -1) {
            nvgDeletePath(nvg, cacheId);
            cacheId = -1;
            nvg = nullptr;
        }
    }

    bool isValid()
    {
        return cacheId != -1;
    }
    
    void save(NVGcontext* ctx)
    {
        if(nvg == ctx && cacheId != -1) nvgDeletePath(nvg, cacheId);
        nvg = ctx;
        cacheId = nvgSavePath(nvg, cacheId);
    }
    
    bool stroke()
    {
        if(!nvg || cacheId == -1) return false;
        return nvgStrokeCachedPath(nvg, cacheId);
    }
    
    bool fill()
    {
        if(!nvg || cacheId == -1) return false;
        return nvgFillCachedPath(nvg, cacheId);
    }

private:
    static inline std::set<NVGCachedPath*> allCachedPaths;
    NVGcontext* nvg = nullptr;
    int cacheId = -1;
};

struct NVGScopedState
{
    NVGScopedState(NVGcontext* nvg) : nvg(nvg)
    {
        nvgSave(nvg);
    }
    
    ~NVGScopedState()
    {
        nvgRestore(nvg);
    }
    
    NVGcontext* nvg;
};
