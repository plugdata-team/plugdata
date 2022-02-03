/* Copyright (c) 2004 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

/* CHECKED apparently c74's formula has not been carefully tuned (yet?).
   It has 5% deviation from the straight line for ccinput = 0 at half-domain,
   range 1, and generates nans for ccinput > .995 (cf comment in clc.h). */


// updated 2016 by Derek Kwan to change arg specification to A_GIMME
// and add defaults for initial value and curve param.
// also adding factor message that calls curve_factor (new method)


//NOTE FOR CURVE_COEFS TRANSPLANTED FROM CLC.C


/* Problem:  find a function f : p -> q (where p is user's curve control
   parameter, q is log factor) such that the curves will bend in
   a semi-linear way over the p's range of 0..1.  The curve function is
   then g(x, p) = (exp(f(p) * x) - 1) / (exp(f(p)) - 1), where x is
   curve's domain.  If, for example, the points g(0.5, p) are to make
   a semi-linear pattern, then the solution is a function f that minimizes
   the integral of the error function e(p) = sqr(((1-p)/2)-g(.5, p))
   over 0..1.  Until someone does this analytically, we are left with
   a lame formula, which has been tweaked and tested in gnuplot:
   f(p) = h(p) / (1 - h(p)), where h(p) = (((p + 1e-20) * 1.2) ** .41) * .91.
   The file curve.gp, in the sickle's source directory, may come handy,
   in case there is anyone, who fancy tweaking it even further
   (only in very old versions now, pre 0.3)

   To implement this, start from these equations:
     nhops = npoints - 1
     bb * mm ^ nhops = bb + 1
     (bb ^ 2) * (mm ^ nhops) = ((exp(ff/2) - 1) / (exp(ff) - 1)) ^ 2

   and calculate:
     hh = pow(((p + c1) * c2), c3) * c4
     ff = hh / (1 - hh)
     eff = exp(ff) - 1
     gh = (exp(ff * .5) - 1) / eff
     bb = gh * (gh / (1 - (gh + gh)))
     mm = ((exp(ff * (1/nhops)) - 1) / (eff * bb)) + 1

   The loop is:
     for (vv = bb, i = 0; i <= nhops; vv *= mm, i++)
         result = (vv - bb) * (y1 - y0) + y0
   where y0, y1 are start and destination values

   This formula generates curves with < .000004% deviation from the straight
   line for p = 0 at half-domain, range 1.  There are no nans for -1 <= p <= 1.
*/


//definitions for curve-coefs
#define CURVE_C1   1e-20
#define CURVE_C2   1.2
#define CURVE_C3   0.41
#define CURVE_C4   0.91

#define PDCYCURVEINITVAL 0.
#define PDCYCURVEPARAM 0.
#define CURVE_MAXSIZE  42

typedef struct _curveseg
{
    float   s_target;
    float   s_delta;
    int     s_nhops;
    float   s_ccinput;
    double  s_bb;
    double  s_mm;
} t_curveseg;

typedef struct _curve
{
    t_object    x_obj;
    float       x_value;
    float       x_ccinput;
    float       x_target;
    float       x_delta;
    int         x_deltaset;
    double      x_vv;
    double      x_bb;
    double      x_mm;
    float       x_y0;
    float       x_dy;
    float       x_ksr;
    int         x_nleft;
    int         x_retarget;
    int         x_size;   /* as allocated */
    int         x_nsegs;  /* as used */
    int         x_pause;
    t_curveseg  *x_curseg;
    t_curveseg  *x_segs;
    t_curveseg  x_segini[CURVE_MAXSIZE];
    t_clock     *x_clock;
    t_outlet    *x_bangout;
} t_curve;

static t_class *curve_class;
static double curve_coef;


static void curve_coefs(int nhops, double crv, double *bbp, double *mmp)
{
    if (nhops > 0)
    {
	double hh, ff, eff, gh;
	if (crv < 0)
	{
	    if (crv < -1.)
		crv = -1.;
	    hh = pow(((CURVE_C1 - crv) * CURVE_C2), CURVE_C3)
		* CURVE_C4;
	    ff = hh / (1. - hh);
	    eff = exp(ff) - 1.;
	    gh = (exp(ff * .5) - 1.) / eff;
	    *bbp = gh * (gh / (1. - (gh + gh)));
	    *mmp = 1. / (((exp(ff * (1. / (double)nhops)) - 1.) /
			  (eff * *bbp)) + 1.);
	    *bbp += 1.;
	}
	else
	{
	    if (crv > 1.)
		crv = 1.;
	    hh = pow(((crv + CURVE_C1) * CURVE_C2), CURVE_C3)
		* CURVE_C4;
	    ff = hh / (1. - hh);
	    eff = exp(ff) - 1.;
	    gh = (exp(ff * .5) - 1.) / eff;
	    *bbp = gh * (gh / (1. - (gh + gh)));
	    *mmp = ((exp(ff * (1. / (double)nhops)) - 1.) /
		    (eff * *bbp)) + 1.;
	}
    }
    else if (crv < 0)
	*bbp = 2., *mmp = 1.;
    else
	*bbp = *mmp = 1.;
}


