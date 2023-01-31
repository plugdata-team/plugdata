#pragma once

#include <JuceHeader.h>

class ObjectComponentBoundsConstrainer: public ComponentBoundsConstrainer
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
                      bool isStretchingRight) override;
};