/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

//made the right inlet a proxy inlet with corresponding proxy class to take care of anythings - DK 2016

//proxy inlet to take care of anythings
typedef struct _onebang_proxy
{
    t_pd            l_pd;
    t_inlet         *p_inlet;
    struct _onebang *p_owner;
} t_onebang_proxy;

static t_class *onebang_proxy_class;

typedef struct _onebang
{
    t_object  x_obj;
    t_onebang_proxy pxy; //proxy inlet
    int       x_isopen;
    t_outlet * x_r_bng;
} t_onebang;

static t_class *onebang_class;

//proxy inlet stuff

static void onebang_proxy_init(t_onebang_proxy * p, t_onebang *x){
    p->l_pd = onebang_proxy_class;
    p->p_owner = (void *) x;
}

static void *onebang_proxy_new(t_symbol *s, int argc, t_atom *argv){
    t_onebang_proxy *p = (t_onebang_proxy *)pd_new(onebang_proxy_class);
    return p;
}

static void onebang_proxy_free(t_onebang_proxy *p){
 //i guess we don't really need this?
}

static void onebang_proxy_anything(t_onebang_proxy *p, t_symbol *s, int argc, t_atom * argv){
    t_onebang *x = p->p_owner;
    x->x_isopen = 1;
}

static void onebang_proxy_setup(void){
    onebang_proxy_class = (t_class *)class_new(gensym("onebang_proxy"),
            (t_newmethod)onebang_proxy_new, (t_method)onebang_proxy_free, sizeof(t_onebang_proxy),
            0, A_GIMME, 0);
    class_addanything(onebang_proxy_class, (t_method)onebang_proxy_anything);
}

static void onebang_bang(t_onebang *x)
{
    if (x->x_isopen)
    {
        outlet_bang(((t_object *)x)->ob_outlet);
        x->x_isopen = 0;
    }
    else
        outlet_bang(x->x_r_bng);
}

static void onebang_float(t_onebang *x, t_float f)
{
    onebang_bang(x);
}


static void onebang_symbol(t_onebang *x, t_symbol *s)
{
    onebang_bang(x);
}


static void onebang_list(t_onebang *x, t_symbol *s, int ac, t_atom *av)
{
    onebang_bang(x);
}

static void onebang_anything(t_onebang *x, t_symbol *s, int ac, t_atom *av)
{
    onebang_bang(x);
}


static void onebang_stop(t_onebang *x)
{
    x->x_isopen = 0;
}

static void onebang_bang1(t_onebang *x){
    x->x_isopen = 1;
}

static void *onebang_new(t_floatarg f)
{
    t_onebang *x = (t_onebang *)pd_new(onebang_class);
    onebang_proxy_init(&x->pxy, x);
    x->x_isopen = ((int)f != 0);  /* CHECKED */
    inlet_new(&x->x_obj, &x->pxy.l_pd, 0, 0); // change to anything
    outlet_new((t_object *)x, &s_bang);
    x->x_r_bng = outlet_new((t_object *)x, &s_bang);
    return (x);
}

CYCLONE_OBJ_API void onebang_setup(void)
{
    onebang_class = class_new(gensym("onebang"),
			      (t_newmethod)onebang_new, 0,
			      sizeof(t_onebang), 0, A_DEFFLOAT, 0);
    class_addbang(onebang_class, onebang_bang);
    class_addmethod(onebang_class, (t_method)onebang_bang1,
		    gensym("bang1"), 0);
    class_addmethod(onebang_class, (t_method)onebang_stop,
                    gensym("stop"), 0);
    class_addfloat(onebang_class, onebang_float);
    class_addsymbol(onebang_class, onebang_symbol);
    class_addlist(onebang_class, onebang_list);
    class_addanything(onebang_class, onebang_anything);

    onebang_proxy_setup();
}
