/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _edge
{
    t_object   x_obj;
    t_inlet   *edge;
    t_outlet  *x_outlet;
    t_float    x_last;
    int        x_zeroleft;
    int        x_zerohit;
    t_outlet  *x_out2;
    t_clock   *x_clock;
} t_edge;

static t_class *edge_class;

static void edge_tick(t_edge *x)
{ /* CHECKED both may fire simultaneously */
    if (x->x_zeroleft)
    {
	outlet_bang(((t_object *)x)->ob_outlet);
	x->x_zeroleft = 0;
    }
    if (x->x_zerohit)
    {
	outlet_bang(x->x_out2);
	x->x_zerohit = 0;
    }
}

static t_int *edge_perform(t_int *w)
{
    t_edge *x = (t_edge *)(w[1]); 
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float last = x->x_last;
    while (nblock--)
    {
	float f = *in++;
	if (last == 0.)
	{
	    if (f != 0.)
	    {
		x->x_zeroleft = 1;
		if (x->x_zerohit)
		{
		    clock_delay(x->x_clock, 0);
		    x->x_last = in[nblock - 1];
		    return (w + 4);
		}
	    }
	}
	else
	{
	    if (f == 0.)
	    {
		x->x_zerohit = 1;
		if (x->x_zeroleft)
		{
		    clock_delay(x->x_clock, 0);
		    x->x_last = in[nblock - 1];
		    return (w + 4);
		}
	    }
	}
	last = f;
    }
    if (x->x_zeroleft || x->x_zerohit) clock_delay(x->x_clock, 0);
    x->x_last = last;
    return (w + 4);
}

static void edge_dsp(t_edge *x, t_signal **sp)
{
    dsp_add(edge_perform, 3, x, sp[0]->s_n, sp[0]->s_vec);
}

static void edge_free(t_edge *x)
{
    if (x->x_clock) clock_free(x->x_clock);
}

static void *edge_new(void)
{
    t_edge *x = (t_edge *)pd_new(edge_class);
    x->x_last = 0.;  /* CHECKED fires at startup */
    x->x_zeroleft = x->x_zerohit = 0;
    x->x_outlet = outlet_new((t_object *)x, &s_bang);
    x->x_out2 = outlet_new((t_object *)x, &s_bang);
    x->x_clock = clock_new(x, (t_method)edge_tick);
    return (x);
}

CYCLONE_OBJ_API void edge_tilde_setup(void)
{
    edge_class = class_new(gensym("edge~"),
			   (t_newmethod)edge_new,
			   (t_method)edge_free,
			   sizeof(t_edge), CLASS_DEFAULT, 0);
    class_addmethod(edge_class, nullfn, gensym("signal"), 0);
    class_addmethod(edge_class, (t_method) edge_dsp, gensym("dsp"), A_CANT, 0);
}
