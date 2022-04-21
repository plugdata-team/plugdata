// porres 2017

#include "m_pd.h"

static t_class *changed_class;
static t_class *changed_proxy_class;

typedef struct _changed_proxy{
    t_pd              p_pd;
    struct _changed  *p_owner;
}t_changed_proxy;

typedef struct _changed{
    t_object        x_obj;
    t_changed_proxy x_proxy;
    t_atom          x_a[4096];
    int             x_c;
    int             x_change;
    t_symbol       *x_sym;
    t_outlet       *x_unchanged_out;
}t_changed;

static void changed_proxy_init(t_changed_proxy * p, t_changed *x){
    p->p_pd = changed_proxy_class;
    p->p_owner = x;
}

static void changed_proxy_anything(t_changed_proxy *p, t_symbol *s, int ac, t_atom *av){
    t_changed *x = p->p_owner;
    x->x_sym = s;
    x->x_c = ac;
    for(int i = 0; i < ac; i++)
        x->x_a[i] = av[i];
}

static void changed_bang(t_changed *x){
    outlet_anything(x->x_obj.ob_outlet, x->x_sym, x->x_c, x->x_a);
}

static void changed_anything(t_changed *x, t_symbol *s, int ac, t_atom *av){
    int i;
    if(x->x_sym != s || x->x_c != ac)
        x->x_change = 1;
    else if(x->x_c == ac){
        for(i = 0; i < ac; i++){
            if(x->x_a[i].a_type == A_FLOAT){
                if(x->x_a[i].a_w.w_float != av[i].a_w.w_float){
                    x->x_change = 1;
                    break;
                }
            }
            else if(x->x_a[i].a_type == A_SYMBOL){
                if(x->x_a[i].a_w.w_symbol != av[i].a_w.w_symbol){
                    x->x_change = 1;
                    break;
                }
            }
        }
    }
    x->x_sym = s;
    x->x_c = ac;
    if(x->x_change){
        for(i = 0; i < ac; i++)
            x->x_a[i] = av[i];
        outlet_anything(x->x_obj.ob_outlet, s, ac, av);
        x->x_change = 0;
    }
    else
        outlet_anything(x->x_unchanged_out, s, ac, av);
}

static void *changed_new(t_symbol *s, int ac, t_atom *av){
    t_changed *x = (t_changed *)pd_new(changed_class);
    s = NULL;
    int i;
    if(ac == 0)
        x->x_sym = &s_;
    else if(ac == 1){
        if((av)->a_type == A_SYMBOL){
            x->x_sym = atom_getsymbol(av);
            ac--;
        }
        else if((av)->a_type == A_FLOAT)
                x->x_sym = &s_float;
    }
    else{
        if((av)->a_type == A_SYMBOL){
            x->x_sym = atom_getsymbol(av++);
            ac--;
        }
        else
            x->x_sym = &s_list;
    }
    x->x_c = ac;
    for(i = 0; i < ac; i++)
        x->x_a[i] = av[i];
    x->x_change = 0;
    changed_proxy_init(&x->x_proxy, x);
    inlet_new(&x->x_obj, &x->x_proxy.p_pd, 0, 0);
    outlet_new(&x->x_obj, &s_anything);
    x->x_unchanged_out = outlet_new(&x->x_obj, &s_bang);
    return(x);
}

void changed_setup(void){
    changed_class = class_new(gensym("changed"), (t_newmethod)changed_new, 0,
    	sizeof(t_changed), 0, A_GIMME, 0);
    class_addbang(changed_class, changed_bang);
    class_addanything(changed_class, changed_anything);
    changed_proxy_class = (t_class *)class_new(gensym("changed proxy"),
        0, 0, sizeof(t_changed_proxy), 0, 0);
    class_addanything(changed_proxy_class, changed_proxy_anything);
}
