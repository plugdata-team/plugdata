// License: BSD 2 Clause
// Created by Timothy Schoen, based on LabSound polyBLEP oscillators

// Adapted from "Phaseshaping Oscillator Algorithms for Musical Sound
// Synthesis" by Jari Kleimola, Victor Lazzarini, Joseph Timoney, and Vesa
// Valimaki. http://www.acoustics.hut.fi/publications/papers/smc2010-phaseshaping/

#include "m_pd.h"
#include <math.h>
#include <stdint.h>

typedef enum _waveshape
{
    SAWTOOTH,
    SQUARE,
    TRIANGLE
} t_waveshape;

#define PI     3.1415926535897931
#define TWO_PI 6.2831853071795862

typedef struct _polyblep
{
    t_float pulse_width;  // Pulse width for square, morph-to-saw for triangle
    t_float phase;  // The current phase of the oscillator.
    t_float freq_in_seconds_per_sample;
    t_float last_phase_offset;
    t_waveshape shape;
} t_polyblep;

typedef struct blosc
{
    t_object x_obj;
    t_float x_f;
    t_polyblep x_polyblep;
    t_inlet* x_inlet_sync;
    t_inlet* x_inlet_phase;
    t_inlet* x_inlet_width;
} t_blosc;

t_class *bl_oscillators[3];

static int64_t bitwise_or_zero(const t_float x) { return ((int64_t)x) | 0; }

static t_float square(t_float x) { return x * x; }

static t_float blep(t_float phase, t_float dt)
{
    if (phase < dt) return -square(phase / dt - 1);
    else if (phase > 1 - dt)  return square((phase - 1) / dt + 1);
    else return 0.0;
}

static t_float blamp(t_float phase, t_float dt)
{
    if (phase < dt)
    {
        phase = phase / dt - 1.0;
        return -1.0 / 3.0 * square(phase) * phase;
    }
    else if (phase > 1.0 - dt)
    {
        phase = (phase - 1.0) / dt + 1.0;
        return 1.0 / 3.0 * square(phase) * phase;
    }
    else return 0.0;
}

static t_float tri(const t_polyblep* x)
{
    t_float pulse_width = fmax(0.0001, fmin(0.9999, x->pulse_width));
    
    t_float t1 = x->phase + 0.5 * pulse_width;
    t1 -= bitwise_or_zero(t1);
    
    t_float t2 = x->phase + 1 - 0.5 * pulse_width;
    t2 -= bitwise_or_zero(t2);
    
    t_float y = x->phase * 2;
    
    if (y >= 2 - x->pulse_width)
    {
        y = (y - 2) / pulse_width;
    }
    else if (y >= x->pulse_width)
    {
        y = 1 - (y - pulse_width) / (1 - pulse_width);
    }
    else
    {
        y /= pulse_width;
    }
    
    y += x->freq_in_seconds_per_sample / (x->pulse_width - x->pulse_width * x->pulse_width) * (blamp(t1, x->freq_in_seconds_per_sample) - blamp(t2, x->freq_in_seconds_per_sample));
    
    return y;
}

static t_float sqr(const t_polyblep* x)
{
    t_float t2 = x->phase + 0.5;
    t2 -= bitwise_or_zero(t2);
    
    t_float y = x->phase < 0.5 ? 1 : -1;
    y += blep(x->phase, x->freq_in_seconds_per_sample) - blep(t2, x->freq_in_seconds_per_sample);
    
    return y;
}

static t_float saw(const t_polyblep* x)
{
    t_float _t = x->phase;
    _t -= bitwise_or_zero(_t);
    
    t_float y = 1 - 2 * _t;
    y += blep(_t, x->freq_in_seconds_per_sample);
    
    return y;
}

static void blosc_init(t_blosc* x, int ac, t_atom *av) {
    x->x_polyblep.pulse_width = 0.5;
    x->x_polyblep.freq_in_seconds_per_sample = 0;
    x->x_polyblep.phase = 0.0;
    
    t_float init_freq = 0, init_phase = 0;
    if (ac && av->a_type == A_FLOAT){
        init_freq = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT){
            init_phase = av->a_w.w_float;
            ac--; av++;
        }
    }
    x->x_f = init_freq;
    
    // Outlet
    outlet_new(&x->x_obj, &s_signal);
    
    if(x->x_polyblep.shape != SAWTOOTH) {
        // Pulse width inlet
        x->x_inlet_width = inlet_new(&x->x_obj, &x->x_obj.ob_pd,  &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_width, 0.5);
    }
    else {
        x->x_inlet_width = NULL;
    }
    
    // Sync inlet
    x->x_inlet_sync = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
    
    // Phase inlet
    x->x_inlet_phase = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_phase, init_phase);
}

