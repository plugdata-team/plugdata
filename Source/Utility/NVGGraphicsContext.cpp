//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NVGGraphicsContext.h"
#include <bit>
#include <BinaryData.h>

#if PERFETTO
#    include <melatonin_perfetto/melatonin_perfetto.h>
#endif

static constexpr int maxImageCacheSize = 256;
static Rectangle<int> const maxClipBounds { 0, 0, 1'000'000, 1'000'000 };

static NVGcolor nvgColour(Colour const& c)
{
    return std::bit_cast<NVGcolor>(c);
}

static uint64_t getImageHash(Image const& image)
{
    Image::BitmapData src(image, Image::BitmapData::readOnly);
    return reinterpret_cast<uint64_t>(src.data);
}

static bool isIntegerTranslation(AffineTransform const& transform)
{
    if (!transform.isOnlyTranslation())
        return false;

    auto const x = transform.getTranslationX();
    auto const y = transform.getTranslationY();
    return approximatelyEqual(x, static_cast<float>(roundToInt(x)))
        && approximatelyEqual(y, static_cast<float>(roundToInt(y)));
}

static uint8 alphaToByte(float const alpha)
{
    return static_cast<uint8>(roundToInt(jlimit(0.0f, 1.0f, alpha) * 255.0f));
}

//==============================================================================

int const NVGGraphicsContext::imageCacheSize = 256;

//==============================================================================

NVGGraphicsContext::NVGGraphicsContext(NVGcontext* nativeHandle)
    : nvg(nativeHandle)
{
    jassert(nvg);
    resetClipRegion();
}

NVGGraphicsContext::~NVGGraphicsContext()
{
    for (auto& [hash, path] : pathCache) {
        path.clearWithoutDelete();
    }
}

bool NVGGraphicsContext::isVectorDevice() const { return false; }

void NVGGraphicsContext::setOrigin(Point<int> const origin)
{
    currentTransform = AffineTransform::translation(static_cast<float>(origin.getX()), static_cast<float>(origin.getY()))
                           .followedBy(currentTransform);
    nanovg::nvgTranslate(nvg, origin.getX(), origin.getY());
}

