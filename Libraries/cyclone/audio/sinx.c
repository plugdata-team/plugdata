/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _sinx {
    t_object x_obj;
    t_inlet *sinx;
    t_outlet *x_outlet;
} t_sinx;

void *sinx_new(void);
static t_int * sinx_perform(t_int *w);
static void sinx_dsp(t_sinx *x, t_signal **sp);

static t_class *sinx_class;

static t_int *sinx_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = sinf(f);  /* CHECKED no protection against NaNs */
    }
    return (w + 4);
}

static void sinx_dsp(t_sinx *x, t_signal **sp)
{
    dsp_add(sinx_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *sinx_new(void)
{
    t_sinx *x = (t_sinx *)pd_new(sinx_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void sinx_tilde_setup(void)
{
    sinx_class = class_new(gensym("sinx~"), (t_newmethod)sinx_new, 0,
                           sizeof(t_sinx), CLASS_DEFAULT, 0);
    class_addmethod(sinx_class, nullfn, gensym("signal"), 0);
    class_addmethod(sinx_class, (t_method) sinx_dsp, gensym("dsp"), A_CANT, 0);
}
