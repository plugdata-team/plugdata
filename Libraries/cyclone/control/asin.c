/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

// Porres 2016 - checked no protection agains nan and fixed

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _asin
{
    t_object  x_ob;
    float     x_value;
} t_asin;

static t_class *asin_class;

static void asin_bang(t_asin *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void asin_float(t_asin *x, t_float f)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = asinf(f));
}

static void *asin_new(t_floatarg f)
{
    t_asin *x = (t_asin *)pd_new(asin_class);
    x->x_value = asinf(f);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void asin_setup(void)
{
    asin_class = class_new(gensym("asin"), (t_newmethod)asin_new, 0,
			   sizeof(t_asin), 0, A_DEFFLOAT, 0);
    class_addbang(asin_class, asin_bang);
    class_addfloat(asin_class, asin_float);
}
