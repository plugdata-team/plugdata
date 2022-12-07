/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

#define GATE_MINOUTS       1
#define GATE_C74MAXOUTS  100
#define GATE_DEFOUTS       1

typedef struct _gate
{
    t_object    x_ob;
    int         x_open;
    t_pd       *x_proxy;
    int         x_nouts;  /* requested + 1 (as allocated) */
    t_outlet  **x_outs;
} t_gate;

typedef struct _gate_proxy
{
    t_object  p_ob;
    t_gate   *p_master;
} t_gate_proxy;

static t_class *gate_class;
static t_class *gate_proxy_class;

static void gate_proxy_bang(t_gate_proxy *x)
{
    t_gate *master = x->p_master;
    if (master->x_open)
	outlet_bang(master->x_outs[master->x_open]);
}

static void gate_proxy_float(t_gate_proxy *x, t_float f)
{
    t_gate *master = x->p_master;
    if (master->x_open)
	outlet_float(master->x_outs[master->x_open], f);
}

static void gate_proxy_symbol(t_gate_proxy *x, t_symbol *s)
{
    t_gate *master = x->p_master;
    if (master->x_open)
	outlet_symbol(master->x_outs[master->x_open], s);
}

static void gate_proxy_pointer(t_gate_proxy *x, t_gpointer *gp)
{
    t_gate *master = x->p_master;
    if (master->x_open)
	outlet_pointer(master->x_outs[master->x_open], gp);
}

static void gate_proxy_list(t_gate_proxy *x,
			    t_symbol *s, int ac, t_atom *av)
{
    t_gate *master = x->p_master;
    if (master->x_open)
	outlet_list(master->x_outs[master->x_open], s, ac, av);
}

static void gate_proxy_anything(t_gate_proxy *x,
				t_symbol *s, int ac, t_atom *av)
{
    t_gate *master = x->p_master;
    if (master->x_open)
	outlet_anything(master->x_outs[master->x_open], s, ac, av);
}

static void gate_float(t_gate *x, t_float f)
{
    int i = (int)f;
    if (i < 0) i = 1;
    if (i >= x->x_nouts) i = x->x_nouts - 1;
    x->x_open = i;
}

static void gate_bang(t_gate *x)
{
    outlet_float(x->x_outs[1], x->x_open);
}

static void gate_free(t_gate *x)
{
    if (x->x_proxy) pd_free(x->x_proxy);
    if (x->x_outs)
	freebytes(x->x_outs, x->x_nouts * sizeof(*x->x_outs));
}

static void *gate_new(t_floatarg f1, t_floatarg f2)
{
    t_gate *x;
    int i, nouts = (int)f1;
    t_outlet **outs;
    t_pd *proxy;
    if (nouts < GATE_MINOUTS)
        nouts = GATE_DEFOUTS;
    if (nouts > GATE_C74MAXOUTS)
        nouts = GATE_C74MAXOUTS;
    nouts++;  // for convenience (the cost is one pointer)
    if (!(outs = (t_outlet **)getbytes(nouts * sizeof(*outs))))
        return (0);
    if (!(proxy = pd_new(gate_proxy_class))){
        freebytes(outs, nouts * sizeof(*outs));
        return (0);
    }
    x = (t_gate *)pd_new(gate_class);
    x->x_nouts = nouts;
    x->x_outs = outs;
    x->x_proxy = proxy;
    ((t_gate_proxy *)proxy)->p_master = x;
    /* from max sdk manual: ``The dst parameter can be changed (or set to 0)
       dynamically with the inlet_to function...  The gate object uses this
       technique to assign its inlet to one of several outlets, or no outlet
       at all.''  We have to use a proxy, because Pd's outlet is not a t_pd
       (besides, Pd does not handle inlets with null destination). */
    inlet_new((t_object *)x, proxy, 0, 0);
    for (i = 1; i < nouts; i++)
	x->x_outs[i] = outlet_new((t_object *)x, &s_anything);
    gate_float(x, (f2 > 0 ? f2 : 0));  /* CHECKED */
    return (x);
}

CYCLONE_OBJ_API void gate_setup(void)
{
    gate_class = class_new(gensym("gate"),
			   (t_newmethod)gate_new,
			   (t_method)gate_free,
			   sizeof(t_gate), 0,
			   A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(gate_class, gate_float);
    class_addbang(gate_class, gate_bang);
    gate_proxy_class = class_new(gensym("_gate_proxy"), 0, 0,
				 sizeof(t_gate_proxy),
				 CLASS_PD | CLASS_NOINLET, 0);
    class_addfloat(gate_proxy_class, gate_proxy_float);
    class_addbang(gate_proxy_class, gate_proxy_bang);
    class_addsymbol(gate_proxy_class, gate_proxy_symbol);
    class_addpointer(gate_proxy_class, gate_proxy_pointer);
    class_addlist(gate_proxy_class, gate_proxy_list);
    class_addanything(gate_proxy_class, gate_proxy_anything);
}
