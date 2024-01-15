#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// This needs to be defined before using namespace JUCE
namespace melatonin
{
    class DropShadow;
}

struct StackShadow : public juce::DeletedAtShutdown
{
    StackShadow();
    
    ~StackShadow();
    
    static void renderDropShadow(juce::Graphics& g, juce::Path const& path, juce::Colour color, int const radius = 1, juce::Point<int> const offset = { 0, 0 }, int spread = 0);
    
    melatonin::DropShadow* dropShadow;
    
    JUCE_DECLARE_SINGLETON(StackShadow, false)
};
