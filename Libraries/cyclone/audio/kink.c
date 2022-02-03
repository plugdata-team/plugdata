/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKED negative float in 2nd inlet: "illegal slope value %f",
   but no complaints for signal input -- this is impossible in Pd.
   The only thing we could do (and a bit stupid one) would be to
   clock this exception out from the perf routine.
   
   UPDATE 02/17 - used the "magic trick" to fix this -- MB */

#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h"

static t_class *kink_class;

typedef struct _kink {
    t_object x_obj;
    t_float x_slope;
    t_glist *x_glist;
    t_float *x_signalscalar;
    int x_hasfeeders;
} t_kink;


static t_int *kink_perform(t_int *w)
{
	t_kink *x = (t_kink *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float *scalar = x->x_signalscalar;
    if (*scalar < 0.0)
    {
    	pd_error(x, "kink~: illegal float value %.2f", *scalar);
    	*scalar = x->x_slope;
    }
    x->x_slope = *scalar;
    while (nblock--)
    {
		t_float slope;
		t_float iph = *in1++;
        if (x->x_hasfeeders)
            {
            slope = *in2++;
            if (slope < 0) slope = 0;
            }
		else
			slope = x->x_slope;
		if (slope)
		{
			t_float breakpoint = 0.5/slope;
			if (iph <= breakpoint)
				*out++ = iph * slope;
			else
				*out++ = 0.5/(1.0-breakpoint)*(iph-breakpoint) + 0.5;
		}
		else
			*out++ = 0;
	
    }
    return (w + 6);
}

static void kink_dsp(t_kink *x, t_signal **sp)
{
	x->x_hasfeeders = magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal);
    dsp_add(kink_perform, 5, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}


static void *kink_new(t_floatarg f)
{
    t_kink *x = (t_kink *)pd_new(kink_class);
    if (f < 0.0)
    {
    	pd_error(x, "kink~: illegal float value %.2f", f);
    	f = 1.0;
    }
    f = (f == 0.0 ? 1.0 : f);
    x->x_slope = f;
    pd_float((t_pd *)inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal), f);
    outlet_new((t_object *)x, &s_signal);
    x->x_glist = canvas_getcurrent();
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 1);
    return (x);
}

CYCLONE_OBJ_API void kink_tilde_setup(void)
{
    kink_class = class_new(gensym("kink~"),
			   (t_newmethod)kink_new, 0,
			   sizeof(t_kink), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(kink_class, nullfn, gensym("signal"), 0);
    class_addmethod(kink_class, (t_method)kink_dsp, gensym("dsp"), A_CANT, 0);
}
