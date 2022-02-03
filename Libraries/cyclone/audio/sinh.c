/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _sinh {
    t_object x_obj;
    t_inlet *sinh;
    t_outlet *x_outlet;
} t_sinh;

void *sinh_new(void);
static t_int * sinh_perform(t_int *w);
static void sinh_dsp(t_sinh *x, t_signal **sp);

static t_class *sinh_class;

static t_int *sinh_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = sinhf(f);  /* CHECKED no protection against NaNs */
    }
    return (w + 4);
}

static void sinh_dsp(t_sinh *x, t_signal **sp)
{
    dsp_add(sinh_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *sinh_new(void)
{
    t_sinh *x = (t_sinh *)pd_new(sinh_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void sinh_tilde_setup(void)
{
    sinh_class = class_new(gensym("sinh~"), (t_newmethod)sinh_new, 0,
                           sizeof(t_sinh), CLASS_DEFAULT, 0);
    class_addmethod(sinh_class, nullfn, gensym("signal"), 0);
    class_addmethod(sinh_class, (t_method) sinh_dsp, gensym("dsp"), A_CANT, 0);
}
