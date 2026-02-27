//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#pragma once
#include <juce_opengl/juce_opengl.h>

using namespace juce;
using namespace gl;

#include "Utility/Containers.h"
#include "Utility/NVGUtils.h"
#include "Utility/Hash.h"
#include <nanovg.h>
/**
    JUCE low level graphics context backed by nanovg.

    @note This is not a perfect translation of the JUCE
          graphics, but its still quite usable.
*/

class NVGGraphicsContext final : public LowLevelGraphicsContext {
public:
    explicit NVGGraphicsContext(NVGcontext* nativeHandle);
    ~NVGGraphicsContext() override;

    bool isVectorDevice() const override;
    void setOrigin(Point<int>) override;
    void addTransform(AffineTransform const&) override;
    float getPhysicalPixelScaleFactor() const override;
    void setPhysicalPixelScaleFactor(float newScale);

    bool clipToRectangle(Rectangle<int> const&) override;
    bool clipToRectangleList(RectangleList<int> const&) override;
    void excludeClipRectangle(Rectangle<int> const&) override;
    void clipToPath(Path const&, AffineTransform const&) override;
    void clipToImageAlpha(Image const&, AffineTransform const&) override;

    bool clipRegionIntersects(Rectangle<int> const&) override;
    Rectangle<int> getClipBounds() const override;
    bool isClipEmpty() const override;

    void saveState() override;
    void restoreState() override;

    void beginTransparencyLayer(float opacity) override;
    void endTransparencyLayer() override;

    void setFill(FillType const&) override;
    void setOpacity(float) override;
    void setInterpolationQuality(Graphics::ResamplingQuality) override;

    void fillRect(Rectangle<int> const&, bool) override;
    void fillRect(Rectangle<float> const&) override;
    void fillRectList(RectangleList<float> const&) override;

    void setPath(Path const& path, AffineTransform const& transform);

    void strokePath(Path const&, PathStrokeType const&, AffineTransform const&) override;
    void fillPath(Path const&, AffineTransform const&) override;
    void drawImage(Image const&, AffineTransform const&) override;
    void drawLine(Line<float> const&) override;

    std::unique_ptr<ImageType> getPreferredImageTypeForTemporaryImages() const override
    {
        return std::make_unique<NativeImageType>();
    }

    void setFont(Font const&) override;
    Font const& getFont() override;

    uint64_t getFrameId() const override { return 0; }

    void drawGlyphs(Span<uint16_t const>, Span<Point<float> const>, AffineTransform const&) override;

    void removeCachedImages();

    NVGcontext* getContext() const { return nvg; }

    static String const defaultTypefaceName;
    static int const imageCacheSize;

private:
    int getNvgImageId(Image const& image);
    void reduceImageCache();

    NVGcontext* nvg;

    float scale = 1.0f;
    Font font = Font(FontOptions());

    // Tracking images mapped tomtextures.
    struct NvgImage {
        int id { -1 };           ///< Image/texture ID.
        int accessCounter { 0 }; ///< Usage counter.
    };

    UnorderedMap<uint64, NvgImage> images;
    UnorderedMap<uint64_t, NVGCachedPath> pathCache;
};
