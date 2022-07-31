// Porres 2018

#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *floor_class;

typedef struct _floor{
    t_object    x_obj;
}t_floor;

static void floor_list(t_floor *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1)
        outlet_float(x->x_obj.ob_outlet, floor(atom_getfloat(av)));
    else if(ac > 1){
        t_atom* at = calloc(ac, sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, floor(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void *floor_new(void){
    t_floor *x = (t_floor *)pd_new(floor_class);
    outlet_new(&x->x_obj, 0);
    return(x);
}

void floor_setup(void){
    floor_class = class_new(gensym("floor"), (t_newmethod)floor_new, 0,
        sizeof(t_floor), 0, 0);
    class_addlist(floor_class, (t_method)floor_list);
}

