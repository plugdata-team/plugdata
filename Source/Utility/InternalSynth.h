/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "FluidLite/include/fluidlite.h"
#include "FluidLite/src/fluid_sfont.h"

#include <StandaloneBinaryData.h>

// InternalSynth is an internal General MIDI synthesizer that can be used as a MIDI output device
// The goal is to get something similar to the "AU DLS Synth" in Max/MSP on macOS, but cross-platform
// Since fluidsynth is alraedy included for the sfont~ object, we can reuse it here to read a GM soundfont

class InternalSynth : public Thread{

public:
    InternalSynth() : Thread("InternalSynthInit")
    {
        // Unpack soundfont
        if (!soundFont.existsAsFile()) {
            FileOutputStream ostream(soundFont);
            ostream.write(StandaloneBinaryData::FluidR3Mono_GM_sf3, StandaloneBinaryData::FluidR3Mono_GM_sf3Size);
            ostream.flush();
        }
    }

    ~InternalSynth()
    {
        stopThread(6000);
        
        if (synth)
            delete_fluid_synth(synth);
        if (settings)
            delete_fluid_settings(settings);
    }
    
    
    // Initialise fluidsynth on another thread, because it takes a while
    void run() override {
        
        unprepareLock.lock();
        
        internalBuffer.setSize(lastNumChannels, lastBlockSize);
        internalBuffer.clear();
        
        // Check if soundfont exists to prevent crashing
        if (soundFont.existsAsFile()) {
            auto pathName = soundFont.getFullPathName();

            // Initialise fluidsynth
            settings = new_fluid_settings();
            fluid_settings_setint(settings, "synth.ladspa.active", 0);
            fluid_settings_setint(settings, "synth.midi-channels", 16);
            fluid_settings_setnum(settings, "synth.gain", 0.9f);
            fluid_settings_setnum(settings, "synth.audio-channels", lastNumChannels);
            fluid_settings_setnum(settings, "synth.sample-rate", lastSampleRate);
            synth = new_fluid_synth(settings); // Create fluidsynth instance:

            // Load the soundfont
            int ret = fluid_synth_sfload(synth, pathName.toRawUTF8(), 0);

            if (ret >= 0) {
                fluid_synth_program_reset(synth);
            }

            ready = true;
        }
        
        unprepareLock.unlock();
    }

    void unprepare()
    {
        unprepareLock.lock();
        
        if (ready) {
            if (synth)
                delete_fluid_synth(synth);
            if (settings)
                delete_fluid_settings(settings);

            lastSampleRate = 0;
            lastBlockSize = 0;
            lastNumChannels = 0;

            ready = false;
        }
        
        unprepareLock.unlock();
    }

    void prepare(int sampleRate, int blockSize, int numChannels)
    {
        if (ready && !isThreadRunning() && sampleRate == lastSampleRate && blockSize == lastBlockSize && numChannels == lastNumChannels) {
            return;
        }
        else {
            lastSampleRate = sampleRate;
            lastBlockSize = blockSize;
            lastNumChannels = numChannels;
            
            startThread();
        }
    }

    void process(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
    {
        if(buffer.getNumChannels() != lastNumChannels || buffer.getNumSamples() > lastBlockSize) {
            unprepare();
            return;
        }
        
        // Pass MIDI messages to fluidsynth
        for (auto const& event : midiMessages) {
            auto const message = event.getMessage();

            auto channel = message.getChannel() - 1;

            if (message.isNoteOn()) {
                fluid_synth_noteon(synth, channel, message.getNoteNumber(), message.getVelocity());
            }
            if (message.isNoteOff()) {
                fluid_synth_noteoff(synth, channel, message.getNoteNumber());
            }
            if (message.isAftertouch()) {
                fluid_synth_key_pressure(synth, channel, message.getNoteNumber(), message.getAfterTouchValue());
            }
            if (message.isChannelPressure()) {
                fluid_synth_channel_pressure(synth, channel, message.getAfterTouchValue());
            }
            if (message.isController()) {
                fluid_synth_cc(synth, channel, message.getControllerNumber(), message.getControllerValue());
            }
            if (message.isProgramChange()) {
                fluid_synth_program_change(synth, channel, message.getProgramChangeNumber());
            }
            if (message.isPitchWheel()) {
                fluid_synth_pitch_bend(synth, channel, message.getPitchWheelValue());
            }
            if (message.isSysEx()) {
                fluid_synth_sysex(synth, reinterpret_cast<char const*>(message.getSysExData()), message.getSysExDataSize(), nullptr, nullptr, nullptr, 0);
            }
        }

        // Run audio through fluidsynth
        fluid_synth_process(synth, buffer.getNumSamples(), buffer.getNumChannels(), const_cast<float**>(internalBuffer.getArrayOfReadPointers()), buffer.getNumChannels(), const_cast<float**>(internalBuffer.getArrayOfWritePointers()));

        for (int ch = 0; ch < buffer.getNumChannels(); ch++) {
            buffer.addFrom(ch, 0, internalBuffer, ch, 0, buffer.getNumSamples());
        }
    }

    bool isReady()
    {
        return ready;
    }

private:
    File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

    File soundFont = homeDir.getChildFile("Library").getChildFile("Extra").getChildFile("GS").getChildFile("FluidR3Mono_GM.sf3");

    // Fluidsynth state
    fluid_synth_t* synth = nullptr;
    fluid_settings_t* settings = nullptr;

    std::atomic<bool> ready = false;
    std::mutex unprepareLock;

    int lastSampleRate = 0;
    int lastBlockSize = 0;
    int lastNumChannels = 0;

    AudioBuffer<float> internalBuffer;
};
