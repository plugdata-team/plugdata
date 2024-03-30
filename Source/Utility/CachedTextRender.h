#pragma once

class CachedTextRender
{

public:
    void renderText(NVGcontext* nvg, String const& text, Font const& font, Colour const& colour, Rectangle<int> const& bounds, float scale)
    {
        if(imageId < 0 || lastTextHash != hash(text) || scale != lastScale || colour != lastColour || lastWidth != bounds.getWidth())
        {
            layoutReady = false;
            renderTextToImage(nvg, text, font, colour, Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth() + 3, bounds.getHeight()), scale);
        }
        
        nvgBeginPath(nvg);
        nvgSave(nvg);
        nvgIntersectScissor(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
        nvgRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth() + 3, bounds.getHeight());
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, bounds.getWidth() + 3, bounds.getHeight(), 0, imageId, 1.0f));
        nvgFill(nvg);
        nvgRestore(nvg);
    }
    
    // If you want to use this for text measuring as well, you might want the measuring to be ready before
    void prepareLayout(String const& text, Font const& font, Colour const& colour, int const width)
    {
        if(lastTextHash != hash(text) || colour != lastColour || width != lastWidth) {
            auto attributedText = AttributedString(text);
            attributedText.setColour(colour);
            attributedText.setJustification(Justification::centredLeft);
            attributedText.setFont(Font(15));
            
            layout = TextLayout();
            layout.createLayout(attributedText, width+1);
            
            idealHeight = layout.getHeight();
            idealWidth = CachedFontStringWidth::get()->calculateStringWidth(font, text);
            lastWidth = width;
            
            lastTextHash = hash(text);
            lastColour = colour;
        }
        
        layoutReady = true;
    }
    
    void renderTextToImage(NVGcontext* nvg, String const& text, Font const& font, const Colour& colour, Rectangle<int> const& bounds, float scale)
    {
        int width = std::floor(bounds.getWidth() * scale);
        int height = std::floor(bounds.getHeight() * scale);
        
        if(!layoutReady)
        {
            prepareLayout(text, font, colour, bounds.getWidth() - 3);
        }
        
        Image textImage = Image(Image::ARGB, width, height, true);
        {
            Graphics g(textImage);
            g.addTransform(AffineTransform::scale(scale, scale));
            g.reduceClipRegion(bounds.withTrimmedRight(4)); // If it touches the edges of the image, it'll look bad
            layout.draw(g, bounds.toFloat());
        }
        
        Image::BitmapData imageData(textImage, juce::Image::BitmapData::readOnly);

        uint8* pixelData = imageData.data;
        for (int y = 0; y < height; ++y)
        {
            auto* scanLine = (uint32*) imageData.getLinePointer(y);

            for (int x = 0; x < width; ++x)
            {
                juce::uint32 argb = scanLine[x];

                juce::uint8 a = argb >> 24;
                juce::uint8 r = argb >> 16;
                juce::uint8 g = argb >> 8;
                juce::uint8 b = argb;

                // order bytes as abgr
                scanLine[x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
        }

        if(imageId && imageWidth == width && imageHeight == height && scale == lastScale) {
            nvgUpdateImage(nvg, imageId, pixelData);
        }
        else {
            if(imageId) nvgDeleteImage(nvg, imageId);
            imageId = nvgCreateImageRGBA(nvg, width, height, NVG_IMAGE_PREMULTIPLIED, pixelData);
            imageWidth = width;
            imageHeight = height;
            lastScale = scale;
        }
    }
    
    Rectangle<int> getTextBounds()
    {
        return {idealWidth, idealHeight};
    }
    
private:
    int imageId = -1;
    int imageWidth = 0, imageHeight = 0;
    hash32 lastTextHash;
    float lastScale = 1.0f;
    Colour lastColour;
    int lastWidth = 0;
    int idealWidth = 0, idealHeight = 0;
    
    TextLayout layout;
    bool layoutReady = false;
};
