//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#pragma once
#include <juce_opengl/juce_opengl.h>
using namespace juce::gl;
#include <nanovg.h>
/**
    JUCE low level graphics context backed by nanovg.

    @note This is not a perfect translation of the JUCE
          graphics, but its still quite usable.
*/

class NanoVGGraphicsContext : public juce::LowLevelGraphicsContext {
public:
    NanoVGGraphicsContext(NVGcontext* nativeHandle);
    ~NanoVGGraphicsContext() override;

    bool isVectorDevice() const override;
    void setOrigin(juce::Point<int>) override;
    void addTransform(juce::AffineTransform const&) override;
    float getPhysicalPixelScaleFactor() override;
    void setPhysicalPixelScaleFactor(float newScale);

    bool clipToRectangle(juce::Rectangle<int> const&) override;
    bool clipToRectangleList(juce::RectangleList<int> const&) override;
    void excludeClipRectangle(juce::Rectangle<int> const&) override;
    void clipToPath(juce::Path const&, juce::AffineTransform const&) override;
    void clipToImageAlpha(juce::Image const&, juce::AffineTransform const&) override;

    bool clipRegionIntersects(juce::Rectangle<int> const&) override;
    juce::Rectangle<int> getClipBounds() const override;
    bool isClipEmpty() const override;

    void saveState() override;
    void restoreState() override;

    void beginTransparencyLayer(float opacity) override;
    void endTransparencyLayer() override;

    void setFill(juce::FillType const&) override;
    void setOpacity(float) override;
    void setInterpolationQuality(juce::Graphics::ResamplingQuality) override;

    void fillRect(juce::Rectangle<int> const&, bool) override;
    void fillRect(juce::Rectangle<float> const&) override;
    void fillRectList(juce::RectangleList<float> const&) override;

    void setPath(juce::Path const& path, juce::AffineTransform const& transform);
    void setPathWithIntersections(juce::Path const& path, juce::AffineTransform const& transform);
    
    void strokePath(juce::Path const&, juce::PathStrokeType const&, juce::AffineTransform const&);
    void fillPath(juce::Path const&, juce::AffineTransform const&) override;
    void drawImage(juce::Image const&, juce::AffineTransform const&) override;
    void drawLine(juce::Line<float> const&) override;

    void setFont(juce::Font const&) override;
    juce::Font const& getFont() override;
    void drawGlyph(int glyphNumber, juce::AffineTransform const&) override;
    bool drawTextLayout(juce::AttributedString const&, juce::Rectangle<float> const&) override;

    void removeCachedImages();

    NVGcontext* getContext() const { return nvg; }

    bool loadFont(juce::String const& name, char const* ptr, int size);

    static juce::String const defaultTypefaceName;
    static int const imageCacheSize;

private:
    
    juce::juce_wchar getCharForGlyph(int glyphIndex);
    
    int getNvgImageId(juce::Image const& image);
    void reduceImageCache();

    NVGcontext* nvg;

    float scale = 1.0f;

    juce::Font font;

    // Mapping glyph number to a character
    using GlyphToCharMap = std::unordered_map<int, wchar_t>;

    // Mapping font names to glyph-to-character tables
    static inline std::unordered_map<juce::String, GlyphToCharMap> loadedFonts = std::unordered_map<juce::String, GlyphToCharMap>();
    GlyphToCharMap* currentGlyphToCharMap;

    // Tracking images mapped tomtextures.
    struct NvgImage {
        int id { -1 };           ///< Image/texture ID.
        int accessCounter { 0 }; ///< Usage counter.
    };

    std::unordered_map<juce::uint64, NvgImage> images;
};
