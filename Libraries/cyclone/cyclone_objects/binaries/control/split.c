/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

/* CHECKED:
   'list <symbol>' silently ignored (LATER remove a warning)
   '<number> <symbol>' as '<number>' (LATER remove a warning)
   LATER more compatibility checks are needed...
   LATER sort out float/int dilemmas
*/

typedef struct _split
{
    t_object   x_ob;
    t_float    x_min;
    t_float    x_max;
    t_outlet  *x_out2;
} t_split;

static t_class *split_class;

static void split_float(t_split *x, t_float f)
{
	if (f >= x->x_min && f <= x->x_max)
	    outlet_float(((t_object *)x)->ob_outlet, (int)f);
	else outlet_float(x->x_out2, (int)f);
}

static void *split_new(t_floatarg f1, t_floatarg f2)
{
    t_split *x = (t_split *)pd_new(split_class);
    /* CHECKED: defaults are [0..0] and [0..f1] (for positive f1) or [f1..0] */
    if (f1 < f2)  /* CHECKED */
	x->x_min = f1, x->x_max = f2;
    else
	x->x_min = f2, x->x_max = f1;
    floatinlet_new((t_object *)x, &x->x_min);
    floatinlet_new((t_object *)x, &x->x_max);
    outlet_new((t_object *)x, &s_float);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void split_setup(void)
{
    split_class = class_new(gensym("split"),
			    (t_newmethod)split_new, 0,
			    sizeof(t_split), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(split_class, split_float);
}
