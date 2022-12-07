/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>


typedef struct _atan {
    t_object x_obj;
    t_inlet *atan;
    t_outlet *x_outlet;
} t_atan;

void *atan_new(void);
static t_int * atan_perform(t_int *w);
static void atan_dsp(t_atan *x, t_signal **sp);

static t_class *atan_class;

static t_int *atan_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = atanf(f);  // CHECKED no protection against NaNs
    }
    return (w + 4);
}

static void atan_dsp(t_atan *x, t_signal **sp)
{
    dsp_add(atan_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *atan_new(void)
{
    t_atan *x = (t_atan *)pd_new(atan_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void atan_tilde_setup(void)
{
    atan_class = class_new(gensym("atan~"), (t_newmethod)atan_new, 0,
                            sizeof(t_atan), CLASS_DEFAULT, 0);
    class_addmethod(atan_class, nullfn, gensym("signal"), 0);
    class_addmethod(atan_class, (t_method) atan_dsp, gensym("dsp"), A_CANT, 0);
}
