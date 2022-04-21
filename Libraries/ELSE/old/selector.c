// porres

#include "m_pd.h"

typedef struct _selector{
    t_object  x_obj;
    int       x_open;
    int       x_n;         // number of channels
    t_pd    **x_proxies;
}t_selector;

typedef struct _selector_proxy{
    t_object       p_obj;
    t_selector    *p_master;
    t_int          p_id;
    t_int          p_n;
}t_selector_proxy;

static t_class *selector_class;
static t_class *selector_proxy_class;

static void selector_anything(t_selector *x, t_symbol *s, int ac, t_atom *av){
    if(x->x_open == 1)
        outlet_anything(((t_object *)x)->ob_outlet, s, ac, av);
}

static void selector_proxy_float(t_selector_proxy *x, t_float f){
    t_selector *master = x->p_master;
    if(x->p_id <= x->p_n){
        if(master->x_open == x->p_id)
            outlet_float(((t_object *)master)->ob_outlet, f);
    }
    else
        master->x_open = (int)f + 1;
}

static void selector_proxy_anything(t_selector_proxy *x, t_symbol *s, int ac, t_atom *av){
    if(x->p_id <= x->p_n){
        t_selector *master = x->p_master;
        if(master->x_open == x->p_id)
            outlet_anything(((t_object *)master)->ob_outlet, s, ac, av);
    }
}

static void selector_free(t_selector *x){
    if(x->x_proxies){
        int i = x->x_n;
        while (i--) pd_free(x->x_proxies[i]);
        freebytes(x->x_proxies, x->x_n * sizeof(*x->x_proxies));
    }
}

static void *selector_new(t_floatarg f1, t_floatarg f2){
    t_selector *x = (t_selector *)pd_new(selector_class);
    t_int n = (int)f1;
    x->x_open = (int)f2 + 1;
    x->x_n = n < 2 ? 2 : n > 512 ? 512 : n;
    t_pd **proxies;
    if(!(proxies = (t_pd **)getbytes(x->x_n * sizeof(*proxies))))
        return(0);
    x->x_proxies = proxies;
    for(t_int i = 0; i < x->x_n; i++)
        if(!(proxies[i] = pd_new(selector_proxy_class))) break;
    for(int i = 0; i < x->x_n; i++){
        t_selector_proxy *y = (t_selector_proxy *)proxies[i];
        y->p_master = x;
        y->p_id = i + 2;
        y->p_n = x->x_n;
        inlet_new((t_object *)x, (t_pd *)y, 0, 0);
    }
    outlet_new((t_object *)x, &s_anything);
    return(x);
}

void selector_setup(void){
    selector_class = class_new(gensym("selector"), (t_newmethod)selector_new,
        (t_method)selector_free, sizeof(t_selector), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addanything(selector_class, selector_anything);
    selector_proxy_class = class_new(gensym("_selector_proxy"), 0, 0,
        sizeof(t_selector_proxy), CLASS_PD | CLASS_NOINLET, 0);
    class_addfloat(selector_proxy_class, selector_proxy_float);
    class_addanything(selector_proxy_class, selector_proxy_anything);
}
