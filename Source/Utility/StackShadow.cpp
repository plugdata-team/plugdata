
#include "StackShadow.h"
#include <melatonin_blur/melatonin_blur.h>


void StackShadow::renderDropShadow(juce::Graphics& g, juce::Path const& path, juce::Colour color, int const radius, juce::Point<int> const offset, int spread)
{
    dropShadow->setColor(color);
    dropShadow->setOffset(offset);
    dropShadow->setRadius(radius);
    dropShadow->render(g, path);
}

std::unique_ptr<melatonin::DropShadow> StackShadow::dropShadow = std::make_unique<melatonin::DropShadow>();
