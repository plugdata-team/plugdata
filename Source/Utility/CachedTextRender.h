#pragma once
#include <nanovg.h>
#include <nanovg_gl_utils.h>
#include "NanoVGGraphicsContext.h"

class CachedTextRender
{

public:
    void renderText(NVGcontext* nvg, String const& text, Font const& font, Colour const& colour, Rectangle<int> const& bounds, float scale)
    {
        if(imageId < 0 || lastTextHash != hash(text) || scale != lastScale || colour != lastColour || bounds != lastBounds) // TODO: compare colour, bounds, everything
        {
            renderTextToImage(nvg, text, font, colour, Rectangle<int>(bounds.getX(), bounds.getY(), bounds.getWidth() + 1, bounds.getHeight()), scale);
            lastTextHash = hash(text);
            lastBounds = bounds;
            lastColour = colour;
            lastScale = scale;
        }
        
        nvgBeginPath(nvg);
        nvgSave(nvg);
        nvgScissor(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
        nvgRect(nvg, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight());
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, bounds.getWidth() + 1, bounds.getHeight(), 0, imageId, 1.0f));
        nvgFill(nvg);
        nvgRestore(nvg);
    }
    
    void renderTextToImage(NVGcontext* nvg, String const& text, Font const& font, const Colour& colour, Rectangle<int> const& bounds, float scale)
    {
        int width = std::ceil(bounds.getWidth() * scale);
        int height = std::ceil(bounds.getHeight() * scale);
        
        Image textImage = Image(Image::ARGB, width, height, true);
        {
            auto attributedText = AttributedString(text);
            attributedText.setColour(colour);
            attributedText.setJustification(Justification::centredLeft);
            attributedText.setFont(Font(15));
            
            TextLayout textLayout = TextLayout();
            textLayout.createLayout(attributedText, bounds.getWidth());
            
            Graphics g(textImage);
            g.addTransform(AffineTransform::scale(scale, scale));
            textLayout.draw(g, bounds.toFloat());
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
            imageId = nvgCreateImageRGBA(nvg, width, height, 0, pixelData);
            imageWidth = width;
            imageHeight = height;
            lastScale = scale;
        }
    }
    
private:
    int imageId = -1;
    int imageWidth, imageHeight;
    hash32 lastTextHash;
    float lastScale;
    Colour lastColour;
    Rectangle<int> lastBounds;
};
