/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _delta
{
    t_object x_obj;
    t_inlet *delta;
    t_outlet *x_outlet;
    t_float  x_last;
} t_delta;

static t_class *delta_class;

static t_int *delta_perform(t_int *w)
{
    t_delta *x = (t_delta *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float last = x->x_last;
    while (nblock--)
    {
	t_float f = *in++;
    	*out++ = f - last;
	last = f;
    }
    x->x_last = last;
    return (w + 5);
}

static void delta_dsp(t_delta *x, t_signal **sp)
{
    dsp_add(delta_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *delta_new(void)
{
    t_delta *x = (t_delta *)pd_new(delta_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    x->x_last = 0;
    return (x);
}

CYCLONE_OBJ_API void delta_tilde_setup(void)
{
    delta_class = class_new(gensym("delta~"),
			    (t_newmethod)delta_new, 0,
			    sizeof(t_delta), CLASS_DEFAULT, 0);
    class_addmethod(delta_class, nullfn, gensym("signal"), 0);
    class_addmethod(delta_class, (t_method) delta_dsp, gensym("dsp"), A_CANT, 0);
}
