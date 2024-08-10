#pragma once

class CachedTextRender {
public:
    CachedTextRender() = default;

    void renderText(NVGcontext* nvg, Rectangle<int> const& bounds, float scale)
    {
        if (updateImage || !image.isValid() || lastRenderBounds != bounds || lastScale != scale) {
            renderTextToImage(nvg, Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth() + 3, bounds.getHeight()), scale);
            lastRenderBounds = bounds;
            lastScale = scale;
            updateImage = false;
        }

        NVGScopedState scopedState(nvg);
        nvgIntersectScissor(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, bounds.getWidth() + 3, bounds.getHeight(), 0, image.getImageId(), 1.0f));
        nvgFillRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth() + 3, bounds.getHeight());
    }

    // If you want to use this for text measuring as well, you might want the measuring to be ready before
    bool prepareLayout(String const& text, Font const& font, Colour const& colour, int const width, int const cachedWidth)
    {
        auto textHash = hash(text);
        bool needsUpdate = lastTextHash != textHash || colour != lastColour || cachedWidth != lastWidth;
        if (needsUpdate) {
            auto attributedText = AttributedString(text);
            attributedText.setColour(colour);
            attributedText.setJustification(Justification::centredLeft);
            attributedText.setFont(font);

            layout = TextLayout();
            layout.createLayout(attributedText, width);

            idealHeight = layout.getHeight();
            lastWidth = cachedWidth;

            lastTextHash = textHash;
            lastColour = colour;
            updateImage = true;
        }

        return needsUpdate;
    }

    void renderTextToImage(NVGcontext* nvg, Rectangle<int> const& bounds, float scale)
    {
        int width = std::floor(bounds.getWidth() * scale);
        int height = std::floor(bounds.getHeight() * scale);

        image = NVGImage(nvg, width, height, [this, bounds, scale](Graphics& g) {
            g.addTransform(AffineTransform::scale(scale, scale));
            g.reduceClipRegion(bounds.withTrimmedRight(4)); // If it touches the edges of the image, it'll look bad
            layout.draw(g, bounds.toFloat());
        });
    }

    Rectangle<int> getTextBounds()
    {
        return { idealWidth, idealHeight };
    }

private:
    NVGImage image;
    hash32 lastTextHash = 0;
    float lastScale = 1.0f;
    Colour lastColour;
    int lastWidth = 0;
    int idealWidth = 0, idealHeight = 0;
    Rectangle<int> lastRenderBounds;

    TextLayout layout;
    bool updateImage = false;
};
