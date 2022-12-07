/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _cosh {
    t_object x_obj;
    t_inlet *cosh;
    t_outlet *x_outlet;
} t_cosh;

void *cosh_new(void);
static t_int * cosh_perform(t_int *w);
static void cosh_dsp(t_cosh *x, t_signal **sp);

static t_class *cosh_class;

static t_int *cosh_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        *out++ = coshf(f);  /* CHECKED no protection against NaNs */
    }
    return (w + 4);
}

static void cosh_dsp(t_cosh *x, t_signal **sp)
{
    dsp_add(cosh_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *cosh_new(void)
{
    t_cosh *x = (t_cosh *)pd_new(cosh_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void cosh_tilde_setup(void)
{
    cosh_class = class_new(gensym("cosh~"), (t_newmethod)cosh_new, 0,
                            sizeof(t_cosh), CLASS_DEFAULT, 0);
    class_addmethod(cosh_class, nullfn, gensym("signal"), 0);
    class_addmethod(cosh_class, (t_method) cosh_dsp, gensym("dsp"), A_CANT, 0);
}
