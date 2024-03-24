#pragma once
#include "NVGSurface.h"
#include "NanoVGGraphicsContext.h"

class NVGComponent
{
    
public:
            
    NVGComponent(Component*);

    void renderComponentFromImage(NVGcontext* nvg, Component& component, float scale);
    
    int convertImage(NVGcontext* nvg, Image& image, int imageToUpdate = -1);
    
    static NVGcolor convertColour(Colour c);
    NVGcolor findNVGColour(int colourId);
    
    
    static void setJUCEPath(NVGcontext* nvg, Path const& p);
    
    virtual void render(NVGcontext*) {};
    
private:
    std::unique_ptr<NanoVGGraphicsContext> nvgLLGC;
    
    struct CachedImage
    {
        int imageId = 0;
        int lastHeight = 0;
        int lastWidth = 0;
    };
    
    CachedImage cachedImage;
    Component& component;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(NVGComponent)
};
