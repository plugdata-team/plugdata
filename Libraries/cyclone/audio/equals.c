// Copyright (c) 2016 Porres.

#include "m_pd.h"
#include <common/api.h>

static t_class *equals_class;

typedef struct _equals
{
    t_object x_obj;
    t_inlet  *x_inlet;
} t_equals;

static t_int *equals_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *in2 = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while (nblock--)
    {
	t_float f1 = *in1++;
	t_float f2 = *in2++;
    *out++ = (f1 == f2);
    }
    return (w + 5);
}

static void equals_dsp(t_equals *x, t_signal **sp)
{
    dsp_add(equals_perform, 4, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

// FREE
static void *equals_free(t_equals *x)
{
    inlet_free(x->x_inlet);
    return (void *)x;
}

static void *equals_new(t_floatarg f)
{
    t_equals *x = (t_equals *)pd_new(equals_class);
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void equals_tilde_setup(void)
{
    equals_class = class_new(gensym("equals~"),
            (t_newmethod)equals_new,
            (t_method)equals_free,
            sizeof(t_equals), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(equals_class, nullfn, gensym("signal"), 0);
    class_addmethod(equals_class, (t_method)equals_dsp, gensym("dsp"), A_CANT, 0);
}