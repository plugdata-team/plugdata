/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/shared.h"

typedef struct _phasewrap
{
    t_object x_obj;
    t_inlet  *x_inlet;
    int    x_algo;
} t_phasewrap;

static t_class *phasewrap_class;

static t_int *phasewrap_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    t_shared_wrappy wrappy;
    while (nblock--)
    {
	/* FIXME here we have pi -> pi, 3pi -> -pi, -pi -> -pi, -3pi -> pi,
	   while in msp it is pi -> -pi, 3pi -> -pi, -pi -> pi, -3pi -> pi */

	double dnorm = *in++ * (1. / SHARED_2PI);
	wrappy.w_d = dnorm + SHARED_UNITBIT0;
	/* Speeding up the int-to-double conversion below would be nice,
	   but all attempts failed.  Even this is slower (which works only
	   for nonnegative input):

	   wrappy.w_i[SHARED_HIOFFSET] = SHARED_UNITBIT0_HIPART;
	   *out++ = (dnorm - (wrappy.w_d - SHARED_UNITBIT0)) * SHARED_2PI;
	*/
	dnorm -= wrappy.w_i[SHARED_LOWOFFSET];
	*out++ = dnorm * SHARED_2PI;
    }
    return (w + 4);
}

/* This is the slowest algo.  It is slower than fmod in all cases,
   except for input being zero. */
static t_int *phasewrap_perform1(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
	float f = *in++;
	double dnorm;
	if (f < -SHARED_PI)
	{
	    dnorm = (double)f * (1. / SHARED_2PI) + .5;
	    *out++ = (dnorm - (int)dnorm) * SHARED_2PI + SHARED_PI;
	}
	else if (f > SHARED_PI)
	{
	    dnorm = (double)f * (1. / SHARED_2PI) + .5;
	    *out++ = (dnorm - (int)dnorm) * SHARED_2PI - SHARED_PI;
	}
	else *out++ = f;
    }
    return (w + 4);
}

static t_int *phasewrap_perform2(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
	double dnorm = *in++ + SHARED_PI;
	if (dnorm < 0)
	    *out++ = SHARED_PI - fmod(-dnorm, SHARED_2PI);
	else
	    *out++ = fmod(dnorm, SHARED_2PI) - SHARED_PI;
    }
    return (w + 4);
}

static void phasewrap_dsp(t_phasewrap *x, t_signal **sp)
{
    switch (x->x_algo)
    {
    case 1:
	dsp_add(phasewrap_perform1, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
	break;
    case 2:
	dsp_add(phasewrap_perform2, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
	break;
    default:
	dsp_add(phasewrap_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
    }
}

static void phasewrap__algo(t_phasewrap *x, t_floatarg f)
{
    x->x_algo = f;
}

static void *phasewrap_new(t_symbol *s, int ac, t_atom *av)
{
    t_phasewrap *x = (t_phasewrap *)pd_new(phasewrap_class);
    if (s == gensym("_phasewrap1~"))
	x->x_algo = 1;
    else if (s == gensym("_phasewrap2~"))
	x->x_algo = 2;
    else
	x->x_algo = 0;
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void phasewrap_tilde_setup(void)
{
    phasewrap_class = class_new(gensym("phasewrap~"),
				(t_newmethod)phasewrap_new, 0,
				sizeof(t_phasewrap), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)phasewrap_new,
		     gensym("_phasewrap1~"), A_GIMME, 0);
    class_addcreator((t_newmethod)phasewrap_new,
		     gensym("_phasewrap2~"), A_GIMME, 0);
    class_addmethod(phasewrap_class, nullfn, gensym("signal"), 0);
    class_addmethod(phasewrap_class, (t_method)phasewrap_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(phasewrap_class, (t_method)phasewrap__algo,
		    gensym("_algo"), A_FLOAT, 0);
}
