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

    nanovg::nvgBeginPath(nvg);

    while (i.next()) {
        switch (i.elementType) {
        case Path::Iterator::startNewSubPath:
            nanovg::nvgMoveTo(nvg, i.x1, i.y1);
            break;
        case Path::Iterator::lineTo:
            nanovg::nvgLineTo(nvg, i.x1, i.y1);
            break;
        case Path::Iterator::quadraticTo:
            nanovg::nvgQuadTo(nvg, i.x1, i.y1, i.x2, i.y2);
            break;
        case Path::Iterator::cubicTo:
            nanovg::nvgBezierTo(nvg, i.x1, i.y1, i.x2, i.y2, i.x3, i.y3);
            break;
        case Path::Iterator::closePath:
            nanovg::nvgClosePath(nvg);
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
            for (auto const& subImage : subImages) {
                nanovg::nvgDeleteImage(nvg, subImage.imageId);
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
                nanovg::nvgDeleteImage(image->nvg, subImage.imageId);
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
    nanovg::nvgSave(nvg);
    nanovg::nvgScale(nvg, 1.0f / scale, 1.0f / scale);

    Point<float> offset;
    // TODO: fix this
    //nanovg::nvgTransformGetSubpixelOffset(nvg, &offset.x, &offset.y);

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
    nanovg::nvgRestore(nvg);
}

void NVGImage::deleteImage()
{
    if (subImages.size() && nvg) {
        for (auto const& subImage : subImages) {
            nanovg::nvgDeleteImage(nvg, subImage.imageId);
        }
        subImages.clear();
    }
}

void NVGImage::loadJUCEImage(NVGcontext* context, Image const& image, int const repeatImage, int const withMipmaps)
{
    totalWidth = image.getWidth();
    totalHeight = image.getHeight();
    nvg = context;

    constexpr int textureSizeLimit = 8192;

    // Most of the time, the image is small enough, so we optimise for that
    if (totalWidth <= textureSizeLimit && totalHeight <= textureSizeLimit) {
        Image::BitmapData const imageData(image, Image::BitmapData::readOnly);

        if (subImages.size() && subImages[0].bounds == image.getBounds() && nvg == context) {
            nanovg::nvgUpdateImage(nvg, subImages[0].imageId, imageData.data);
            return;
        }

        SubImage subImage;
        auto flags = repeatImage ? NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY : 0;
        flags |= withMipmaps ? NVG_IMAGE_GENERATE_MIPMAPS : 0;

        if (image.isARGB())
            subImage.imageId = nanovg::nvgCreateImageARGB(nvg, totalWidth, totalHeight, flags, imageData.data);
        else if (image.isSingleChannel())
            subImage.imageId = nanovg::nvgCreateImageAlpha(nvg, totalWidth, totalHeight, flags, imageData.data);

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
                subImage.imageId = nanovg::nvgCreateImageARGB(nvg, w, h, flags, imageData.data);
            else if (image.isSingleChannel())
                subImage.imageId = nanovg::nvgCreateImageAlpha(nvg, w, h, flags, imageData.data);

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
    nanovg::nvgSave(nvg);

    nanovg::nvgScale(nvg, b.getWidth() / static_cast<float>(totalWidth), b.getHeight() / static_cast<float>(totalHeight));
    for (auto const& subImage : subImages) {
        auto scaledBounds = subImage.bounds;
        nanovg::nvgFillPaint(nvg, nanovg::nvgImageAlphaPattern(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight(), 0, subImage.imageId, col));

        nanovg::nvgFillRect(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight());
    }
    nanovg::nvgRestore(nvg);
}

void NVGImage::render(NVGcontext* nvg, Rectangle<int> const b, bool const quantize)
{
    nanovg::nvgSave(nvg);

    float const scaleW = b.getWidth() / static_cast<float>(totalWidth);
    float const scaleH = b.getHeight() / static_cast<float>(totalHeight);
    nanovg::nvgScale(nvg, scaleW, scaleH);
    if (quantize) {
        // Make sure image pixel grid aligns with physical pixels
        nanovg::nvgTransformQuantize(nvg);
    }
    for (auto const& subImage : subImages) {
        auto scaledBounds = subImage.bounds;
        nanovg::nvgFillPaint(nvg, nanovg::nvgImagePattern(nvg, b.getX() + scaledBounds.getX(), b.getY() + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight(), 0, subImage.imageId, 1.0f));

        nanovg::nvgFillRect(nvg, b.getX() / scaleW + scaledBounds.getX(), b.getY() / scaleH + scaledBounds.getY(), scaledBounds.getWidth(), scaledBounds.getHeight());
    }
    nanovg::nvgRestore(nvg);
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
        nanovg::deleteFramebuffer(nvg, fb);
        fb = nullptr;
        fbImage = -1;
    }
    allFramebuffers.erase(this);
}

void NVGFramebuffer::clearAll(NVGcontext const* nvg)
{
    for (auto* buffer : allFramebuffers) {
        if (buffer->nvg == nvg && buffer->fb) {
            nanovg::deleteFramebuffer(buffer->nvg, buffer->fb);
            buffer->fb = nullptr;
            buffer->fbImage = -1;
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
            nanovg::deleteFramebuffer(nvg, fb);
        fb = nanovg::createFramebuffer(nvg, width, height, 0);
        fbImage = nanovg::framebufferImage(nvg, fb);
        fbWidth = width;
        fbHeight = height;
    }

    nanovg::bindFramebuffer(nvg, fb);
}

void NVGFramebuffer::unbind(NVGcontext* nvg)
{
    nanovg::bindFramebuffer(nvg, nullptr);
}

void NVGFramebuffer::renderToFramebuffer(NVGcontext* nvg, int const width, int const height, std::function<void(NVGcontext*)> renderCallback)
{
    bind(nvg, width, height);
    renderCallback(nvg);
    unbind(nvg);
    fbDirty = false;
}

void NVGFramebuffer::render(NVGcontext* nvg, Rectangle<int> const b)
{
    if (fb && fbImage != -1) {
        nanovg::nvgFillPaint(nvg, nanovg::nvgImagePattern(nvg, 0, 0, b.getWidth(), b.getHeight(), 0, fbImage, 1));
        nanovg::nvgFillRect(nvg, b.getX(), b.getY(), b.getWidth(), b.getHeight());
    }
}

int NVGFramebuffer::getImage() const
{
    if (!fb)
        return -1;

    return fbImage;
}

NVGCachedPath::NVGCachedPath()
{
    allCachedPaths.insert(this);
}

NVGCachedPath::~NVGCachedPath()
{
    if (cacheId != -1) {
        nanovg::nvgDeletePath(nvg, cacheId);
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
        nanovg::nvgDeletePath(nvg, cacheId);
        cacheId = -1;
        nvg = nullptr;
    }
}

void NVGCachedPath::clearWithoutDelete()
{
    if (cacheId != -1) {
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
        nanovg::nvgDeletePath(nvg, cacheId);
    nvg = ctx;
    cacheId = nanovg::nvgSavePath(nvg, cacheId);
}

bool NVGCachedPath::stroke()
{
    if (!nvg || cacheId == -1)
        return false;
    return nanovg::nvgStrokeCachedPath(nvg, cacheId);
}

bool NVGCachedPath::fill()
{
    if (!nvg || cacheId == -1)
        return false;
    return nanovg::nvgFillCachedPath(nvg, cacheId);
}

NVGScopedState::NVGScopedState(NVGcontext* nvg)
    : nvg(nvg)
{
    nanovg::nvgSave(nvg);
}

NVGScopedState::~NVGScopedState()
{
    nanovg::nvgRestore(nvg);
}
