/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class PlugDataAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    PlugDataAudioProcessorEditor (PlugDataAudioProcessor&);
    ~PlugDataAudioProcessorEditor() override;

    MainComponent& main_component;
    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    
    OpenGLContext openGLContext;
    
    ComponentBoundsConstrainer restrainer;
    std::unique_ptr<ResizableCornerComponent> resizer;
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PlugDataAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlugDataAudioProcessorEditor)
};
