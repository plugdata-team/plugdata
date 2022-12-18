#include "ObjectComponentBoundsConstrainer.h"
#include "Object.h"

void ObjectComponentBoundsConstrainer::checkBounds (Rectangle<int>& bounds,
                                                    const Rectangle<int>& old,
                                                    const Rectangle<int>& limits,
                                                    bool isStretchingTop,
                                                    bool isStretchingLeft,
                                                    bool isStretchingBottom,
                                                    bool isStretchingRight)
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