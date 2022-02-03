/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

// magic needed for manual isnan and isinf checks, which don't work
// reliably with -ffast-math compiler option
//#include "magicbit.h"

typedef struct _acos {
    t_object x_obj;
    t_inlet *acos;
    t_outlet *x_outlet;
} t_acos;

void *acos_new(void);
static t_int * acos_perform(t_int *w);
static void acos_dsp(t_acos *x, t_signal **sp);

static t_class *acos_class;

static t_int *acos_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {// CHECKED now there's protection against nan/inf
        float f = *in++;
        f = acosf(f);
        *out++ = (PD_BADFLOAT(f)) ? 0 : f;
    }
    return (w + 4);
}

static void acos_dsp(t_acos *x, t_signal **sp)
{
    dsp_add(acos_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *acos_new(void)
{
    t_acos *x = (t_acos *)pd_new(acos_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void acos_tilde_setup(void)
{
    acos_class = class_new(gensym("acos~"), (t_newmethod)acos_new, 0,
        sizeof(t_acos), CLASS_DEFAULT, 0);
    class_addmethod(acos_class, nullfn, gensym("signal"), 0);
    class_addmethod(acos_class, (t_method) acos_dsp, gensym("dsp"), A_CANT, 0);
}
