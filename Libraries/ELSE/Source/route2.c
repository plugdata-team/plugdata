#include "m_pd.h"
#include <stdlib.h>

static t_class *route2_class;

typedef struct _route2element{
    t_word      e_w;
    t_outlet   *e_outlet;
    t_atomtype  e_type;
}t_route2element;

typedef struct _route2{
    t_object            x_obj;
    int                 x_nelement;
    t_route2element    *x_vec;
    t_outlet           *x_rejectout;
}t_route2;

static void route2_list(t_route2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 0){ // empty list, go to rejected as bang
        outlet_bang(x->x_rejectout);
        return;
    }
    t_route2element *e;
    int nelement;
    for(nelement = x->x_nelement, e = x->x_vec; nelement--; e++){
        if(e->e_type == A_FLOAT){
            if(e->e_w.w_float == atom_getfloat(av)){
                if(ac == 1)
                    outlet_bang(e->e_outlet);
                else if(ac == 2){
                    if(av[1].a_type == A_SYMBOL)
                        outlet_symbol(e->e_outlet, av[1].a_w.w_symbol);
                    else if(av[1].a_type == A_FLOAT)
                        outlet_float(e->e_outlet, atom_getfloat(av+1));
                }
                else // > 2
                    outlet_list(e->e_outlet, 0, ac-1, av+1);
                return;
            }
        }
        else{
            if(e->e_w.w_symbol == atom_getsymbol(av)){
                if(ac == 1){
                    outlet_bang(e->e_outlet);
                    return;
                }
                if(ac == 2){
                    if(av[1].a_type == A_SYMBOL)
                        outlet_symbol(e->e_outlet, av[1].a_w.w_symbol);
                    else
                        outlet_float(e->e_outlet, atom_getfloat(av+1));
                }
                else // output list
                    outlet_list(e->e_outlet, 0, ac-1, av+1);
                return;
            }
        }
    }
    if(ac == 1){
        if(av->a_type == A_FLOAT)
            outlet_float(x->x_rejectout, atom_getfloat(av));
        else if(av->a_type == A_SYMBOL)
            outlet_symbol(x->x_rejectout, atom_getsymbol(av));
    }
    else
        outlet_list(x->x_rejectout, 0, ac, av);
}

static void route2_anything(t_route2 *x, t_symbol *s, int ac, t_atom *av){
    t_atom* at = calloc(ac + 1, sizeof(t_atom));
    SETSYMBOL(at, s);
    for(int i = 0; i < ac; i++){
        if((av+i)->a_type == A_FLOAT)
            SETFLOAT(at+i+1, atom_getfloat(av+i));
        else if((av+i)->a_type == A_SYMBOL){
            SETSYMBOL(at+i+1, atom_getsymbol(av+i));
        }
    }
    route2_list(x, s = &s_list, ac+1, at);
    free(at);
}

static void route2_free(t_route2 *x){
    freebytes(x->x_vec, x->x_nelement * sizeof(*x->x_vec));
}

static void *route2_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int n;
    t_route2element *e;
    t_route2 *x = (t_route2 *)pd_new(route2_class);
    t_atom a;
    if(ac == 0){
        ac = 1;
        SETFLOAT(&a, 0);
        av = &a;
    }
    x->x_nelement = ac;
    x->x_vec = (t_route2element *)getbytes(ac * sizeof(*x->x_vec));
    for(n = 0, e = x->x_vec; n < ac; n++, e++){
        e->e_outlet = outlet_new(&x->x_obj, &s_list);
        e->e_type = av[n].a_type;
        if(e->e_type == A_FLOAT)
            e->e_w.w_float = atom_getfloatarg(n, ac, av);
        else
            e->e_w.w_symbol = atom_getsymbolarg(n, ac, av);
    }
    x->x_rejectout = outlet_new(&x->x_obj, &s_list);
    return(x);
}

void route2_setup(void){
    route2_class = class_new(gensym("route2"), (t_newmethod)route2_new,
        (t_method)route2_free, sizeof(t_route2), 0, A_GIMME, 0);
    class_addlist(route2_class, route2_list);
    class_addanything(route2_class, route2_anything);
}
