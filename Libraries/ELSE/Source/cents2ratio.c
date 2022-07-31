// Porres 2016
 
#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *cents2ratio_class;

typedef struct _cents2ratio{
    t_object      x_obj;
    t_outlet     *x_outlet;
    t_float       x_f;
}t_cents2ratio;

static t_float convert(t_float f){
    return(pow(2, (f/1200)));
}

static void cents2ratio_list(t_cents2ratio *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 0)
        outlet_float(x->x_outlet, convert(x->x_f));
    if(ac == 1)
        outlet_float(x->x_outlet, convert(x->x_f = atom_getfloat(av)));
    else if(ac > 1){
        t_atom* at = calloc(ac, sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, convert(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void cents2ratio_set(t_cents2ratio *x, t_float f){
    x->x_f = f;
}

static void *cents2ratio_new(t_floatarg f){
    t_cents2ratio *x = (t_cents2ratio *) pd_new(cents2ratio_class);
    x->x_f = f;
    x->x_outlet = outlet_new(&x->x_obj, 0);
    return(x);
}

void cents2ratio_setup(void){
    cents2ratio_class = class_new(gensym("cents2ratio"), (t_newmethod)cents2ratio_new,
        0, sizeof(t_cents2ratio), 0, A_DEFFLOAT, 0);
    class_addlist(cents2ratio_class, (t_method)cents2ratio_list);
    class_addmethod(cents2ratio_class, (t_method)cents2ratio_set, gensym("set"), A_DEFFLOAT,0 );
}
