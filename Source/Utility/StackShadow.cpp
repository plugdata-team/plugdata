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
    dropShadow->setSpread(spread);
    dropShadow->render(g, path);
}

JUCE_IMPLEMENT_SINGLETON(StackShadow)
