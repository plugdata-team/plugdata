// porres

#include "m_pd.h"

typedef struct _router{
    t_object    x_obj;
    t_float     x_open;
    int         x_nouts;
    t_outlet  **x_outs;
}t_router;

static t_class *router_class;

static void router_anything(t_router *x, t_symbol *s, int ac, t_atom *av){
    int i = (int)x->x_open;
    if(i >= 0 && i < x->x_nouts)
        outlet_anything(x->x_outs[i], s, ac, av);
}

static void router_free(t_router *x){
    if(x->x_outs) freebytes(x->x_outs, x->x_nouts * sizeof(*x->x_outs));
}

static void *router_new(t_floatarg f1, t_floatarg f2){
    t_router *x = (t_router *)pd_new(router_class);
    int n = (int)f1;
    x->x_nouts = n < 2 ? 2 : n > 512 ? 512 : n;
    t_outlet **outs;
    if(!(outs = (t_outlet **)getbytes(x->x_nouts * sizeof(*outs))))
        return(0);
    x->x_outs = outs;
    x->x_open = f2;
    floatinlet_new(&x->x_obj, &x->x_open);
    for(int i = 0; i < x->x_nouts; i++)
        x->x_outs[i] = outlet_new((t_object *)x, &s_anything);
    return(x);
}

void router_setup(void){
    router_class = class_new(gensym("router"), (t_newmethod)router_new, (t_method)router_free,
        sizeof(t_router), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addanything(router_class, router_anything);
}
