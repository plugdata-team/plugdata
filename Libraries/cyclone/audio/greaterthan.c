// Copyright (c) 2016 Porres.

#include "m_pd.h"
#include <common/api.h>

// ---------------------------------------------------
// Class definition
// ---------------------------------------------------
static t_class *greaterthan_class;

// ---------------------------------------------------
// Data structure definition
// ---------------------------------------------------
typedef struct _greaterthan
{
    t_object x_obj;
    t_inlet  *x_inlet;
} t_greaterthan;

// ---------------------------------------------------
// Perform
// ---------------------------------------------------
static t_int *greaterthan_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *in2 = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while (nblock--)
    {
        t_float f1 = *in1++;
        t_float f2 = *in2++;
        *out++ = (f1 > f2);
    }
    return (w + 5);
}

// ---------------------------------------------------
// DSP Function
// ---------------------------------------------------
static void greaterthan_dsp(t_greaterthan *x, t_signal **sp)
{
    dsp_add(greaterthan_perform, 4, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

// FREE
static void *greaterthan_free(t_greaterthan *x)
{
    inlet_free(x->x_inlet);
    return (void *)x;
}

// ---------------------------------------------------
// Functions signature
// ---------------------------------------------------
static void *greaterthan_new(t_floatarg f)
{
    t_greaterthan *x = (t_greaterthan *)pd_new(greaterthan_class);
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

// ---------------------------------------------------
// Setup
// ---------------------------------------------------
CYCLONE_OBJ_API void greaterthan_tilde_setup(void)
{
    greaterthan_class = class_new(gensym("greaterthan~"),
                             (t_newmethod)greaterthan_new,
                             (t_method)greaterthan_free,
                             sizeof(t_greaterthan), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(greaterthan_class, nullfn, gensym("signal"), 0);
    class_addmethod(greaterthan_class, (t_method)greaterthan_dsp, gensym("dsp"), A_CANT, 0);
}