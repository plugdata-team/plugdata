/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _tanx {
    t_object x_obj;
    t_inlet *tanx;
    t_outlet *x_outlet;
} t_tanx;

void *tanx_new(void);
static t_int * tanx_perform(t_int *w);
static void tanx_dsp(t_tanx *x, t_signal **sp);

static t_class *tanx_class;

static t_int *tanx_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = tanf(f);  /* CHECKED no protection against NaNs */
    }
    return (w + 4);
}

static void tanx_dsp(t_tanx *x, t_signal **sp)
{
    dsp_add(tanx_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *tanx_new(void)
{
    t_tanx *x = (t_tanx *)pd_new(tanx_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void tanx_tilde_setup(void)
{
    tanx_class = class_new(gensym("tanx~"), (t_newmethod)tanx_new, 0,
                           sizeof(t_tanx), CLASS_DEFAULT, 0);
    class_addmethod(tanx_class, nullfn, gensym("signal"), 0);
    class_addmethod(tanx_class, (t_method) tanx_dsp, gensym("dsp"), A_CANT, 0);
}
