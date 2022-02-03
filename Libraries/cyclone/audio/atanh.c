/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

// magic needed for manual isnan and isinf checks, which don't work
// reliably with -ffast-math compiler option
//#include "magicbit.h"

typedef struct _atanh {
    t_object x_obj;
    t_inlet *atanh;
    t_outlet *x_outlet;
} t_atanh;

void *atanh_new(void);
static t_int * atanh_perform(t_int *w);
static void atanh_dsp(t_atanh *x, t_signal **sp);

static t_class *atanh_class;

static t_int *atanh_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {// CHECKED now there's protection against nan/inf
        float f = *in++;
        f = atanhf(f);
        *out++ = (PD_BADFLOAT(f)) ? 0 : f;
    }
    return (w + 4);
}

static void atanh_dsp(t_atanh *x, t_signal **sp)
{
    dsp_add(atanh_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *atanh_new(void)
{
    t_atanh *x = (t_atanh *)pd_new(atanh_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void atanh_tilde_setup(void)
{
    atanh_class = class_new(gensym("atanh~"), (t_newmethod)atanh_new, 0,
                           sizeof(t_atanh), CLASS_DEFAULT, 0);
    class_addmethod(atanh_class, nullfn, gensym("signal"), 0);
    class_addmethod(atanh_class, (t_method) atanh_dsp, gensym("dsp"), A_CANT, 0);
}
