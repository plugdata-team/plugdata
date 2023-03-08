/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */


// InternalSynth is an internal General MIDI synthesizer that can be used as a MIDI output device
// The goal is to get something similar to the "AU DLS Synth" in Max/MSP on macOS, but cross-platform
// Since fluidsynth is alraedy included for the sfont~ object, we can reuse it here to read a GM soundfont


#include <juce_events/juce_events.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "Utility/Config.h"


typedef struct _fluid_synth_t FluidSynth;
typedef struct _fluid_hashtable_t FluidSettings;

class InternalSynth final : public Thread
{

public:
    InternalSynth();

    ~InternalSynth();

    // Initialise fluidsynth on another thread, because it takes a while
    void run() override;

    void unprepare();

    void prepare(int sampleRate, int blockSize, int numChannels);

    void process(AudioBuffer<float>& buffer, MidiBuffer& midiMessages);

    bool isReady();

private:
    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    File soundFont = homeDir.getChildFile("Library").getChildFile("Extra").getChildFile("GS").getChildFile("GeneralUser_GS.sf3");

    // Fluidsynth state
    FluidSynth* synth = nullptr;
    FluidSettings* settings = nullptr;

    std::atomic<bool> ready = false;
    std::mutex unprepareLock;

    int lastSampleRate = 0;
    int lastBlockSize = 0;
    int lastNumChannels = 0;

    AudioBuffer<float> internalBuffer;
};