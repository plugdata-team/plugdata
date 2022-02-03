/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _cosx {
    t_object x_obj;
    t_inlet *cosx;
    t_outlet *x_outlet;
} t_cosx;

void *cosx_new(void);
static t_int * cosx_perform(t_int *w);
static void cosx_dsp(t_cosx *x, t_signal **sp);

static t_class *cosx_class;

static t_int *cosx_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = cosf(f);  /* CHECKED no protection against NaNs */
    }
    return (w + 4);
}

static void cosx_dsp(t_cosx *x, t_signal **sp)
{
    dsp_add(cosx_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *cosx_new(void)
{
    t_cosx *x = (t_cosx *)pd_new(cosx_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void cosx_tilde_setup(void)
{
    cosx_class = class_new(gensym("cosx~"), (t_newmethod)cosx_new, 0,
                           sizeof(t_cosx), CLASS_DEFAULT, 0);
    class_addmethod(cosx_class, nullfn, gensym("signal"), 0);
    class_addmethod(cosx_class, (t_method) cosx_dsp, gensym("dsp"), A_CANT, 0);
}
