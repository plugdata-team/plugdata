/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PlugDataAudioProcessorEditor::PlugDataAudioProcessorEditor (PlugDataAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), main_component(p.main_component)
{
    addAndMakeVisible(main_component);
    
    addAndMakeVisible (resizer = new ResizableCornerComponent (this, &restrainer));
    restrainer.setSizeLimits (150, 150, 1200, 1200);
    setSize (audioProcessor.lastUIWidth, audioProcessor.lastUIHeight);
    
}

PlugDataAudioProcessorEditor::~PlugDataAudioProcessorEditor()
{
}

//==============================================================================
void PlugDataAudioProcessorEditor::paint (juce::Graphics& g)
{
}

void PlugDataAudioProcessorEditor::resized()
{
    resizer->setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
    audioProcessor.lastUIWidth = getWidth();
    audioProcessor.lastUIHeight = getHeight();
    
    main_component.setBounds(getLocalBounds());
    
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
