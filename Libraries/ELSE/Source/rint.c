// Porres 2018

#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *rint_class;

typedef struct _rint{
    t_object    x_obj;
}t_rint;

static void rint_list(t_rint *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1)
        outlet_float(x->x_obj.ob_outlet, rint(atom_getfloat(av)));
    else if(ac > 1){
        t_atom* at = calloc(ac, sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, rint(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void *rint_new(void){
    t_rint *x = (t_rint *)pd_new(rint_class);
    outlet_new(&x->x_obj, 0);
    return(x);
}

void rint_setup(void){
    rint_class = class_new(gensym("rint"), (t_newmethod)rint_new, 0,
        sizeof(t_rint), 0, 0);
    class_addlist(rint_class, (t_method)rint_list);
}
