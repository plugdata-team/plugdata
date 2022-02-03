// Porres 2016

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _atanh
{
    t_object  x_ob;
    float     x_value;
} t_atanh;

static t_class *atanh_class;

static void atanh_bang(t_atanh *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void atanh_float(t_atanh *x, t_float f)  // checked: no protection against NaNs
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = atanhf(f));
}

static void *atanh_new(t_floatarg f) // checked: no protection against NaNs
{
    t_atanh *x = (t_atanh *)pd_new(atanh_class);
    x->x_value = atanhf(f);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void atanh_setup(void)
{
    atanh_class = class_new(gensym("atanh"), (t_newmethod)atanh_new, 0,
			   sizeof(t_atanh), 0, A_DEFFLOAT, 0);
    class_addbang(atanh_class, atanh_bang);
    class_addfloat(atanh_class, atanh_float);
}