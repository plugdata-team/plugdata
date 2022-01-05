// porres 2019

#include "m_pd.h"
#include "g_canvas.h"

static t_class *canvas_vis_class, *canvas_vis_proxy_class;

typedef struct _canvas_vis_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _canvas_vis *p_cnv;
}t_canvas_vis_proxy;

typedef struct _canvas_vis{
    t_object            x_obj;
    t_canvas_vis_proxy *x_proxy;
    t_canvas           *x_canvas;
}t_canvas_vis;

static void canvas_vis_proxy_any(t_canvas_vis_proxy *p, t_symbol *s, int ac, t_atom *av){
    ac = 0;
    if(p->p_cnv && s == gensym("map"))
        outlet_float(p->p_cnv->x_obj.ob_outlet, (int)(av->a_w.w_float));
    else if(p->p_cnv && s == gensym("menuclose"))
        outlet_float(p->p_cnv->x_obj.ob_outlet, 0);
}

static void canvas_vis_proxy_free(t_canvas_vis_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_canvas_vis_proxy * canvas_vis_proxy_new(t_canvas_vis *x, t_symbol*s){
    t_canvas_vis_proxy *p = (t_canvas_vis_proxy*)pd_new(canvas_vis_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)canvas_vis_proxy_free);
    return(p);
}

static void canvas_vis_free(t_canvas_vis *x){
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
}

void canvas_vis_bang(t_canvas_vis *x){
    outlet_float(x->x_obj.ob_outlet, glist_isvisible(x->x_canvas));
}

static void *canvas_vis_new(t_floatarg f1){
    t_canvas_vis *x = (t_canvas_vis *)pd_new(canvas_vis_class);
    x->x_canvas = canvas_getcurrent();
    int depth = f1 < 0 ? 0 : (int)f1;
    while(depth-- && x->x_canvas->gl_owner)
        x->x_canvas = x->x_canvas->gl_owner;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_canvas);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = canvas_vis_proxy_new(x, gensym(buf));
    outlet_new(&x->x_obj, 0);
    return(x);
}

void setup_canvas0x2evis(void){
    canvas_vis_class = class_new(gensym("canvas.vis"), (t_newmethod)canvas_vis_new,
        (t_method)canvas_vis_free, sizeof(t_canvas_vis), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addbang(canvas_vis_class, canvas_vis_bang);
    canvas_vis_proxy_class = class_new(0, 0, 0, sizeof(t_canvas_vis_proxy),
        CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(canvas_vis_proxy_class, canvas_vis_proxy_any);
}
