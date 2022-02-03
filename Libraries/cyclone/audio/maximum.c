/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// LATER use hasfeeders (why?)

#include "m_pd.h"
#include <common/api.h>

typedef struct _maximum {
    t_object    x_obj;
    t_float     x_input;    //dummy var
    t_inlet    *x_inlet1;  // main 1st inlet
    t_inlet    *x_inlet2;  // 2nd inlet
    t_outlet   *x_outlet;
} t_maximum;

static t_class *maximum_class;

static t_int *maximum_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *in2 = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while (nblock--)
    {
        t_float f1 = *in1++;
        t_float f2 = *in2++;
        *out++ = (f1 > f2 ? f1 : f2);
    }
    return (w + 5);
}

static void maximum_dsp(t_maximum *x, t_signal **sp)
{
    dsp_add(maximum_perform, 4, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *maximum_new(t_floatarg f)
{
    t_maximum *x = (t_maximum *)pd_new(maximum_class);
    x->x_inlet2 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet2, f);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void maximum_tilde_setup(void)
{
    maximum_class = class_new(gensym("maximum~"),
                              (t_newmethod)maximum_new, 0,
                              sizeof(t_maximum), 0, A_DEFFLOAT, 0);
    class_addmethod(maximum_class, (t_method)maximum_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(maximum_class, t_maximum, x_input);
}
