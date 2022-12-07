/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// Porres 2016 - checked large numbers in Max, it crashes,
// decided to leave this unprotected against 'inf' as cosh

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _sinh
{
    t_object  x_ob;
    float     x_value;
} t_sinh;

static t_class *sinh_class;

static void sinh_bang(t_sinh *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void sinh_float(t_sinh *x, t_float f)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = sinhf(f));
}

static void *sinh_new(t_floatarg f)
{
    t_sinh *x = (t_sinh *)pd_new(sinh_class);
    x->x_value = sinhf(f);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void sinh_setup(void)
{
    sinh_class = class_new(gensym("sinh"), (t_newmethod)sinh_new, 0,
			   sizeof(t_sinh), 0, A_DEFFLOAT, 0);
    class_addbang(sinh_class, sinh_bang);
    class_addfloat(sinh_class, sinh_float);
}
