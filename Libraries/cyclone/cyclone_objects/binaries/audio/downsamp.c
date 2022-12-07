// Porres 2016

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

static t_class *downsamp_class;

typedef struct _downsamp
{
    t_object x_obj;
    t_float  x_phase;
    t_float  x_lastout;
    t_inlet  *x_inlet;
} t_downsamp;

static t_int *downsamp_perform(t_int *w)
{
    t_downsamp *x = (t_downsamp *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float phase = x->x_phase;
    t_float lastout = x->x_lastout;
while (nblock--)
{
    float input = *in1++;
    float phase_step;
    float samples = *in2++;
    phase_step = 1. / (samples > 1. ? samples : 1.);
        if (phase >= 1.)
        lastout = input;
        *out++ = lastout;
    phase = fmod(phase, 1.) + phase_step;
}
x->x_phase = phase;
x->x_lastout = lastout;
return (w + 6);
}

static void downsamp_dsp(t_downsamp *x, t_signal **sp)
{
    dsp_add(downsamp_perform, 5, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *downsamp_free(t_downsamp *x)
{
    inlet_free(x->x_inlet);
    return (void *)x;
}

static void *downsamp_new(t_floatarg f)
{
    t_downsamp *x = (t_downsamp *)pd_new(downsamp_class);
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, (f > 1. ? f : 1.));
    outlet_new((t_object *)x, &s_signal);
    x->x_lastout = 0;
    x->x_phase = 0;
    return (x);
}

CYCLONE_OBJ_API void downsamp_tilde_setup(void)
{
    downsamp_class = class_new(gensym("downsamp~"),
        (t_newmethod)downsamp_new,
        (t_method)downsamp_free,
        sizeof(t_downsamp),
        CLASS_DEFAULT,
        A_DEFFLOAT,
        0);
        class_addmethod(downsamp_class, nullfn, gensym("signal"), 0);
        class_addmethod(downsamp_class, (t_method)downsamp_dsp, gensym("dsp"), A_CANT, 0);
}
