/*
 // Copyright (c) 2026 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>

#if JUCE_MAC || JUCE_IOS
#include <Accelerate/Accelerate.h>
#endif

#include "StackShadow.h"
#include "Config.h"

// Thanks for sudara's melatonin_blur for the vimage and floatvector stack blur implementations!
// This class takes those implementations and optimises them for easier caching and a smaller memory footprint

bool StackShadow::vImageStackBlur(juce::Image& img, int radius)
{
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && (__MAC_OS_X_VERSION_MAX_ALLOWED >= 110000)
#    if JUCE_MAC
    if (__builtin_available(macOS 11.0, *))
#    elif JUCE_IOS
    if (__builtin_available(iOS 14.0, *))
#    endif
    {
        auto const w = static_cast<size_t>(img.getWidth());
        auto const h = static_cast<size_t>(img.getHeight());

        juce::Image::BitmapData data(img, juce::Image::BitmapData::readWrite);

        auto kernel = [](int radius) {
            size_t kernelSize = radius * 2 + 1;
            auto divisor = float(radius + 1) * (float)(radius + 1);

            std::vector<float> kernel(kernelSize);
            for (size_t i = 0; i < kernelSize; ++i) {
                auto distance = (size_t)std::abs((int)i - (int)radius);
                kernel[i] = (float)(radius + 1 - distance) / divisor;
            }
            return kernel;
        }(radius);

        // vdsp convolution isn't happy operating in-place, unfortunately
        auto copy = img.createCopy();
        juce::Image::BitmapData copyData(copy, juce::Image::BitmapData::readOnly);

        vImage_Buffer src = { copyData.getLinePointer(0), h, w, static_cast<size_t>(data.lineStride) };
        vImage_Buffer dst = { data.getLinePointer(0), h, w, static_cast<size_t>(data.lineStride) };
        vImageSepConvolve_Planar8(&src, &dst, nullptr, 0, 0,
            kernel.data(), (unsigned int)kernel.size(), kernel.data(), (unsigned int)kernel.size(),
            0, Pixel_16U(), kvImageEdgeExtend);
        return true;
    }
#endif

    return false;
}


void StackShadow::floatVectorStackBlur(Image& img, int radius)
{
    auto const w = static_cast<size_t>(img.getWidth());
    auto const h = static_cast<size_t>(img.getHeight());

    Image::BitmapData data(img, Image::BitmapData::readWrite);

    unsigned int queueIndex = 0;
    auto const divisor = 1.0f / float((radius + 1) * (radius + 1));
    auto vectorSize = (size_t)std::max(w, h);
    std::vector<std::vector<float>> queue;
    for (size_t i = 0; i < radius * 2 + 1; ++i) {
        queue.emplace_back(vectorSize, 0.0f);
    }

    std::vector<float> stackSumVector(vectorSize, 0);
    std::vector<float> sumInVector(vectorSize, 0);
    std::vector<float> sumOutVector(vectorSize, 0);
    std::vector<float> tempPixelVector(vectorSize, 0);
    for (size_t i = 0; i < h; ++i)
        tempPixelVector[(uint8_t)i] = (float)data.getLinePointer((int)i)[0];

    for (size_t i = 0; i <= size_t(radius); ++i) {
        FloatVectorOperations::copy(queue[i].data(), tempPixelVector.data(), h);
        FloatVectorOperations::add(sumOutVector.data(), tempPixelVector.data(), h);
        FloatVectorOperations::addWithMultiply(stackSumVector.data(), tempPixelVector.data(), (float)i + 1, h);
    }

    for (size_t i = 1; i <= radius; ++i) {
        if (i <= w - 1) {
            for (size_t row = 0; row < h; ++row)
                tempPixelVector[row] = data.getLinePointer((int)row)[(size_t)data.pixelStride * i];
        } else {
            for (size_t row = 0; row < h; ++row)
                tempPixelVector[row] = data.getLinePointer((int)row)[(size_t)data.pixelStride * (w - 1)];
        }

        FloatVectorOperations::copy(queue[radius + i].data(), tempPixelVector.data(), h);
        FloatVectorOperations::add(sumInVector.data(), tempPixelVector.data(), h);
        FloatVectorOperations::addWithMultiply(stackSumVector.data(), tempPixelVector.data(), (float)(radius + 1 - i), h);
    }

    for (size_t x = 0; x < w; ++x) {
        FloatVectorOperations::copy(tempPixelVector.data(), stackSumVector.data(), h);
        FloatVectorOperations::multiply(tempPixelVector.data(), divisor, h);

        for (size_t i = 0; i < h; ++i)
            data.getLinePointer((int)i)[(size_t)data.pixelStride * x] = (unsigned char)tempPixelVector[i];

        FloatVectorOperations::subtract(stackSumVector.data(), sumOutVector.data(), h);
        FloatVectorOperations::subtract(sumOutVector.data(), queue[queueIndex].data(), h);

        if (x + radius + 1 < w) {
            for (size_t row = 0; row < h; ++row)
                queue[queueIndex][row] = data.getLinePointer((int)row)[(size_t)data.pixelStride * (x + radius + 1)];
        } else {
            for (size_t row = 0; row < h; ++row)
                queue[queueIndex][row] = data.getLinePointer((int)row)[(size_t)data.pixelStride * ((size_t)w - 1)];
        }

        FloatVectorOperations::add(sumInVector.data(), queue[queueIndex].data(), h);
        if (++queueIndex == queue.size())
            queueIndex = 0;

        FloatVectorOperations::add(stackSumVector.data(), sumInVector.data(), h);

        auto middleIndex = (queueIndex + radius) % queue.size();
        FloatVectorOperations::add(sumOutVector.data(), queue[middleIndex].data(), h);
        FloatVectorOperations::subtract(sumInVector.data(), queue[middleIndex].data(), h);
    }

    std::fill(stackSumVector.begin(), stackSumVector.end(), 0.0f);
    std::fill(sumInVector.begin(), sumInVector.end(), 0.0f);
    std::fill(sumOutVector.begin(), sumOutVector.end(), 0.0f);
    queueIndex = 0;

    for (size_t i = 0; i < static_cast<size_t>(w); ++i)
        tempPixelVector[i] = (float)data.getLinePointer(0)[i];

    for (size_t i = 0; i <= static_cast<size_t>(radius); ++i) {
        // these init left side AND middle of the stack
        FloatVectorOperations::copy(queue[i].data(), tempPixelVector.data(), w);
        FloatVectorOperations::add(sumOutVector.data(), tempPixelVector.data(), w);
        FloatVectorOperations::addWithMultiply(stackSumVector.data(), tempPixelVector.data(), (float)i + 1, w);
    }

    for (size_t i = 1; i <= radius; ++i) {
        if (i <= h - 1) {
            for (size_t col = 0; col < (size_t)w; ++col)
                tempPixelVector[col] = (float)data.getLinePointer((int)i)[col];
        } else {
            for (size_t col = 0; col < (size_t)w; ++col)
                tempPixelVector[col] = (float)data.getLinePointer((int)h - 1)[col];
        }

        FloatVectorOperations::copy(queue[radius + i].data(), tempPixelVector.data(), w);
        FloatVectorOperations::add(sumInVector.data(), tempPixelVector.data(), w);
        FloatVectorOperations::addWithMultiply(stackSumVector.data(), tempPixelVector.data(), (float)(radius + 1 - i), w);
    }

    for (size_t y = 0; y < h; ++y) {
        FloatVectorOperations::copy(tempPixelVector.data(), stackSumVector.data(), w);
        FloatVectorOperations::multiply(tempPixelVector.data(), divisor, w);

        for (size_t i = 0; i < static_cast<size_t>(w); ++i)
            data.getLinePointer((int)y)[i] = (uint8_t)tempPixelVector[i];
        FloatVectorOperations::subtract(stackSumVector.data(), sumOutVector.data(), w);
        FloatVectorOperations::subtract(sumOutVector.data(), queue[queueIndex].data(), w);

        if (y + radius + 1 < h) {
            for (size_t col = 0; col < (size_t)w; ++col)
                queue[queueIndex][col] = (float)data.getLinePointer((int)(y + radius + 1))[col];
        } else {
            for (size_t col = 0; col < (size_t)w; ++col)
                queue[queueIndex][col] = (float)data.getLinePointer((int)h - 1)[col];
        }

        FloatVectorOperations::add(sumInVector.data(), queue[queueIndex].data(), w);
        if (++queueIndex == queue.size())
            queueIndex = 0;

        FloatVectorOperations::add(stackSumVector.data(), sumInVector.data(), w);

        auto middleIndex = (queueIndex + radius) % queue.size();
        FloatVectorOperations::add(sumOutVector.data(), queue[middleIndex].data(), w);
        FloatVectorOperations::subtract(sumInVector.data(), queue[middleIndex].data(), w);
    }
}

Image StackShadow::generateBaseShadowImage(Path& p, int radius, int logicalW, int logicalH, float scale)
{
    int const physicalRadius = roundToInt(radius * scale);
    auto img = Image(Image::SingleChannel,
        roundToInt(logicalW * scale) + physicalRadius,
        roundToInt(logicalH * scale) + physicalRadius, true);
    {
        Graphics g(img);
        g.addTransform(AffineTransform::scale(scale));
        g.setColour(Colours::white);
        g.fillPath(p);
    }

    if (vImageStackBlur(img, physicalRadius))
        return img;

    floatVectorStackBlur(img, physicalRadius);
    return img;
}

StackShadow::RectShadowImage StackShadow::generateShadowImages(int radius, float cornerRadius, float scale)
{
    int const cSize = radius + cornerRadius;
    int const scaledCornerSize = roundToInt(cSize * scale);
    int const edgeH = radius * 2;
    int const scaledEdgeH = roundToInt(edgeH * scale);

    Path p;
    p.addRoundedRectangle(radius, radius, cSize * 3, jmax(cSize, edgeH), cornerRadius);

    Image src = generateBaseShadowImage(p, radius, cSize * 3, jmax(cSize, edgeH), scale);
    Image::BitmapData srcData(src, Image::BitmapData::readOnly);

    RectShadowImage imgs = {
        Image(Image::ARGB, scaledCornerSize, scaledCornerSize, true),
        Image(Image::ARGB, 1.0f, scaledEdgeH, true)
    };

    auto cornerData = Image::BitmapData(imgs.corner, Image::BitmapData::writeOnly);
    auto edgeData = Image::BitmapData(imgs.edge, Image::BitmapData::writeOnly);

    for (int y = 0; y < src.getHeight(); ++y) {
        auto const* srcRow = srcData.getLinePointer(y);
        if (y < scaledCornerSize) {
            auto* cornerRow = cornerData.getLinePointer(y);
            for (int x = 0; x < scaledCornerSize; ++x)
                cornerRow[x * 4 + PixelARGB::indexA] = srcRow[x];
        }
        if (y < scaledEdgeH) {
            auto* edgeRow = edgeData.getLinePointer(y);
            edgeRow[PixelARGB::indexA] = srcRow[cSize * 2];
        }
    }
    return imgs;
}

void StackShadow::drawShadowForRect(Graphics& g, Rectangle<int> bounds, int radius, float shadowCornerRadius, float opacity, int verticalOffset)
{
    auto pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();
    auto shadowHash = std::hash<float> { }(pixelScale);
    shadowHash ^= std::hash<int> { }(radius) + 0x9e3779b9 + (shadowHash << 6) + (shadowHash >> 2);
    shadowHash ^= std::hash<float> { }(shadowCornerRadius) + 0x9e3779b9 + (shadowHash << 6) + (shadowHash >> 2);

    auto& inst = *getInstance();

    if (!inst.shadowMap.count(shadowHash))
        inst.shadowMap.emplace(shadowHash, generateShadowImages(radius, shadowCornerRadius, pixelScale));

    auto const& imgs = inst.shadowMap.at(shadowHash);

    float const bx = bounds.getX() * pixelScale;
    float const by = bounds.getY() * pixelScale;
    float const bw = bounds.getWidth() * pixelScale;
    float const bh = bounds.getHeight() * pixelScale;

    g.saveState();
    g.setOpacity(opacity);
    g.addTransform(AffineTransform::scale(1.0f / pixelScale).translated(0, verticalOffset));

    float const r = radius * pixelScale;
    float const cr = shadowCornerRadius * pixelScale;
    float const vEdgeRot = MathConstants<float>::halfPi;

    // Corners
    g.drawImageAt(imgs.corner, bx - r, by - r);
    g.drawImageTransformed(imgs.corner, AffineTransform::scale(-1.0f, 1.0f).translated(bx + bw + r, by - r));
    g.drawImageTransformed(imgs.corner, AffineTransform::scale(1.0f, -1.0f).translated(bx - r, by + bh + r));
    g.drawImageTransformed(imgs.corner, AffineTransform::scale(-1.0f, -1.0f).translated(bx + bw + r, by + bh + r));

    // Edges
    g.drawImageTransformed(imgs.edge, AffineTransform::scale(bw - 2 * cr, 1.0f).translated(bx + cr, by - r));
    g.drawImageTransformed(imgs.edge, AffineTransform::scale(bw - 2 * cr, -1.0f).translated(bx + cr, by + bh + r));
    g.drawImageTransformed(imgs.edge, AffineTransform::rotation(-vEdgeRot).scaled(1.0f, bh - 2 * cr).translated(bx - r, by + bh - cr));
    g.drawImageTransformed(imgs.edge, AffineTransform::rotation(vEdgeRot).scaled(1.0f, bh - 2 * cr).translated(bx + bw + r, by + cr));

    g.restoreState();
}

void StackShadow::drawShadowForPath(Graphics& g, hash32 id, Path const& path, int radius, Colour colour, int verticalOffset, int horizontalOffset)
{
    auto pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();
    auto shadowHash = (static_cast<uint64_t>(id) << 32) | std::bit_cast<uint32_t>(pixelScale);
    auto& inst = *getInstance();

    if (!inst.pathShadowMap.count(shadowHash)) {
        auto bounds = path.getBounds();
        int logicalW = roundToInt(bounds.getWidth());
        int logicalH = roundToInt(bounds.getHeight());

        Path normalised = path;
        normalised.applyTransform(AffineTransform::translation(radius - bounds.getX(), radius - bounds.getY()));

        PathShadowImage entry;
        entry.image = generateBaseShadowImage(normalised, radius, logicalW + radius, logicalH + radius, pixelScale);
        entry.x = bounds.getX() - radius;
        entry.y = bounds.getY() - radius;
        inst.pathShadowMap.emplace(shadowHash, std::move(entry));
    }

    auto const& entry = inst.pathShadowMap.at(shadowHash);
    auto const& img = entry.image;

    g.saveState();
    g.setColour(colour);
    g.drawImageTransformed(img, AffineTransform::translation((entry.x + horizontalOffset) * pixelScale, (entry.y + verticalOffset) * pixelScale).scaled(1.0f / pixelScale), true);
    g.restoreState();
}