static void curve_cc(t_curve *x, t_curveseg *segp, float f)
{
    int nhops = segp->s_delta * x->x_ksr + 0.5;  /* LATER rethink */
    segp->s_ccinput = f;
    segp->s_nhops = (nhops > 0 ? nhops : 0);
    curve_coefs(segp->s_nhops, (double)f, &segp->s_bb, &segp->s_mm);
}


static void curve_tick(t_curve *x)
{
    outlet_bang(x->x_bangout);
}

static t_int *curve_perform(t_int *w)
{
    t_curve *x = (t_curve *)(w[1]);
    t_float *out = (t_float *)(w[2]);
    int nblock = (int)(w[3]);
    int nxfer = x->x_nleft;
    float curval = x->x_value;
    double vv = x->x_vv;
    double bb = x->x_bb;
    double mm = x->x_mm;
    float dy = x->x_dy;
    float y0 = x->x_y0;
    if (x->x_pause)
    {
        while (nblock--) *out++ = curval;
        return (w + 4);
    }
    if (PD_BIGORSMALL(curval))  /* LATER rethink */
	curval = x->x_value = 0;
retarget:
    if (x->x_retarget)
    {
	float target = x->x_curseg->s_target;
	float delta = x->x_curseg->s_delta;
    	int nhops = x->x_curseg->s_nhops;
	bb = x->x_curseg->s_bb;
	mm = x->x_curseg->s_mm;
	if (x->x_curseg->s_ccinput < 0)
	    dy = x->x_value - target;
	else
	    dy = target - x->x_value;
	x->x_nsegs--;
	x->x_curseg++;
    	while (nhops <= 0)
	{
	    curval = x->x_value = target;
	    if (x->x_nsegs)
	    {
		target = x->x_curseg->s_target;
		delta = x->x_curseg->s_delta;
		nhops = x->x_curseg->s_nhops;
		bb = x->x_curseg->s_bb;
		mm = x->x_curseg->s_mm;
		if (x->x_curseg->s_ccinput < 0)
		    dy = x->x_value - target;
		else
		    dy = target - x->x_value;
		x->x_nsegs--;
		x->x_curseg++;
	    }
	    else
	    {
		while (nblock--) *out++ = curval;
		x->x_nleft = 0;
		clock_delay(x->x_clock, 0);
		x->x_retarget = 0;
		return (w + 4);
	    }
	}
    	nxfer = x->x_nleft = nhops;
	x->x_vv = vv = bb;
	x->x_bb = bb;
	x->x_mm = mm;
	x->x_dy = dy;
	x->x_y0 = y0 = x->x_value;
	x->x_target = target;
    	x->x_retarget = 0;
    }
    if (nxfer >= nblock)
    {
	int silly = ((x->x_nleft -= nblock) == 0);  /* LATER rethink */
    	while (nblock--)
	{
	    *out++ = curval = (vv - bb) * dy + y0;
	    vv *= mm;
	}
	if (silly)
	{
	    if (x->x_nsegs) x->x_retarget = 1;
	    else
	    {
		clock_delay(x->x_clock, 0);
	    }
	    x->x_value = x->x_target;
	}
	else
	{
	    x->x_value = curval;
	    x->x_vv = vv;
	}
    }
    else if (nxfer > 0)
    {
	nblock -= nxfer;
	do
	    *out++ = (vv - bb) * dy + y0, vv *= mm;
	while (--nxfer);
	curval = x->x_value = x->x_target;
	if (x->x_nsegs)
	{
	    x->x_retarget = 1;
	    goto retarget;
	}
	else
	{
	    while (nblock--) *out++ = curval;
	    x->x_nleft = 0;
	    clock_delay(x->x_clock, 0);
	}
    }
    else while (nblock--) *out++ = curval;
    return (w + 4);
}

static void curve_float(t_curve *x, t_float f)
{
    if (x->x_deltaset)
    {
    	x->x_deltaset = 0;
    	x->x_target = f;
	x->x_nsegs = 1;
	x->x_curseg = x->x_segs;
	x->x_curseg->s_target = f;
	x->x_curseg->s_delta = x->x_delta;
	curve_cc(x, x->x_curseg, x->x_ccinput);
    	x->x_retarget = 1;
    }
    else
    {
    	x->x_value = x->x_target = f;
	x->x_nsegs = 0;
	x->x_curseg = 0;
    	x->x_nleft = 0;
	x->x_retarget = 0;
    }
    x->x_pause = 0;
}

/* CHECKED delta is not persistent, but ccinput is */
static void curve_ft1(t_curve *x, t_floatarg f)
{
    x->x_delta = f;
    x->x_deltaset = (f > 0);
}

