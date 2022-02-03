/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

#define RAMPSMOOTH_DEFNUP    0.
#define RAMPSMOOTH_DEFNDOWN  0.

typedef struct _rampsmooth
{
    t_object x_obj;
    t_inlet  *rampsmooth;
    int      x_nup;
    int      x_ndown;
    double   x_upcoef;
    double   x_downcoef;
    t_float  x_last;
    t_float  x_target;
    double   x_incr;
    int      x_nleft;
    int      x_change; //if we reset coeffs
} t_rampsmooth;

static t_class *rampsmooth_class;

static t_int *rampsmooth_perform(t_int *w){
    t_rampsmooth *x = (t_rampsmooth *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float last = x->x_last;
    t_float target = x->x_target;
    int change = x->x_change;
    double incr = x->x_incr;
    int nleft = x->x_nleft;
    while (nblock--){
    	t_float f = *in++;
        if(f != target || change){
            target = f;
            change = 0;
            if(f > last){
                if(x->x_nup > 1){
                    incr = (f - last) * x->x_upcoef;
                    nleft = x->x_nup;
                    *out++ = (last += incr);
                    continue;
                }
            }
            else if(f < last){
                if(x->x_ndown > 1){
                    incr = (f - last) * x->x_downcoef;
                    nleft = x->x_ndown;
                    *out++ = (last += incr);
                    continue;
                }
            }
            incr = 0.;
            nleft = 0;
            *out++ = last = f;
        }
        else if (nleft > 0){
            *out++ = (last += incr);
            if(--nleft == 1){
                incr = 0.;
                last = target;
            }
        }
        else
            *out++ = target;
    };
    x->x_change = change;
    x->x_last = (PD_BIGORSMALL(last) ? 0. : last);
    x->x_target = (PD_BIGORSMALL(target) ? 0. : target);
    x->x_incr = incr;
    x->x_nleft = nleft;
    return (w + 5);
}

static void rampsmooth_dsp(t_rampsmooth *x, t_signal **sp)
{
    dsp_add(rampsmooth_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void rampsmooth_rampup(t_rampsmooth *x, t_floatarg f)
{
    double upcoef;
    int i = (int)f;
    if (i > 1)
    {
	x->x_nup = i;
	upcoef = 1. / (float)i;
    }
    else
    {
	x->x_nup = 0;
	upcoef = 0.;
    };
    if(upcoef != x->x_upcoef)
    {
        x->x_upcoef = upcoef;
        x->x_change = 1;
    };
}

static void rampsmooth_rampdown(t_rampsmooth *x, t_floatarg f)
{
    double downcoef;
    int i = (int)f;
    if (i > 1)
    {
	x->x_ndown = i;
	downcoef = 1. / (float)i;
    }
    else
    {
	x->x_ndown = 0;
	downcoef = 0.;
    };
    if(downcoef != x->x_downcoef)
    {
        x->x_downcoef = downcoef;
        x->x_change = 1;
    };
}

static void rampsmooth_ramp(t_rampsmooth *x, t_floatarg f)
{
    rampsmooth_rampup(x, f);
    rampsmooth_rampdown(x, f);
}

static void *rampsmooth_new(t_symbol *s, int ac, t_atom *av)
{
    t_rampsmooth *x = (t_rampsmooth *)pd_new(rampsmooth_class);
    float f1 = RAMPSMOOTH_DEFNUP;
    float f2 = RAMPSMOOTH_DEFNDOWN;
    if (ac && av->a_type == A_FLOAT)
    {
	f1 = av->a_w.w_float;
	ac--; av++;
	if (ac && av->a_type == A_FLOAT)
	    f2 = av->a_w.w_float;
    }
    rampsmooth_rampup(x, f1);
    rampsmooth_rampdown(x, f2);
    x->x_change = 1;
    x->x_last = 0.;
    x->x_target = 0.;
    x->x_incr = 0.;
    x->x_nleft = 0;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("rampup"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("rampdown"));
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void rampsmooth_tilde_setup(void)
{
    rampsmooth_class = class_new(gensym("rampsmooth~"),
				 (t_newmethod)rampsmooth_new, 0,
				 sizeof(t_rampsmooth), 0, A_GIMME, 0);
    class_addmethod(rampsmooth_class, nullfn, gensym("signal"), 0);
    class_addmethod(rampsmooth_class, (t_method) rampsmooth_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(rampsmooth_class, (t_method)rampsmooth_rampup,
		    gensym("rampup"), A_FLOAT, 0);
    class_addmethod(rampsmooth_class, (t_method)rampsmooth_rampdown,
		    gensym("rampdown"), A_FLOAT, 0);
    class_addmethod(rampsmooth_class, (t_method)rampsmooth_ramp,
		    gensym("ramp"), A_FLOAT, 0);
}
