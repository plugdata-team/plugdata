#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

#include "NVGComponent.h"
#include "NanoVGGraphicsContext.h"
#include "nanovg.h"


NVGComponent::NVGComponent(Component* comp) : component(*comp)
{
}

void NVGComponent::renderComponentFromImage(NVGcontext* nvg, Component& component, float scale)
{
    Image componentImage = component.createComponentSnapshot(Rectangle<int>(0, 0, component.getWidth() + 1, component.getHeight()), true, scale);

    if(cachedImage.imageId && cachedImage.lastWidth == componentImage.getWidth() && cachedImage.lastHeight == componentImage.getHeight()) {
        cachedImage.imageId = convertImage(nvg, componentImage, cachedImage.imageId);
    }
    else {
        if(cachedImage.imageId) nvgDeleteImage(nvg, cachedImage.imageId);
        cachedImage.imageId = convertImage(nvg, componentImage);
        cachedImage.lastWidth = componentImage.getWidth();
        cachedImage.lastHeight = componentImage.getHeight();
    }

    nvgBeginPath(nvg);
    nvgRect(nvg, 0, 0, component.getWidth(), component.getHeight());
    nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, component.getWidth() + 1, component.getHeight(), 0, cachedImage.imageId, 1.0f));
    nvgFill(nvg);
}

int NVGComponent::convertImage(NVGcontext* nvg, Image& image, int imageToUpdate)
{
    Image::BitmapData imageData(image, Image::BitmapData::readOnly);

    int width = imageData.width;
    int height = imageData.height;
    uint8* pixelData = imageData.data;

    for (int y = 0; y < height; ++y)
    {
        auto* scanLine = (uint32*) imageData.getLinePointer(y);

        for (int x = 0; x < width; ++x)
        {
            uint32 argb = scanLine[x];

            uint8 a = argb >> 24;
            uint8 r = argb >> 16;
            uint8 g = argb >> 8;
            uint8 b = argb;

            // order bytes as abgr
            scanLine[x] = (a << 24) | (b << 16) | (g << 8) | r;
        }
    }
    
    if(imageToUpdate >= 0)
    {
        nvgUpdateImage(nvg, imageToUpdate, pixelData);
        return imageToUpdate;
    }
  
    return nvgCreateImageRGBA(nvg, image.getWidth(), image.getHeight(), NVG_IMAGE_PREMULTIPLIED, pixelData);
}

NVGcolor NVGComponent::convertColour(Colour c)
{
    return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}


NVGcolor NVGComponent::findNVGColour(int colourId)
{
    return convertColour(component.findColour(colourId));
}