static void curve_list(t_curve *x, t_symbol *s, int ac, t_atom *av)
{
    int natoms, nsegs, odd;
    t_atom *ap;
    t_curveseg *segp;
    for (natoms = 0, ap = av; natoms < ac; natoms++, ap++)
    {
	if (ap->a_type != A_FLOAT)
	{
        pd_error(x, "curve~: list needs to only contain floats");
	    return;  /* CHECKED */
	}
    }
    if (!natoms)
	return;  /* CHECKED */
    odd = natoms % 3;
    nsegs = natoms / 3;
    if (odd) nsegs++;
    //clip at maxsize
    if(nsegs > CURVE_MAXSIZE)
    {
        nsegs = CURVE_MAXSIZE;
        odd = 0;
    };
    x->x_nsegs = nsegs;
    segp = x->x_segs;
    if (odd) nsegs--;
    while (nsegs--)
    {
	segp->s_target = av++->a_w.w_float;
	segp->s_delta = av++->a_w.w_float;
	curve_cc(x, segp, av++->a_w.w_float);
	segp++;
    }
    if (odd)
    {
	segp->s_target = av->a_w.w_float;
	if (odd > 1)
	    segp->s_delta = av[1].a_w.w_float;
	else
	    segp->s_delta = 0;
	curve_cc(x, segp, x->x_ccinput);
    }
    x->x_deltaset = 0;
    x->x_target = x->x_segs->s_target;
    x->x_curseg = x->x_segs;
    x->x_retarget = 1;
    x->x_pause = 0;
}

static void curve_dsp(t_curve *x, t_signal **sp)
{
    float ksr = sp[0]->s_sr * 0.001;
    if (ksr != x->x_ksr)
    {
	int nsegs = x->x_nsegs;
	t_curveseg *segp = x->x_segs;
	x->x_ksr = ksr;
	while (nsegs--)
	{
	    curve_cc(x, segp, segp->s_ccinput);
	    segp++;
	}
    }
    dsp_add(curve_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void curve_factor(t_curve *x, t_float f){
	if(f < -1.){
		x->x_ccinput = -1.;
	}
	else if(f > 1.){
		x->x_ccinput = 1.;
	}
	else{
		x->x_ccinput = f;
	};

};


static void curve_stop(t_curve *x)
{
    x->x_nsegs = 0;
    x->x_nleft = 0;
}

static void curve_pause(t_curve *x)
{
    x->x_pause = 1;
}

static void curve_resume(t_curve *x)
{
    x->x_pause = 0;
}

static void curve_free(t_curve *x)
{
    if (x->x_segs != x->x_segini)
	freebytes(x->x_segs, x->x_size * sizeof(*x->x_segs));
    if (x->x_clock) clock_free(x->x_clock);
}

static void *curve_new(t_symbol *s, int argc, t_atom *argv)
{
    t_curve *x = (t_curve *)pd_new(curve_class);
    t_float initval = PDCYCURVEINITVAL;
	t_float param = PDCYCURVEPARAM;
	int argnum = 0;
	while(argc > 0){
		if(argv -> a_type == A_FLOAT){
			t_float argval = atom_getfloatarg(0, argc, argv);
			switch(argnum){
				case 0: initval = argval;
					break;
				case 1: param = argval;
					break;
				default:
					break; 
			};
			argnum++;
			argc--;
			argv++;

		}
		else{
			goto errstate;
		};

	};

	x->x_value = x->x_target = initval;
    curve_factor(x, param);
    x->x_deltaset = 0;
    x->x_ksr = sys_getsr() * 0.001;
    x->x_nleft = 0;
    x->x_retarget = 0;
    x->x_pause = 0;
    x->x_size = CURVE_MAXSIZE;
    x->x_nsegs = 0;
    x->x_segs = x->x_segini;
    x->x_curseg = 0;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("factor"));
    outlet_new((t_object *)x, &s_signal);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    x->x_clock = clock_new(x, (t_method)curve_tick);
    return (x);
	errstate:
		pd_error(x, "curve~: improper args");
		return NULL;
}

CYCLONE_OBJ_API void curve_tilde_setup(void)
{
    curve_class = class_new(gensym("curve~"),
			    (t_newmethod)curve_new,
			    (t_method)curve_free,
			    sizeof(t_curve), 0,
			    A_GIMME, 0);
    class_domainsignalin(curve_class, -1);
    class_addmethod(curve_class, (t_method)curve_dsp, gensym("dsp"), A_CANT, 0);
    class_addfloat(curve_class, curve_float);
    class_addlist(curve_class, curve_list);
    class_addmethod(curve_class, (t_method)curve_ft1,
		    gensym("ft1"), A_FLOAT, 0);
	class_addmethod(curve_class, (t_method)curve_factor,
		    gensym("factor"), A_FLOAT, 0);
    class_addmethod(curve_class, (t_method)curve_stop,
                    gensym("stop"), 0);
    class_addmethod(curve_class, (t_method)curve_pause,
                    gensym("pause"), 0);
    class_addmethod(curve_class, (t_method)curve_resume,
                    gensym("resume"), 0);
}
