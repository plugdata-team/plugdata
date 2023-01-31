#pragma once

#include <JuceHeader.h>
#include "Object.h"

class ObjectBoundsConstrainer: public ComponentBoundsConstrainer
{
public:
    /*
     * Custom version of checkBounds that takes into consideration
     * the padding around plugdata node objects when resizing
     * to allow the aspect ratio to be interpreted correctly.
     * Otherwise resizing objects with an aspect ratio will
     * resize the object size **including** margins, and not follow the
     * actual size of the visible object
     */
    void checkBounds (Rectangle<int>& bounds,
                      const Rectangle<int>& old,
                      const Rectangle<int>& limits,
                      bool isStretchingTop,
                      bool isStretchingLeft,
                      bool isStretchingBottom,
                      bool isStretchingRight) override
    {
        // we remove the margin from the resizing object
        BorderSize<int> border(Object::margin);
        border.subtractFrom(bounds);
        
        // we also have to remove the margin from the old object, but don't alter the old object
        ComponentBoundsConstrainer::checkBounds(bounds, border.subtractedFrom(old), limits, isStretchingTop,
                                                isStretchingLeft,
                                                isStretchingBottom,
                                                isStretchingRight);
        
        // put back the margins
        border.addTo(bounds);
    }
    
};
