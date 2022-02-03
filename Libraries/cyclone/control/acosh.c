// Porres 2016

#include <math.h>
#include "m_pd.h"
#include <common/api.h>

typedef struct _acosh
{
    t_object  x_ob;
    float     x_value;
} t_acosh;

static t_class *acosh_class;

static void acosh_bang(t_acosh *x)  // checked: no protection against NaNs
{
    float value = acoshf(x->x_value);
    outlet_float(((t_object *)x)->ob_outlet, value);
}

static void acosh_float(t_acosh *x, t_float f)  // checked: no protection against NaNs
{
    float value = acoshf(value);
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = value);
}

static void *acosh_new(t_floatarg f)
{
    t_acosh *x = (t_acosh *)pd_new(acosh_class);
    x->x_value = isnan(acoshf(f)) ? 0 : acoshf(f);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void acosh_setup(void)
{
    acosh_class = class_new(gensym("acosh"), (t_newmethod)acosh_new, 0,
			   sizeof(t_acosh), 0, A_DEFFLOAT, 0);
    class_addbang(acosh_class, acosh_bang);
    class_addfloat(acosh_class, acosh_float);
}