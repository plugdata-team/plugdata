/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginEditor.h"
#include "Console.h"
#include "Pd/PdInstance.hpp"

//==============================================================================
/**
*/

class PlugDataPluginEditor;
class PlugDataAudioProcessor  : public AudioProcessor, public pd::Instance
{
    
public:
    //==============================================================================
    PlugDataAudioProcessor();
    ~PlugDataAudioProcessor() override;

    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //virtual void receiveList(const std::string& dest, const std::vector<Atom>& list) {}
    //virtual void receiveMessage(const std::string& dest, const std::string& msg, const std::vector<Atom>& list) {}
    
    void receivePrint(const std::string& message) override {
        if(console) {
            console->logMessage(message);
        }
    };
    
    void process(AudioSampleBuffer&, MidiBuffer&);
    
    // PD ENVIRONMENT VARIABLES
    static inline std::string getPatchPath() {return "/Users/timschoen/Documents/CircuitLab/.work/"; }
    
    //! @brief Gets the name of the Pd patch.
    static std::string getPatchName() {return "patch"; }
    
    void setBypass(bool bypass) {
        m_bypass = bypass;
    }
    
    void setCallbackLock(const CriticalSection* lock) {
        audioLock = lock;
    };
    
    
    const CriticalSection* getCallbackLock() {
        return audioLock;
    };
    
    
    void messageEnqueued() override {
        dequeueMessages();
        processMessages();
    }
    
    std::unique_ptr<Console> console;
    
    int lastUIWidth = 900, lastUIHeight = 600;
    
    AudioBuffer<float> processingBuffer;
    
    std::vector<const float*> bufferin;
    std::vector<float*> bufferout;
    
    int numin;
    int numout;
    int sampsperblock = 512;
    
    
    ValueTree mainTree = ValueTree("Main");
    void loadPatch(String patch);
    
private:
    
    void processInternal();
    

    
    String const            m_name              = String("PlugData");
    bool const              m_accepts_midi      = false;
    bool const              m_produces_midi     = false;
    bool const              m_is_midi_effect    = false;
    bool                    m_bypass            = true;
    
    int                      m_audio_advancement;
    std::vector<float>       m_audio_buffer_in;
    std::vector<float>       m_audio_buffer_out;
    
    MidiBuffer               m_midi_buffer_in;
    MidiBuffer               m_midi_buffer_out;
    MidiBuffer               m_midi_buffer_temp;

    
    const CriticalSection* audioLock;
    double samplerate;
    
    MainLook mainLook;
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlugDataAudioProcessor)
};
