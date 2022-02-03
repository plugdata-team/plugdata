/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

#if defined(_WIN32) || defined(__APPLE__)
/* cf pd/src/x_arithmetic.c */
#define tanhf  tanh
#endif

typedef struct _tanh
{
    t_object  x_ob;
    float     x_value;
} t_tanh;

static t_class *tanh_class;

static void tanh_bang(t_tanh *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void tanh_float(t_tanh *x, t_float f)
{
    /* CHECKME large values */
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = tanhf(f));
}

static void *tanh_new(t_floatarg f)
{
    t_tanh *x = (t_tanh *)pd_new(tanh_class);
    /* CHECKME large values */
    x->x_value = tanhf(f);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void tanh_setup(void)
{
    tanh_class = class_new(gensym("tanh"),
			   (t_newmethod)tanh_new, 0,
			   sizeof(t_tanh), 0, A_DEFFLOAT, 0);
    class_addbang(tanh_class, tanh_bang);
    class_addfloat(tanh_class, tanh_float);
}
