/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PlugDataAudioProcessorEditor::PlugDataAudioProcessorEditor (PlugDataAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), mainComponent(p.main_component)
{
    // Using openGL could really speed up the rendering
    // But currently it causes issues because we draw outside of object bounds in a few places...
    //openGLContext.attachTo(*this);
    
    addAndMakeVisible(mainComponent);
    
    
    resizer.reset(new ResizableCornerComponent (this, &restrainer));
    addAndMakeVisible(resizer.get());
    
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
    
    mainComponent.setBounds(getLocalBounds());
    
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
