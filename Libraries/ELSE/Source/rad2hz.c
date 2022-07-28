// Porres 2016
 
#include "m_pd.h"
#include <math.h>

#define TWO_PI 3.14159265358979323846 * 2

static t_class *rad2hz_class;

typedef struct _rad2hz{
    t_object    x_obj;
    t_outlet   *x_outlet;
    t_float     x_f;
}t_rad2hz;

static t_float convert(t_float f){
    return(f * sys_getsr() / TWO_PI);
}

static void rad2hz_bang(t_rad2hz *x){
    outlet_float(x->x_outlet, convert(x->x_f));
}

static void rad2hz_list(t_rad2hz *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 0)
        rad2hz_bang(x);
    if(ac == 1)
        outlet_float(x->x_outlet, convert(x->x_f = atom_getfloat(av)));
    else if(ac > 1){
        t_atom at[ac];
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, convert(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
    }
}

static void rad2hz_set(t_rad2hz *x, t_float f){
    x->x_f = f;
}

static void *rad2hz_new(t_floatarg f){
    t_rad2hz *x = (t_rad2hz *) pd_new(rad2hz_class);
    x->x_f = f;
    x->x_outlet = outlet_new(&x->x_obj, 0);
    return(x);
}

void rad2hz_setup(void){
    rad2hz_class = class_new(gensym("rad2hz"), (t_newmethod)rad2hz_new,
        0, sizeof(t_rad2hz),0, A_DEFFLOAT, 0);
    class_addlist(rad2hz_class,(t_method)rad2hz_list);
    class_addmethod(rad2hz_class,(t_method)rad2hz_set,gensym("set"), A_DEFFLOAT, 0);
}
