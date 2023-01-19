/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "FluidLite/include/fluidlite.h"
#include "FluidLite/src/fluid_sfont.h"

// InternalSynth is an internal General MIDI synthesizer that can be used as a MIDI output device
// The goal is to get something similar to the "AU DLS Synth" in Max/MSP on macOS, but cross-platform
// Since fluidsynth is alraedy included for the sfont~ object, we can reuse it here to read a GM soundfont

struct InternalSynth {
    
    ~InternalSynth()
    {
        if (synth)
            delete_fluid_synth(synth);
        if (settings)
            delete_fluid_settings(settings);
    }

    void prepare(int sampleRate, int blockSize, int numChannels)
    {
        File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");

        auto soundFont = homeDir.getChildFile("Library").getChildFile("Extra").getChildFile("GS").getChildFile("FluidR3Mono_GM.sf3");

        internalBuffer.setSize(2, blockSize);

        // Check if soundfont exists to prevent crashing
        if (soundFont.existsAsFile()) {
            auto pathName = soundFont.getFullPathName();

            // Initialise fluidsynth
            settings = new_fluid_settings();
            fluid_settings_setint(settings, "synth.ladspa.active", 0);
            fluid_settings_setint(settings, "synth.midi-channels", 16);
            fluid_settings_setnum(settings, "synth.gain", 0.75f);
            fluid_settings_setnum(settings, "synth.sample-rate", sampleRate);
            fluid_settings_setnum(settings, "synth.sample-rate", sampleRate);
            synth = new_fluid_synth(settings); // Create fluidsynth instance:

            // Load the soundfont
            int ret = fluid_synth_sfload(synth, pathName.toRawUTF8(), 0);

            if (ret >= 0) {
                fluid_synth_program_reset(synth);
            }

            ready = true;
        }
    }

    void process(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
    {
        if (!ready)
            return;

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
        }

        // Run audio through fluidsynth
        fluid_synth_write_float(synth, buffer.getNumSamples(), internalBuffer.getWritePointer(0), 0, 1, internalBuffer.getWritePointer(1), 0, 1);

        buffer.addFrom(0, 0, internalBuffer, 0, 0, buffer.getNumSamples());
        buffer.addFrom(1, 0, internalBuffer, 1, 0, buffer.getNumSamples());
    }

private:
    // Fluidsynth state
    fluid_synth_t* synth = nullptr;
    fluid_settings_t* settings = nullptr;

    bool ready = false;

    AudioBuffer<float> internalBuffer;
};
