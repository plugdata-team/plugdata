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
class NVGComponent;
class NVGSurface final :
#if NANOVG_METAL_IMPLEMENTATION && JUCE_MAC
    public NSViewComponent
#elif NANOVG_METAL_IMPLEMENTATION && JUCE_IOS
    public UIViewComponent
#else
    public Component
#endif
{
public:
    NVGSurface(PluginEditor* editor);
    ~NVGSurface() override;

    void initialise();
    void updateBufferSize();

    void renderAll();
    void render();

    bool makeContextActive();

    void detachContext();

    void lookAndFeelChanged() override;

    Rectangle<int> getInvalidArea() const { return invalidArea; }

    float getRenderScale() const;

    void updateBounds(Rectangle<int> bounds);

    class InvalidationListener final : public CachedComponentImage {
    public:
        InvalidationListener(NVGSurface& s, Component* origin, bool const passRepaintEvents = false)
            : surface(s)
            , originComponent(origin)
            , passEvents(passRepaintEvents)
        {
        }

        void paint(Graphics& g) override { }

        bool invalidate(Rectangle<int> const& rect) override
        {
            // Translate from canvas coords to viewport coords as float to prevent rounding errors
            auto invalidatedBounds = surface.getLocalArea(originComponent, rect.expanded(2).toFloat()).getSmallestIntegerContainer();
            invalidatedBounds = invalidatedBounds.getIntersection(surface.getLocalBounds());

            if (originComponent->isVisible() && !invalidatedBounds.isEmpty()) {
                surface.invalidateArea(invalidatedBounds);
            }

            return surface.renderThroughImage || passEvents;
        }

        bool invalidateAll() override
        {
            if (originComponent->isVisible()) {
                surface.invalidateArea(originComponent->getLocalBounds());
            }
            return surface.renderThroughImage || passEvents;
        }

        void releaseResources() override { }

        NVGSurface& surface;
        Component* originComponent;
        bool passEvents;
    };

    void invalidateArea(Rectangle<int> area);
    void invalidateAll();

    void setRenderThroughImage(bool renderThroughImage);

    static NVGSurface* getSurfaceForContext(NVGcontext*);

    void renderFrameToImage(Image& image, Rectangle<int> area);

    void resized() override;
    
    void addBufferedObject(NVGComponent* component);
    void removeBufferedObject(NVGComponent* component);
    

private:
    float calculateRenderScale() const;

    // Sets the surface context to render through floating window, or inside editor as image
    void updateWindowContextVisibility();

    PluginEditor* editor;
    NVGcontext* nvg = nullptr;
    bool needsBufferSwap = false;
    std::unique_ptr<VBlankAttachment> vBlankAttachment;

    Rectangle<int> invalidArea;
    NVGframebuffer* invalidFBO = nullptr;
    int fbWidth = 0, fbHeight = 0;

    static inline UnorderedMap<NVGcontext*, NVGSurface*> surfaces;

    juce::Image backupRenderImage;
    bool renderThroughImage = false;
    ImageComponent backupImageComponent;
    HeapArray<uint32> backupPixelData;
    
    UnorderedSegmentedSet<WeakReference<NVGComponent>> bufferedObjects;

    float lastRenderScale = 0.0f;
    uint32 lastRenderTime;

#if NANOVG_GL_IMPLEMENTATION
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

    virtual ~NVGComponent() { }

    static NVGcolor convertColour(Colour const c)
    {
        return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
    }

    static Colour convertColour(NVGcolor const c)
    {
        return Colour(c.r, c.b, c.g, c.a);
    }

    NVGcolor findNVGColour(int const colourId) const
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

    virtual void updateFramebuffers(NVGcontext*) { }
    
    virtual void render(NVGcontext*) { }

private:
    Component& component;

    JUCE_DECLARE_WEAK_REFERENCEABLE(NVGComponent)
};

class NVGImage {
public:
    enum NVGImageFlags {
        RepeatImage = 1 << 0,
        DontClear = 1 << 1,
        AlphaImage = 1 << 2,
        MipMap = 1 << 3
    };

