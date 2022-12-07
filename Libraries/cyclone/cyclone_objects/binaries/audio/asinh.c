/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>


typedef struct _asinh {
    t_object x_obj;
    t_inlet *asinh;
    t_outlet *x_outlet;
} t_asinh;

void *asinh_new(void);
static t_int * asinh_perform(t_int *w);
static void asinh_dsp(t_asinh *x, t_signal **sp);

static t_class *asinh_class;

static t_int *asinh_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = asinhf(f);  /* CHECKED no protection against NaNs */
    }
    return (w + 4);
}

static void asinh_dsp(t_asinh *x, t_signal **sp)
{
    dsp_add(asinh_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *asinh_new(void)
{
    t_asinh *x = (t_asinh *)pd_new(asinh_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void asinh_tilde_setup(void)
{
    asinh_class = class_new(gensym("asinh~"), (t_newmethod)asinh_new, 0,
                           sizeof(t_asinh), CLASS_DEFAULT, 0);
    class_addmethod(asinh_class, nullfn, gensym("signal"), 0);
    class_addmethod(asinh_class, (t_method) asinh_dsp, gensym("dsp"), A_CANT, 0);
}
