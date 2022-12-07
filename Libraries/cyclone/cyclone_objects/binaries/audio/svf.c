/* Copyright (c) 2003-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Based on Chamberlin's prototype from "Musical Applications of
   Microprocessors" (csound's svfilter).  Slightly distorted,
   no upsampling. */

/* CHECKED scalar case: input preserved (not coefs) after changing mode */
/* CHECKME if creation args (or defaults) restored after signal disconnection */

#define _USE_MATH_DEFINES
#include <math.h>
#include "m_pd.h"
#include <common/api.h>

#define SVF_HZ        0
#define SVF_LINEAR    1
#define SVF_RADIANS   2
#define SVF_DRIVE     .0001
#define SVF_QSTRETCH  1.2   /* CHECKED */
#define SVF_MINR      0.    /* CHECKME */
#define SVF_MAXR      1.2   /* CHECKME */
#define SVF_MINOMEGA  0.    /* CHECKME */
#define SVF_MAXOMEGA  (M_PI * .5)  /* CHECKME */
#define TWO_PI            (M_PI * 2.)

#define SVF_DEFFREQ   0.
#define SVF_DEFQ       .01  /* CHECKME */

typedef struct _svf
{
    t_object x_obj;
    t_inlet *svf;
    t_inlet  *x_freq_inlet;
    t_inlet  *x_q_inlet;
    int    x_mode;
    float  x_srcoef;
    float  x_band;
    float  x_low;
} t_svf;

static t_class *svf_class;

static t_symbol *ps_hz;
static t_symbol *ps_linear;
static t_symbol *ps_radians;

static void svf_clear(t_svf *x)
{
    x->x_band = x->x_low = 0.;
}

static void svf_hz(t_svf *x)
{
    x->x_mode = SVF_HZ;
}

static void svf_linear(t_svf *x)
{
    x->x_mode = SVF_LINEAR;
}

static void svf_radians(t_svf *x)
{
    x->x_mode = SVF_RADIANS;
}

/* LATER make ready for optional audio-rate modulation
   (separate scalar case routines, use sic_makecostable(), etc.) */
static t_int *svf_perform(t_int *w)
{
    t_svf *x = (t_svf *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float fin0 = *(t_float *)(w[4]);
    t_float rin0 = *(t_float *)(w[5]);
    t_float *lout = (t_float *)(w[6]);
    t_float *hout = (t_float *)(w[7]);
    t_float *bout = (t_float *)(w[8]);
    t_float *nout = (t_float *)(w[9]);
    float band = x->x_band;
    float low = x->x_low;
    /* CHECKME sampled once per block */
    float c1, c2;
    float r = (1. - rin0) * SVF_QSTRETCH;  /* CHECKED */
    if (r < SVF_MINR)
	r = SVF_MINR;
    else if (r > SVF_MAXR)
	r = SVF_MAXR;
    c2 = r * r;
    if (x->x_mode == SVF_HZ)
    {
	float omega = fin0 * x->x_srcoef;
	if (omega < SVF_MINOMEGA)
	    omega = SVF_MINOMEGA;
	else if (omega > SVF_MAXOMEGA)
	    omega = SVF_MAXOMEGA;
	c1 = sinf(omega);
	/* CHECKED irs slightly drift apart at high omega, LATER investigate */
    }
    else if (x->x_mode == SVF_LINEAR)
	c1 = sinf(fin0 * (M_PI * .5));  /* CHECKME actual range of fin0 */
    else
	c1 = fin0;  /* CHECKME range */
    while (nblock--)
    {
	float high, xn = *xin++;
	*lout++ = low = low + c1 * band;
	*hout++ = high = xn - low - c2 * band;
	*bout++ = band = c1 * high + band;
	*nout++ = low + high;
	band -= band * band * band * SVF_DRIVE;
    }
    /* LATER rethink */
    x->x_band = (PD_BIGORSMALL(band) ? 0. : band);
    x->x_low = (PD_BIGORSMALL(low) ? 0. : low);
    return (w + 10);
}

static void svf_dsp(t_svf *x, t_signal **sp)
{
    x->x_srcoef = TWO_PI / sp[0]->s_sr;
    svf_clear(x);
    dsp_add(svf_perform, 9, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
	    sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec);
}

static void *svf_new(t_symbol *s, int ac, t_atom *av)
{
    t_svf *x = (t_svf *)pd_new(svf_class);
    t_float freq = SVF_DEFFREQ, qcoef = SVF_DEFQ;
    t_symbol *modesym = 0;
    int i;
    for (i = 0; i < ac; i++) if (av[i].a_type == A_SYMBOL)
    {
	modesym = av[i].a_w.w_symbol;
	break;
    }
    while (ac && av->a_type != A_FLOAT) ac--, av++;
    if (ac)
    {
	freq = av->a_w.w_float;
	ac--; av++;
	while (ac && av->a_type != A_FLOAT) ac--, av++;
	if (ac)
	    qcoef = av->a_w.w_float;
    }
    x->x_srcoef = M_PI / sys_getsr();
//  sic_newinlet((t_sic *)x, freq);
//  sic_newinlet((t_sic *)x, qcoef);
    x->x_freq_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_freq_inlet, freq);
    x->x_q_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_q_inlet, qcoef);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    svf_clear(x);
    if (modesym == ps_linear)
	x->x_mode = SVF_LINEAR;
    else if (modesym == ps_radians)
	x->x_mode = SVF_RADIANS;
    else
    {
	x->x_mode = SVF_HZ;
	if (modesym && modesym != &s_ &&
	    modesym != ps_hz && modesym != gensym("Hz"))
	{
	    /* CHECKED no warning */
	}
    }
    return (x);
}

CYCLONE_OBJ_API void svf_tilde_setup(void)
{
    ps_hz = gensym("hz");
    ps_linear = gensym("linear");
    ps_radians = gensym("radians");
    svf_class = class_new(gensym("svf~"),
			  (t_newmethod)svf_new, 0,
			  sizeof(t_svf), 0, A_GIMME, 0);
    class_addmethod(svf_class, nullfn, gensym("signal"), 0);
    class_addmethod(svf_class, (t_method)svf_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(svf_class, (t_method)svf_clear, gensym("clear"), 0);
    class_addmethod(svf_class, (t_method)svf_hz, ps_hz, 0);
    class_addmethod(svf_class, (t_method)svf_hz, gensym("Hz"), 0);
    class_addmethod(svf_class, (t_method)svf_linear, ps_linear, 0);
    class_addmethod(svf_class, (t_method)svf_radians, ps_radians, 0);
}
