/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _tanh {
    t_object x_obj;
    t_inlet *tanh;
    t_outlet *x_outlet;
} t_tanh;

void *tanh_new(void);
static t_int * tanh_perform(t_int *w);
static void tanh_dsp(t_tanh *x, t_signal **sp);

static t_class *tanh_class;

static t_int *tanh_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = tanhf(f);  /* CHECKED no protection against NaNs */
    }
    return (w + 4);
}

static void tanh_dsp(t_tanh *x, t_signal **sp)
{
    dsp_add(tanh_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *tanh_new(void)
{
    t_tanh *x = (t_tanh *)pd_new(tanh_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void tanh_tilde_setup(void)
{
    tanh_class = class_new(gensym("tanh~"), (t_newmethod)tanh_new, 0,
                           sizeof(t_tanh), CLASS_DEFAULT, 0);
    class_addmethod(tanh_class, nullfn, gensym("signal"), 0);
    class_addmethod(tanh_class, (t_method) tanh_dsp, gensym("dsp"), A_CANT, 0);
}
