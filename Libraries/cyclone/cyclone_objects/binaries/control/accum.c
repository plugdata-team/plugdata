/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is a slightly edited version of Joseph A. Sarlo's code.
   The most important changes are listed in "pd-lib-notes.txt" file.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _accum
{
    t_object  x_ob;
    t_float   x_total;
} t_accum;

static t_class *accum_class;

static void accum_set(t_accum *x, t_floatarg val)
{
    x->x_total = val;
}

static void accum_bang(t_accum *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_total);
}

static void accum_float(t_accum *x, t_floatarg val)
{
    /* LATER reconsider int/float dilemma */
    accum_set(x, val);
    accum_bang(x);
}

static void accum_add(t_accum *x, t_floatarg val)
{
    /* LATER reconsider int/float dilemma */
    x->x_total += val;
}

static void accum_mult(t_accum *x, t_floatarg val)
{
    x->x_total *= val;
}

static void *accum_new(t_floatarg val)
{
    t_accum *x = (t_accum *)pd_new(accum_class);
    x->x_total = val;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft2"));
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void accum_setup(void)
{
    accum_class = class_new(gensym("accum"), (t_newmethod)accum_new, 0,
			    sizeof(t_accum), 0, A_DEFFLOAT, 0);
    class_addbang(accum_class, accum_bang);
    class_addfloat(accum_class, accum_float);
    class_addmethod(accum_class, (t_method)accum_add,
		    gensym("ft1"), A_FLOAT, 0);
    class_addmethod(accum_class, (t_method)accum_mult,
		    gensym("ft2"), A_FLOAT, 0);
    class_addmethod(accum_class, (t_method)accum_set,
		    gensym("set"), A_FLOAT, 0);
}
