// porres 2020

#include "m_pd.h"
#include "g_canvas.h"

static t_class *fontsize_class, *fontsize_proxy_class;

typedef struct _fontsize_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _fontsize *p_cnv;
}t_fontsize_proxy;

typedef struct _fontsize{
    t_object        x_obj;
    t_fontsize_proxy *x_proxy;
    t_canvas       *x_cv;
}t_fontsize;

static void fontsize_proxy_any(t_fontsize_proxy *p, t_symbol *s, int ac, t_atom *av){
    ac = 0;
    if(p->p_cnv && s == gensym("font"))
        outlet_float(p->p_cnv->x_obj.ob_outlet, (av)->a_w.w_float);
}

static void fontsize_proxy_free(t_fontsize_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_fontsize_proxy * fontsize_proxy_new(t_fontsize *x, t_symbol*s){
    t_fontsize_proxy *p = (t_fontsize_proxy*)pd_new(fontsize_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)fontsize_proxy_free);
    return(p);
}

static void font_bang(t_fontsize *x){
    outlet_float(x->x_obj.ob_outlet, glist_getfont(x->x_cv));
}

static void fontsize_loadbang(t_fontsize *x, t_float f){
    if((int)f == LB_LOAD)
        outlet_float(x->x_obj.ob_outlet, glist_getfont(x->x_cv));
}

static void fontsize_free(t_fontsize *x){
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
}

static void *fontsize_new(t_floatarg f1){
    t_fontsize *x = (t_fontsize *)pd_new(fontsize_class);
    x->x_cv = canvas_getrootfor(canvas_getcurrent());
    int depth = f1 < 0 ? 0 : (int)f1;
    while(depth-- && x->x_cv->gl_owner)
        x->x_cv = canvas_getrootfor(x->x_cv->gl_owner);
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_cv);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = fontsize_proxy_new(x, gensym(buf));
    outlet_new(&x->x_obj, 0);
    return(x);
}

void fontsize_setup(void){
    fontsize_class = class_new(gensym("fontsize"), (t_newmethod)fontsize_new,
        (t_method)fontsize_free, sizeof(t_fontsize), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(fontsize_class, (t_method)fontsize_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    fontsize_proxy_class = class_new(0, 0, 0, sizeof(t_fontsize_proxy),
        CLASS_NOINLET | CLASS_PD, 0);
    class_addbang(fontsize_class, font_bang);
    class_addanything(fontsize_proxy_class, fontsize_proxy_any);
}
