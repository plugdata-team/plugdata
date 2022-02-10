// Porres 2021

#include "m_pd.h"
#include "g_canvas.h"
#include <string.h>

static t_class *default_class;
static t_class *default_proxy_class;

typedef struct _default_proxy{
    t_pd              p_pd;
    struct _default  *p_owner;
}t_default_proxy;

typedef struct _default{
    t_object        x_obj;
    t_default_proxy x_proxy;
    t_int           x_ac;
    t_atom         *x_atom;
    t_atom         *x_av;
    t_symbol       *x_s;
    t_symbol       *x_sel;
}t_default;

static void default_proxy_init(t_default_proxy * p, t_default *x){
    p->p_pd = default_proxy_class;
    p->p_owner = x;
}

static void default_proxy_anything(t_default_proxy *p, t_symbol *s, int ac, t_atom *av){
    t_default *x = p->p_owner;
    x->x_sel = s;
    if(!ac){
        x->x_ac = 0;
        x->x_av = NULL;
    }
    else{
        x->x_ac = ac;
        x->x_av = x->x_atom = (t_atom *)getbytes(x->x_ac * sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            x->x_av[i] = x->x_atom[i] = av[i];
    }
}

static void default_output(t_default *x){
    if(x->x_sel != NULL)
        outlet_anything(x->x_obj.ob_outlet, x->x_sel, x->x_ac, x->x_atom);
    else
        outlet_bang(x->x_obj.ob_outlet);
}

static void default_any(t_default *x, t_symbol *s, int ac, t_atom *av){
    outlet_anything(x->x_obj.ob_outlet, s, ac, av);
}

static void default_free(t_default *x){
    if(x->x_av)
        freebytes(x->x_av, x->x_ac * sizeof(t_atom));
}

static void *default_new(t_symbol *s, int ac, t_atom *av){
    t_default *x = (t_default *)pd_new(default_class);
    x->x_sel = s; // get rid of warning
    if(!ac){
        x->x_ac = 0;
        x->x_av = NULL;
        x->x_sel = NULL;
    }
    else{
        if(av->a_type == A_SYMBOL){ // get selector
            x->x_s = x->x_sel = atom_getsymbol(av++);
            ac--;
        }
        else
            x->x_s = x->x_sel = &s_list; // list selector otherwise
        x->x_ac = ac;
        x->x_av = x->x_atom = (t_atom *)getbytes(x->x_ac * sizeof(t_atom));
        int i;
        for(i = 0; i < ac; i++)
            x->x_av[i] = x->x_atom[i] = av[i];
    }
    default_proxy_init(&x->x_proxy, x);
    inlet_new(&x->x_obj, &x->x_proxy.p_pd, 0, 0);
    outlet_new(&x->x_obj, &s_anything);
    return(x);
}

void default_setup(void){
    default_class = class_new(gensym("default"), (t_newmethod)default_new,
        (t_method)default_free, sizeof(t_default), 0, A_GIMME, 0);
    class_addbang(default_class, (t_method)default_output);
    class_addanything(default_class, (t_method)default_any);
    default_proxy_class = (t_class *)class_new(gensym("default proxy"),
        0, 0, sizeof(t_default_proxy), 0, 0);
    class_addanything(default_proxy_class, default_proxy_anything);
}
