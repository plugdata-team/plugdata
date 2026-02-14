#include "StackShadow.h"
#include <melatonin_blur/melatonin_blur.h>

StackShadow::StackShadow() = default;

StackShadow::~StackShadow()
{
    clearSingletonInstance();
}

void StackShadow::renderDropShadow(hash32 const id, juce::Graphics& g, juce::Path const& path, juce::Colour const color, int const radius, juce::Point<int> const offset, int const spread)
{
    auto& dropShadow = StackShadow::getInstance()->dropShadows[id];
    if (!dropShadow)
        dropShadow = std::make_unique<melatonin::DropShadow>();
    dropShadow->setColor(color);
    dropShadow->setOffset(offset);
    dropShadow->setRadius(radius);
    dropShadow->setSpread(spread);
    dropShadow->render(g, path);
}

JUCE_IMPLEMENT_SINGLETON(StackShadow)
