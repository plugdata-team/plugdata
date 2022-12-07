/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* The general two-pole loresator (csound's lp2).  For a nice analysis
   see section 9.1.3 of ``Introduction to Digital Filters'' by Smith. */

/* CHECKME if creation args (or defaults) restored after signal disconnection */

#define _USE_MATH_DEFINES
#include <math.h>
#include "m_pd.h"
#include <common/api.h>

/* CHECKME negative loresance */
/* CHECKME max loresance, esp. at low freqs (watch out, gain not normalized) */
#define LORES_MAXloresANCE  (1. - 1e-20)
#define LORES_MINOMEGA      .0001         /* CHECKME */
#define LORES_MAXOMEGA      M_PI     /* CHECKME */
#define TWO_PI              (M_PI * 2.)

typedef struct _lores
{
    t_object  x_obj;
    t_inlet  *x_freq_inlet;
    t_inlet  *x_q_inlet;
    float  x_srcoef;
    float  x_ynm1;
    float  x_ynm2;
} t_lores;

static t_class *lores_class;

static void lores_clear(t_lores *x)
{
    x->x_ynm1 = x->x_ynm2 = 0.;
}

/* LATER make ready for optional audio-rate modulation
   (separate scalar case routines, use sic_makecostable(), etc.) */
static t_int *lores_perform(t_int *w)
{
    t_lores *x = (t_lores *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float fin0 = *(t_float *)(w[4]);
    t_float rin0 = *(t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    float ynm1 = x->x_ynm1;
    float ynm2 = x->x_ynm2;
    /* CHECKME sampled once per block */
    float omega = fin0 * x->x_srcoef;
    float radius, c1, c2, b0;
    if (omega < LORES_MINOMEGA)
	omega = LORES_MINOMEGA;
    else if (omega > LORES_MAXOMEGA)
	omega = LORES_MAXOMEGA;
    if (rin0 > LORES_MAXloresANCE)
	rin0 = LORES_MAXloresANCE;
    /* radius = pow(base, rin0 - 1), which maps rin0 of 0..1 to radius
       of 1/base..1, where base=exp(.125), and 1/base=.882496902585 */
    radius = expf(rin0 * .125) * .882496902585;
    c1 = 2. * radius * cos(omega);
    c2 = radius * radius;
    b0 = 1. - c1 + c2;
    while (nblock--)
    {
	float yn;
	*out++ = yn = b0 * *xin++ + c1 * ynm1 - c2 * ynm2;
	ynm2 = ynm1;
	ynm1 = yn;
    }
    /* LATER rethink */
    x->x_ynm1 = (PD_BIGORSMALL(ynm1) ? 0. : ynm1);
    x->x_ynm2 = (PD_BIGORSMALL(ynm2) ? 0. : ynm2);
    return (w + 7);
}

static void lores_dsp(t_lores *x, t_signal **sp)
{
    x->x_srcoef = TWO_PI / sp[0]->s_sr;
    lores_clear(x);
    dsp_add(lores_perform, 6, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *lores_new(t_floatarg f1, t_floatarg f2)
{
    t_lores *x = (t_lores *)pd_new(lores_class);
    x->x_srcoef = TWO_PI / sys_getsr();
    x->x_freq_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_freq_inlet, f1);
    x->x_q_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_q_inlet, f2);
    outlet_new((t_object *)x, &s_signal);
    lores_clear(x);
    return (x);
}

CYCLONE_OBJ_API void lores_tilde_setup(void)
{
    lores_class = class_new(gensym("lores~"),
			    (t_newmethod)lores_new, 0,
			    sizeof(t_lores), 0,
			    A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(lores_class, nullfn, gensym("signal"), 0);
    class_addmethod(lores_class, (t_method)lores_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(lores_class, (t_method)lores_clear, gensym("clear"), 0);
}
