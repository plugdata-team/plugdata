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
    if(!StackShadow::getInstance()->dropShadows.contains(id))
        StackShadow::getInstance()->dropShadows.emplace_back(id, std::make_unique<melatonin::DropShadow>());
        
    auto* dropShadow = StackShadow::getInstance()->dropShadows[id];
    dropShadow->setColor(color);
    dropShadow->setOffset(offset);
    dropShadow->setRadius(radius);
    dropShadow->setSpread(spread);
    dropShadow->render(g, path);
}

JUCE_IMPLEMENT_SINGLETON(StackShadow)
