#pragma once
#include "NVGSurface.h"
#include "NanoVGGraphicsContext.h"

class NVGComponent
{
    
public:
            
    NVGComponent(Component*);
    
    static NVGcolor convertColour(Colour c);
    NVGcolor findNVGColour(int colourId);
    
    static void setJUCEPath(NVGcontext* nvg, Path const& p);
    
    virtual void render(NVGcontext*) {};
    
private:
    std::unique_ptr<NanoVGGraphicsContext> nvgLLGC;
    
    Component& component;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(NVGComponent)
};

class NVGImageRenderer : public NVGContextListener
{
    NVGSurface& surface;
public:
    NVGImageRenderer(NVGSurface& nvgSurace) : surface(nvgSurace)
    {
        surface.addNVGContextListener(this);
    }
    
    ~NVGImageRenderer()
    {
        surface.removeNVGContextListener(this);
    }
    
    static int convertImage(NVGcontext* nvg, Image& image, int imageToUpdate = -1)
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
    
    void renderComponentFromImage(NVGcontext* nvg, Component& component, float scale)
    {
        Image componentImage = component.createComponentSnapshot(Rectangle<int>(0, 0, component.getWidth() + 1, component.getHeight()), false, scale);

        if(imageId && lastWidth == componentImage.getWidth() && lastHeight == componentImage.getHeight()) {
            imageId = convertImage(nvg, componentImage, imageId);
        }
        else {
            if(imageId) nvgDeleteImage(nvg, imageId);
            imageId = convertImage(nvg, componentImage);
            lastWidth = componentImage.getWidth();
            lastHeight = componentImage.getHeight();
        }

        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, component.getWidth() + 1, component.getHeight());
        nvgFillPaint(nvg, nvgImagePattern(nvg, 0, 0, component.getWidth() + 1, component.getHeight(), 0, imageId, 1.0f));
        nvgFill(nvg);
    }

    
    void nvgContextDeleted(NVGcontext* nvg) override
    {
        if(imageId) nvgDeleteImage(nvg, imageId);
        imageId = 0;
    }
    
private:
    int imageId = 0;
    int lastHeight = 0;
    int lastWidth = 0;
};
