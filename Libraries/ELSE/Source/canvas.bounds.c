// porres 2019

#include "m_pd.h"
#include "g_canvas.h"

static t_class *bounds_class, *bounds_proxy_class;

typedef struct _bounds_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _bounds *p_cnv;
}t_bounds_proxy;

typedef struct _bounds{
    t_object        x_obj;
    t_bounds_proxy *x_proxy;
    t_canvas       *x_canvas;
}t_bounds;

static void bounds_proxy_any(t_bounds_proxy *p, t_symbol *s, int ac, t_atom *av){
    ac = 0;
    if(p->p_cnv && s == gensym("setbounds")){
        t_atom at[4];
        SETFLOAT(at, (av)->a_w.w_float);
        SETFLOAT(at+1, (av+1)->a_w.w_float);
        SETFLOAT(at+2, (av+2)->a_w.w_float);
        SETFLOAT(at+3, (av+3)->a_w.w_float);
        outlet_list(p->p_cnv->x_obj.ob_outlet, &s_list, 4, at);
    }
}

static void bounds_proxy_free(t_bounds_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_bounds_proxy * bounds_proxy_new(t_bounds *x, t_symbol*s){
    t_bounds_proxy *p = (t_bounds_proxy*)pd_new(bounds_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)bounds_proxy_free);
    return(p);
}

static void bounds_free(t_bounds *x){
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
}

static void *bounds_new(t_floatarg f1){
    t_bounds *x = (t_bounds *)pd_new(bounds_class);
    x->x_canvas = canvas_getcurrent();
    int depth = f1 < 0 ? 0 : (int)f1;
    while(depth-- && x->x_canvas->gl_owner)
        x->x_canvas = x->x_canvas->gl_owner;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_canvas);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = bounds_proxy_new(x, gensym(buf));
    outlet_new(&x->x_obj, 0);
    return(x);
}

void setup_canvas0x2ebounds(void){
    bounds_class = class_new(gensym("canvas.bounds"), (t_newmethod)bounds_new,
        (t_method)bounds_free, sizeof(t_bounds), CLASS_NOINLET, A_DEFFLOAT, 0);
    bounds_proxy_class = class_new(0, 0, 0, sizeof(t_bounds_proxy),
        CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(bounds_proxy_class, bounds_proxy_any);
}
