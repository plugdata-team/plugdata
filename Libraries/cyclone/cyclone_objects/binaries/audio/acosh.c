/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

// magic needed for manual isnan and isinf checks, which don't work
// reliably with -ffast-math compiler option
//#include "magicbit.h"

typedef struct _acosh {
    t_object x_obj;
    t_inlet *acosh;
    t_outlet *x_outlet;
} t_acosh;

void *acosh_new(void);
static t_int * acosh_perform(t_int *w);
static void acosh_dsp(t_acosh *x, t_signal **sp);

static t_class *acosh_class;

static t_int *acosh_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {// CHECKED now there's protection against nan/inf
        float f = *in++;
        f = acoshf(f);
        *out++ = (PD_BADFLOAT(f)) ? 0 : f;
    }
    return (w + 4);
}

static void acosh_dsp(t_acosh *x, t_signal **sp)
{
    dsp_add(acosh_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *acosh_new(void)
{
    t_acosh *x = (t_acosh *)pd_new(acosh_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void acosh_tilde_setup(void)
{
    acosh_class = class_new(gensym("acosh~"), (t_newmethod)acosh_new, 0,
                           sizeof(t_acosh), CLASS_DEFAULT, 0);
    class_addmethod(acosh_class, nullfn, gensym("signal"), 0);
    class_addmethod(acosh_class, (t_method) acosh_dsp, gensym("dsp"), A_CANT, 0);
}
