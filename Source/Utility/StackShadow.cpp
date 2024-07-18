#include "StackShadow.h"
#include "NVGSurface.h"
#include <melatonin_blur/melatonin_blur.h>

StackShadow::StackShadow()
{
    dropShadow = new melatonin::DropShadow();
}

StackShadow::~StackShadow()
{
    delete dropShadow;
    clearSingletonInstance();
}

void StackShadow::renderDropShadow(juce::Graphics& g, juce::Path const& path, juce::Colour color, int const radius, juce::Point<int> const offset, int spread)
{
    auto dropShadow = StackShadow::getInstance()->dropShadow;
    dropShadow->setColor(color);
    dropShadow->setOffset(offset);
    dropShadow->setRadius(radius);
    dropShadow->render(g, path);
}

NVGImage StackShadow::createActivityDropShadowImage(NVGcontext* nvg, juce::Rectangle<int> bounds, juce::Path const& path, juce::Colour color, int radius, juce::Point<int> offset, int spread, bool isTransparent)
{
    return NVGImage(nvg, bounds.getWidth(), bounds.getHeight(), [=](Graphics& g) {
        // make a hole in the middle of the drop shadow so that transparent objects doesn't render internal activity shadow
        if (isTransparent) {
            Path outside;
            outside.addRectangle(0, 0, bounds.getWidth(), bounds.getHeight());
            outside.setUsingNonZeroWinding(false);
            Path inside;
            auto boundsRounded = bounds.toFloat().reduced(6.5);
            inside.addRoundedRectangle(boundsRounded, radius - 2.5f, radius - 2.5f);
            outside.addPath(inside);
            g.reduceClipRegion(outside);
        }

        renderDropShadow(g, path, color, radius, offset, spread);
    });
}

JUCE_IMPLEMENT_SINGLETON(StackShadow)
