#pragma once
#include <nanovg.h>

class NVGHelper
{
public:
    
    class NVGCachedImage
    {
        int imageId = 0, lastWidth = 0, lastHeight = 0;
        friend class NVGHelper;
    };
    
    static void renderComponent(NVGcontext* nvg, Component& component, float scale, NVGCachedImage& cache);

    static NVGcolor convertColour(Colour c);
};
