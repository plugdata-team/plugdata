/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This filter (cmusic's nres and csound's resonr) originates from
   CMJv6n4 article by Smith & Angell.  See also section 9.6 of
   ``Introduction to Digital Filters'' by Smith. */

/* CHECKME if creation args (or defaults) restored after signal disconnection */

#define _USE_MATH_DEFINES
#include <math.h>
#include "m_pd.h"
#include <common/api.h>

#define RESON_DEFQ      .01
#define RESON_MINQ      1e-20      /* CHECKME */
#define RESON_MINOMEGA  .0001      /* CHECKME */
#define RESON_MAXOMEGA      M_PI     /* CHECKME */
#define TWO_PI              (M_PI * 2.)

typedef struct _reson
{
    t_object  x_obj;
    t_inlet  *x_gain_inlet;
    t_inlet  *x_freq_inlet;
    t_inlet  *x_q_inlet;
    float  x_srcoef;
    float  x_xnm1;
    float  x_xnm2;
    float  x_ynm1;
    float  x_ynm2;
} t_reson;

static t_class *reson_class;

static void reson_clear(t_reson *x)
{
    x->x_xnm1 = x->x_xnm2 = x->x_ynm1 = x->x_ynm2 = 0.;
}

/* LATER make ready for optional audio-rate modulation
   (separate scalar case routines, use sic_makecostable(), etc.) */
static t_int *reson_perform(t_int *w)
{
    t_reson *x = (t_reson *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *gin = (t_float *)(w[4]);
    t_float fin0 = *(t_float *)(w[5]);
    t_float qin0 = *(t_float *)(w[6]);
    t_float *out = (t_float *)(w[7]);
    float xnm1 = x->x_xnm1;
    float xnm2 = x->x_xnm2;
    float ynm1 = x->x_ynm1;
    float ynm2 = x->x_ynm2;
    /* CHECKME sampled once per block */
    float qinv = (qin0 > RESON_MINQ ? -1. / qin0 : (-1. / RESON_MINQ));
    float omega = fin0 * x->x_srcoef;
    float radius, c1, c2, scale;
    if (omega < RESON_MINOMEGA)
	omega = RESON_MINOMEGA;
    else if (omega > RESON_MAXOMEGA)
	omega = RESON_MAXOMEGA;
    radius = expf(omega * qinv);  /* radius < 1 (because omega * qinv < 0) */
    c1 = 2. * radius * cosf(omega);
    c2 = radius * radius;
    scale = 1. - radius;
    while (nblock--)
    {
	float yn, xn = *xin++;
	/* CHECKED gain control */
	*out++ = yn =
	    *gin++ * scale * (xn - radius * xnm2) + c1 * ynm1 - c2 * ynm2;
	xnm2 = xnm1;
	xnm1 = xn;
	ynm2 = ynm1;
	ynm1 = yn;
    }
    x->x_xnm1 = xnm1;
    x->x_xnm2 = xnm2;
    /* LATER rethink */
    x->x_ynm1 = (PD_BIGORSMALL(ynm1) ? 0. : ynm1);
    x->x_ynm2 = (PD_BIGORSMALL(ynm2) ? 0. : ynm2);
    return (w + 8);
}

static void reson_dsp(t_reson *x, t_signal **sp)
{
    x->x_srcoef = TWO_PI / sp[0]->s_sr;
    reson_clear(x);
    dsp_add(reson_perform, 7, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
	    sp[4]->s_vec);
}

static void *reson_new(t_floatarg f1, t_floatarg f2, t_floatarg f3)
{
    t_reson *x = (t_reson *)pd_new(reson_class);
    x->x_srcoef = TWO_PI / sys_getsr();
    if (f1 < 0.) f1 = 0.;
    if (f2 < 0.) f2 = 0.;
    if (f3 <= 0.)
	f3 = RESON_DEFQ;  /* CHECKED */
    x->x_gain_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_gain_inlet, f1);
    x->x_freq_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_freq_inlet, f2);
    x->x_q_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_q_inlet, f3);
    outlet_new((t_object *)x, &s_signal);
    reson_clear(x);
    return (x);
}

CYCLONE_OBJ_API void reson_tilde_setup(void)
{
    reson_class = class_new(gensym("reson~"),
			    (t_newmethod)reson_new, 0,
			    sizeof(t_reson), 0,
			    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(reson_class, nullfn, gensym("signal"), 0);
    class_addmethod(reson_class, (t_method)reson_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(reson_class, (t_method)reson_clear, gensym("clear"), 0);
}
