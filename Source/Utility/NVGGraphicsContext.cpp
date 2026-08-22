//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NVGGraphicsContext.h"
#include <bit>
#include <BinaryData.h>
#include <cstring>
#include <memory>

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
    Image::BitmapData const src(image, Image::BitmapData::readOnly);

    uint64_t hash = 14695981039346656037ULL;
    auto hashValue = [&hash](uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            hash ^= static_cast<uint8>(value >> (i * 8));
            hash *= 1099511628211ULL;
        }
    };
    auto hashBytes = [&hash](uint8 const* data, size_t const numBytes) {
        for (size_t i = 0; i < numBytes; ++i) {
            hash ^= data[i];
            hash *= 1099511628211ULL;
        }
    };

    hashValue(static_cast<uint64_t>(src.pixelFormat));
    hashValue(static_cast<uint64_t>(src.width));
    hashValue(static_cast<uint64_t>(src.height));
    hashValue(static_cast<uint64_t>(src.pixelStride));

    auto const bytesPerRow = static_cast<size_t>(src.width) * static_cast<size_t>(src.pixelStride);
    for (int y = 0; y < src.height; ++y)
        hashBytes(src.getLinePointer(y), bytesPerRow);

    return hash;
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

enum class DeferredTextMode : uint8_t {
    Baseline,
    Rectangle,
};

struct PreparedGlyph {
    int glyph = 0;
    Point<float> position;
};

struct PreparedText {
    Font font { FontOptions() };
    Typeface::Ptr typeface;
    std::vector<PreparedGlyph> glyphs;
};

struct DeferredTextPayload {
    Font font { FontOptions() };
    String text;
    int justificationFlags = Justification::left;
    Rectangle<int> bounds;
    DeferredTextMode mode = DeferredTextMode::Baseline;
    std::unique_ptr<PreparedText> prepared;
};

static void setRawNvgPath(NVGcontext* nvg, Path path, AffineTransform const& transform = {})
{
    path.applyTransform(transform);

    ::nvgBeginPath(nvg);

    Path::Iterator i(path);

    while (i.next()) {
        switch (i.elementType) {
        case Path::Iterator::startNewSubPath:
            ::nvgMoveTo(nvg, i.x1, i.y1);
            ::nvgPathWinding(nvg, NVG_NONZERO);
            break;
        case Path::Iterator::lineTo:
            ::nvgLineTo(nvg, i.x1, i.y1);
            break;
        case Path::Iterator::quadraticTo:
            ::nvgQuadTo(nvg, i.x1, i.y1, i.x2, i.y2);
            break;
        case Path::Iterator::cubicTo:
            ::nvgBezierTo(nvg, i.x1, i.y1, i.x2, i.y2, i.x3, i.y3);
            break;
        case Path::Iterator::closePath:
            ::nvgClosePath(nvg);
            break;
        default:
            break;
        }
    }
}

static uint64_t getSDFGlyphHash(Typeface const* typeface, int glyph)
{
    auto pathHash = reinterpret_cast<uint64_t>(typeface);
    pathHash ^= static_cast<uint64_t>(glyph) + 0x9e3779b97f4a7c15ULL + (pathHash << 6) + (pathHash >> 2);
    return pathHash;
}

// Function to prepare text layouts on the render thread, to reduce text layouting load on the message thread
static std::unique_ptr<PreparedText> prepareDeferredText(DeferredTextPayload const& payload)
{
    auto prepared = std::make_unique<PreparedText>();
    prepared->font = payload.font;
    prepared->typeface = prepared->font.getTypefacePtr();

    if (prepared->typeface == nullptr)
        return {};

    GlyphArrangement arrangement;
    auto const bounds = payload.bounds.toFloat();

    if (payload.mode == DeferredTextMode::Rectangle) {
        arrangement.addCurtailedLineOfText(prepared->font, payload.text, 0.0f, 0.0f, bounds.getWidth(), false);
        arrangement.justifyGlyphs(0, arrangement.getNumGlyphs(), 0.0f, 0.0f, bounds.getWidth(), bounds.getHeight(), Justification(payload.justificationFlags));
        arrangement.moveRangeOfGlyphs(0, arrangement.getNumGlyphs(), bounds.getX(), bounds.getY());
    } else {
        arrangement.addLineOfText(prepared->font, payload.text, 0.0f, 0.0f);

        auto offsetX = bounds.getX();
        auto const horizontalFlags = payload.justificationFlags
            & (Justification::right | Justification::horizontallyCentred | Justification::horizontallyJustified);

        if (horizontalFlags != 0) {
            auto width = arrangement.getBoundingBox(0, -1, true).getWidth();

            if ((horizontalFlags & (Justification::horizontallyCentred | Justification::horizontallyJustified)) != 0)
                width *= 0.5f;

            offsetX -= width;
        }

        arrangement.moveRangeOfGlyphs(0, arrangement.getNumGlyphs(), offsetX, bounds.getY());
    }

    prepared->glyphs.reserve(static_cast<size_t>(arrangement.getNumGlyphs()));

    for (int i = 0; i < arrangement.getNumGlyphs(); ++i) {
        auto const& glyph = arrangement.getGlyph(i);

        if (!glyph.isWhitespace())
            prepared->glyphs.push_back({ glyph.getGlyphIndex(), { glyph.getLeft(), glyph.getBaselineY() } });
    }

    return prepared;
}

