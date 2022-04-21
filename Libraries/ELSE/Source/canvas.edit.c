// porres 2019

#include "m_pd.h"
#include "g_canvas.h"

static t_class *edit_class, *edit_proxy_class;

typedef struct _edit_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _edit *p_cnv;
}t_edit_proxy;

typedef struct _edit{
    t_object        x_obj;
    t_edit_proxy   *x_proxy;
    t_canvas       *x_canvas;
    int            x_edit;
}t_edit;

static void edit_bang(t_edit *x){
    outlet_float(x->x_obj.ob_outlet, x->x_edit = x->x_canvas->gl_edit);
}

static void edit_loadbang(t_edit *x, t_float f){
    if((int)f == LB_LOAD)
        outlet_float(x->x_obj.ob_outlet, x->x_edit = x->x_canvas->gl_edit);
}

static void edit_proxy_any(t_edit_proxy *p, t_symbol *s, int ac, t_atom *av){
    ac = 0;
    if(p->p_cnv){
        if(s == gensym("editmode")){
            int arg = (int)(av->a_w.w_float);
            if(p->p_cnv->x_edit != arg)
                outlet_float(p->p_cnv->x_obj.ob_outlet, p->p_cnv->x_edit = arg);
        }
        else if(s == gensym("obj") || s == gensym("msg") || s == gensym("floatatom")
        || s == gensym("symbolatom") || s == gensym("text") || s == gensym("bng")
        || s == gensym("toggle") || s == gensym("numbox") || s == gensym("vslider")
        || s == gensym("hslider") || s == gensym("vradio") || s == gensym("hradio")
        || s == gensym("vumeter") || s == gensym("mycnv") || s == gensym("selectall")){
            if(p->p_cnv->x_edit == 0)
                outlet_float(p->p_cnv->x_obj.ob_outlet, p->p_cnv->x_edit = 1);
        }
    }
}

static void edit_proxy_free(t_edit_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_edit_proxy * edit_proxy_new(t_edit *x, t_symbol*s){
    t_edit_proxy *p = (t_edit_proxy*)pd_new(edit_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)edit_proxy_free);
    return(p);
}

static void edit_free(t_edit *x){
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
}

static void *edit_new(t_floatarg f1){
    t_edit *x = (t_edit *)pd_new(edit_class);
    x->x_canvas = canvas_getcurrent();
    int depth = f1 < 0 ? 0 : (int)f1;
    while(depth-- && x->x_canvas->gl_owner)
        x->x_canvas = x->x_canvas->gl_owner;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_canvas);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = edit_proxy_new(x, gensym(buf));
    outlet_new(&x->x_obj, 0);
    return(x);
}

void setup_canvas0x2eedit(void){
    edit_class = class_new(gensym("canvas.edit"), (t_newmethod)edit_new,
        (t_method)edit_free, sizeof(t_edit), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(edit_class, (t_method)edit_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy),
        CLASS_NOINLET | CLASS_PD, 0);
    class_addbang(edit_class, edit_bang);
    class_addanything(edit_proxy_class, edit_proxy_any);
}
