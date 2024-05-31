#include <juce_gui_basics/juce_gui_basics.h>
#include "Utility/Config.h"

#include "NVGComponent.h"
#include "nanovg.h"


NVGComponent::NVGComponent(Component* comp) : component(*comp) {}

void NVGComponent::setJUCEPath(NVGcontext* nvg, Path const& p)
{
    Path::Iterator i (p);

    nvgBeginPath (nvg);

    while (i.next())
    {
        switch (i.elementType)
        {
            case Path::Iterator::startNewSubPath:
                nvgBeginPath (nvg);
                nvgMoveTo (nvg, i.x1, i.y1);
                break;
            case Path::Iterator::lineTo:
                nvgLineTo (nvg, i.x1, i.y1);
                break;
            case Path::Iterator::quadraticTo:
                nvgQuadTo (nvg, i.x1, i.y1, i.x2, i.y2);
                break;
            case Path::Iterator::cubicTo:
                nvgBezierTo (nvg, i.x1, i.y1, i.x2, i.y2, i.x3, i.y3);
                break;
            case Path::Iterator::closePath:
                nvgClosePath (nvg);
                break;
        default:
            break;
        }
    }
}

NVGcolor NVGComponent::convertColour(Colour c)
{
    return nvgRGBA(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}


NVGcolor NVGComponent::findNVGColour(int colourId)
{
    return convertColour(component.findColour(colourId));
}
