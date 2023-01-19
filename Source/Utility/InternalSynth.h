/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "FluidLite/include/fluidlite.h"
#include "FluidLite/src/fluid_sfont.h"

struct InternalSynth
{
    fluid_synth_t* synth;
    fluid_settings_t* settings;
    fluid_sfont_t* sfont;
    fluid_preset_t* preset;
        
    InternalSynth() {

    }
    
    bool ready = false;
    
    AudioBuffer<float> internalBuffer;
    
    void prepare(int sampleRate, int blockSize, int numChannels)
    {
        File homeDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata");
        
        auto soundFont = homeDir.getChildFile("Library").getChildFile("Extra").getChildFile("GS").getChildFile("FluidR3Mono_GM.sf3");
        
        if(soundFont.existsAsFile()) {
            auto pathName = soundFont.getFullPathName();
            
            settings = new_fluid_settings();
            fluid_settings_setint(settings, "synth.ladspa.active", 0);
            fluid_settings_setint(settings, "synth.midi-channels", 16);
            fluid_settings_setnum(settings, "synth.gain", 0.75f);
            fluid_settings_setnum(settings, "synth.sample-rate", sampleRate);
            fluid_settings_setnum(settings, "synth.sample-rate", sampleRate);
            synth = new_fluid_synth(settings); // Create fluidsynth instance:
            
            int ret = fluid_synth_sfload(synth, pathName.toRawUTF8(), 0);
            
            internalBuffer.setSize(2, blockSize);
            
            if(ret >= 0){
                fluid_synth_program_reset(synth);
                sfont = fluid_synth_get_sfont_by_id(synth, ret);
            }
            
            ready = true;
        }
    }
    
    void process(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
        
        if(!ready) return;
        
        fluid_synth_write_float(synth, buffer.getNumSamples(), internalBuffer.getWritePointer(0), 0, 1, internalBuffer.getWritePointer(1), 0, 1);
        
        for (auto const& event : midiMessages) {
            auto const message = event.getMessage();
            
            auto channel = message.getChannel() - 1;
            
            if(message.isNoteOn()) {
                fluid_synth_noteon(synth, channel, message.getNoteNumber(), message.getVelocity());
            }
            if(message.isNoteOff()) {
                fluid_synth_noteoff(synth, channel, message.getNoteNumber());
            }
            if(message.isAftertouch()) {
                fluid_synth_key_pressure(synth, channel, message.getNoteNumber(), message.getAfterTouchValue());
            }
            if(message.isChannelPressure()) {
                fluid_synth_channel_pressure(synth, channel, message.getAfterTouchValue());
            }
            if(message.isController()) {
                fluid_synth_cc(synth, channel, message.getControllerNumber(), message.getControllerValue());
            }
            if(message.isProgramChange()) {
                fluid_synth_program_change(synth, channel, message.getProgramChangeNumber());
            }
            if(message.isPitchWheel()) {
                fluid_synth_pitch_bend(synth, channel, message.getPitchWheelValue());
            }
        }
        
        buffer.addFrom(0, 0, internalBuffer, 0, 0, buffer.getNumSamples());
        buffer.addFrom(1, 0, internalBuffer, 1, 0, buffer.getNumSamples());
        
        /*

        case 0xA0: // group 160-175 (POLYPHONIC AFTERTOUCH)
            SETFLOAT(&x->x_at[0], val); // aftertouch pressure value
            SETFLOAT(&x->x_at[1], (t_float)x->x_data); // key
            SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
            sfont_polytouch(x, &s_list, 3, x->x_at);
            break;
        case 0xB0: // group 176-191 (CONTROL CHANGE)
            SETFLOAT(&x->x_at[0], val); // control value
            SETFLOAT(&x->x_at[1], (t_float)x->x_data); // control number
            SETFLOAT(&x->x_at[2], (t_float)x->x_channel);
            sfont_control_change(x, &s_list, 3, x->x_at);
            break;
        case 0xC0: // group 192-207 (PROGRAM CHANGE)
            SETFLOAT(&x->x_at[0], val); // control value
            SETFLOAT(&x->x_at[1], (t_float)x->x_channel);
            sfont_program_change(x, &s_list, 2, x->x_at);
            break;
        case 0xD0: // group 208-223 (AFTERTOUCH)
            SETFLOAT(&x->x_at[0], val); // control value
            SETFLOAT(&x->x_at[1], (t_float)x->x_channel);
            sfont_aftertouch(x, &s_list, 2, x->x_at);
            break;
        case 0xE0: // group 224-239 (PITCH BEND)
            SETFLOAT(&x->x_at[0], (val << 7) + x->x_data);
            SETFLOAT(&x->x_at[1], x->x_channel);
            sfont_pitch_bend(x, &s_list, 2, x->x_at);
            break;
        default:
            break;
        } */
    }
};
