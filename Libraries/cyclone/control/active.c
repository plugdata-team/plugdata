/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include "g_canvas.h"
#include <common/api.h>
#include "control/gui.h"

static t_class *active_class, *mouse_proxy_class;

typedef struct _mouse_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _active *p_parent;
}t_mouse_proxy;

typedef struct _active{
    t_object   x_obj;
    t_mouse_proxy   *x_proxy;
    t_symbol  *x_cname;
    int        x_right_click;
    int        x_on;
} t_active;

static void mouse_proxy_any(t_mouse_proxy *p, t_symbol*s, int ac, t_atom *av){
    ac = 0;
    if(p->p_parent && s == gensym("mouse"))
        p->p_parent->x_right_click = (av+2)->a_w.w_float != 1;
}



static void active_dofocus(t_active *x, t_symbol *s, t_floatarg f){
    int active = (int)(f != 0);
    if(active){ // some window is active
        int this_window = (s == x->x_cname);
        if(x->x_on != this_window)
            outlet_float(x->x_obj.ob_outlet, x->x_on = this_window);
    }
    else{ // f = 0
        if(s == x->x_cname && x->x_on && !x->x_right_click)
            outlet_float(x->x_obj.ob_outlet, x->x_on = 0);
    }
}

static void mouse_proxy_free(t_mouse_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_mouse_proxy * mouse_proxy_new(t_active *x, t_symbol*s){
    t_mouse_proxy *p = (t_mouse_proxy*)pd_new(mouse_proxy_class);
    p->p_parent = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)mouse_proxy_free);
    return(p);
}

static void active_free(t_active *x){
    hammergui_unbindfocus((t_pd *)x);
    x->x_proxy->p_parent = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
}

static void *active_new(void){
    t_active *x = (t_active *)pd_new(active_class);
    t_canvas *cnv = canvas_getcurrent();
    x->x_right_click = x->x_on = 0;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)cnv);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = mouse_proxy_new(x, gensym(buf));
    snprintf(buf, MAXPDSTRING-1, ".x%lx.c", (unsigned long)cnv);
    buf[MAXPDSTRING-1] = 0;
    x->x_cname = gensym(buf);
    outlet_new((t_object *)x, &s_float);
    hammergui_bindfocus((t_pd *)x);
    return (x);
}

CYCLONE_OBJ_API void active_setup(void){
    active_class = class_new(gensym("active"), (t_newmethod)active_new,
        (t_method)active_free, sizeof(t_active), CLASS_NOINLET, 0);
    class_addmethod(active_class, (t_method)active_dofocus, gensym("_focus"), A_SYMBOL, A_FLOAT, 0);
    mouse_proxy_class = class_new(0, 0, 0, sizeof(t_mouse_proxy),
        CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(mouse_proxy_class, mouse_proxy_any);
    class_addmethod(active_class, (t_method)active_dofocus, gensym("_focus"), A_SYMBOL, A_FLOAT, 0);
}
