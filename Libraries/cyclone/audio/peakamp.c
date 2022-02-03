/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _peakamp
{
    t_object  x_obj;
    t_inlet  *peakamp;
    t_float   x_value;
    int       x_nwait;
    int       x_nleft;
    int       x_precount;
    float     x_waittime;
    float     x_ksr;
    t_clock  *x_clock;
} t_peakamp;

static t_class *peakamp_class;

static void peakamp_tick(t_peakamp *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
    x->x_value = 0;
    if ((x->x_nleft = x->x_nwait - x->x_precount) < 0)  /* CHECKME */
	x->x_nleft = 0;
}

static void peakamp_bang(t_peakamp *x)
{
    peakamp_tick(x);  /* CHECKME */
}

static void peakamp_ft1(t_peakamp *x, t_floatarg f)
{
    if ((x->x_waittime = f) < 0.)
	x->x_waittime = 0.;
    if ((x->x_nwait = (int)(x->x_waittime * x->x_ksr)) < 0)
	x->x_nwait = 0;
}

static t_int *peakamp_perform(t_int *w)
{
    t_peakamp *x = (t_peakamp *)(w[1]); 
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float value = x->x_value;
    if (x->x_nwait)
    {
	if (x->x_nleft < nblock)
	{
	    clock_delay(x->x_clock, 0);
	    x->x_precount = nblock - x->x_nleft;
	    x->x_nleft = 0;  /* LATER rethink */
	}
	else x->x_nleft -= nblock;
    }
    while (nblock--)
    {
	t_float f = *in++;
	if (f > value)
	    value = f;
	else if (f < -value)
	    value = -f;
    }
    x->x_value = value;
    return (w + 4);
}

static void peakamp_dsp(t_peakamp *x, t_signal **sp)
{
    x->x_ksr = sp[0]->s_sr * 0.001;
    x->x_nwait = (int)(x->x_waittime * x->x_ksr);
    dsp_add(peakamp_perform, 3, x, sp[0]->s_n, sp[0]->s_vec);
}

static void peakamp_free(t_peakamp *x)
{
    if (x->x_clock) clock_free(x->x_clock);
}

static void *peakamp_new(t_floatarg f)
{
    t_peakamp *x = (t_peakamp *)pd_new(peakamp_class);
    x->x_value = 0.;
    x->x_nleft = 0;
    x->x_ksr = sys_getsr() * 0.001;
    peakamp_ft1(x, f);
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_clock = clock_new(x, (t_method)peakamp_tick);
    return (x);
}

CYCLONE_OBJ_API void peakamp_tilde_setup(void)
{
    peakamp_class = class_new(gensym("peakamp~"),
			      (t_newmethod)peakamp_new,
			      (t_method)peakamp_free,
			      sizeof(t_peakamp), 0,
			      A_DEFFLOAT, 0);
    class_addmethod(peakamp_class, nullfn, gensym("signal"), 0);
    class_addmethod(peakamp_class, (t_method) peakamp_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(peakamp_class, peakamp_bang);
    class_addmethod(peakamp_class, (t_method)peakamp_ft1,
		    gensym("ft1"), A_FLOAT, 0);
}
