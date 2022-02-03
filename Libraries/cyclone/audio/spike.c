/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _spike
{
    t_object  x_obj;
    t_inlet  *spike;
    t_float   x_last;
    int       x_count;
    int       x_precount;
    int       x_nwait;
    float     x_waittime;
    double     x_ksr;
    double     x_rcpksr;
    t_clock  *x_clock;
} t_spike;

static t_class *spike_class;

static void spike_tick(t_spike *x)
{
    outlet_float(((t_object *)x)->ob_outlet, (x->x_count + 1) * x->x_rcpksr);
    x->x_count = x->x_precount;
}

static void spike_bang(t_spike *x)
{
x->x_count = 0;
}


static void spike_ft1(t_spike *x, t_floatarg f)
{
    if ((x->x_waittime = f) < 0.)
	x->x_waittime = 0.;
    x->x_nwait = (int)(x->x_waittime * x->x_ksr);
}

static t_int *spike_perform(t_int *w)
{
    t_spike *x = (t_spike *)(w[1]); 
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float last = x->x_last;
    int count = x->x_count;
    int nwait = x->x_nwait;
    if (count + nblock > nwait)
    {
	/* LATER efficiency tricks */
	while (nblock--)
	{
	    float f = *in++;
	    if (last == 0. && f != 0.  /* CHECKED zero-to-nonzero */
		&& count  /* CHECKED no firing at startup */
		&& count >= nwait)
	    {
		clock_delay(x->x_clock, 0);
		x->x_last = in[nblock - 1];
		x->x_count = count;
		x->x_precount = nblock;
		return (w + 4);
	    }
	    count++;
	    last = f;
	}
	x->x_last = last;
	x->x_count = count;
    }
    else
    {
	x->x_last = in[nblock - 1];
	x->x_count += nblock;
    }
    return (w + 4);
}

static void spike_dsp(t_spike *x, t_signal **sp)
{
    x->x_ksr = sp[0]->s_sr * 0.001;
    x->x_rcpksr = 1000.0 / sp[0]->s_sr;
    x->x_nwait = (int)(x->x_waittime * x->x_ksr);
    dsp_add(spike_perform, 3, x, sp[0]->s_n, sp[0]->s_vec);
}

static void spike_free(t_spike *x)
{
    if (x->x_clock) clock_free(x->x_clock);
}

static void *spike_new(t_floatarg f)
{
    t_spike *x = (t_spike *)pd_new(spike_class);
    x->x_last = 0.;
    x->x_ksr = sys_getsr() * 0.001;
    spike_ft1(x, f);
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_clock = clock_new(x, (t_method)spike_tick);
    return (x);
}

CYCLONE_OBJ_API void spike_tilde_setup(void)
{
    spike_class = class_new(gensym("spike~"),
			    (t_newmethod)spike_new,
			    (t_method)spike_free,
			    sizeof(t_spike), CLASS_DEFAULT,
			    A_DEFFLOAT, 0);
    class_addmethod(spike_class, nullfn, gensym("signal"), 0);
    class_addmethod(spike_class, (t_method) spike_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(spike_class, (t_method)spike_ft1, gensym("ft1"), A_FLOAT, 0);
    class_addbang (spike_class, spike_bang);
}
