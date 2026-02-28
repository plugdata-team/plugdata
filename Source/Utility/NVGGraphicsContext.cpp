//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NVGGraphicsContext.h"
#include <BinaryData.h>

#if PERFETTO
#    include <melatonin_perfetto/melatonin_perfetto.h>
#endif

static constexpr int maxImageCacheSize = 256;

static NVGcolor nvgColour(Colour const& c)
{
    return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}

static uint64_t getImageHash(Image const& image)
{
    Image::BitmapData src(image, Image::BitmapData::readOnly);
    return reinterpret_cast<uint64_t>(src.data);
}

//==============================================================================

int const NVGGraphicsContext::imageCacheSize = 256;

//==============================================================================

NVGGraphicsContext::NVGGraphicsContext(NVGcontext* nativeHandle)
    : nvg(nativeHandle)
{
    jassert(nvg);
}

NVGGraphicsContext::~NVGGraphicsContext()
{
    for(auto& [hash, path] : pathCache)
    {
        path.clearWithoutDelete();
    }
}

bool NVGGraphicsContext::isVectorDevice() const { return false; }

void NVGGraphicsContext::setOrigin(Point<int> const origin)
{
    nvgTranslate(nvg, origin.getX(), origin.getY());
}

void NVGGraphicsContext::addTransform(AffineTransform const& t)
{
    nvgTransform(nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
}

float NVGGraphicsContext::getPhysicalPixelScaleFactor() const { return scale; }

void NVGGraphicsContext::setPhysicalPixelScaleFactor(float const newScale) { scale = newScale; }

bool NVGGraphicsContext::clipToRectangle(Rectangle<int> const& rect)
{
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

bool NVGGraphicsContext::clipToRectangleList(RectangleList<int> const& rects)
{
    auto const rect = rects.getBounds();
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

void NVGGraphicsContext::excludeClipRectangle(Rectangle<int> const& rectangle)
{
    RectangleList<int> rectangles;
    rectangles.add(getClipBounds());
    rectangles.subtract(rectangle);

    clipToRectangleList(rectangles);
}

void NVGGraphicsContext::clipToPath(Path const& path, AffineTransform const& t)
{
    auto const rect = path.getBoundsTransformed(t);
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void NVGGraphicsContext::clipToImageAlpha(Image const& sourceImage, AffineTransform const& transform)
{
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
        auto const image = nvgCreateImageARGB_sRGB(nvg, width, height, 0, pixelData);
        auto const paint = nvgImagePattern(nvg, 0, 0, width, height, 0, image, 1);

        nvgSave(nvg);
        nvgTransform(nvg, transform.mat00, transform.mat10, transform.mat01, transform.mat11, transform.mat02, transform.mat12);
        nvgScale(nvg, 1.0f, -1.0f);

        // Clip the graphics context to the alpha mask of the Nanovg image
        nvgBeginPath(nvg);
        nvgRect(nvg, 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        nvgPathWinding(nvg, NVG_HOLE);
        nvgFillPaint(nvg, paint);
        nvgFill(nvg);

        // Restore the original transformations
        nvgRestore(nvg);
        nvgDeleteImage(nvg, image);
    }
}

bool NVGGraphicsContext::clipRegionIntersects(Rectangle<int> const& rect)
{
    auto const clip = getClipBounds();
    return clip.intersects(rect);
}

Rectangle<int> NVGGraphicsContext::getClipBounds() const
{
    // auto scissorBounds = nvgCurrentScissor(nvg); TODO: fix this!
    return Rectangle<int>(0, 0, 999999, 999999);
}

bool NVGGraphicsContext::isClipEmpty() const
{
    float x, y, w, h;
    nvgCurrentScissor(nvg, &x, &y, &w, &h);
    return w <= 0 || h <= 0;
}

void NVGGraphicsContext::saveState()
{
    nvgSave(nvg);
}

void NVGGraphicsContext::restoreState()
{
    nvgRestore(nvg);
}

void NVGGraphicsContext::beginTransparencyLayer(float const op)
{
    saveState();
    nvgGlobalAlpha(nvg, op);
}

void NVGGraphicsContext::endTransparencyLayer()
{
    restoreState();
}

void NVGGraphicsContext::setFill(FillType const& fillType)
{
    if (fillType.isColour()) {
        auto c = nvgColour(fillType.colour);
        nvgFillColor(nvg, c);
        nvgStrokeColor(nvg, c);
    } else if (fillType.isGradient()) {
        if (ColourGradient* gradient = fillType.gradient.get()) {
            auto const numColours = gradient->getNumColours();

            if (numColours == 1) {
                // Just a solid fill
                nvgFillColor(nvg, nvgColour(gradient->getColour(0)));
            } else if (numColours > 1) {
                NVGpaint p;

                if (gradient->isRadial) {
                    p = nvgRadialGradient(nvg,
                        gradient->point1.getX(), gradient->point1.getY(),
                        gradient->point2.getX(), gradient->point2.getY(),
                        nvgColour(gradient->getColour(0)), nvgColour(gradient->getColour(numColours - 1)));
                } else {
                    p = nvgLinearGradient(nvg,
                        gradient->point1.getX(), gradient->point1.getY(),
                        gradient->point2.getX(), gradient->point2.getY(),
                        nvgColour(gradient->getColour(0)), nvgColour(gradient->getColour(numColours - 1)));
                }

                nvgFillPaint(nvg, p);
            }
        }
    }
}

void NVGGraphicsContext::setOpacity(float op)
{
    /*
    auto c = nvgGetFillColor(nvg);
    nvgRGBAf(c.r, c.g, c.b, op);
    nvgFillColor(nvg, c);
    nvgStrokeColor(nvg, c); */
}

void NVGGraphicsContext::setInterpolationQuality(Graphics::ResamplingQuality)
{
    // @todo
}

void NVGGraphicsContext::fillRect(Rectangle<int> const& rect, bool /* replaceExistingContents */)
{
    nvgBeginPath(nvg);
    nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill(nvg);
}

void NVGGraphicsContext::fillRect(Rectangle<float> const& rect)
{
    nvgBeginPath(nvg);
    nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill(nvg);
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
        nvgLineCap(nvg, NVG_BUTT);
        break;
    case PathStrokeType::EndCapStyle::rounded:
        nvgLineCap(nvg, NVG_ROUND);
        break;
    case PathStrokeType::EndCapStyle::square:
        nvgLineCap(nvg, NVG_SQUARE);
        break;
    }

    switch (strokeType.getJointStyle()) {
    case PathStrokeType::JointStyle::mitered:
        nvgLineJoin(nvg, NVG_MITER);
        break;
    case PathStrokeType::JointStyle::curved:
        nvgLineJoin(nvg, NVG_ROUND);
        break;
    case PathStrokeType::JointStyle::beveled:
        nvgLineJoin(nvg, NVG_BEVEL);
        break;
    }

    nvgStrokeWidth(nvg, strokeType.getStrokeThickness());
    nvgPathWinding(nvg, NVG_SOLID);
    setPath(path, transform);
    nvgStroke(nvg);
}

void NVGGraphicsContext::setPath(Path const& path, AffineTransform const& transform)
{
    Path p(path);
    p.applyTransform(transform);

    nvgBeginPath(nvg);

    Path::Iterator i(p);

    while (i.next()) {
        switch (i.elementType) {
        case Path::Iterator::startNewSubPath:
            nvgMoveTo(nvg, i.x1, i.y1);
            nvgPathWinding(nvg, NVG_NONZERO);
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

void NVGGraphicsContext::fillPath(Path const& path, AffineTransform const& transform)
{
    setPath(path, transform);
    nvgFill(nvg);
}

void NVGGraphicsContext::drawImage(Image const& image, AffineTransform const& t)
{
    if (image.isARGB()) {
        Image::BitmapData srcData(image, Image::BitmapData::readOnly);
        auto const id = getNvgImageId(image);

        if (id < 0)
            return; // invalid image.

        Rectangle<float> const rect(0.0f, 0.0f, image.getWidth(), image.getHeight());

        // rect = rect.transformedBy (t);

        NVGpaint const imgPaint = nvgImagePattern(nvg,
            0, 0,
            rect.getWidth(), rect.getHeight(),
            0.0f, // angle
            id,
            1.0f // alpha
        );

        nvgSave(nvg);
        nvgTransform(nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
        nvgBeginPath(nvg);
        nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
        nvgFillPaint(nvg, imgPaint);
        nvgFill(nvg);
        nvgRestore(nvg);
    } else if (image.isRGB()) {

        auto argbImage = Image(Image::ARGB, image.getWidth(), image.getHeight(), true);

        // TODO: can this be done more efficiently?
        for (int y = 0; y < image.getHeight(); ++y) {
            for (int x = 0; x < image.getWidth(); ++x) {
                argbImage.setPixelAt(x, y, image.getPixelAt(x, y).withAlpha(1.0f));
            }
        }

        // Render using ARGB image data
        drawImage(argbImage, t);
    } else if (image.isSingleChannel()) {

        auto argbImage = Image(Image::ARGB, image.getWidth(), image.getHeight(), true);

        for (int y = 0; y < image.getHeight(); ++y) {
            for (int x = 0; x < image.getWidth(); ++x) {
                auto const alpha = image.getPixelAt(x, y).getAlpha();
                argbImage.setPixelAt(x, y, Colour::fromRGBA(0, 0, 0, alpha));
            }
        }

        // Render using ARGB image data
        drawImage(argbImage, t);
    }
}

void NVGGraphicsContext::drawLine(Line<float> const& line)
{
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, line.getStartX(), line.getStartY());
    nvgLineTo(nvg, line.getEndX(), line.getEndY());
    nvgStroke(nvg);
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
    for (auto const [i, glyph] : enumerate(glyphs, size_t {})) {
        auto const scale = font.getHeight();
        auto tx = AffineTransform::scale(scale * font.getHorizontalScale(), scale).translated(positions[i]).followedBy(t);

        nvgSave(nvg);
        nvgTransform(nvg, tx.mat00, tx.mat10, tx.mat01, tx.mat11, tx.mat02, tx.mat12);

        float xform[6];
        nvgCurrentTransform(nvg, xform);

        // NOTE: currently, path hashing assumes uniform and non-negative scaling. This is always true for plugdata
        uint64_t pathHash = (uint64_t)font.getTypefacePtr().get();
        pathHash ^= (uint64_t)glyph + 0x9e3779b97f4a7c15ULL + (pathHash << 6) + (pathHash >> 2);
        pathHash ^= (uint64_t)static_cast<int>(xform[0]) + 0x9e3779b97f4a7c15ULL + (pathHash << 6) + (pathHash >> 2);

        // Cache glyphs so that nanovg doesn't have to calculate nonzero winding and path tesselation every single time
        auto cacheHit = pathCache[pathHash].fill();
        if (!cacheHit) {
            Path p;
            auto f = getFont();
            f.getTypefacePtr()->getOutlineForGlyph(f.getMetricsKind(), glyph, p);

            setPath(p, AffineTransform());
            nvgFill(nvg);
            pathCache[pathHash].save(nvg);
        }
        nvgRestore(nvg);
    }
}

void NVGGraphicsContext::removeCachedImages()
{
    for (auto it = images.begin(); it != images.end(); ++it)
        nvgDeleteImage(nvg, it->second.id);

    images.clear();
}

int NVGGraphicsContext::getNvgImageId(Image const& image)
{
    int id = -1;
    auto const hash = getImageHash(image);
    auto const it = images.find(hash);

    if (it == images.end()) {
        // Nanovg expects images in RGBA format, so we do conversion here
        Image argbImage(image);
        argbImage.duplicateIfShared();

        argbImage = argbImage.convertedToFormat(Image::PixelFormat::ARGB);
        Image::BitmapData const bitmap(argbImage, Image::BitmapData::readOnly);

        id = nvgCreateImageARGB(nvg, argbImage.getWidth(), argbImage.getHeight(), 0, bitmap.data);

        if (images.size() >= maxImageCacheSize)
            reduceImageCache();

        images[hash] = { id, 1 };
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
            nvgDeleteImage(nvg, it->second.id);
            it = images.erase(it);
        } else {
            it->second.accessCounter -= minAccessCounter;
            ++it;
        }
    }
}
