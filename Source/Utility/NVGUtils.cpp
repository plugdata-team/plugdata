/*
 // Copyright (c) 2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include "Config.h"

#include "NVGUtils.h"
#include "NVGSurface.h"

NVGComponent::NVGComponent(Component* comp)
    : component(*comp)
{
}

NVGComponent::~NVGComponent() { }

void NVGComponent::setJUCEPath(NVGcontext* nvg, Path const& p)
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

void NVGComponent::updateFramebuffers(NVGcontext*)
{
}

void NVGComponent::render(NVGcontext*)
{
}

NVGImage::NVGImage(NVGcontext* nvg, int width, int height, std::function<void(Graphics&)> renderCall, int const imageFlags, Colour const clearColour)
{
    bool const clearImage = !(imageFlags & NVGImageFlags::DontClear);
    bool const repeatImage = imageFlags & NVGImageFlags::RepeatImage;
    bool const withMipmaps = imageFlags & NVGImageFlags::MipMap;

    // When JUCE image format is SingleChannel the graphics context will render only the alpha component
    // into the image data, it is not a greyscale image of the graphics context.
    auto const imageFormat = imageFlags & NVGImageFlags::AlphaImage ? Image::SingleChannel : Image::ARGB;

    auto image = Image(imageFormat, width, height, false, SoftwareImageType());
    if (clearImage)
        image.clear({ 0, 0, width, height }, clearColour);
    Graphics g(image); // Render resize handles with JUCE, since rounded rect exclusion is hard with nanovg
    renderCall(g);
    loadJUCEImage(nvg, image, repeatImage, withMipmaps);
    allImages.insert(this);
}

NVGImage::NVGImage()
{
    allImages.insert(this);
}

NVGImage::NVGImage(NVGImage& other)
{
    // Check for self-assignment
    if (this != &other) {
        nvg = other.nvg;
        subImages = other.subImages;
        totalWidth = other.totalWidth;
        totalHeight = other.totalHeight;
        onImageInvalidate = other.onImageInvalidate;
        isDirty = false;

        other.subImages.clear();
        allImages.insert(this);
    }
}

NVGImage& NVGImage::operator=(NVGImage&& other) noexcept
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
        isDirty = false;

        other.subImages.clear(); // Important, makes sure the old buffer can't delete this buffer
        allImages.insert(this);
    }

    return *this;
}

NVGImage::~NVGImage()
{
    deleteImage();
    allImages.erase(this);
}

void NVGImage::clearAll(NVGcontext const* nvg)
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

bool NVGImage::isValid() const
{
    return subImages.not_empty();
}

void NVGImage::renderJUCEComponent(NVGcontext* nvg, Component& component, float const scale)
{
    nvgSave(nvg);
    nvgScale(nvg, 1.0f / scale, 1.0f / scale);

    Point<float> offset;
    nvgTransformGetSubpixelOffset(nvg, &offset.x, &offset.y);

    auto w = roundToInt(scale * static_cast<float>(component.getWidth()));
    auto h = roundToInt(scale * static_cast<float>(component.getHeight()));

    if (w > 0 && h > 0) {
        Image componentImage(component.isOpaque() ? Image::RGB : Image::ARGB, w, h, true);
        {
            Graphics g(componentImage);
            g.addTransform(AffineTransform::translation(offset.x, offset.y));
            g.addTransform(AffineTransform::scale(scale, scale));
            component.paintEntireComponent(g, true);
        }

        loadJUCEImage(nvg, componentImage);

        render(nvg, { 0, 0, w, h }, true);
    }
    nvgRestore(nvg);
}

void NVGImage::deleteImage()
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

void NVGImage::loadJUCEImage(NVGcontext* context, Image const& image, int const repeatImage, int const withMipmaps)
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
            subImage.imageId = nvgCreateImageARGB_sRGB(nvg, totalWidth, totalHeight, flags, imageData.data);
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
                subImage.imageId = nvgCreateImageARGB_sRGB(nvg, w, h, flags, imageData.data);
            else if (image.isSingleChannel())
                subImage.imageId = nvgCreateImageAlpha(nvg, w, h, flags, imageData.data);

            y += textureSizeLimit;
            subImage.bounds = bounds;
            subImages.add(subImage);
        }
        x += textureSizeLimit;
    }
    isDirty = false;
}

void NVGImage::renderAlphaImage(NVGcontext* nvg, Rectangle<int> const b, NVGcolor const col)
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

void NVGImage::render(NVGcontext* nvg, Rectangle<int> const b, bool const quantize)
{
    nvgSave(nvg);

    float const scaleW = b.getWidth() / static_cast<float>(totalWidth);
    float const scaleH = b.getHeight() / static_cast<float>(totalHeight);
    nvgScale(nvg, scaleW, scaleH);
    if (quantize) {
        // Make sure image pixel grid aligns with physical pixels
        nvgTransformQuantize(nvg);
    }
    for (auto const& subImage : subImages) {
        auto scaledBounds = subImage.bounds;
        nvgFillPaint(nvg, nvgImagePattern(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight(), 0, subImage.imageId, 1.0f));

        nvgFillRect(nvg, b.getX() / scaleW + scaledBounds.getX(), b.getY() / scaleH + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight());
    }
    nvgRestore(nvg);
}

bool NVGImage::needsUpdate(int const width, int const height) const
{
    return subImages.empty() || width != totalWidth || height != totalHeight || isDirty;
}

int NVGImage::getImageId()
{
    // This is only correct when we are absolutely sure that the size doesn't exceed maximum texture size
    assert(subImages.size() == 1);
    // TODO: handle multiple images (or get rid of this function)
    return subImages.size() ? subImages[0].imageId : 0;
}

void NVGImage::setDirty()
{
    isDirty = true;
}

NVGFramebuffer::NVGFramebuffer()
{
    allFramebuffers.insert(this);
}

NVGFramebuffer::~NVGFramebuffer()
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

void NVGFramebuffer::clearAll(NVGcontext const* nvg)
{
    for (auto* buffer : allFramebuffers) {
        if (buffer->nvg == nvg && buffer->fb) {
            nvgDeleteFramebuffer(buffer->fb);
            buffer->fb = nullptr;
        }
    }
}

bool NVGFramebuffer::needsUpdate(int const width, int const height) const
{
    return fb == nullptr || width != fbWidth || height != fbHeight || fbDirty;
}

bool NVGFramebuffer::isValid() const
{
    return fb != nullptr;
}

void NVGFramebuffer::setDirty()
{
    fbDirty = true;
}

void NVGFramebuffer::bind(NVGcontext* ctx, int const width, int const height)
{
    if (!fb || fbWidth != width || fbHeight != height) {
        nvg = ctx;
        if (fb)
            nvgDeleteFramebuffer(fb);
        fb = nvgCreateFramebuffer(nvg, width, height, 0);
        fbWidth = width;
        fbHeight = height;
    }

    nvgBindFramebuffer(fb);
}

void NVGFramebuffer::unbind()
{
    nvgBindFramebuffer(nullptr);
}

void NVGFramebuffer::renderToFramebuffer(NVGcontext* nvg, int const width, int const height, std::function<void(NVGcontext*)> renderCallback)
{
    bind(nvg, width, height);
    renderCallback(nvg);
    unbind();
    fbDirty = false;
}

void NVGFramebuffer::render(NVGcontext* nvg, Rectangle<int> const b)
{
    if (fb) {
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, b.getWidth(), b.getHeight(), 0, fb->image, 1));
        nvgFillRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
    }
}

int NVGFramebuffer::getImage() const
{
    if (!fb)
        return -1;

    return fb->image;
}

NVGCachedPath::NVGCachedPath()
{
    allCachedPaths.insert(this);
}

NVGCachedPath::~NVGCachedPath()
{
    if (cacheId != -1) {
        nvgDeletePath(nvg, cacheId);
        cacheId = -1;
    }
    allCachedPaths.erase(this);
}

void NVGCachedPath::clearAll(NVGcontext const* nvg)
{
    for (auto* buffer : allCachedPaths) {
        if (buffer->nvg == nvg) {
            buffer->clear();
        }
    }
}

void NVGCachedPath::resetAll()
{
    for (auto* buffer : allCachedPaths) {
        buffer->clear();
    }
}

void NVGCachedPath::clear()
{
    if (cacheId != -1) {
        nvgDeletePath(nvg, cacheId);
        cacheId = -1;
        nvg = nullptr;
    }
}

bool NVGCachedPath::isValid() const
{
    return cacheId != -1;
}

void NVGCachedPath::save(NVGcontext* ctx)
{
    if (nvg == ctx && cacheId != -1)
        nvgDeletePath(nvg, cacheId);
    nvg = ctx;
    cacheId = nvgSavePath(nvg, cacheId);
}

bool NVGCachedPath::stroke()
{
    if (!nvg || cacheId == -1)
        return false;
    return nvgStrokeCachedPath(nvg, cacheId);
}

bool NVGCachedPath::fill()
{
    if (!nvg || cacheId == -1)
        return false;
    return nvgFillCachedPath(nvg, cacheId);
}

NVGScopedState::NVGScopedState(NVGcontext* nvg)
    : nvg(nvg)
{
    nvgSave(nvg);
}

NVGScopedState::~NVGScopedState()
{
    nvgRestore(nvg);
}
