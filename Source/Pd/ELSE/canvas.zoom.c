// porres 2020

#include "m_pd.h"
#include "g_canvas.h"

static t_class *zoom_class, *zoom_proxy_class;

typedef struct _zoom_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _zoom *p_cnv;
}t_zoom_proxy;

typedef struct _zoom{
    t_object        x_obj;
    t_zoom_proxy   *x_proxy;
    t_canvas       *x_canvas;
    int            x_zoom;
}t_zoom;

static void zoom_bang(t_zoom *x){
    outlet_float(x->x_obj.ob_outlet, (x->x_zoom = x->x_canvas->gl_zoom) == 2);
}

static void zoom_loadbang(t_zoom *x, t_float f){
    if((int)f == LB_LOAD)
        outlet_float(x->x_obj.ob_outlet, (x->x_zoom = x->x_canvas->gl_zoom) == 2);
}

static void zoom_proxy_any(t_zoom_proxy *p, t_symbol *s, int ac, t_atom *av){
    ac = 0;
    if(p->p_cnv){
        if(s == gensym("zoom")){
            int arg = (int)(av->a_w.w_float);
            if(p->p_cnv->x_zoom != arg)
                outlet_float(p->p_cnv->x_obj.ob_outlet, (p->p_cnv->x_zoom = arg) == 2);
        }
    }
}

static void zoom_proxy_free(t_zoom_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_zoom_proxy * zoom_proxy_new(t_zoom *x, t_symbol*s){
    t_zoom_proxy *p = (t_zoom_proxy*)pd_new(zoom_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)zoom_proxy_free);
    return(p);
}

static void zoom_free(t_zoom *x){
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
}

static void *zoom_new(t_floatarg f1){
    t_zoom *x = (t_zoom *)pd_new(zoom_class);
    x->x_canvas = canvas_getcurrent();
    int depth = f1 < 0 ? 0 : (int)f1;
    while(depth-- && x->x_canvas->gl_owner)
        x->x_canvas = x->x_canvas->gl_owner;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_canvas);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = zoom_proxy_new(x, gensym(buf));
    outlet_new(&x->x_obj, 0);
    return(x);
}

void setup_canvas0x2ezoom(void){
    zoom_class = class_new(gensym("canvas.zoom"), (t_newmethod)zoom_new,
        (t_method)zoom_free, sizeof(t_zoom), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(zoom_class, (t_method)zoom_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    zoom_proxy_class = class_new(0, 0, 0, sizeof(t_zoom_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addbang(zoom_class, zoom_bang);
    class_addanything(zoom_proxy_class, zoom_proxy_any);
}
