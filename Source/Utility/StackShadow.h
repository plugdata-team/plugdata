#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "NVGSurface.h"

// This needs to be defined before using namespace JUCE
namespace melatonin {
class DropShadow;
}

struct StackShadow : public juce::DeletedAtShutdown {
    StackShadow();

    ~StackShadow() override;

    static void renderDropShadow(juce::Graphics& g, juce::Path const& path, juce::Colour color, int radius = 1, juce::Point<int> offset = { 0, 0 }, int spread = 0);

    static NVGImage createActivityDropShadowImage(NVGcontext* nvg, juce::Rectangle<int> bounds, juce::Path const& path, juce::Colour color, int radius = 1, juce::Point<int> offset = { 0, 0 }, int spread = 0, bool isCanvas = false);

    melatonin::DropShadow* dropShadow;

    JUCE_DECLARE_SINGLETON(StackShadow, false)
};
