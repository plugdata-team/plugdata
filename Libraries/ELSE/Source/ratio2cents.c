// Porres 2016
 
#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *ratio2cents_class;

typedef struct _ratio2cents{
  t_object      x_obj;
  t_outlet     *x_outlet;
  t_float       x_f;
}t_ratio2cents;

static t_float convert(t_float f){
    if(f < 0.f)
        f = 0.f;
    return(log2(f) * 1200);
}

static void ratio2cents_list(t_ratio2cents *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 0)
        outlet_float(x->x_outlet, convert(x->x_f));
    else if(ac == 1)
        outlet_float(x->x_outlet, convert(x->x_f = atom_getfloat(av)));
    else if(ac > 1){
        t_atom* at = calloc(ac, sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, convert(atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void ratio2cents_set(t_ratio2cents *x, t_float f){
    x->x_f = f;
}

static void *ratio2cents_new(t_floatarg f){
    t_ratio2cents *x = (t_ratio2cents *) pd_new(ratio2cents_class);
    x->x_f = f;
    x->x_outlet = outlet_new(&x->x_obj, 0);
    return(x);
}

void ratio2cents_setup(void){
    ratio2cents_class = class_new(gensym("ratio2cents"), (t_newmethod)ratio2cents_new,
        0, sizeof(t_ratio2cents), 0, A_DEFFLOAT, 0);
    class_addlist(ratio2cents_class, (t_method)ratio2cents_list);
    class_addmethod(ratio2cents_class, (t_method)ratio2cents_set, gensym("set"), A_DEFFLOAT,0 );
}
