//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NVGGraphicsContext.h"
#include <BinaryData.h>

#if PERFETTO
#    include <melatonin_perfetto/melatonin_perfetto.h>
#endif

static constexpr int maxImageCacheSize = 256;

static NVGcolor nvgColour(juce::Colour const& c)
{
    return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}

static uint64_t getImageHash(juce::Image const& image)
{
    juce::Image::BitmapData src(image, juce::Image::BitmapData::readOnly);
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
}

bool NVGGraphicsContext::isVectorDevice() const { return false; }

void NVGGraphicsContext::setOrigin(juce::Point<int> const origin)
{
    nvgTranslate(nvg, origin.getX(), origin.getY());
}

void NVGGraphicsContext::addTransform(juce::AffineTransform const& t)
{
    nvgTransform(nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
}

float NVGGraphicsContext::getPhysicalPixelScaleFactor() const { return scale; }

void NVGGraphicsContext::setPhysicalPixelScaleFactor(float const newScale) { scale = newScale; }

bool NVGGraphicsContext::clipToRectangle(juce::Rectangle<int> const& rect)
{
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

bool NVGGraphicsContext::clipToRectangleList(juce::RectangleList<int> const& rects)
{
    auto const rect = rects.getBounds();
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

void NVGGraphicsContext::excludeClipRectangle(juce::Rectangle<int> const& rectangle)
{
    juce::RectangleList<int> rectangles;
    rectangles.add(getClipBounds());
    rectangles.subtract(rectangle);

    clipToRectangleList(rectangles);
}

void NVGGraphicsContext::clipToPath(juce::Path const& path, juce::AffineTransform const& t)
{
    auto const rect = path.getBoundsTransformed(t);
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void NVGGraphicsContext::clipToImageAlpha(juce::Image const& sourceImage, juce::AffineTransform const& transform)
{
    if (!transform.isSingularity()) {
        // Convert the image to a single-channel image if necessary
        juce::Image singleChannelImage(sourceImage);
        if (sourceImage.getFormat() != juce::Image::SingleChannel) {
            singleChannelImage = sourceImage.convertedToFormat(juce::Image::SingleChannel);
        }

        juce::Image::BitmapData const bitmapData(singleChannelImage, juce::Image::BitmapData::readOnly);
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

bool NVGGraphicsContext::clipRegionIntersects(juce::Rectangle<int> const& rect)
{
    auto const clip = getClipBounds();
    return clip.intersects(rect);
}

juce::Rectangle<int> NVGGraphicsContext::getClipBounds() const
{
    // auto scissorBounds = nvgCurrentScissor(nvg); TODO: fix this!
    return juce::Rectangle<int>(0, 0, 999999, 999999);
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

void NVGGraphicsContext::setFill(juce::FillType const& fillType)
{
    if (fillType.isColour()) {
        auto c = nvgColour(fillType.colour);
        nvgFillColor(nvg, c);
        nvgStrokeColor(nvg, c);
    } else if (fillType.isGradient()) {
        if (juce::ColourGradient* gradient = fillType.gradient.get()) {
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

void NVGGraphicsContext::setInterpolationQuality(juce::Graphics::ResamplingQuality)
{
    // @todo
}

void NVGGraphicsContext::fillRect(juce::Rectangle<int> const& rect, bool /* replaceExistingContents */)
{
    nvgBeginPath(nvg);
    nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill(nvg);
}

void NVGGraphicsContext::fillRect(juce::Rectangle<float> const& rect)
{
    nvgBeginPath(nvg);
    nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill(nvg);
}

void NVGGraphicsContext::fillRectList(juce::RectangleList<float> const& rects)
{
    for (auto const& rect : rects)
        fillRect(rect);
}

void NVGGraphicsContext::strokePath(juce::Path const& path, juce::PathStrokeType const& strokeType, juce::AffineTransform const& transform)
{
    // First set options
    switch (strokeType.getEndStyle()) {
    case juce::PathStrokeType::EndCapStyle::butt:
        nvgLineCap(nvg, NVG_BUTT);
        break;
    case juce::PathStrokeType::EndCapStyle::rounded:
        nvgLineCap(nvg, NVG_ROUND);
        break;
    case juce::PathStrokeType::EndCapStyle::square:
        nvgLineCap(nvg, NVG_SQUARE);
        break;
    }

    switch (strokeType.getJointStyle()) {
    case juce::PathStrokeType::JointStyle::mitered:
        nvgLineJoin(nvg, NVG_MITER);
        break;
    case juce::PathStrokeType::JointStyle::curved:
        nvgLineJoin(nvg, NVG_ROUND);
        break;
    case juce::PathStrokeType::JointStyle::beveled:
        nvgLineJoin(nvg, NVG_BEVEL);
        break;
    }

    nvgStrokeWidth(nvg, strokeType.getStrokeThickness());
    nvgPathWinding(nvg, NVG_SOLID);
    setPath(path, transform);
    nvgStroke(nvg);
}

void NVGGraphicsContext::setPath(juce::Path const& path, juce::AffineTransform const& transform)
{
    juce::Path p(path);
    p.applyTransform(transform);

    nvgBeginPath(nvg);

    juce::Path::Iterator i(p);

    while (i.next()) {
        switch (i.elementType) {
        case juce::Path::Iterator::startNewSubPath:
            nvgMoveTo(nvg, i.x1, i.y1);
            nvgPathWinding(nvg, NVG_NONZERO);
            break;
        case juce::Path::Iterator::lineTo:
            nvgLineTo(nvg, i.x1, i.y1);
            break;
        case juce::Path::Iterator::quadraticTo:
            nvgQuadTo(nvg, i.x1, i.y1, i.x2, i.y2);
            break;
        case juce::Path::Iterator::cubicTo:
            nvgBezierTo(nvg, i.x1, i.y1, i.x2, i.y2, i.x3, i.y3);
            break;
        case juce::Path::Iterator::closePath:
            nvgClosePath(nvg);
            break;
        default:
            break;
        }
    }
}

void NVGGraphicsContext::fillPath(juce::Path const& path, juce::AffineTransform const& transform)
{
    setPath(path, transform);
    nvgFill(nvg);
}

void NVGGraphicsContext::drawImage(juce::Image const& image, juce::AffineTransform const& t)
{
    if (image.isARGB()) {
        juce::Image::BitmapData srcData(image, juce::Image::BitmapData::readOnly);
        auto const id = getNvgImageId(image);

        if (id < 0)
            return; // invalid image.

        juce::Rectangle<float> const rect(0.0f, 0.0f, image.getWidth(), image.getHeight());

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

        auto argbImage = juce::Image(juce::Image::ARGB, image.getWidth(), image.getHeight(), true);

        // TODO: can this be done more efficiently?
        for (int y = 0; y < image.getHeight(); ++y) {
            for (int x = 0; x < image.getWidth(); ++x) {
                argbImage.setPixelAt(x, y, image.getPixelAt(x, y).withAlpha(1.0f));
            }
        }

        // Render using ARGB image data
        drawImage(argbImage, t);
    } else if (image.isSingleChannel()) {

        auto argbImage = juce::Image(juce::Image::ARGB, image.getWidth(), image.getHeight(), true);

        for (int y = 0; y < image.getHeight(); ++y) {
            for (int x = 0; x < image.getWidth(); ++x) {
                auto const alpha = image.getPixelAt(x, y).getAlpha();
                argbImage.setPixelAt(x, y, juce::Colour::fromRGBA(0, 0, 0, alpha));
            }
        }

        // Render using ARGB image data
        drawImage(argbImage, t);
    }
}

void NVGGraphicsContext::drawLine(juce::Line<float> const& line)
{
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, line.getStartX(), line.getStartY());
    nvgLineTo(nvg, line.getEndX(), line.getEndY());
    nvgStroke(nvg);
}

void NVGGraphicsContext::setFont(juce::Font const& f)
{
    font = f;
}

juce::Font const& NVGGraphicsContext::getFont()
{
    return font;
}

void NVGGraphicsContext::drawGlyphs (juce::Span<const uint16_t> glyphs, juce::Span<const juce::Point<float>> positions, const juce::AffineTransform& t)
{
    for(int i = 0; i < glyphs.size(); i++)
    {
        juce::Path p;
        auto f = getFont();
        f.getTypefacePtr()->getOutlineForGlyph (f.getMetricsKind(), glyphs[i], p);
        setPath(p,t.scaled(f.getHeight() * f.getHorizontalScale(), f.getHeight()).translated(positions[i].x, positions[i].y + 1.5f));
        nvgFill(nvg);
    }
}

void NVGGraphicsContext::removeCachedImages()
{
    for (auto it = images.begin(); it != images.end(); ++it)
        nvgDeleteImage(nvg, it->second.id);

    images.clear();
}

int NVGGraphicsContext::getNvgImageId(juce::Image const& image)
{
    int id = -1;
    auto const hash = getImageHash(image);
    auto const it = images.find(hash);

    if (it == images.end()) {
        // Nanovg expects images in RGBA format, so we do conversion here
        juce::Image argbImage(image);
        argbImage.duplicateIfShared();

        argbImage = argbImage.convertedToFormat(juce::Image::PixelFormat::ARGB);
        juce::Image::BitmapData const bitmap(argbImage, juce::Image::BitmapData::readOnly);
        
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
                                                 : juce::jmin(minAccessCounter, it->second.accessCounter);
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
