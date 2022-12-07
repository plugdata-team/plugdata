/* Copyright (c) 2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is Pd's lop~ with signal-controlled cutoff. */

/* CHECKED scalar case: input preserved (not coefs) after changing mode */
/* CHECKME if creation arg (or a default) restored after signal disconnection */

#define _USE_MATH_DEFINES
#include <math.h>
#include "m_pd.h"
#include <common/api.h>

#define ONEPOLE_HZ        0
#define ONEPOLE_LINEAR    1
#define ONEPOLE_RADIANS   2
#define ONEPOLE_MINB0     .0001  /* CHECKED 1st term of ir for b0=0 */
#define ONEPOLE_MAXB0     .99    /* CHECKED 1st term of ir for b0=1 */
#define ONEPOLE_MINOMEGA  0.     /* CHECKME */
#define ONEPOLE_MAXOMEGA  (M_PI * .5)  // later remove
#define TWO_PI            (M_PI * 2.)

typedef struct _onepole
{
    t_object x_obj;
    t_inlet  *x_inlet;
    int    x_mode;
    float  x_srcoef;
    float  x_ynm1;
} t_onepole;

static t_class *onepole_class;

static t_symbol *ps_hz;
static t_symbol *ps_linear;
static t_symbol *ps_radians;

static void onepole_clear(t_onepole *x)
{
    x->x_ynm1 = 0.;
}

static void onepole_hz(t_onepole *x)
{
    x->x_mode = ONEPOLE_HZ;
}

static void onepole_linear(t_onepole *x)
{
    x->x_mode = ONEPOLE_LINEAR;
}

static void onepole_radians(t_onepole *x)
{
    x->x_mode = ONEPOLE_RADIANS;
}

/* LATER make ready for optional audio-rate modulation
   (separate scalar case routine, use sic_makecostable(), etc.) */
static t_int *onepole_perform(t_int *w)
{
    t_onepole *x = (t_onepole *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float fin0 = *(t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    int mode = x->x_mode;
    float ynm1 = x->x_ynm1;
    /* CHECKME sampled once per block */
    float b0;
    if (mode == ONEPOLE_HZ)
    {
	float omega = fin0 * x->x_srcoef;
	if (omega < ONEPOLE_MINOMEGA)
	    omega = ONEPOLE_MINOMEGA;
	else if (omega > ONEPOLE_MAXOMEGA)
	    omega = ONEPOLE_MAXOMEGA;
	/* The actual solution for a half-power cutoff is:
	   b0 = sqrt(sqr(2-cos(omega))-1) + cos(omega) - 1.
	   The sin(omega) below is only slightly better approximation than
	   Miller's b0=omega, except for the two highest octaves (or so),
	   where it is much better (but far from good). */
	b0 = sinf(omega);
    }
    else if (mode == ONEPOLE_LINEAR)
	b0 = sinf(fin0 * (M_PI * .5));  /* CHECKME actual range of fin0 */
    else
	b0 = fin0;
    if (b0 < ONEPOLE_MINB0)
	b0 = ONEPOLE_MINB0;
    else if (b0 > ONEPOLE_MAXB0)
	b0 = ONEPOLE_MAXB0;
    /* b0 is the standard 1-|a1| (where |a1| is pole's radius),
       specifically: a1=b0-1 => a1 in [-.9999 .. -.01] => lowpass (stable) */
    while (nblock--)
	*out++ = ynm1 = b0 * (*xin++ - ynm1) + ynm1;
    x->x_ynm1 = (PD_BIGORSMALL(ynm1) ? 0. : ynm1);
    return (w + 6);
}

static void onepole_dsp(t_onepole *x, t_signal **sp)
{
    x->x_srcoef = TWO_PI / sp[0]->s_sr;
    onepole_clear(x);
    dsp_add(onepole_perform, 5, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}


static void *onepole_new(t_symbol *s, int ac, t_atom *av)
{
    t_onepole *x = (t_onepole *)pd_new(onepole_class);
    t_float f = 1;
    t_symbol *sym;
    switch(ac)
    {
        default:
        case 2:
            sym=atom_getsymbol(av+1);
        case 1:
            f=atom_getfloat(av);
            break;
        case 0:
            break;
    }
    
    x->x_srcoef = TWO_PI / sys_getsr();
    /* CHECKED no int-to-float conversion (any int bashed to 0.) */
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    onepole_clear(x);
    if (sym == ps_linear)
	x->x_mode = ONEPOLE_LINEAR;
    else if (sym == ps_radians)
	x->x_mode = ONEPOLE_RADIANS;
    else
    {
	x->x_mode = ONEPOLE_HZ;
	if (s && s != &s_ && s != ps_hz && s != gensym("Hz"))
	{
	    /* CHECKED no warning */
	}
    }
    return (x);
}

// anything for hz?
CYCLONE_OBJ_API void onepole_tilde_setup(void)
{
    ps_hz = gensym("hz");
    ps_linear = gensym("linear");
    ps_radians = gensym("radians");
    onepole_class = class_new(gensym("onepole~"), (t_newmethod)onepole_new, 0,
        sizeof(t_onepole), 0, A_GIMME, 0);
    class_addmethod(onepole_class, nullfn, gensym("signal"), 0);
    class_addmethod(onepole_class, (t_method)onepole_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(onepole_class, (t_method)onepole_clear, gensym("clear"), 0);
    class_addmethod(onepole_class, (t_method)onepole_hz, ps_hz, 0);
    class_addmethod(onepole_class, (t_method)onepole_hz, gensym("Hz"), 0);
    class_addmethod(onepole_class, (t_method)onepole_linear, ps_linear, 0);
    class_addmethod(onepole_class, (t_method)onepole_radians, ps_radians, 0);
}
