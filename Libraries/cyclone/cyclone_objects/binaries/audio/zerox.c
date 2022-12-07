/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

#define ZEROX_DEFVOLUME  1.

typedef struct _zerox
{
    t_object x_obj;
    t_inlet  *x_inlet;
    t_float  x_volume;
    int      x_lastsign;
} t_zerox;

static t_class *zerox_class;

static void zerox_set(t_zerox *x, t_floatarg f)
{
    x->x_volume = f;  /* CHECKED anything goes (including 0.) */
}

static t_int *zerox_perform(t_int *w)
{
    t_zerox *x = (t_zerox *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out1 = (t_float *)(w[4]);
    t_float *out2 = (t_float *)(w[5]);
    t_float volume = x->x_volume;
    int lastsign = x->x_lastsign;
    int i = nblock;
    int count = 0;
    t_float fcount;
    while (i--)
    {
	/* CHECKED -1 -> 0 and 0 -> -1 are hits, 1 -> 0, 0 -> 1 are not */
	int sign = (*in++ < 0 ? 1. : 0.);  /* LATER read the sign bit */
	if (sign != lastsign)
	{
	    count++;
	    *out2++ = volume;
	    lastsign = sign;
	}
	else *out2++ = 0.;
    }
    fcount = (t_float)count;
    while (nblock--) *out1++ = fcount;
    x->x_lastsign = lastsign;
    return (w + 6);
}

static void zerox_dsp(t_zerox *x, t_signal **sp)
{
    dsp_add(zerox_perform, 5, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *zerox_new(t_floatarg f)
{
    t_zerox *x = (t_zerox *)pd_new(zerox_class);
    x->x_volume = (f == 0. ? ZEROX_DEFVOLUME : f);
    x->x_lastsign = 0;  /* CHECKED the very first sample hits if negative */
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void zerox_tilde_setup(void)
{
    zerox_class = class_new(gensym("zerox~"),
			    (t_newmethod)zerox_new, 0,
			    sizeof(t_zerox), 0, A_DEFFLOAT, 0);
    class_addmethod(zerox_class, nullfn, gensym("signal"), 0);
    class_addmethod(zerox_class, (t_method)zerox_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(zerox_class, (t_method)zerox_set,
		    gensym("set"), A_FLOAT, 0);  /* CHECKED arg obligatory */
}
