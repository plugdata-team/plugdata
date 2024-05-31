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
    Component& component;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(NVGComponent)
};
