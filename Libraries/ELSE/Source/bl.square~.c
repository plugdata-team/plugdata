// License: BSD 2 Clause
// Created by Timothy Schoen and Porres, based on LabSound polyBLEP oscillators

// Adapted from "Phaseshaping Oscillator Algorithms for Musical Sound
// Synthesis" by Jari Kleimola, Victor Lazzarini, Joseph Timoney, and Vesa
// Valimaki. http://www.acoustics.hut.fi/publications/papers/smc2010-phaseshaping/

#include "m_pd.h"
#include <math.h>
#include <stdint.h>

#define PI     3.1415926535897931
#define TWO_PI 6.2831853071795862

typedef struct _polyblep{
    t_float pulse_width;    // Pulse width for square, morph-to-saw for triangle
    t_float phase;          // The current phase of the oscillator.
    t_float freq_in_seconds_per_sample;
    t_int midi;
    t_float sr;
    t_int soft;
    t_float last_phase_offset;
}t_polyblep;

typedef struct blsquare{
    t_object x_obj;
    t_float x_f;
    t_polyblep x_polyblep;
    t_inlet* x_inlet_sync;
    t_inlet* x_inlet_phase;
    t_inlet* x_inlet_width;
}t_blsquare;

t_class *bl_square;

static void blsquare_midi(t_blsquare *x, t_floatarg f){
    x->x_polyblep.midi = (int)(f != 0);
}

static void blsquare_soft(t_blsquare *x, t_floatarg f){
    x->x_polyblep.soft = (int)(f != 0);
}

static t_float phasewrap(t_float phase){
    while (phase < 0.0)
        phase += 1.0;
    while(phase >= 1.0)
        phase -= 1.0;
    return(phase);
}

static t_float square(t_float x){
    return(x * x);
}

static t_float blep(t_float phase, t_float dt){
    if(phase < dt)
        return(-square(phase / dt - 1));
    else if(phase > 1 - dt)
        return(square((phase - 1) / dt + 1));
    else
        return(0.0);
}

static t_float sqr(const t_polyblep* x){
    t_float pulse_width = fmax(0.0001, fmin(0.9999, x->pulse_width));
    t_float t2 = x->phase + 0.5;
    t2 = phasewrap(t2);
    t_float y = x->phase < pulse_width ? 1 : -1;
    y += blep(x->phase, x->freq_in_seconds_per_sample) - blep(t2, x->freq_in_seconds_per_sample);
    return(y);
}

static t_int* blsquare_perform(t_int *w) {
    t_polyblep* x      = (t_polyblep*)(w[1]);
    t_int n            = (t_int)(w[2]);
    t_float* freq_vec  = (t_float *)(w[3]);
    t_float* width_vec = (t_float *)(w[4]);
    t_float* sync_vec  = (t_float *)(w[5]);
    t_float* phase_vec = (t_float *)(w[6]);
    t_float* out       = (t_float *)(w[7]);
    while(n--){
        t_float freq = *freq_vec++;
        t_float sync = *sync_vec++;
        t_float phase_offset = *phase_vec++;
        t_float pulse_width = *width_vec++;
        if(x->midi)
            freq = pow(2, (freq - 69)/12) * 440;
        x->freq_in_seconds_per_sample = freq / x->sr; // Update frequency
        // Update pulse width, limit between 0 and 1
        x->pulse_width = fmax(fmin(0.9999, pulse_width), 0.0001);
        if(x->soft)
            x->freq_in_seconds_per_sample *= x->soft;
        if(sync > 0 && sync <= 1){ // Phase sync
            if(x->soft)
                x->soft = x->soft == 1 ? -1 : 1;
            else{
                x->phase = sync;
                x->phase = phasewrap(x->phase);
            }
        }
        else{ // Phase modulation
            double phase_dev = phase_offset - x->last_phase_offset;
            if(phase_dev >= 1 || phase_dev <= -1)
                phase_dev = fmod(phase_dev, 1);
            x->phase = phasewrap(x->phase + phase_dev);
        }
        t_float y = sqr(x);
        x->phase += x->freq_in_seconds_per_sample;
        x->phase = phasewrap(x->phase);
        x->last_phase_offset = phase_offset;
        *out++ = y;  // Send to output
    }
    return(w+8);
}

static void blsquare_dsp(t_blsquare *x, t_signal **sp){
    x->x_polyblep.sr = sp[0]->s_sr;
    dsp_add(blsquare_perform, 7, &x->x_polyblep, sp[0]->s_n, sp[0]->s_vec,
        sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
 }

static void blsquare_free(t_blsquare *x){
    inlet_free(x->x_inlet_sync);
    inlet_free(x->x_inlet_phase);
    inlet_free(x->x_inlet_width);
}

static void* blsquare_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_blsquare* x = (t_blsquare *)pd_new(bl_square);
    x->x_polyblep.pulse_width = 0.5;
    x->x_polyblep.freq_in_seconds_per_sample = 0;
    x->x_polyblep.phase = 0.0;
    x->x_polyblep.midi = 0;
    x->x_polyblep.soft = 0;
    t_float init_freq = 0, init_phase = 0;
    while(ac && av->a_type == A_SYMBOL){
        if(atom_getsymbol(av) == gensym("-midi"))
            x->x_polyblep.midi = 1;
        else if(atom_getsymbol(av) == gensym("-soft"))
            x->x_polyblep.soft = 1;
        ac--, av++;
    }
    if(ac && av->a_type == A_FLOAT){
        init_freq = av->a_w.w_float;
        ac--; av++;
        if(ac && av->a_type == A_FLOAT){
            x->x_polyblep.pulse_width = av->a_w.w_float;
            ac--; av++;
        }
        if(ac && av->a_type == A_FLOAT){
            init_phase = av->a_w.w_float;
            ac--; av++;
        }
    }
    x->x_f = init_freq;
    outlet_new(&x->x_obj, &s_signal);
    x->x_inlet_width = inlet_new(&x->x_obj, &x->x_obj.ob_pd,  &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_width, x->x_polyblep.pulse_width);
    x->x_inlet_sync = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
    x->x_inlet_phase = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_phase, init_phase);
    return(void *)x;
}

void setup_bl0x2esquare_tilde(void){
    bl_square = class_new(gensym("bl.square~"), (t_newmethod)blsquare_new,
        (t_method)blsquare_free, sizeof(t_blsquare), 0, A_GIMME, A_NULL);
    CLASS_MAINSIGNALIN(bl_square, t_blsquare, x_f);
    class_addmethod(bl_square, (t_method)blsquare_soft, gensym("soft"), A_DEFFLOAT, 0);
    class_addmethod(bl_square, (t_method)blsquare_midi, gensym("midi"), A_DEFFLOAT, 0);
    class_addmethod(bl_square, (t_method)blsquare_dsp, gensym("dsp"), A_NULL);
}
