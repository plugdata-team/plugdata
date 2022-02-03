/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// Porres 2016 - checked no protection agains nan and fixed

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _acos
{
    t_object  x_ob;
    float     x_value;
} t_acos;

static t_class *acos_class;

static void acos_bang(t_acos *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void acos_float(t_acos *x, t_float f)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = acosf(f));
}

static void *acos_new(t_floatarg f)
{
    t_acos *x = (t_acos *)pd_new(acos_class);
    x->x_value = acosf(f);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void acos_setup(void)
{
    acos_class = class_new(gensym("acos"), (t_newmethod)acos_new, 0,
			   sizeof(t_acos), 0, A_DEFFLOAT, 0);
    class_addbang(acos_class, acos_bang);
    class_addfloat(acos_class, acos_float);
}
