// Porres 2018

#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *ceil_class;

typedef struct _ceil{
    t_object    x_obj;
}t_ceil;

static void ceil_list(t_ceil *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1)
        outlet_float(x->x_obj.ob_outlet, ceil(atom_getfloat(av)));
    else if(ac > 1){
        t_atom* at = calloc(ac, sizeof(t_atom));
        
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, ceil(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void *ceil_new(void){
    t_ceil *x = (t_ceil *)pd_new(ceil_class);
    outlet_new(&x->x_obj, 0);
    return(x);
}

void ceil_setup(void){
    ceil_class = class_new(gensym("ceil"), (t_newmethod)ceil_new, 0,
        sizeof(t_ceil), 0, 0);
    class_addlist(ceil_class, (t_method)ceil_list);
}