    NVGImage(NVGcontext* nvg, int width, int height, std::function<void(Graphics&)> renderCall, int const imageFlags = 0, Colour const clearColour = Colours::transparentBlack)
    {
        bool const clearImage = !(imageFlags & NVGImageFlags::DontClear);
        bool const repeatImage = imageFlags & NVGImageFlags::RepeatImage;
        bool const withMipmaps = imageFlags & NVGImageFlags::MipMap;

        // When JUCE image format is SingleChannel the graphics context will render only the alpha component
        // into the image data, it is not a greyscale image of the graphics context.
        auto const imageFormat = imageFlags & NVGImageFlags::AlphaImage ? Image::SingleChannel : Image::ARGB;

        auto image = Image(imageFormat, width, height, false);
        if (clearImage)
            image.clear({ 0, 0, width, height }, clearColour);
        Graphics g(image); // Render resize handles with JUCE, since rounded rect exclusion is hard with nanovg
        renderCall(g);
        loadJUCEImage(nvg, image, repeatImage, withMipmaps);
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
            subImages = other.subImages;
            totalWidth = other.totalWidth;
            totalHeight = other.totalHeight;
            onImageInvalidate = other.onImageInvalidate;

            other.subImages.clear();
            allImages.insert(this);
        }
    }

    NVGImage& operator=(NVGImage&& other) noexcept
    {
        // Check for self-assignment
        if (this != &other) {
            // Delete current image
            if (subImages.not_empty() && nvg) {
                if (auto* surface = NVGSurface::getSurfaceForContext(nvg)) {
                    surface->makeContextActive();
                }
                for (auto const& subImage : subImages) {
                    nvgDeleteImage(nvg, subImage.imageId);
                }
            }

            nvg = other.nvg;
            subImages = other.subImages;
            totalWidth = other.totalWidth;
            totalHeight = other.totalHeight;
            onImageInvalidate = other.onImageInvalidate;

            other.subImages.clear(); // Important, makes sure the old buffer can't delete this buffer
            allImages.insert(this);
        }

        return *this;
    }

    ~NVGImage()
    {
        deleteImage();
        allImages.erase(this);
    }

    static void clearAll(NVGcontext* nvg)
    {
        for (auto* image : allImages) {
            if (image->isValid() && image->nvg == nvg) {
                for (auto const& subImage : image->subImages) {
                    nvgDeleteImage(image->nvg, subImage.imageId);
                }
                image->subImages.clear();
                if (image->onImageInvalidate)
                    image->onImageInvalidate();
            }
        }
    }

    bool isValid() const
    {
        return subImages.not_empty();
    }

    void renderJUCEComponent(NVGcontext* nvg, Component& component, float const scale)
    {
        Image componentImage = component.createComponentSnapshot(Rectangle<int>(0, 0, component.getWidth(), component.getHeight()), false, scale);
        if (componentImage.isNull())
            return;

        loadJUCEImage(nvg, componentImage);
        render(nvg, { 0, 0, component.getWidth(), component.getHeight() });
    }
    
    
    void deleteImage()
    {
        if (subImages.size() && nvg) {
            for (auto const& subImage : subImages) {
                if (auto* surface = NVGSurface::getSurfaceForContext(nvg)) {
                    surface->makeContextActive();
                }

                nvgDeleteImage(nvg, subImage.imageId);
            }
            subImages.clear();
        }
    }

    void loadJUCEImage(NVGcontext* context, Image& image, int const repeatImage = false, int const withMipmaps = false)
    {
        totalWidth = image.getWidth();
        totalHeight = image.getHeight();
        nvg = context;

        static int maximumTextureSize = 0;
        if (!maximumTextureSize) {
            if (auto* ctx = NVGSurface::getSurfaceForContext(nvg)) {
                ctx->makeContextActive();
                nvgMaxTextureSize(maximumTextureSize);
            }
        }
        int const textureSizeLimit = maximumTextureSize == 0 ? 8192 : maximumTextureSize;

        // Most of the time, the image is small enough, so we optimise for that
        if (totalWidth <= textureSizeLimit && totalHeight <= textureSizeLimit) {
            Image::BitmapData const imageData(image, Image::BitmapData::readOnly);

            if (subImages.size() && subImages[0].bounds == image.getBounds() && nvg == context) {
                nvgUpdateImage(nvg, subImages[0].imageId, imageData.data);
                return;
            }

            SubImage subImage;
            auto flags = repeatImage ? NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY : 0;
            flags |= withMipmaps ? NVG_IMAGE_GENERATE_MIPMAPS : 0;

            if (image.isARGB())
                subImage.imageId = nvgCreateImageARGB(nvg, totalWidth, totalHeight, flags | NVG_IMAGE_PREMULTIPLIED, imageData.data);
            else if (image.isSingleChannel())
                subImage.imageId = nvgCreateImageAlpha(nvg, totalWidth, totalHeight, flags, imageData.data);

            deleteImage();

            subImage.bounds = image.getBounds();
            subImages.add(subImage);
            return;
        }

        deleteImage();

        int x = 0;
        while (x < totalWidth) {
            int y = 0;
            int const w = std::min(textureSizeLimit, totalWidth - x);
            while (y < totalHeight) {
                int const h = std::min(textureSizeLimit, totalHeight - y);
                auto bounds = Rectangle<int>(x, y, w, h);
                auto clip = image.getClippedImage(bounds);

                // We need to create copies to make sure the pixels are lined up :(
                // At least we only take this hit for very large images
                clip.duplicateIfShared();
                Image::BitmapData const imageData(clip, Image::BitmapData::readOnly);

                SubImage subImage;
                auto flags = repeatImage ? NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY : 0;
                flags |= withMipmaps ? NVG_IMAGE_GENERATE_MIPMAPS : 0;

                if (image.isARGB())
                    subImage.imageId = nvgCreateImageARGB(nvg, w, h, flags | NVG_IMAGE_PREMULTIPLIED, imageData.data);
                else if (image.isSingleChannel())
                    subImage.imageId = nvgCreateImageAlpha(nvg, w, h, flags, imageData.data);

                y += textureSizeLimit;
                subImage.bounds = bounds;
                subImages.add(subImage);
            }
            x += textureSizeLimit;
        }
    }

    void renderAlphaImage(NVGcontext* nvg, Rectangle<int> b, NVGcolor const col)
    {
        nvgSave(nvg);

        nvgScale(nvg, b.getWidth() / static_cast<float>(totalWidth), b.getHeight() / static_cast<float>(totalHeight));
        for (auto const& subImage : subImages) {
            auto scaledBounds = subImage.bounds;
            nvgFillPaint(nvg, nvgImageAlphaPattern(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight(), 0, subImage.imageId, col));

            nvgFillRect(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight());
        }
        nvgRestore(nvg);
    }

    void render(NVGcontext* nvg, Rectangle<int> b)
    {
        nvgSave(nvg);

        nvgScale(nvg, b.getWidth() / static_cast<float>(totalWidth), b.getHeight() / static_cast<float>(totalHeight));
        for (auto const& subImage : subImages) {
            auto scaledBounds = subImage.bounds;
            nvgFillPaint(nvg, nvgImagePattern(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight(), 0, subImage.imageId, 1.0f));

            nvgFillRect(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight());
        }
        nvgRestore(nvg);
    }

    bool needsUpdate(int const width, int const height) const
    {
        return subImages.empty() || width != totalWidth || height != totalHeight || isDirty;
    }

    int getImageId()
    {
        // This is only correct when we are absolutely sure that the size doesn't exceed maximum texture size
        assert(subImages.size() == 1);
        // TODO: handle multiple images (or get rid of this function)
        return subImages.size() ? subImages[0].imageId : 0;
    }

    void setDirty()
    {
        isDirty = true;
    }

    struct SubImage {
        int imageId = 0;
        Rectangle<int> bounds;
    };

    NVGcontext* nvg = nullptr;
    SmallArray<SubImage> subImages;
    int totalWidth = 0, totalHeight = 0;
    bool isDirty = false;

    std::function<void()> onImageInvalidate = nullptr;

    static inline UnorderedSet<NVGImage*> allImages = UnorderedSet<NVGImage*>();
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

    bool needsUpdate(int const width, int const height) const
    {
        return fb == nullptr || width != fbWidth || height != fbHeight || fbDirty;
    }

    bool isValid() const
    {
        return fb != nullptr;
    }

    void setDirty()
    {
        fbDirty = true;
    }

    void bind(NVGcontext* ctx, int const width, int const height)
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

    static void unbind()
    {
        nvgBindFramebuffer(nullptr);
    }

    void renderToFramebuffer(NVGcontext* nvg, int const width, int const height, std::function<void(NVGcontext*)> renderCallback)
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

    int getImage() const
    {
        if (!fb)
            return -1;

        return fb->image;
    }

