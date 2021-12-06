// Porres 2016

#include "m_pd.h"
#include <math.h>

#define HALF_PI (3.14159265358979323846 * 0.5)

static t_class *pan2_class;

typedef struct _pan2
{
    t_object x_obj;
    t_inlet  *x_panlet;
} t_pan2;

static t_int *pan2_perform(t_int *w)
{
    t_pan2 *x = (t_pan2 *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // in
    t_float *in2 = (t_float *)(w[4]); // pan
    t_float *out1 = (t_float *)(w[5]); // L
    t_float *out2 = (t_float *)(w[6]); // R
    while (nblock--)
    {
        float in = *in1++;
        float pan = *in2++;
        if (pan < -1) pan = -1;
        if (pan > 1) pan = 1;
        pan = (pan + 1) * 0.5;
        *out1++ = in * (pan == 1 ? 0 : cos(pan * HALF_PI));
        *out2++ = in * sin(pan * HALF_PI);
    }
    return (w + 7);
}

static void pan2_dsp(t_pan2 *x, t_signal **sp)
{
    dsp_add(pan2_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *pan2_free(t_pan2 *x)
{
    inlet_free(x->x_panlet);
    return (void *)x;
}

static void *pan2_new(t_floatarg f)
{
    t_pan2 *x = (t_pan2 *)pd_new(pan2_class);
    if(f < -1) f = -1;
    if(f > 1) f = 1;
    x->x_panlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_panlet, f);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

void pan2_tilde_setup(void)
{
    pan2_class = class_new(gensym("pan2~"), (t_newmethod)pan2_new,
        (t_method)pan2_free, sizeof(t_pan2), CLASS_DEFAULT, A_DEFFLOAT, 0);
        class_addmethod(pan2_class, nullfn, gensym("signal"), 0);
        class_addmethod(pan2_class, (t_method)pan2_dsp, gensym("dsp"), A_CANT, 0);
}

