#pragma once

#include "Pd/PdInstance.hpp"
#include <JuceHeader.h>
#include "Console.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/



class PlugData : public pd::Instance
{
public:
    
    Component::SafePointer<Console> console;
    //==============================================================================
    PlugData(Console* debug_console) :
    pd::Instance("PlugData"),
    console(debug_console),
    m_name("PlugData"),
    m_accepts_midi(true),
    m_produces_midi(false),
    m_is_midi_effect(false),
    numin(2), numout(2), m_bypass(false){
        
        m_midi_buffer_in.ensureSize(2048);
        m_midi_buffer_out.ensureSize(2048);
        m_midi_buffer_temp.ensureSize(2048);
        
        dequeueMessages();
        
        //openPatch(PlugData::getPatchPath(), PlugData::getPatchName());
        
        processMessages();
        
    }
    
    ~PlugData() override {
        
    }
    

    
    //virtual void receiveList(const std::string& dest, const std::vector<Atom>& list) {}
    //virtual void receiveMessage(const std::string& dest, const std::string& msg, const std::vector<Atom>& list) {}
    
    void receivePrint(const std::string& message) override {
        if(console) {
            console->logMessage(message);
        }
    };
    
    void prepareToPlay (double sampleRate, int samplesPerBlock, int oversamp);
    void releaseResources() {};
    void processBlock (AudioSampleBuffer&, MidiBuffer&);
    
    // PD ENVIRONMENT VARIABLES
    static inline std::string getPatchPath() {return "/Users/timschoen/Documents/CircuitLab/.work/"; }
    
    //! @brief Gets the name of the Pd patch.
    static std::string getPatchName() {return "patch"; }
    
    void setBypass(bool bypass) {
        m_bypass = bypass;
    }
    
    void setCallbackLock(CriticalSection* lock) {
        audioLock = lock;
    };
    
    
    CriticalSection* getCallbackLock() {
        return audioLock;
    };
    
    void messageEnqueued() override {
        dequeueMessages();
        processMessages();
    }

    void reloadPatch();
    
    
    std::vector<const float*> bufferin;
    std::vector<float*> bufferout;
    
    int numin;
    int numout;
    int sampsperblock = 512;
    int osfactor = 1;
    int oversample = 0;

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

    
    CriticalSection* audioLock;
    double samplerate;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlugData)
};