void NVGGraphicsContext::addTransform(AffineTransform const& t)
{
    currentTransform = t.followedBy(currentTransform);
    nanovg::nvgTransform(nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
}

float NVGGraphicsContext::getPhysicalPixelScaleFactor() const { return scale; }

void NVGGraphicsContext::setPhysicalPixelScaleFactor(float const newScale) { scale = newScale; }

bool NVGGraphicsContext::clipToRectangle(Rectangle<int> const& rect)
{
    clipRegion.clipTo(getTransformedClipBounds(rect.toFloat(), getCurrentTransform()));
    nanovg::nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !clipRegion.isEmpty();
}

bool NVGGraphicsContext::clipToRectangleList(RectangleList<int> const& rects)
{
    auto const rect = rects.getBounds();
    clipRegion.clipTo(getTransformedClipRegion(rects, getCurrentTransform()));
    nanovg::nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !clipRegion.isEmpty();
}

void NVGGraphicsContext::excludeClipRectangle(Rectangle<int> const& rectangle)
{
    clipRegion.subtract(getTransformedClipBounds(rectangle.toFloat(), getCurrentTransform()));

    auto const rect = getClipBounds();
    nanovg::nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void NVGGraphicsContext::clipToPath(Path const& path, AffineTransform const& t)
{
    auto const rect = path.getBoundsTransformed(t);
    clipRegion.clipTo(getTransformedClipBounds(rect, getCurrentTransform()));
    nanovg::nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void NVGGraphicsContext::clipToImageAlpha(Image const& sourceImage, AffineTransform const& transform)
{
    if (transform.isSingularity()) {
        clipRegion.clear();
        return;
    }

    auto const totalTransform = transform.followedBy(getCurrentTransform());
    clipRegion.clipTo(getTransformedClipBounds(sourceImage.getBounds().toFloat(), totalTransform));

    if (!transform.isSingularity()) {
        // Convert the image to a single-channel image if necessary
        Image singleChannelImage(sourceImage);
        if (sourceImage.getFormat() != Image::SingleChannel) {
            singleChannelImage = sourceImage.convertedToFormat(Image::SingleChannel);
        }

        Image::BitmapData const bitmapData(singleChannelImage, Image::BitmapData::readOnly);
        auto const* pixelData = bitmapData.data;

        // Create a new Nanovg image from the bitmap data
        int const width = singleChannelImage.getWidth();
        int const height = singleChannelImage.getHeight();
        auto const image = nanovg::nvgCreateImageARGB_sRGB(nvg, width, height, 0, pixelData);
        auto const paint = nanovg::nvgImagePattern(nvg, 0, 0, width, height, 0, image, 1);

        nanovg::nvgSave(nvg);
        nanovg::nvgTransform(nvg, transform.mat00, transform.mat10, transform.mat01, transform.mat11, transform.mat02, transform.mat12);
        nanovg::nvgScale(nvg, 1.0f, -1.0f);

        // Clip the graphics context to the alpha mask of the Nanovg image
        nanovg::nvgBeginPath(nvg);
        nanovg::nvgRect(nvg, 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        nanovg::nvgPathWinding(nvg, NVG_HOLE);
        nanovg::nvgFillPaint(nvg, paint);
        nanovg::nvgFill(nvg);

        // Restore the original transformations
        nanovg::nvgRestore(nvg);
        nanovg::nvgDeleteImage(nvg, image);
    }
}

bool NVGGraphicsContext::clipRegionIntersects(Rectangle<int> const& rect)
{
    if (clipRegion.isEmpty())
        return false;

    auto const transform = getCurrentTransform();

    if (transform.isSingularity())
        return false;

    if (isIntegerTranslation(transform)) {
        return clipRegion.intersectsRectangle(rect.translated(
            roundToInt(transform.getTranslationX()),
            roundToInt(transform.getTranslationY())));
    }

    return clipRegion.intersectsRectangle(getTransformedClipBounds(rect.toFloat(), transform));
}

Rectangle<int> NVGGraphicsContext::getClipBounds() const
{
    if (clipRegion.isEmpty())
        return {};

    auto const transform = getCurrentTransform();

    if (transform.isSingularity())
        return {};

    auto const bounds = clipRegion.getBounds();

    if (isIntegerTranslation(transform)) {
        return bounds.translated(
            -roundToInt(transform.getTranslationX()),
            -roundToInt(transform.getTranslationY()));
    }

    return bounds.toFloat().transformedBy(transform.inverted()).getSmallestIntegerContainer();
}

bool NVGGraphicsContext::isClipEmpty() const
{
    return clipRegion.isEmpty();
}

void NVGGraphicsContext::setImageBlendMode(BlendMode newMode)
{
    switch (newMode)
    {
        case BlendMode::sourceOver:
            nanovg::nvgGlobalCompositeOperation(nvg, NVG_SOURCE_OVER);
            break;

        case BlendMode::source:
            nanovg::nvgGlobalCompositeOperation(nvg, NVG_COPY);
            break;

        case BlendMode::destinationIn:
            nanovg::nvgGlobalCompositeOperation(nvg, NVG_DESTINATION_IN);
            break;

        case BlendMode::destinationOut:
            nanovg::nvgGlobalCompositeOperation(nvg, NVG_DESTINATION_OUT);
            break;
    }
}


void NVGGraphicsContext::saveState()
{
    stateStack.push_back({ clipRegion, currentTransform, opacity, lastColour });
    nanovg::nvgSave(nvg);
}

void NVGGraphicsContext::restoreState()
{
    nanovg::nvgRestore(nvg);

    if (!stateStack.empty()) {
        auto state = std::move(stateStack.back());
        stateStack.pop_back();
        clipRegion = std::move(state.clipRegion);
        currentTransform = state.transform;
        opacity = state.opacity;
        lastColour = state.lastColour;
    } else {
        jassertfalse;
        resetClipRegion();
    }
}

void NVGGraphicsContext::beginTransparencyLayer(float const op)
{
    saveState();
    nanovg::nvgGlobalAlpha(nvg, op);
}

void NVGGraphicsContext::endTransparencyLayer()
{
    restoreState();
}

void NVGGraphicsContext::setFill(FillType const& fillType)
{
    opacity = fillType.getOpacity();

    if (fillType.isColour()) {
        auto c = nvgColour(fillType.colour);
        nanovg::nvgFillColor(nvg, c);
        nanovg::nvgStrokeColor(nvg, c);
        lastColour = c;
    } else if (fillType.isGradient()) {
        if (ColourGradient* gradient = fillType.gradient.get()) {
            auto const numColours = gradient->getNumColours();

            if (numColours == 1) {
                // Just a solid fill
                auto c = nvgColour(gradient->getColour(0).withMultipliedAlpha(opacity));
                nanovg::nvgFillColor(nvg, c);
                nanovg::nvgStrokeColor(nvg, c);
                lastColour = c;
            } else if (numColours > 1) {
                NVGpaint p;
                auto const startColour = nvgColour(gradient->getColour(0).withMultipliedAlpha(opacity));
                auto const endColour = nvgColour(gradient->getColour(numColours - 1).withMultipliedAlpha(opacity));

                if (gradient->isRadial) {
                    p = nanovg::nvgRadialGradient(nvg,
                        gradient->point1.getX(), gradient->point1.getY(),
                        gradient->point2.getX(), gradient->point2.getY(),
                        startColour, endColour);
                } else {
                    p = nanovg::nvgLinearGradient(nvg,
                        gradient->point1.getX(), gradient->point1.getY(),
                        gradient->point2.getX(), gradient->point2.getY(),
                        startColour, endColour);
                }

                nanovg::nvgFillPaint(nvg, p);
            }
        }
    }
}

void NVGGraphicsContext::setOpacity(float op)
{
    opacity = jlimit(0.0f, 1.0f, op);
    auto c = lastColour;
    c.a = alphaToByte(opacity);
    nanovg::nvgFillColor(nvg, c);
    nanovg::nvgStrokeColor(nvg, c);
    lastColour = c;
}

void NVGGraphicsContext::setInterpolationQuality(Graphics::ResamplingQuality)
{
    // @todo
}

void NVGGraphicsContext::fillRect(Rectangle<int> const& rect, bool /* replaceExistingContents */)
{
    nanovg::nvgBeginPath(nvg);
    nanovg::nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nanovg::nvgFill(nvg);
}

void NVGGraphicsContext::fillRect(Rectangle<float> const& rect)
{
    nanovg::nvgBeginPath(nvg);
    nanovg::nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nanovg::nvgFill(nvg);
}

void NVGGraphicsContext::fillRectList(RectangleList<float> const& rects)
{
    for (auto const& rect : rects)
        fillRect(rect);
}

void NVGGraphicsContext::strokePath(Path const& path, PathStrokeType const& strokeType, AffineTransform const& transform)
{
    // First set options
    switch (strokeType.getEndStyle()) {
    case PathStrokeType::EndCapStyle::butt:
        nanovg::nvgLineCap(nvg, NVG_BUTT);
        break;
    case PathStrokeType::EndCapStyle::rounded:
        nanovg::nvgLineCap(nvg, NVG_ROUND);
        break;
    case PathStrokeType::EndCapStyle::square:
        nanovg::nvgLineCap(nvg, NVG_SQUARE);
        break;
    }

    switch (strokeType.getJointStyle()) {
    case PathStrokeType::JointStyle::mitered:
        nanovg::nvgLineJoin(nvg, NVG_MITER);
        break;
    case PathStrokeType::JointStyle::curved:
        nanovg::nvgLineJoin(nvg, NVG_ROUND);
        break;
    case PathStrokeType::JointStyle::beveled:
        nanovg::nvgLineJoin(nvg, NVG_BEVEL);
        break;
    }

    nanovg::nvgStrokeWidth(nvg, strokeType.getStrokeThickness());
    nanovg::nvgPathWinding(nvg, NVG_SOLID);
    setPath(path, transform);
    nanovg::nvgStroke(nvg);
}

void NVGGraphicsContext::setPath(Path const& path, AffineTransform const& transform)
{
    Path p(path);
    p.applyTransform(transform);

    nanovg::nvgBeginPath(nvg);

    Path::Iterator i(p);

    while (i.next()) {
        switch (i.elementType) {
        case Path::Iterator::startNewSubPath:
            nanovg::nvgMoveTo(nvg, i.x1, i.y1);
            nanovg::nvgPathWinding(nvg, NVG_NONZERO);
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

void NVGGraphicsContext::fillPath(Path const& path, AffineTransform const& transform)
{
    setPath(path, transform);
    nanovg::nvgFill(nvg);
}

void NVGGraphicsContext::drawImage(Image const& image, AffineTransform const& t)
{
    if (image.isARGB()) {
        Image::BitmapData srcData(image, Image::BitmapData::readOnly);

        auto const id = getNvgImageId(image);

        if (id < 0)
            return; // invalid image.

        Rectangle<float> const rect(0.0f, 0.0f, image.getWidth(), image.getHeight());

        NVGpaint const imgPaint = nanovg::nvgImagePattern(nvg, 0, 0, rect.getWidth(), rect.getHeight(), 0.0f, id, opacity);

        nanovg::nvgSave(nvg);
        nanovg::nvgTransform(nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
        nanovg::nvgBeginPath(nvg);
        nanovg::nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
        nanovg::nvgFillPaint(nvg, imgPaint);
        nanovg::nvgFill(nvg);
        nanovg::nvgRestore(nvg);
    } else if (image.isRGB()) {
        auto argbImage = Image(Image::ARGB, image.getWidth(), image.getHeight(), true);
        for (int y = 0; y < image.getHeight(); ++y) {
            for (int x = 0; x < image.getWidth(); ++x) {
                argbImage.setPixelAt(x, y, image.getPixelAt(x, y).withAlpha(1.0f));
            }
        }

        // Render using ARGB image data
        drawImage(argbImage, t);
    } else if (image.isSingleChannel()) {
        Image::BitmapData srcData(image, Image::BitmapData::readOnly);
        auto const id = getNvgImageId(image);
        if (id < 0)
            return; // invalid image.

        Rectangle<float> const rect(0.0f, 0.0f, image.getWidth(), image.getHeight());
        NVGpaint const imgPaint = nanovg::nvgImageAlphaPattern(nvg, 0, 0, rect.getWidth(), rect.getHeight(), 0.0f, id, lastColour);

        nanovg::nvgSave(nvg);
        nanovg::nvgTransform(nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
        nanovg::nvgBeginPath(nvg);
        nanovg::nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
        nanovg::nvgFillPaint(nvg, imgPaint);
        nanovg::nvgFill(nvg);
        nanovg::nvgRestore(nvg);
    }
}

void NVGGraphicsContext::drawLine(Line<float> const& line)
{
    nanovg::nvgBeginPath(nvg);
    nanovg::nvgMoveTo(nvg, line.getStartX(), line.getStartY());
    nanovg::nvgLineTo(nvg, line.getEndX(), line.getEndY());
    nanovg::nvgStroke(nvg);
}

void NVGGraphicsContext::setFont(Font const& f)
{
    font = f;
}

Font const& NVGGraphicsContext::getFont()
{
    return font;
}

void NVGGraphicsContext::drawGlyphs(Span<uint16_t const> glyphs, Span<Point<float> const> positions, AffineTransform const& t)
{
    for (auto const [i, glyph] : enumerate(glyphs, size_t { })) {
        auto const scale = font.getHeightInPoints();
        auto tx = AffineTransform::scale(scale * font.getHorizontalScale(), scale).translated(positions[i]).followedBy(t);

        nanovg::nvgSave(nvg);
        nanovg::nvgTransform(nvg, tx.mat00, tx.mat10, tx.mat01, tx.mat11, tx.mat02, tx.mat12);

        uint64_t pathHash = (uint64_t)font.getTypefacePtr().get();
        pathHash ^= (uint64_t)glyph + 0x9e3779b97f4a7c15ULL + (pathHash << 6) + (pathHash >> 2);

        // SDF text rendering: upload JUCE glyph paths into nanovg and render using SDF (much better than regular nanovg AA)
        if (!nanovg::nvgSDFGlyphCached(nvg, pathHash)) {
            constexpr float referenceEmPx = 32.0f;

            Path p;
            font.getTypefacePtr()->getOutlineForGlyph(glyph, p);

            nanovg::nvgSave(nvg);
            nanovg::nvgResetTransform(nvg);
            nanovg::nvgScale(nvg, referenceEmPx, referenceEmPx);
            setPath(p, AffineTransform());
            nanovg::nvgSaveSDFGlyph(nvg, pathHash);
            nanovg::nvgRestore(nvg);
        }

        // Draw the tile at the glyph transform. On the miss frame the tile was just generated
        // earlier in this same command buffer, so this still draws it.
        nanovg::nvgFillSDFGlyph(nvg, pathHash, lastColour);

        nanovg::nvgRestore(nvg);
    }
}

NVGGraphicsContext::ScopedAnchoredDraw::ScopedAnchoredDraw(NVGGraphicsContext& context, Rectangle<float> clipBounds)
    : ctx(context)
{
    // Push the tracked state (and nvgSave); restored in the destructor. This keeps the outer paint's
    // state stack balanced — we only add and remove our own frame.
    ctx.saveState();

    // Clip pixels to the requested bounds via the nvg scissor (in nvg's current local space), matching
    // the old image renderer. We use the nvg scissor rather than JUCE's clip so we can safely blank the
    // tracked clip below without losing the intended clipping.
    nanovg::nvgIntersectScissor(ctx.nvg, clipBounds.getX(), clipBounds.getY(), clipBounds.getWidth(), clipBounds.getHeight());

    // Neutralise the tracked transform/clip so getClipBounds() reports "everything": JUCE then culls
    // nothing, and glyphs land correctly because they are drawn relative to nvg's current matrix.
    ctx.currentTransform = AffineTransform();
    ctx.clipRegion.clear();
    ctx.clipRegion.add(maxClipBounds);
}

NVGGraphicsContext::ScopedAnchoredDraw::~ScopedAnchoredDraw()
{
    ctx.restoreState();
}

void NVGGraphicsContext::renderComponent(Component& component)
{
    ScopedAnchoredDraw anchor(*this, component.getLocalBounds().toFloat());
    Graphics g(*this);
    component.paintEntireComponent(g, true);
}

void NVGGraphicsContext::removeCachedImages()
{
    for (auto it = images.begin(); it != images.end(); ++it)
        nanovg::nvgDeleteImage(nvg, it->second.id);

    images.clear();
}

void NVGGraphicsContext::resetClipRegion(AffineTransform initialTransform)
{
    clipRegion.clear();
    clipRegion.add(maxClipBounds);
    stateStack.clear();
    currentTransform = initialTransform;
    opacity = 1.0f;
    lastColour = nanovg::nvgRGBA(0, 0, 0, 255);
}

AffineTransform NVGGraphicsContext::getCurrentTransform() const
{
    return currentTransform;
}

Rectangle<int> NVGGraphicsContext::getTransformedClipBounds(Rectangle<float> const& bounds, AffineTransform const& transform) const
{
    if (bounds.isEmpty() || transform.isSingularity())
        return {};

    if (isIntegerTranslation(transform)) {
        return bounds.getSmallestIntegerContainer().translated(
            roundToInt(transform.getTranslationX()),
            roundToInt(transform.getTranslationY()));
    }

    return bounds.transformedBy(transform).getSmallestIntegerContainer();
}

RectangleList<int> NVGGraphicsContext::getTransformedClipRegion(RectangleList<int> const& region, AffineTransform const& transform) const
{
    RectangleList<int> transformed;

    if (region.isEmpty() || transform.isSingularity())
        return transformed;

    if (isIntegerTranslation(transform)) {
        transformed = region;
        transformed.offsetAll(roundToInt(transform.getTranslationX()), roundToInt(transform.getTranslationY()));
        return transformed;
    }

    for (auto const& rect : region)
        transformed.add(getTransformedClipBounds(rect.toFloat(), transform));

    return transformed;
}

int NVGGraphicsContext::getNvgImageId(Image const& image)
{
    int id = -1;
    auto const hash = getImageHash(image);
    auto const it = images.find(hash);
    if (it == images.end()) {
        if (image.isSingleChannel()) {
            Image::BitmapData const bitmap(image, Image::BitmapData::readOnly);
            id = nanovg::nvgCreateImageAlpha(nvg, image.getWidth(), image.getHeight(), 0, bitmap.data);
            if (images.size() >= maxImageCacheSize)
                reduceImageCache();

            images[hash] = { id, 1 };
        } else {
            Image argbImage(image);
            argbImage.duplicateIfShared();

            argbImage = argbImage.convertedToFormat(Image::PixelFormat::ARGB);
            Image::BitmapData const bitmap(argbImage, Image::BitmapData::readOnly);

            id = nanovg::nvgCreateImageARGB(nvg, argbImage.getWidth(), argbImage.getHeight(), 0, bitmap.data);

            if (images.size() >= maxImageCacheSize)
                reduceImageCache();

            images[hash] = { id, 1 };
        }
    } else {
        it->second.accessCounter++;
        id = it->second.id;
    }

    return id;
}

void NVGGraphicsContext::reduceImageCache()
{
    int minAccessCounter = 0;

    for (auto it = images.begin(); it != images.end(); ++it) {
        minAccessCounter = minAccessCounter == 0 ? it->second.accessCounter
                                                 : jmin(minAccessCounter, it->second.accessCounter);
    }

    auto it = images.begin();

    while (it != images.end()) {
        if (it->second.accessCounter == minAccessCounter) {
            nanovg::nvgDeleteImage(nvg, it->second.id);
            it = images.erase(it);
        } else {
            it->second.accessCounter -= minAccessCounter;
            ++it;
        }
    }
}
