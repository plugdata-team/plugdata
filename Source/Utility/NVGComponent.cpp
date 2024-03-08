#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

#include "NVGComponent.h"
#include "nanovg.h"


NVGComponent::NVGComponent(Component& comp) : component(comp)
{
    
}

void NVGComponent::renderComponentFromImage(NVGcontext* nvg, Component& component, float scale)
{
    auto componentImage = component.createComponentSnapshot(component.getLocalBounds(), true, scale);
    Image::BitmapData imageData(componentImage, juce::Image::BitmapData::readOnly);

    int width = imageData.width;
    int height = imageData.height;
    uint8* pixelData = imageData.data;
    size_t imageSize = width * height * 4; // 4 bytes per pixel for RGBA

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

    if(cachedImage.imageId && cachedImage.lastWidth == width && cachedImage.lastHeight == height) {
        nvgUpdateImage(nvg, cachedImage.imageId, pixelData);
    }
    else {
        if(cachedImage.imageId) nvgDeleteImage(nvg, cachedImage.imageId);
        cachedImage.imageId = nvgCreateImageRGBA(nvg, width, height, NVG_IMAGE_PREMULTIPLIED, pixelData);
        cachedImage.lastWidth = width;
        cachedImage.lastHeight = height;
    }

    nvgBeginPath(nvg);
    nvgRect(nvg, 0, 0, component.getWidth(), component.getHeight());
    nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, component.getWidth(), component.getHeight(), 0, cachedImage.imageId, 1.0f));
    nvgFill(nvg);
}

NVGcolor NVGComponent::convertColour(Colour c)
{
    return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}


NVGcolor NVGComponent::findNVGColour(int colourId)
{
    return convertColour(component.findColour(colourId));
}
