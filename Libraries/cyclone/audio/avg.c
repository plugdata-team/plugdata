/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _avg
{
    t_object x_obj;
    t_inlet *avg;
    float    x_count;
    float    x_accum;
} t_avg;

static t_class *avg_class;

static void avg_bang(t_avg *x)
{
    outlet_float(((t_object *)x)->ob_outlet,
		 (x->x_count ? x->x_accum / x->x_count : 0));
    x->x_count = 0;
    x->x_accum = 0;
}

static t_int *avg_perform(t_int *w)
{
    t_avg *x = (t_avg *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    float accum = 0;
    x->x_count += nblock;  /* LATER consider blockcount++ */
    while (nblock--)
    {
	float f = *in++;
    	accum += (f >= 0 ? f : -f);
    }
    x->x_accum += accum;
    return (w + 4);
}

static void avg_dsp(t_avg *x, t_signal **sp)
{
    dsp_add(avg_perform, 3, x, sp[0]->s_n, sp[0]->s_vec);
}

static void *avg_new(void)
{
    t_avg *x = (t_avg *)pd_new(avg_class);
    outlet_new((t_object *)x, &s_float);
    x->x_count = 0;
    x->x_accum = 0;
    return (x);
}

CYCLONE_OBJ_API void avg_tilde_setup(void)
{
    avg_class = class_new(gensym("avg~"),
			  (t_newmethod)avg_new, 0,
			  sizeof(t_avg), 0, 0);
    class_addmethod(avg_class, nullfn, gensym("signal"), 0);
    class_addmethod(avg_class, (t_method) avg_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(avg_class, avg_bang);
}
