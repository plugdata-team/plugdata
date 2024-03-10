#pragma once
#include <nanovg.h>
#include "NanoVGGraphicsContext.h"

class NVGComponent
{
    class MoveListener : public ComponentListener
    {
        void componentMovedOrResized (Component& component, bool wasMoved, bool wasResized) override;
    public:
        std::function<void(Rectangle<int>)> callback;
    };
public:
            
    NVGComponent(Component&);
    
    void renderComponentFromImage(NVGcontext* nvg, Component& component, float scale);
    
    Rectangle<int> getSafeBounds() const;
    Rectangle<int> getSafeLocalBounds() const;
    
    static NVGcolor convertColour(Colour c);
    NVGcolor findNVGColour(int colourId);
    
    virtual void render(NVGcontext*) {};
    
private:
    std::unique_ptr<NanoVGGraphicsContext> nvgLLGC;
    
    Component& component;
    CriticalSection boundsLock;
    Rectangle<int> safeComponentBounds;
    MoveListener moveListener;
    

};
