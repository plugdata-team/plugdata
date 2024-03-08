#pragma once
#include <nanovg.h>

class NVGComponent
{
public:
        
    class NVGCachedImage
    {
        int imageId = 0, lastWidth = 0, lastHeight = 0;
        friend class NVGComponent;
    };
    
    NVGComponent(Component&);
    
    void renderComponentFromImage(NVGcontext* nvg, Component& component, float scale);

    static NVGcolor convertColour(Colour c);
    NVGcolor findNVGColour(int colourId);
    
    virtual void render(NVGcontext*) {};
    
    Component& component;
    NVGCachedImage cachedImage;
};
