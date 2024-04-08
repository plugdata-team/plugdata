#include "StackShadow.h"
#include "NVGComponent.h"
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


int StackShadow::createDropShadowImage(NVGcontext* nvg, juce::Rectangle<int> bounds, juce::Path const& path, juce::Colour color, int radius, juce::Point<int> offset, int spread)
{
    Image shadow(Image::ARGB, bounds.getWidth(), bounds.getHeight(), true);
    Graphics g(shadow);
    renderDropShadow(g, path, color, radius, offset, spread);
    
    return NVGImageRenderer::convertImage(nvg, shadow);
}

JUCE_IMPLEMENT_SINGLETON(StackShadow)