static void renderPreparedText(NVGcontext* nvg, PreparedText const& prepared)
{
    if (prepared.typeface == nullptr || prepared.glyphs.empty())
        return;

    auto const scale = prepared.font.getHeightInPoints();
    auto const hscale = prepared.font.getHorizontalScale();
    auto const color = ::nvgCurrentFillColor(nvg);

    // Reused across calls on the render thread (no per-call allocation).
    static thread_local std::vector<uint64_t> hashes;
    static thread_local std::vector<float> xforms;
    hashes.clear();
    xforms.clear();

    for (auto const& glyph : prepared.glyphs) {
        auto const hash = getSDFGlyphHash(prepared.typeface.get(), glyph.glyph);

        if (!::nvgSDFGlyphCached(nvg, hash)) {
            constexpr float referenceEmPx = 32.0f;

            Path path;
            prepared.typeface->getOutlineForGlyph(glyph.glyph, path);

            ::nvgSave(nvg);
            ::nvgResetTransform(nvg);
            ::nvgScale(nvg, referenceEmPx, referenceEmPx);
            setRawNvgPath(nvg, std::move(path));
            ::nvgSaveSDFGlyph(nvg, hash);
            ::nvgRestore(nvg);
        }

        auto const tx = AffineTransform::scale(scale * hscale, scale).translated(glyph.position);
        hashes.push_back(hash);
        xforms.push_back(tx.mat00); xforms.push_back(tx.mat10);
        xforms.push_back(tx.mat01); xforms.push_back(tx.mat11);
        xforms.push_back(tx.mat02); xforms.push_back(tx.mat12);
    }

    ::nvgFillSDFGlyphRun(nvg, hashes.data(), xforms.data(), static_cast<int>(hashes.size()), color);
}

static void renderDeferredText(NVGcontext* nvg, DeferredTextPayload& payload)
{
    if (payload.text.isEmpty())
        return;

    if (payload.prepared == nullptr)
        payload.prepared = prepareDeferredText(payload);

    if (payload.prepared != nullptr)
        renderPreparedText(nvg, *payload.prepared);
}

