#include "StackShadow.h"
#include "NVGSurface.h"
#include <melatonin_blur/melatonin_blur.h>

StackShadow::StackShadow()
{
}

StackShadow::~StackShadow()
{
    clearSingletonInstance();
}

void StackShadow::renderDropShadow(hash32 id, juce::Graphics& g, juce::Path const& path, juce::Colour color, int const radius, juce::Point<int> const offset, int spread)
{
    auto& dropShadow = StackShadow::getInstance()->dropShadows[id];
    if (!dropShadow)
        dropShadow.reset(new melatonin::DropShadow);
    dropShadow->setColor(color);
    dropShadow->setOffset(offset);
    dropShadow->setRadius(radius, 0);
    dropShadow->setSpread(spread, 0);
    dropShadow->render(g, path);
}

JUCE_IMPLEMENT_SINGLETON(StackShadow)
