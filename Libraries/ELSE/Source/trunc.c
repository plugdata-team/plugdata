// Porres 2018

#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *trunc_class;

typedef struct _trunc{
	t_object    x_obj;
}t_trunc;

static void trunc_list(t_trunc *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1)
        outlet_float(x->x_obj.ob_outlet, trunc(atom_getfloat(av)));
    else if(ac > 1){
        t_atom* at = calloc(ac, sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, trunc(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void *trunc_new(void){
    t_trunc *x = (t_trunc *)pd_new(trunc_class);
    outlet_new(&x->x_obj, 0);
    return(x);
}

void trunc_setup(void){
	trunc_class = class_new(gensym("trunc"), (t_newmethod)trunc_new, 0,
        sizeof(t_trunc), 0, 0);
	class_addlist(trunc_class, (t_method)trunc_list);
}