static void* bl_saw_new(t_symbol *s, int ac, t_atom *av)
{
    t_blosc* x = (t_blosc *)pd_new(bl_oscillators[SAWTOOTH]);
    x->x_polyblep.shape = SAWTOOTH;
    
    blosc_init(x, ac, av);
    
    return (void *)x;
}

static void* bl_square_new(t_symbol *s, int ac, t_atom *av)
{
    t_blosc* x = (t_blosc *)pd_new(bl_oscillators[SQUARE]);
    x->x_polyblep.shape = SQUARE;
    
    blosc_init(x, ac, av);
    
    return (void *)x;
}

static void* bl_triangle_new(t_symbol *s, int ac, t_atom *av)
{
    t_blosc* x = (t_blosc *)pd_new(bl_oscillators[TRIANGLE]);
    x->x_polyblep.shape = TRIANGLE;
    
    blosc_init(x, ac, av);
    
    return (void *)x;
}

static t_int* blosc_perform(t_int *w) {
    
    t_polyblep* x  = (t_polyblep*)(w[1]);
    t_int n        = (t_int)(w[2]);
    int is_sawtooth = x->shape == SAWTOOTH;
    
    t_float* freq_vec  = (t_float *)(w[3]);
    t_float* width_vec = (t_float *)(w[4]);
    t_float* sync_vec  = (t_float *)(w[is_sawtooth ? 4 : 5]);
    t_float* phase_vec  = (t_float *)(w[is_sawtooth ? 5 : 6]);
    
    t_float* out      = (t_float *)(w[is_sawtooth ? 6 : 7]);
    
    while (n--) {
        t_float freq  = *freq_vec++;
        t_float sync = *sync_vec++;
        t_float phase_offset = *phase_vec++;
        t_float pulse_width = *width_vec++;
        
        // Update frequency
        x->freq_in_seconds_per_sample = fabs(freq) / sys_getsr();
        
        // Update pulse width, limit between 0 and 1
        x->pulse_width = fmax(fmin(0.99, pulse_width), 0.01);
        
        t_float y;
        switch (x->shape)
        {
            case TRIANGLE:{
                y = tri(x);
                break;
            }
            case SQUARE: {
                y = sqr(x);
                break;
            }
            case SAWTOOTH: {
                y = saw(x);
                break;
            }
            default: y = 0.0;
        }
        
        // Send to output
        *out++ = y;
        
        // Phase sync
        if (sync > 0 && sync <= 1) {
            x->phase = sync;
            if (x->phase >= 0.0) x->phase -= bitwise_or_zero(x->phase);
            else x->phase += 1.0 - bitwise_or_zero(x->phase);
        }
        // Phase modulation
        else {
            double phase_dev = phase_offset - x->last_phase_offset;
            if (phase_dev >= 1 || phase_dev <= -1)
                phase_dev = fmod(phase_dev, 1);
            
            x->phase = x->phase + phase_dev;
            if (x->phase >= 0.0) x->phase -= bitwise_or_zero(x->phase);
            else x->phase += 1.0 - bitwise_or_zero(x->phase);
        }
        
        x->phase += x->freq_in_seconds_per_sample;
        x->phase -= bitwise_or_zero(x->phase);
        
        x->last_phase_offset = phase_offset;
    }
    
    return (w + (is_sawtooth ? 7 : 8));
}

static void blosc_dsp(t_blosc *x, t_signal **sp)
{
    if(x->x_polyblep.shape != SAWTOOTH) {
        dsp_add(blosc_perform, 7, &x->x_polyblep, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
    else {
        dsp_add(blosc_perform, 6, &x->x_polyblep, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
}

static void blosc_free(t_blosc *x)
{
    inlet_free(x->x_inlet_sync);
    inlet_free(x->x_inlet_phase);
    
    if(x->x_inlet_width) {
        inlet_free(x->x_inlet_width);
    }
}

void blosc_tilde_setup(void)
{
    bl_oscillators[SAWTOOTH] = class_new(gensym("bl.saw~"), (t_newmethod)bl_saw_new,
                             (t_method)blosc_free, sizeof(t_blosc), 0, A_GIMME, A_NULL);
    
    bl_oscillators[SQUARE] = class_new(gensym("bl.square~"), (t_newmethod)bl_square_new,
                                (t_method)blosc_free, sizeof(t_blosc), 0, A_GIMME, A_NULL);
    
    bl_oscillators[TRIANGLE] = class_new(gensym("bl.tri~"), (t_newmethod)bl_triangle_new,
                                  (t_method)blosc_free, sizeof(t_blosc), 0, A_GIMME, A_NULL);
    
    for(int i = 0; i < 3; i++) {
        CLASS_MAINSIGNALIN(bl_oscillators[i], t_blosc, x_f);
        class_addmethod(bl_oscillators[i], (t_method)blosc_dsp, gensym("dsp"), A_NULL);
    }
}