private:
    static inline UnorderedSet<NVGFramebuffer*> allFramebuffers;

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

    static void resetAll()
    {
        for (auto* buffer : allCachedPaths) {
            buffer->clear();
        }
    }

    void clear()
    {
        if (cacheId != -1) {
            nvgDeletePath(nvg, cacheId);
            cacheId = -1;
            nvg = nullptr;
        }
    }

    bool isValid() const
    {
        return cacheId != -1;
    }

    void save(NVGcontext* ctx)
    {
        if (nvg == ctx && cacheId != -1)
            nvgDeletePath(nvg, cacheId);
        nvg = ctx;
        cacheId = nvgSavePath(nvg, cacheId);
    }

    bool stroke()
    {
        if (!nvg || cacheId == -1)
            return false;
        return nvgStrokeCachedPath(nvg, cacheId);
    }

    bool fill()
    {
        if (!nvg || cacheId == -1)
            return false;
        return nvgFillCachedPath(nvg, cacheId);
    }

private:
    static inline UnorderedSet<NVGCachedPath*> allCachedPaths;
    NVGcontext* nvg = nullptr;
    int cacheId = -1;
};

struct NVGScopedState {
    NVGScopedState(NVGcontext* nvg)
        : nvg(nvg)
    {
        nvgSave(nvg);
    }

    ~NVGScopedState()
    {
        nvgRestore(nvg);
    }

    NVGcontext* nvg;
};
