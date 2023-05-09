// Porres 2016
 
#include "m_pd.h"
#include <stdlib.h>

#define TWO_PI (2 * 3.14159265358979323846)

static t_class *rad2hz_class;

typedef struct _rad2hz{
    t_object    x_obj;
    t_outlet   *x_outlet;
}t_rad2hz;

static t_float convert(t_float f){
    return(sys_getsr() * f / TWO_PI);
}

static void rad2hz_list(t_rad2hz *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 0)
        return;
    if(ac == 1)
        outlet_float(x->x_outlet, convert(atom_getfloat(av)));
    else if(ac > 1){
        t_atom* at = calloc(ac, sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, convert(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void *rad2hz_new(void){
    t_rad2hz *x = (t_rad2hz *) pd_new(rad2hz_class);
    x->x_outlet = outlet_new(&x->x_obj, 0);
    return(x);
}

void rad2hz_setup(void){
    rad2hz_class = class_new(gensym("rad2hz"), (t_newmethod)rad2hz_new,
        0, sizeof(t_rad2hz), 0, 0);
    class_addlist(rad2hz_class,(t_method)rad2hz_list);
}
