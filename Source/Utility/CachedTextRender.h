#pragma once

class CachedTextRender {
public:
    CachedTextRender() = default;

    void renderText(NVGcontext* nvg, Rectangle<int> const& bounds, float const scale)
    {
        if (updateImage || !image.isValid() || lastRenderBounds != bounds || lastScale != scale) {
            renderTextToImage(nvg, Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth() + 6, bounds.getHeight()), scale);
            lastRenderBounds = bounds;
            lastScale = scale;
            updateImage = false;
        }

        NVGScopedState scopedState(nvg);
        nvgIntersectScissor(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
        auto const imagePattern = isSyntaxHighlighted ? nvgImagePattern(nvg, 0, 0, bounds.getWidth() + 6, bounds.getHeight(), 0, image.getImageId(), 1.0f) : nvgImageAlphaPattern(nvg, 0, 0, bounds.getWidth() + 6, bounds.getHeight(), 0, image.getImageId(), NVGComponent::convertColour(lastColour));

        nvgFillPaint(nvg, imagePattern);
        nvgFillRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth() + 6, bounds.getHeight());
    }

    static AttributedString getSyntaxHighlightedString(String const& text, Font const& font, Colour const& colour, Colour const& nameColour)
    {
        auto attributedText = AttributedString();
        auto tokens = StringArray::fromTokens(text, true);

        auto const flagColour = colour.interpolatedWith(LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::signalColourId), 0.7f);
        auto const mathColour = colour.interpolatedWith(Colours::purple, 0.5f);

        bool firstToken = true;
        bool hadFlag = false;
        bool mathExpression = false;
        for (int i = 0; i < tokens.size(); i++) {
            auto token = tokens[i];
            if (i != tokens.size() - 1)
                token += " ";
            if (firstToken) {
                attributedText.append(token, font, nameColour);
                if (token == "expr " || token == "expr~ " || token == "fexpr~ " || token == "op " || token == "op~ ") {
                    mathExpression = true;
                }
                firstToken = false;
            } else if (mathExpression) {
                attributedText.append(token, font, mathColour);
            } else if (token.startsWith("-") && !token.containsOnly("e.-0123456789 ")) {
                attributedText.append(token, font, flagColour);
                hadFlag = true;
            } else if (hadFlag) {
                attributedText.append(token, font, nameColour);
            } else {
                attributedText.append(token, font, colour);
            }
        }

        return attributedText;
    }

    // If you want to use this for text measuring as well, you might want the measuring to be ready before
    bool prepareLayout(String const& text, Font const& font, Colour const& colour, int const width, int const cachedWidth, bool const highlightObjectSyntax)
    {
        auto const textHash = hash(text);
        bool const needsUpdate = lastTextHash != textHash || colour != lastColour || cachedWidth != lastWidth || highlightObjectSyntax != isSyntaxHighlighted || lastFont != font;
        auto const nameColour = colour.interpolatedWith(LookAndFeel::getDefaultLookAndFeel().findColour(PlugDataColour::dataColourId), 0.7f);
        bool const highlightTextNeedsUpdaste = highlightObjectSyntax ? lastTextHighlightedColour != nameColour : false;

        if (needsUpdate || highlightTextNeedsUpdaste) {
            AttributedString attributedText;
            if (highlightObjectSyntax) {
                attributedText = getSyntaxHighlightedString(text, font, colour, nameColour);
                attributedText.setJustification(Justification::centredLeft);
            } else {
                attributedText = AttributedString(text);
                attributedText.setColour(Colours::white);
                attributedText.setJustification(Justification::centredLeft);
                attributedText.setFont(font);
            }

            layout = TextLayout();
            layout.createLayout(attributedText, width);

            idealHeight = layout.getHeight();
            lastWidth = cachedWidth;

            lastTextHash = textHash;
            lastColour = colour;
            lastFont = font;
            lastTextHighlightedColour = nameColour;
            isSyntaxHighlighted = highlightObjectSyntax;
            updateImage = true;
        }

        return needsUpdate;
    }

    void renderTextToImage(NVGcontext* nvg, Rectangle<int> const& bounds, float scale)
    {
        int const width = std::floor(bounds.getWidth() * scale);
        int const height = std::floor(bounds.getHeight() * scale);

        image = NVGImage(nvg, width, height, [this, bounds, scale](Graphics& g) {
            g.addTransform(AffineTransform::scale(scale, scale));
            layout.draw(g, bounds.toFloat()); }, isSyntaxHighlighted ? 0 : NVGImage::AlphaImage);
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
    Colour lastTextHighlightedColour;
    Font lastFont;
    int lastWidth = 0;
    int idealWidth = 0, idealHeight = 0;
    Rectangle<int> lastRenderBounds;

    TextLayout layout;
    bool updateImage = false;
    bool isSyntaxHighlighted = false;
};