static void enqueueDeferredText(NVGcontext* nvg, DeferredTextPayload payload, AffineTransform const& transform)
{
    auto const needsTransform = !transform.isIdentity();

    if (needsTransform) {
        nanovg::nvgSave(nvg);
        nanovg::nvgTransform(nvg, transform.mat00, transform.mat10, transform.mat01, transform.mat11, transform.mat02, transform.mat12);
    }

    nanovg::nvgRenderCallback(nvg, renderDeferredText, std::move(payload));

    if (needsTransform)
        nanovg::nvgRestore(nvg);
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

void NVGGraphicsContext::drawText(StringRef const text, Point<float> const baseline, Justification const justification, AffineTransform const& transform)
{
    auto textString = String(text);

    if (textString.isEmpty())
        return;

    auto const horizontalFlags = justification.getOnlyHorizontalFlags();
    auto const clipBounds = getClipBounds();

    if ((horizontalFlags == Justification::right && baseline.getX() < static_cast<float>(clipBounds.getX()))
        || (horizontalFlags == Justification::left && baseline.getX() > static_cast<float>(clipBounds.getRight())))
        return;

    DeferredTextPayload payload;
    payload.font = font;
    payload.text = std::move(textString);
    payload.justificationFlags = horizontalFlags;
    payload.bounds = { roundToInt(baseline.getX()), roundToInt(baseline.getY()), 0, 0 };
    payload.mode = DeferredTextMode::Baseline;

    enqueueDeferredText(nvg, std::move(payload), transform);
}

void NVGGraphicsContext::drawText(StringRef const text, Rectangle<float> const area, Justification const justification, bool const /*useEllipsesIfTooBig*/, AffineTransform const& transform)
{
    if (area.isEmpty() || !clipRegionIntersects(area.getSmallestIntegerContainer()))
        return;

    auto textString = String(text);

    if (textString.isEmpty())
        return;

    DeferredTextPayload payload;
    payload.font = font;
    payload.text = std::move(textString);
    payload.justificationFlags = justification.getFlags();
    payload.bounds = area.toNearestInt();
    payload.mode = DeferredTextMode::Rectangle;

    enqueueDeferredText(nvg, std::move(payload), transform);
}

void NVGGraphicsContext::drawGlyphs(Span<uint16_t const> glyphs, Span<Point<float> const> positions, AffineTransform const& t)
{
    if (glyphs.empty())
        return;

    auto const typeface = font.getTypefacePtr();
    if (typeface == nullptr)
        return;

    auto const scale = font.getHeightInPoints();
    auto const hscale = font.getHorizontalScale();

    glyphRunHashes.clear();
    glyphRunXforms.clear();
    glyphRunHashes.reserve(glyphs.size());
    glyphRunXforms.reserve(glyphs.size() * 6);

    for (auto const [i, glyph] : enumerate(glyphs, size_t { })) {
        auto const tx = AffineTransform::scale(scale * hscale, scale).translated(positions[i]).followedBy(t);
        auto const pathHash = getSDFGlyphHash(typeface.get(), glyph);

        if (!nanovg::nvgSDFGlyphCached(nvg, pathHash)) {
            constexpr float referenceEmPx = 32.0f;

            Path p;
            typeface->getOutlineForGlyph(glyph, p);

            nanovg::nvgSave(nvg);
            nanovg::nvgResetTransform(nvg);
            nanovg::nvgScale(nvg, referenceEmPx, referenceEmPx);
            setPath(p, AffineTransform());
            nanovg::nvgSaveSDFGlyph(nvg, pathHash);
            nanovg::nvgRestore(nvg);
        }

        glyphRunHashes.push_back(pathHash);
        glyphRunXforms.push_back(tx.mat00); glyphRunXforms.push_back(tx.mat10);
        glyphRunXforms.push_back(tx.mat01); glyphRunXforms.push_back(tx.mat11);
        glyphRunXforms.push_back(tx.mat02); glyphRunXforms.push_back(tx.mat12);
    }

    nanovg::nvgFillSDFGlyphRun(nvg, glyphRunHashes.data(), glyphRunXforms.data(),
                              static_cast<int>(glyphRunHashes.size()), lastColour);
}

NVGGraphicsContext::ScopedAnchoredDraw::ScopedAnchoredDraw(NVGGraphicsContext& context, Rectangle<float> clipBounds)
    : ctx(context)
{
    // Push the tracked state (and nvgSave); restored in the destructor. This keeps the outer paint's
    // state stack balanced — we only add and remove our own frame.
    ctx.saveState();

    if(!clipBounds.isEmpty()) {
        nanovg::nvgIntersectScissor(ctx.nvg, clipBounds.getX(), clipBounds.getY(), clipBounds.getWidth(), clipBounds.getHeight());
    }

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
    ++currentFrameId;
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
            if (bitmap.lineStride == bitmap.width) {
                id = nanovg::nvgCreateImageAlpha(nvg, image.getWidth(), image.getHeight(), 0, bitmap.data);
            } else {
                std::vector<uint8> packed(static_cast<size_t>(bitmap.width) * static_cast<size_t>(bitmap.height));
                for (int y = 0; y < bitmap.height; ++y) {
                    std::memcpy(packed.data() + static_cast<size_t>(y) * static_cast<size_t>(bitmap.width),
                        bitmap.getLinePointer(y), static_cast<size_t>(bitmap.width));
                }
                id = nanovg::nvgCreateImageAlpha(nvg, image.getWidth(), image.getHeight(), 0, packed.data());
            }
            if (images.size() >= maxImageCacheSize)
                reduceImageCache();

            images[hash] = { id, 1, currentFrameId };
        } else {
            Image argbImage(image);
            argbImage.duplicateIfShared();

            argbImage = argbImage.convertedToFormat(Image::PixelFormat::ARGB);
            Image::BitmapData const bitmap(argbImage, Image::BitmapData::readOnly);

            id = nanovg::nvgCreateImageARGB(nvg, argbImage.getWidth(), argbImage.getHeight(), 0, bitmap.data);

            if (images.size() >= maxImageCacheSize)
                reduceImageCache();

            images[hash] = { id, 1, currentFrameId };
        }
    } else {
        it->second.accessCounter++;
        it->second.lastUsedFrame = currentFrameId;
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
        if (it->second.lastUsedFrame == currentFrameId) {
            ++it;
        } else if (it->second.accessCounter == minAccessCounter) {
            nanovg::nvgDeleteImage(nvg, it->second.id);
            it = images.erase(it);
        } else {
            it->second.accessCounter -= minAccessCounter;
            ++it;
        }
    }
}
