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
#include <nanovg_async.h>
#include <vector>
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

    void setImageBlendMode(BlendMode newMode) override;

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
    void drawText(StringRef text, Point<float> baseline, Justification justification = Justification::left, AffineTransform const& transform = {});
    void drawText(StringRef text, Rectangle<float> area, Justification justification = Justification::centredLeft, bool useEllipsesIfTooBig = false, AffineTransform const& transform = {});

    void removeCachedImages();

    NVGcontext* getContext() const { return nvg; }
    void resetClipRegion(AffineTransform initialTransform = {});

    // Anchors JUCE drawing to nvg's CURRENT transform/scissor, for use inside a raw-nvg render pass
    // (e.g. object rendering). JUCE positions glyphs relative to nvg's matrix, but TextLayout/Graphics
    // cull content against getClipBounds(), which reflects this context's own tracked transform/clip.
    // During a raw-nvg pass that tracked state is stale (set for the top-level paint), so culling wrongly
    // drops text depending on scroll/zoom. Scoping a draw with this neutralises the tracked transform and
    // clip (so nothing is culled) and installs a real nvg scissor for `clipBounds` in the current local
    // space. Construct it, then draw with a Graphics wrapping this context; state is restored on scope exit.
    struct ScopedAnchoredDraw {
        ScopedAnchoredDraw(NVGGraphicsContext& context, Rectangle<float> clipBounds);
        ~ScopedAnchoredDraw();
        ScopedAnchoredDraw(ScopedAnchoredDraw const&) = delete;
        ScopedAnchoredDraw& operator=(ScopedAnchoredDraw const&) = delete;

    private:
        NVGGraphicsContext& ctx;
    };

    // Paints a JUCE component through this context, anchored to nvg's current transform (see
    // ScopedAnchoredDraw). Use this instead of a bare `Graphics g(llgc); c.paintEntireComponent(g)`
    // during a raw-nvg render pass: component painting culls against getClipBounds(), so without
    // anchoring the component's text can be dropped depending on scroll/zoom. Draws at nvg's current
    // origin, clipped to the component's local bounds.
    void renderComponent(Component& component);

    static String const defaultTypefaceName;
    static int const imageCacheSize;

private:
    int getNvgImageId(Image const& image);
    void reduceImageCache();
    AffineTransform getCurrentTransform() const;
    Rectangle<int> getTransformedClipBounds(Rectangle<float> const& bounds, AffineTransform const& transform) const;
    RectangleList<int> getTransformedClipRegion(RectangleList<int> const& region, AffineTransform const& transform) const;

    NVGcontext* nvg;

    float scale = 1.0f;
    Font font = Font(FontOptions());

    // Tracking images mapped tomtextures.
    struct NvgImage {
        int id { -1 };           ///< Image/texture ID.
        int accessCounter { 0 }; ///< Usage counter.
        uint64_t lastUsedFrame { 0 };
    };

    uint64_t currentFrameId = 0;

    struct SavedState {
        RectangleList<int> clipRegion;
        AffineTransform transform;
        float opacity = 1.0f;
        NVGcolor lastColour {};
    };

    float opacity = 1.0f;
    NVGcolor lastColour {};
    AffineTransform currentTransform;
    RectangleList<int> clipRegion;
    std::vector<SavedState> stateStack;
    UnorderedSegmentedMap<uint64, NvgImage> images;
    UnorderedSegmentedMap<uint64_t, NVGCachedPath> pathCache;
};
