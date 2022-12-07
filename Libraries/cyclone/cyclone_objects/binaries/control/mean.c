/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _mean
{
    t_object   x_ob;
    double     x_accum;
    unsigned int x_count;
    t_float    x_mean;
    t_outlet  *x_countout;
} t_mean;

static t_class *mean_class;

static void mean_clear(t_mean *x)
{
    x->x_accum = 0;
    x->x_count = 0;
    x->x_mean = 0;
}

static void mean_bang(t_mean *x)
{
    /* CHECKED: count is always sent (first) */
    outlet_float(x->x_countout, x->x_count);
    outlet_float(((t_object *)x)->ob_outlet, x->x_mean);
}

static void mean_float(t_mean *x, t_float f)
{
    x->x_accum += f;
    if (++x->x_count)
	x->x_mean = (t_float)(x->x_accum / (double)x->x_count);
    else mean_clear(x);
    mean_bang(x);
}

static void mean_list(t_mean *x, t_symbol *s, int ac, t_atom *av)
{
    mean_clear(x);
    while (ac--)
    {
	if (av->a_type == A_FLOAT)
	{
	    x->x_accum += av->a_w.w_float;
	    x->x_count++;
	}
	av++;
    }
    if (x->x_count)
	x->x_mean = (t_float)(x->x_accum / (double)x->x_count);
    else mean_clear(x);
    mean_bang(x);
    /* CHECKED: no clear after list -- subsequent floats are added */
}

static void *mean_new(void)
{
    t_mean *x = (t_mean *)pd_new(mean_class);
    mean_clear(x);
    outlet_new((t_object *)x, &s_float);
    x->x_countout = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void mean_setup(void)
{
    mean_class = class_new(gensym("mean"),
			   (t_newmethod)mean_new, 0,
			   sizeof(t_mean), 0, 0);
    class_addbang(mean_class, mean_bang);
    class_addfloat(mean_class, mean_float);
    class_addlist(mean_class, mean_list);
    class_addmethod(mean_class, (t_method)mean_clear,
		    gensym("clear"), 0);
}
