#pragma once

class CachedTextRender : public NVGContextListener
{
public:
    
    CachedTextRender(NVGSurface& nvgSurface) : surface(nvgSurface)
    {
        surface.addNVGContextListener(this);
    }
    
    ~CachedTextRender()
    {
        surface.removeNVGContextListener(this);
    }
    
    void nvgContextDeleted(NVGcontext* nvg) override
    {
        if(imageId) nvgDeleteImage(nvg, imageId);
        imageId = 0;
    }
    
    void renderText(NVGcontext* nvg, String const& text, Font const& font, Colour const& colour, Rectangle<int> const& bounds, float scale, int width)
    {
        if(updateImage || imageId <= 0 || lastTextHash != hash(text) || scale != lastScale || colour != lastColour || lastWidth != width)
        {
            layoutReady = false;
            renderTextToImage(nvg, text, font, colour, Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth() + 3, bounds.getHeight()), scale, width);
            updateImage = false;
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
    bool prepareLayout(String const& text, Font const& font, Colour const& colour, int const width, int const cachedWidth)
    {
        auto textHash = hash(text);
        bool needsUpdate = lastTextHash != textHash || colour != lastColour || cachedWidth != lastWidth;
        if(needsUpdate) {
            auto attributedText = AttributedString(text);
            attributedText.setColour(colour);
            attributedText.setJustification(Justification::centredLeft);
            attributedText.setFont(Font(15));
            
            layout = TextLayout();
            layout.createLayout(attributedText, width);
            
            idealHeight = layout.getHeight();
            lastWidth = cachedWidth;
            
            lastTextHash = textHash;
            lastColour = colour;
            updateImage = true;
        }
        
        layoutReady = true;
        return needsUpdate;
    }
    
    void renderTextToImage(NVGcontext* nvg, String const& text, Font const& font, const Colour& colour, Rectangle<int> const& bounds, float scale, int cachedWidth)
    {
        int width = std::floor(bounds.getWidth() * scale);
        int height = std::floor(bounds.getHeight() * scale);
 
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
    NVGSurface& surface;

    int imageId = 0;
    int imageWidth = 0, imageHeight = 0;
    hash32 lastTextHash;
    float lastScale = 1.0f;
    Colour lastColour;
    int lastWidth = 0;
    int idealWidth = 0, idealHeight = 0;
    
    TextLayout layout;
    bool layoutReady = false;
    bool updateImage = false;
};
