//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NanoVGGraphicsContext.h"
#include <BinaryData.h>

static int const maxImageCacheSize = 256;

static NVGcolor nvgColour(juce::Colour const& c)
{
    return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}

static uint64_t getImageHash(juce::Image const& image)
{
    juce::Image::BitmapData src(image, juce::Image::BitmapData::readOnly);
    return (uint64_t)src.data;
}

//==============================================================================

int const NanoVGGraphicsContext::imageCacheSize = 256;

//==============================================================================

NanoVGGraphicsContext::NanoVGGraphicsContext(NVGcontext* nativeHandle)
    : nvg(nativeHandle)
{
    jassert(nvg);
}

NanoVGGraphicsContext::~NanoVGGraphicsContext()
{
}

bool NanoVGGraphicsContext::isVectorDevice() const { return false; }

void NanoVGGraphicsContext::setOrigin(juce::Point<int> origin)
{
    nvgTranslate(nvg, origin.getX(), origin.getY());
}

void NanoVGGraphicsContext::addTransform(juce::AffineTransform const& t)
{
    nvgTransform(nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
}

float NanoVGGraphicsContext::getPhysicalPixelScaleFactor() { return scale; }

void NanoVGGraphicsContext::setPhysicalPixelScaleFactor(float newScale) { scale = newScale; }

bool NanoVGGraphicsContext::clipToRectangle(juce::Rectangle<int> const& rect)
{
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

bool NanoVGGraphicsContext::clipToRectangleList(juce::RectangleList<int> const& rects)
{
    auto rect = rects.getBounds();
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

void NanoVGGraphicsContext::excludeClipRectangle(juce::Rectangle<int> const& rectangle)
{
    juce::RectangleList<int> rectangles;
    rectangles.add(getClipBounds());
    rectangles.subtract(rectangle);

    clipToRectangleList(rectangles);
}

void NanoVGGraphicsContext::clipToPath(juce::Path const& path, juce::AffineTransform const& t)
{
    auto const rect = path.getBoundsTransformed(t);
    nvgIntersectScissor(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void NanoVGGraphicsContext::clipToImageAlpha(juce::Image const& sourceImage, juce::AffineTransform const& transform)
{
    if (!transform.isSingularity()) {
        // Convert the image to a single-channel image if necessary
        juce::Image singleChannelImage(sourceImage);
        if (sourceImage.getFormat() != juce::Image::SingleChannel) {
            singleChannelImage = sourceImage.convertedToFormat(juce::Image::SingleChannel);
        }

        juce::Image::BitmapData bitmapData(singleChannelImage, juce::Image::BitmapData::readOnly);
        auto* pixelData = bitmapData.data;

        // Create a new Nanovg image from the bitmap data
        int width = singleChannelImage.getWidth();
        int height = singleChannelImage.getHeight();
        auto image = nvgCreateImageRGBA(nvg, width, height, 0, pixelData);
        auto paint = nvgImagePattern(nvg, 0, 0, width, height, 0, image, 1);

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

bool NanoVGGraphicsContext::clipRegionIntersects(juce::Rectangle<int> const& rect)
{
    auto clip = getClipBounds();
    return clip.intersects(rect);
}

juce::Rectangle<int> NanoVGGraphicsContext::getClipBounds() const
{
    // auto scissorBounds = nvgCurrentScissor(nvg); TODO: fix this!
    return juce::Rectangle<int>(0, 0, 999999, 999999);
}

bool NanoVGGraphicsContext::isClipEmpty() const
{
    float x, y, w, h;
    nvgCurrentScissor(nvg, &x, &y, &w, &h);
    return w <= 0 || h <= 0;
}

void NanoVGGraphicsContext::saveState()
{
    nvgSave(nvg);
}

void NanoVGGraphicsContext::restoreState()
{
    nvgRestore(nvg);
}

void NanoVGGraphicsContext::beginTransparencyLayer(float op)
{
    saveState();
    nvgGlobalAlpha(nvg, op);
}

void NanoVGGraphicsContext::endTransparencyLayer()
{
    restoreState();
}

void NanoVGGraphicsContext::setFill(juce::FillType const& fillType)
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

void NanoVGGraphicsContext::setOpacity(float op)
{
    /*
    auto c = nvgGetFillColor(nvg);
    nvgRGBAf(c.r, c.g, c.b, op);
    nvgFillColor(nvg, c);
    nvgStrokeColor(nvg, c); */
}

void NanoVGGraphicsContext::setInterpolationQuality(juce::Graphics::ResamplingQuality)
{
    // @todo
}

void NanoVGGraphicsContext::fillRect(juce::Rectangle<int> const& rect, bool /* replaceExistingContents */)
{
    nvgBeginPath(nvg);
    nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill(nvg);
}

void NanoVGGraphicsContext::fillRect(juce::Rectangle<float> const& rect)
{
    nvgBeginPath(nvg);
    nvgRect(nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill(nvg);
}

void NanoVGGraphicsContext::fillRectList(juce::RectangleList<float> const& rects)
{
    for (auto const& rect : rects)
        fillRect(rect);
}

void NanoVGGraphicsContext::strokePath(juce::Path const& path, juce::PathStrokeType const& strokeType, juce::AffineTransform const& transform)
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

void NanoVGGraphicsContext::setPath(juce::Path const& path, juce::AffineTransform const& transform)
{
    juce::Path p(path);
    p.applyTransform(transform);

    nvgBeginPath(nvg);

    juce::Path::Iterator i(p);

    // Flag is used to flip winding when drawing shapes with holes.
    bool solid = true;
    nvgPathWinding(nvg, NVG_SOLID);

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
            nvgPathWinding(nvg, solid ? NVG_SOLID : NVG_HOLE);
            solid = !solid;
            break;
        default:
            break;
        }
    }
}

// not used now, but may be useful to draw text or other JUCE things in the future
// This tries to convert a non-zero wound path to nanovg. It currently has some issues with antialiasing
void NanoVGGraphicsContext::setPathWithIntersections(juce::Path const& path, juce::AffineTransform const& transform)
{
    if (path.isEmpty()) return;
    
    juce::Path p(path);
    p.applyTransform(transform);

    auto areaSigned = [](std::vector<double>& vertices)
    {
        double area = 0;
        int const nverts = vertices.size() / 2;
 
        // compute area
        for (int v = 0; v < nverts; v++)
        {
            int const i = v * 2;
            int const j = ((v + 1) % nverts) * 2;
 
            area += (vertices[j] - vertices[i]) * (vertices[j + 1] + vertices[i + 1]);
        }
 
        return area / 2;
    };
    
    // Vertex/op storage
    std::vector<double> vert;
    std::vector<int> op;

    // Path iterator
    juce::Path::Iterator pi(p);

    // Start path here
    nvgBeginPath(nvg);
    nvgPathWinding(nvg, NVG_HOLE);

    // Scan the shape and accumulate points
    while (pi.next())
    {
        // Always add segtype
        op.push_back(pi.elementType);

        switch (pi.elementType)
        {
            case juce::Path::Iterator::PathElementType::startNewSubPath:
                vert.push_back(pi.x1);
                vert.push_back(pi.y1);
                break;

            case juce::Path::Iterator::PathElementType::lineTo:
                vert.push_back(pi.x1);
                vert.push_back(pi.y1);
                break;

            case juce::Path::Iterator::PathElementType::quadraticTo:
                vert.push_back(pi.x1);
                vert.push_back(pi.y1);
                vert.push_back(pi.x2);
                vert.push_back(pi.y2);
                break;

            case juce::Path::Iterator::PathElementType::cubicTo:
                vert.push_back(pi.x1);
                vert.push_back(pi.y1);
                vert.push_back(pi.x2);
                vert.push_back(pi.y2);
                vert.push_back(pi.x3);
                vert.push_back(pi.y3);
                break;

            case juce::Path::Iterator::PathElementType::closePath:
            {
                // The path is closed: we transform the vertex storage into a NanoVG path

                // Compute the winding
                NVGwinding winding = areaSigned(vert) > 0 ? NVG_SOLID : NVG_HOLE;

                // Add the path elements to NanoVG
                int id = 0;
                for (int segType : op)
                {
                    switch (segType)
                    {
                        case juce::Path::Iterator::PathElementType::startNewSubPath:
                            nvgMoveTo(nvg, vert[id], vert[id + 1]);
                            id += 2;
                            break;

                        case juce::Path::Iterator::PathElementType::lineTo:
                            nvgLineTo(nvg, vert[id], vert[id + 1]);
                            id += 2;
                            break;

                        case juce::Path::Iterator::PathElementType::quadraticTo:
                            nvgQuadTo(nvg, vert[id], vert[id + 1], vert[id + 2], vert[id + 3]);
                            id += 4;
                            break;

                        case juce::Path::Iterator::PathElementType::cubicTo:
                            nvgBezierTo(nvg, vert[id], vert[id + 1], vert[id + 2], vert[id + 3], vert[id + 4], vert[id + 5]);
                            id += 6;
                            break;

                        case juce::Path::Iterator::PathElementType::closePath:
                            nvgClosePath(nvg);
                            nvgPathWinding(nvg, winding);
                            break;
                    }
                }

                // Done with this path section
                vert.clear();
                op.clear();
            }
            break;
        }
    }
}

void NanoVGGraphicsContext::fillPath(juce::Path const& path, juce::AffineTransform const& transform)
{
    setPath(path, transform);
    nvgFill(nvg);
}

void NanoVGGraphicsContext::drawImage(juce::Image const& image, juce::AffineTransform const& t)
{
    if (image.isARGB()) {
        juce::Image::BitmapData srcData(image, juce::Image::BitmapData::readOnly);
        auto id = getNvgImageId(image);

        if (id < 0)
            return; // invalid image.

        juce::Rectangle<float> rect(0.0f, 0.0f, image.getWidth(), image.getHeight());

        // rect = rect.transformedBy (t);

        NVGpaint imgPaint = nvgImagePattern(nvg,
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

void NanoVGGraphicsContext::drawLine(juce::Line<float> const& line)
{
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, line.getStartX(), line.getStartY());
    nvgLineTo(nvg, line.getEndX(), line.getEndY());
    nvgStroke(nvg);
}

void NanoVGGraphicsContext::setFont(juce::Font const& f)
{
    font = f;
    auto typefaceName = font.getTypefacePtr()->getName();
    if (typefaceName.contains(" "))
        typefaceName = typefaceName.replace(" ", "-");
    else
        typefaceName += "-" + font.getTypefaceStyle();

    if (!loadedFonts.count(typefaceName)) {
        loadedFonts[typefaceName] = {};
    }
    
    currentGlyphToCharMap = &loadedFonts[typefaceName];
    
    if(currentGlyphToCharMap->empty()) {
        if (auto tf = f.getTypefacePtr())
        {
            const static auto allPrintableAsciiCharacters = []() -> juce::String {
                juce::String str;
                for (juce::juce_wchar c = 32; c < 127; ++c) // Only map printable characters
                    str += juce::String::charToString (c);
                str += juce::String::charToString(juce::juce_wchar(41952)); // for some reason we need this char?
                return str;
            }();
            
            juce::Array<int> glyphs;
            juce::Array<float> offsets;
            tf->getGlyphPositions (allPrintableAsciiCharacters, glyphs, offsets);
            
            const auto* wstr = allPrintableAsciiCharacters.toWideCharPointer();
            for (int i = 0; i < allPrintableAsciiCharacters.length(); ++i) {
                currentGlyphToCharMap->insert({glyphs[i], wstr[i]});
            }
        }
    }

    nvgFontFace(nvg, typefaceName.toUTF8());
    nvgFontSize(nvg, font.getHeight() * 0.862f);
    nvgTextLetterSpacing(nvg, -0.275f);
}

juce::Font const& NanoVGGraphicsContext::getFont()
{
    return font;
}

juce::juce_wchar NanoVGGraphicsContext::getCharForGlyph(int glyphIndex)
{
    // Check cache first
    if (currentGlyphToCharMap->find(glyphIndex) != currentGlyphToCharMap->end())
    {
        return currentGlyphToCharMap->at(glyphIndex);
    }
    
    // Dynamic lookup
    if (auto tf = getFont().getTypefacePtr())
    {
        for (juce::juce_wchar wc = 0; wc < 0x10FFFF; ++wc) // Iterate over possible Unicode values
        {
            juce::Array<int> glyphs;
            juce::Array<float> xOffsets;
            tf->getGlyphPositions(juce::String::charToString(wc), glyphs, xOffsets);
            
            if (glyphs[0] == glyphIndex)
            {
                currentGlyphToCharMap->insert({glyphIndex, wc});
                return wc;
            }
        }
    }
    
    return '?'; // Fallback character
}

void NanoVGGraphicsContext::drawGlyph(int glyphNumber, juce::AffineTransform const& transform)
{
    char txt[8] = { '?', 0, 0, 0, 0, 0, 0, 0 };

    juce::juce_wchar wc = getCharForGlyph(glyphNumber);
    
    juce::CharPointer_UTF8 utf8(txt);
    utf8.write(wc);
    utf8.writeNull();

    nvgSave(nvg);
    setFont(getFont());
    nvgTransform(nvg, transform.mat00, transform.mat10, transform.mat01, transform.mat11, transform.mat02, transform.mat12);
    nvgTextAlign(nvg, NVG_ALIGN_BASELINE | NVG_ALIGN_LEFT);
    nvgText(nvg, 0, 1, txt, &txt[1]);
    nvgRestore(nvg);
}

bool NanoVGGraphicsContext::drawTextLayout(juce::AttributedString const& str, juce::Rectangle<float> const& rect)
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

    auto just = str.getJustification();
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

void NanoVGGraphicsContext::removeCachedImages()
{
    for (auto it = images.begin(); it != images.end(); ++it)
        nvgDeleteImage(nvg, it->second.id);

    images.clear();
}

bool NanoVGGraphicsContext::loadFont(juce::String const& name, char const* ptr, int size)
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

int NanoVGGraphicsContext::getNvgImageId(juce::Image const& image)
{
    int id = -1;
    auto const hash = getImageHash(image);
    auto it = images.find(hash);

    if (it == images.end()) {
        // Nanovg expects images in RGBA format, so we do conversion here
        juce::Image argbImage(image);
        argbImage.duplicateIfShared();

        argbImage = argbImage.convertedToFormat(juce::Image::PixelFormat::ARGB);
        juce::Image::BitmapData bitmap(argbImage, juce::Image::BitmapData::readOnly);

        for (int y = 0; y < argbImage.getHeight(); ++y) {
            auto* scanLine = (juce::uint32*)bitmap.getLinePointer(y);

            for (int x = 0; x < argbImage.getWidth(); ++x) {
                juce::uint32 argb = scanLine[x];

                juce::uint8 a = argb >> 24;
                juce::uint8 r = argb >> 16;
                juce::uint8 g = argb >> 8;
                juce::uint8 b = argb;

                // order bytes as abgr
                scanLine[x] = (a << 24) | (b << 16) | (g << 8) | r;
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

void NanoVGGraphicsContext::reduceImageCache()
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
