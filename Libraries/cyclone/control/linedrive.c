/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include <math.h>

#define logf  log
#define expf  exp

#define LINEDRIVE_CURVE   1.01
#define LINEDRIVE_MININPUT   .5  /* CHECKED */
#define LINEDRIVE_MINCURVE  1.   /* CHECKED */

#define LINEDRIVE_INPUTEPSILON  1e-19f
#define LINEDRIVE_CURVEEPSILON  1e-19f

typedef struct _linedrive
{
    t_object  x_ob;
    t_float   x_usermaxin;
    t_float   x_usermaxout;
    t_float   x_usercurve;
    t_float   x_maxin;
    t_float   x_maxout;
    t_float   x_expcoef;
    t_float   x_lincoef;
    int       x_islinear;
    int       x_iscompatible;
    t_atom    x_outvec[2];
} t_linedrive;

static t_class *linedrive_class;

static void linedrive_float(t_linedrive *x, t_floatarg f)
{
    t_float outval;
    if (f >= x->x_maxin) outval = x->x_maxout; // clip
    else if (f * -1 >= x->x_maxin) outval = x->x_maxout * -1; // clip
	else if (x->x_islinear)
	    outval = x->x_maxout + (f - x->x_maxin) * x->x_lincoef;
	else if (f < -LINEDRIVE_INPUTEPSILON)
	    outval = -expf((-f - x->x_maxin) * x->x_expcoef) * x->x_maxout;
	else if (f > LINEDRIVE_INPUTEPSILON)
	    outval = expf((f - x->x_maxin) * x->x_expcoef) * x->x_maxout;
	else
	    outval = 0.;
    SETFLOAT(x->x_outvec, outval);
    outlet_list(((t_object *)x)->ob_outlet, 0, 2, x->x_outvec);
}

static void linedrive_calculate(t_linedrive *x){
    t_float maxin = x->x_usermaxin;
    t_float maxout = x->x_usermaxout;
    t_float curve = x->x_usercurve;
	if (maxin >= -LINEDRIVE_INPUTEPSILON && maxin <= LINEDRIVE_INPUTEPSILON){
	    x->x_maxin = 0.;
	    x->x_expcoef = 0.;
	    x->x_lincoef = 0.;
	    x->x_islinear = 1;
	}
	else if (curve >= (1. - LINEDRIVE_CURVEEPSILON) &&
		 curve <= (1. + LINEDRIVE_CURVEEPSILON)){
            x->x_maxin = maxin;
            x->x_expcoef = 0.;
            x->x_lincoef = maxout / maxin;
            x->x_islinear = 1;
    }
	else{
	    if (maxin < 0.)
	    {
		x->x_maxin = -maxin;
		maxout = -maxout;
	    }
	    else x->x_maxin = maxin;
	    if (curve < LINEDRIVE_CURVEEPSILON)
		curve = LINEDRIVE_CURVEEPSILON;
	    x->x_expcoef = logf(curve);
	    x->x_lincoef = 0.;
	    x->x_islinear = 0;
	}
    x->x_maxout = maxout;  /* CHECKED negative value accepted and used */
}

static void *linedrive_new(t_floatarg maxin, t_floatarg maxout,
			   t_floatarg curve, t_floatarg deltime)
{
    t_linedrive *x = (t_linedrive *)pd_new(linedrive_class);
    x->x_usermaxin = maxin < 0 ? maxin * -1 : maxin; // checked
    x->x_usermaxout = maxout;
    x->x_usercurve = LINEDRIVE_CURVE;
    if (curve >= 1) x->x_usercurve = curve;
    linedrive_calculate(x);
    SETFLOAT(&x->x_outvec[1], deltime);  /* CHECKED any value accepted */
    floatinlet_new((t_object *)x, &x->x_outvec[1].a_w.w_float);
    outlet_new((t_object *)x, &s_list);
    return (x);
}

CYCLONE_OBJ_API void linedrive_setup(void)
{
    linedrive_class = class_new(gensym("linedrive"),
				(t_newmethod)linedrive_new, 0,
				sizeof(t_linedrive), 0,
				A_DEFFLOAT, A_DEFFLOAT,
				A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(linedrive_class, linedrive_float);
}
