// Porres 2016

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _asinh
{
    t_object  x_ob;
    float     x_value;
} t_asinh;

static t_class *asinh_class;

static void asinh_bang(t_asinh *x) // checked: no protection against NaNs
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void asinh_float(t_asinh *x, t_float f)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = asinhf(f));
}

static void *asinh_new(t_floatarg f) // checked: no protection against NaNs
{
    t_asinh *x = (t_asinh *)pd_new(asinh_class);
    x->x_value = asinhf(f);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void asinh_setup(void)
{
    asinh_class = class_new(gensym("asinh"),
			   (t_newmethod)asinh_new, 0,
			   sizeof(t_asinh), 0, A_DEFFLOAT, 0);
    class_addbang(asinh_class, asinh_bang);
    class_addfloat(asinh_class, asinh_float);
}