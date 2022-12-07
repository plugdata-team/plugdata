// Copyright (c) 2016 Porres.

#include "m_pd.h"
#include <common/api.h>
#include "math.h"

// ---------------------------------------------------
// Class definition
// ---------------------------------------------------
static t_class *modulo_class;

// ---------------------------------------------------
// Data structure definition
// ---------------------------------------------------
typedef struct _modulo
{
    t_object x_obj;
    t_inlet  *x_inlet;
} t_modulo;

// ---------------------------------------------------
// Perform
// ---------------------------------------------------
static t_int *modulo_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *in2 = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while (nblock--)
    {
        t_float f1 = *in1++;
        t_float f2 = *in2++;
        *out++ = (f2 == 0. ? 0. : fmod(f1, f2));
    }
    return (w + 5);
}

// ---------------------------------------------------
// DSP Function
// ---------------------------------------------------
static void modulo_dsp(t_modulo *x, t_signal **sp)
{
    dsp_add(modulo_perform, 4, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

// FREE
static void *modulo_free(t_modulo *x)
{
    inlet_free(x->x_inlet);
    return (void *)x;
}

// ---------------------------------------------------
// Functions signature
// ---------------------------------------------------
static void *modulo_new(t_floatarg f)
{
    t_modulo *x = (t_modulo *)pd_new(modulo_class);
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

// ---------------------------------------------------
// Setup
// ---------------------------------------------------
CYCLONE_OBJ_API void modulo_tilde_setup(void)
{
    modulo_class = class_new(gensym("modulo~"),
                             (t_newmethod)modulo_new,
                             (t_method)modulo_free,
                             sizeof(t_modulo), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(modulo_class, nullfn, gensym("signal"), 0);
    class_addmethod(modulo_class, (t_method)modulo_dsp, gensym("dsp"), A_CANT, 0);
}