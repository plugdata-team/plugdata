/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// fixed and rewritten by Porres

#include "m_pd.h"
#include <common/api.h>

#define TRAIN_DEFPERIOD  1000
#define TRAIN_DEFWIDTH   0.5
#define TRAIN_DEFOFFSET  0

typedef struct _train
{
    t_object    x_obj;
    t_float     x_input;
    t_int     x_start;
    t_inlet    *x_width_inlet;
    t_inlet    *x_offset_inlet;
    double       x_phase;
    t_float       x_sr;
    t_outlet   *x_bangout;
    t_clock    *x_clock;
} t_train;

static t_class *train_class;

static void train_tick(t_train *x)
{
    outlet_bang(x->x_bangout);
}

static t_int *train_perform(t_int *w)
{
    t_train *x = (t_train *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float sr = x->x_sr;
    double phase = x->x_phase;
    double phase_step, wrap_low, wrap_high;
    t_float period, phase_offset, width;
    while (nblock--) {
        period = *in1++;
        phase_step = period > 0 ? (double)1000. / ((double)sr * (double)period) : 0.5;
        if (phase_step > 0.5)
            phase_step = 0.5; // smallest period corresponds to nyquist
        width = *in2++;
        if (width < 0.)
            width = 0.;
        if (width > 1.)
            width = 1.;
        phase_offset = *in3++;
        if (phase_offset < 0.)
            phase_offset = 0.;
        if (phase_offset > 1.)
            phase_offset = 1.;
        wrap_low = (double)phase_offset;
        wrap_high = (double)phase_offset + 1.;
       if (phase < wrap_low)
            *out++ = 0.;
       else if (phase >= wrap_low && x->x_start)
            { // force 1st bang & 1st sample = 1 (even when width = 0)
                *out++ = 1.;
                clock_delay(x->x_clock, 0);
                x->x_start = 0;
            }
       else if (phase >= wrap_high) {
            *out++ = 1.; // 1st always = 1
            clock_delay(x->x_clock, 0);
            phase = (phase - wrap_high) + wrap_low; // wrapped phase
            }
        else if(phase + phase_step >= wrap_high)
            *out++ = 0.; // last always = 0
        else
            *out++ = phase < (width + phase_offset);
//        if (phase >= 1) phase = phase - 1;
//        *out++ = phase;
        phase = phase + phase_step; // next phase (unwrapped)
        }
    x->x_phase = phase;
    return (w + 7);
}

static void train_dsp(t_train *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr; // sample rate
    dsp_add(train_perform, 6, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void train_free(t_train *x)
{
    if (x->x_clock) clock_free(x->x_clock);
}

static void *train_new(t_symbol *s, int argc, t_atom *argv)
{
    t_train *x = (t_train *)pd_new(train_class);
    t_float arg_period, arg_width, arg_offset;
    arg_period = TRAIN_DEFPERIOD;
    arg_width = TRAIN_DEFWIDTH;
    arg_offset = TRAIN_DEFOFFSET;

    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0,argc,argv);
            switch(argnum){
                case 0:
                    arg_period = argval;
                    break;
                case 1:
                    arg_width = argval;
                    break;
                case 2:
                    arg_offset = argval;
                    break;
                default:
                    break;
            };
            
            argc--;
            argv++;
            argnum++;
        }
        else{
            goto errstate;
        };
    };
    x->x_phase = 0;
    x->x_start = 1;
    x->x_input = arg_period;
    x->x_width_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_width_inlet, arg_width);
    x->x_offset_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_offset_inlet, arg_offset);
    outlet_new((t_object *)x, &s_signal);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    x->x_clock = clock_new(x, (t_method)train_tick);
    return (x);
errstate:
    pd_error(x, "train~: improper args");
    return NULL;
}

CYCLONE_OBJ_API void train_tilde_setup(void)
{
    train_class = class_new(gensym("train~"), (t_newmethod)train_new,
            (t_method)train_free, sizeof(t_train), 0, A_GIMME, 0);
    class_addmethod(train_class, (t_method)train_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(train_class, t_train, x_input);
}
