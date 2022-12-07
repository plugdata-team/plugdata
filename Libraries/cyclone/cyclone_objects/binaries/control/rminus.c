// Copyright (c) 2016 Porres.

#include "m_pd.h"
#include <common/api.h>

typedef struct _rminus
{
    t_object  x_ob;
    t_float   x_f1;
    t_float   x_f2;
} t_rminus;

static t_class *rminus_class;

static void rminus_bang(t_rminus *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_f2 - x->x_f1);
}

static void rminus_float(t_rminus *x, t_float f)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_f2 - (x->x_f1 = f));
}

static void *rminus_new(t_floatarg f)
{
    t_rminus *x = (t_rminus *)pd_new(rminus_class);
    floatinlet_new((t_object *)x, &x->x_f2);
    outlet_new((t_object *)x, &s_float);
    x->x_f1 = 0;
    x->x_f2 = f;
    return (x);
}

CYCLONE_OBJ_API void rminus_setup(void)
{
    rminus_class = class_new(gensym("rminus"), (t_newmethod)rminus_new,
            0, sizeof(t_rminus), 0, A_DEFFLOAT, 0);
    class_addbang(rminus_class, rminus_bang);
    class_addfloat(rminus_class, rminus_float);
}