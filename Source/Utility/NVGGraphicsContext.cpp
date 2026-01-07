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

float NVGGraphicsContext::getPhysicalPixelScaleFactor() { return scale; }

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
        auto const image = nvgCreateImageRGBA(nvg, width, height, 0, pixelData);
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

    // Flag is used to flip winding when drawing shapes with holes.
    bool solid = true;
    nvgPathWinding(nvg, path.isUsingNonZeroWinding() ? NVG_NONZERO : NVG_SOLID);

    while (i.next()) {
        switch (i.elementType) {
        case juce::Path::Iterator::startNewSubPath:
            nvgMoveTo(nvg, i.x1, i.y1);
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
            if (!path.isUsingNonZeroWinding()) {
                nvgPathWinding(nvg, solid ? NVG_SOLID : NVG_HOLE);
                solid = !solid;
            }
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
    auto typefaceName = font.getTypefacePtr()->getName();

    if (typefaceName.contains(" "))
        typefaceName = typefaceName.replace(" ", "-");
    else if (font.getTypefaceStyle().isNotEmpty())
        typefaceName += "-" + font.getTypefaceStyle();

    if (!loadedFonts.count(typefaceName)) {
        loadedFonts[typefaceName] = {};
    }

    currentGlyphToCharMap = &loadedFonts[typefaceName];

    if (currentGlyphToCharMap->empty()) {
        if (auto const tf = f.getTypefacePtr()) {
            static auto const allPrintableAsciiCharacters = []() -> juce::String {
                juce::String str;
                for (juce::juce_wchar c = 32; c < 127; ++c) // Only map printable characters
                    str += juce::String::charToString(c);
                str += juce::String::charToString(41952); // for some reason we need this char?
                return str;
            }();

            juce::Array<int> glyphs;
            juce::Array<float> offsets;
            tf->getGlyphPositions(allPrintableAsciiCharacters, glyphs, offsets);

            auto const* wstr = allPrintableAsciiCharacters.toWideCharPointer();
            for (int i = 0; i < allPrintableAsciiCharacters.length(); ++i) {
                currentGlyphToCharMap->insert({ glyphs[i], wstr[i] });
            }
        }
    }

    nvgFontFace(nvg, typefaceName.toUTF8());
    nvgFontSize(nvg, font.getHeight() * 0.862f);
    nvgTextLetterSpacing(nvg, -0.275f);
}

juce::Font const& NVGGraphicsContext::getFont()
{
    return font;
}

juce::juce_wchar NVGGraphicsContext::getCharForGlyph(int glyphIndex)
{
    // Check cache first
    if (currentGlyphToCharMap->find(glyphIndex) != currentGlyphToCharMap->end()) {
        return currentGlyphToCharMap->at(glyphIndex);
    }

    // Dynamic lookup
    if (auto const tf = getFont().getTypefacePtr()) {
        for (juce::juce_wchar wc = 0; wc < 0x10FFFF; ++wc) // Iterate over possible Unicode values
        {
            juce::Array<int> glyphs;
            juce::Array<float> xOffsets;
            tf->getGlyphPositions(juce::String::charToString(wc), glyphs, xOffsets);

            if (glyphs[0] == glyphIndex) {
                currentGlyphToCharMap->insert({ glyphIndex, wc });
                return wc;
            }
        }
    }

    return '?'; // Fallback character
}

void NVGGraphicsContext::drawGlyph(int const glyphNumber, juce::AffineTransform const& transform)
{
    char txt[8] = { '?', 0, 0, 0, 0, 0, 0, 0 };

    juce::juce_wchar const wc = getCharForGlyph(glyphNumber);

    juce::CharPointer_UTF8 utf8(txt);
    utf8.write(wc);
    utf8.writeNull();

    /*
    juce::Path p;
    auto f = getFont();
    f.getTypefacePtr()->getOutlineForGlyph (glyphNumber, p);
    p.setUsingNonZeroWinding(true);
    nvgSave(nvg);
    nvgTransform(nvg, transform.mat00, transform.mat10, transform.mat01, transform.mat11, transform.mat02, transform.mat12);
    nvgTranslate(nvg, 0, 0.75f);
    setPath(p, juce::AffineTransform::scale (f.getHeight() * f.getHorizontalScale(), f.getHeight()));
    nvgFill(nvg);
    nvgRestore(nvg); */

    nvgSave(nvg);
    setFont(getFont());
    nvgTransform(nvg, transform.mat00, transform.mat10, transform.mat01, transform.mat11, transform.mat02, transform.mat12);
    nvgTextAlign(nvg, NVG_ALIGN_BASELINE | NVG_ALIGN_LEFT);
    nvgText(nvg, 0, 1, txt, &txt[1]);
    nvgRestore(nvg);
}

bool NVGGraphicsContext::drawTextLayout(juce::AttributedString const& str, juce::Rectangle<float> const& rect)
{
    nvgSave(nvg);
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

    std::string const text = str.getText().toStdString();
    char const* textPtr = text.c_str();

    // NOTE:
    // This will not perform the correct rendering when JUCE's assumed font
    // differs from the one set by nanovg. It will probably look readable
    // and so, by if in editor, cursor position may be off because of
    // different font metrics. This should be fixed by using the same
    // fonts between JUCE and nanovg rendered.
    //
    // JUCE 6's default fonts on desktop are:
    //  Sans-serif: Verdana
    //  Serif:      Times New Roman
    //  Monospace:  Lucida Console

    float x = rect.getX();
    float const y = rect.getY();

    auto const just = str.getJustification();
    int nvgJust = 0;

    if (just.testFlags(juce::Justification::top))
        nvgJust |= NVG_ALIGN_TOP;
    else if (just.testFlags(juce::Justification::verticallyCentred))
        nvgJust |= NVG_ALIGN_MIDDLE;
    else if (just.testFlags(juce::Justification::bottom))
        nvgJust |= NVG_ALIGN_BOTTOM;

    if (just.testFlags(juce::Justification::left))
        nvgJust |= NVG_ALIGN_LEFT;
    else if (just.testFlags(juce::Justification::horizontallyCentred))
        nvgJust |= NVG_ALIGN_CENTER;
    else if (just.testFlags(juce::Justification::bottom))
        nvgJust |= NVG_ALIGN_RIGHT;

    nvgTextAlign(nvg, nvgJust);

    for (int i = 0; i < str.getNumAttributes(); ++i) {
        auto const attr = str.getAttribute(i);
        setFont(attr.font);
        nvgFillColor(nvg, nvgColour(attr.colour));

        char const* begin = &textPtr[attr.range.getStart()];
        char const* end = &textPtr[attr.range.getEnd()];

        // We assume that ranges are sorted by x so that we can move
        // to the next glyph position efficiently.
        x = nvgText(nvg, x, y, begin, end);
    }

    nvgRestore(nvg);
    return true;
}

void NVGGraphicsContext::removeCachedImages()
{
    for (auto it = images.begin(); it != images.end(); ++it)
        nvgDeleteImage(nvg, it->second.id);

    images.clear();
}

bool NVGGraphicsContext::loadFont(juce::String const& name, char const* ptr, int const size)
{
    if (ptr != nullptr && size > 0) {
        nvgCreateFontMem(nvg, name.toRawUTF8(),
            (unsigned char*)ptr, size,
            0 // Tell nvg to take ownership of the font data
        );
    } else {
        std::cerr << "Unable to load " << name << "\n";
        return false;
    }

    return false;
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

        for (int y = 0; y < argbImage.getHeight(); ++y) {
            auto* scanLine = reinterpret_cast<juce::uint32*>(bitmap.getLinePointer(y));

            for (int x = 0; x < argbImage.getWidth(); ++x) {
                juce::uint32 const argb = scanLine[x];

                juce::uint8 const a = argb >> 24;
                juce::uint8 const r = argb >> 16;
                juce::uint8 const g = argb >> 8;
                juce::uint8 const b = argb;

                // order bytes as abgr
                scanLine[x] = a << 24 | b << 16 | g << 8 | r;
            }
        }

        id = nvgCreateImageRGBA(nvg, argbImage.getWidth(), argbImage.getHeight(), NVG_IMAGE_PREMULTIPLIED, bitmap.data);

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
