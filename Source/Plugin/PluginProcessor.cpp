/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PlugDataAudioProcessor::PlugDataAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

PlugDataAudioProcessor::~PlugDataAudioProcessor()
{
}

//==============================================================================
const juce::String PlugDataAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PlugDataAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PlugDataAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PlugDataAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PlugDataAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PlugDataAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PlugDataAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PlugDataAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PlugDataAudioProcessor::getProgramName (int index)
{
    return {};
}

void PlugDataAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PlugDataAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    main_component.pd.prepareToPlay(sampleRate, samplesPerBlock, 0);
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    processing_buffer.setSize(2, samplesPerBlock);
}

void PlugDataAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PlugDataAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PlugDataAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

   //midiCollector.removeNextBlockOfMessages(midiMessages, 512);
    
    processing_buffer.setSize(2, buffer.getNumSamples());
    
    processing_buffer.copyFrom(0, 0, buffer, 0, 0, buffer.getNumSamples());
    processing_buffer.copyFrom(1, 0, buffer, totalNumInputChannels == 2 ? 1 : 0, 0, buffer.getNumSamples());
    
    
    main_component.pd.processBlock(processing_buffer, midiMessages);
    
    buffer.copyFrom(0, 0, processing_buffer, 0, 0, buffer.getNumSamples());
    if(totalNumOutputChannels == 2) {
        buffer.copyFrom(1, 0, processing_buffer, 1, 0, buffer.getNumSamples());
    }
   
    
    // ..do something to the data...
}

//==============================================================================
bool PlugDataAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PlugDataAudioProcessor::createEditor()
{
    return new PlugDataAudioProcessorEditor (*this);
}

//==============================================================================
void PlugDataAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PlugDataAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlugDataAudioProcessor();
}
