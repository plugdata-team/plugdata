// Copyright (c) 2016 Porres.

#include "m_pd.h"
#include <common/api.h>

static t_class *rdiv_class;

typedef struct _rdiv
{
    t_object x_obj;
    t_inlet  *x_inlet;
} t_rdiv;

static t_int *rdiv_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *in2 = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while (nblock--)
    {
        t_float f1 = *in1++;
        t_float f2 = *in2++;
        *out++ = (f1 == 0. ? 0. : f2 / f1);
    }
    return (w + 5);
}

static void rdiv_dsp(t_rdiv *x, t_signal **sp)
{
    dsp_add(rdiv_perform, 4, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *rdiv_free(t_rdiv *x)
{
    inlet_free(x->x_inlet);
    return (void *)x;
}

static void *rdiv_new(t_floatarg f)
{
    t_rdiv *x = (t_rdiv *)pd_new(rdiv_class);
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void rdiv_tilde_setup(void)
{
    rdiv_class = class_new(gensym("rdiv~"), (t_newmethod)rdiv_new,
        (t_method)rdiv_free, sizeof(t_rdiv), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(rdiv_class, nullfn, gensym("signal"), 0);
    class_addmethod(rdiv_class, (t_method)rdiv_dsp, gensym("dsp"), A_CANT, 0);
}