/* Copyright (c) 2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// Updated by Porres in 2016

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _overdrive
{
    t_object x_obj;
    t_inlet  *x_inlet;
    float  x_drivefactor;
} t_overdrive;

static t_class *overdrive_class;

// this is an actual behaviour in Max that was replicated in cyclone before.
// it's a design flaw, redundanct and undocumented in Max and kept for backwards
// compatibilty, but won't be documented anymore (Porres 2016).
static void overdrive_float(t_overdrive *x, t_float f)
{
    x->x_drivefactor = f;
    pd_float((t_pd *)x->x_inlet, x->x_drivefactor);
}

/* CHECKED negative parameter values may cause output to go out of bounds */
static t_int *overdrive_perform(t_int *w)
{
    t_overdrive *x = (t_overdrive *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float df = x->x_drivefactor;
    while (nblock--)
    {
	float f = *in1++;
        df = *in2++;
	if (f >= 1.)  /* CHECKED incompatible (garbage for sig~ 1.) */
	    *out++ = 1.;  /* CHECKED constant for > 1. */
	else if (f > 0.)
	    *out++ = 1. - powf(1. - f, df);  /* CHECKED */
	else if (f > -1.)  /* CHECKED incompatible (garbage for sig~ -1.) */
	    *out++ = powf(1. + f, df) - 1.;  /* CHECKED */
	else
	    *out++ = -1.;  /* CHECKED constant for < -1. */
    }
    return (w + 6);
}

static void overdrive_dsp(t_overdrive *x, t_signal **sp)
{
    dsp_add(overdrive_perform, 5, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

// FREE
static void *overdrive_free(t_overdrive *x)
{
    inlet_free(x->x_inlet);
    return (void *)x;
}

static void *overdrive_new(t_floatarg f)
{
    t_overdrive *x = (t_overdrive *)pd_new(overdrive_class);
    x->x_drivefactor = f;
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, x->x_drivefactor);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void overdrive_tilde_setup(void)
{
    overdrive_class = class_new(gensym("overdrive~"),
				(t_newmethod)overdrive_new, (t_method)overdrive_free,
				sizeof(t_overdrive), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(overdrive_class, nullfn, gensym("signal"), 0);
    class_addmethod(overdrive_class, (t_method)overdrive_dsp, gensym("dsp"), A_CANT, 0);
        class_addfloat(overdrive_class, (t_method)overdrive_float);
}
