/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

#define SWITCH_MININLETS       2  /* LATER consider using 1 (with a warning) */
#define SWITCH_C74MAXINLETS  100
#define SWITCH_DEFINLETS       2

typedef struct _switch
{
    t_object  x_ob;
    int       x_open;
    int       x_ninlets;   /* not counting left one */
    int       x_nproxies;  /* as requested (and allocated) */
    t_pd    **x_proxies;
} t_switch;

typedef struct _switch_proxy
{
    t_object   p_ob;
    t_switch  *p_master;
    int        p_id;
} t_switch_proxy;

static t_class *switch_class;
static t_class *switch_proxy_class;

static void switch_proxy_bang(t_switch_proxy *x)
{
    t_switch *master = x->p_master;
    if (master->x_open == x->p_id)
	outlet_bang(((t_object *)master)->ob_outlet);
}

static void switch_proxy_float(t_switch_proxy *x, t_float f)
{
    t_switch *master = x->p_master;
    if (master->x_open == x->p_id)
	outlet_float(((t_object *)master)->ob_outlet, f);
}

static void switch_proxy_symbol(t_switch_proxy *x, t_symbol *s)
{
    t_switch *master = x->p_master;
    if (master->x_open == x->p_id)
	outlet_symbol(((t_object *)master)->ob_outlet, s);
}

static void switch_proxy_pointer(t_switch_proxy *x, t_gpointer *gp)
{
    t_switch *master = x->p_master;
    if (master->x_open == x->p_id)
	outlet_pointer(((t_object *)master)->ob_outlet, gp);
}

static void switch_proxy_list(t_switch_proxy *x,
			      t_symbol *s, int ac, t_atom *av)
{
    t_switch *master = x->p_master;
    if (master->x_open == x->p_id)
	outlet_list(((t_object *)master)->ob_outlet, s, ac, av);
}

static void switch_proxy_anything(t_switch_proxy *x,
				  t_symbol *s, int ac, t_atom *av)
{
    t_switch *master = x->p_master;
    if (master->x_open == x->p_id)
	outlet_anything(((t_object *)master)->ob_outlet, s, ac, av);
}

static void switch_float(t_switch *x, t_float f)
{
    int i = (int)f;
    if (i < 0) i = -i;
    if (i > x->x_ninlets) i = x->x_ninlets;
    x->x_open = i;
}

static void switch_bang(t_switch *x)
{
    outlet_float(((t_object *)x)->ob_outlet, x->x_open);
}

static void switch_free(t_switch *x)
{
    if (x->x_proxies)
    {
	int i = x->x_ninlets;
	while (i--) pd_free(x->x_proxies[i]);
	freebytes(x->x_proxies, x->x_nproxies * sizeof(*x->x_proxies));
    }
}

static void *switch_new(t_floatarg f1, t_floatarg f2)
{
    t_switch *x;
    int i, ninlets, nproxies = (int)f1;
    t_pd **proxies;
    if (nproxies < SWITCH_MININLETS)
        nproxies = SWITCH_DEFINLETS;
    if (nproxies > SWITCH_C74MAXINLETS)
        nproxies = SWITCH_C74MAXINLETS;
    if (!(proxies = (t_pd **)getbytes(nproxies * sizeof(*proxies))))
        return (0);
    for (ninlets = 0; ninlets < nproxies; ninlets++)
	if (!(proxies[ninlets] = pd_new(switch_proxy_class))) break;
    if (ninlets < SWITCH_MININLETS)
    {
	int i = ninlets;
	while (i--) pd_free(proxies[i]);
	freebytes(proxies, nproxies * sizeof(*proxies));
	return (0);
    }
    x = (t_switch *)pd_new(switch_class);
    x->x_ninlets = ninlets;
    x->x_nproxies = nproxies;
    x->x_proxies = proxies;
    for (i = 0; i < ninlets; i++)
    {
	t_switch_proxy *y = (t_switch_proxy *)proxies[i];
	y->p_master = x;
	y->p_id = i + 1;
	inlet_new((t_object *)x, (t_pd *)y, 0, 0);
    }
    outlet_new((t_object *)x, &s_anything);
    switch_float(x, (f2 > 0 ? f2 : 0));  /* CHECKED */
    return (x);
}

CYCLONE_OBJ_API void switch_setup(void)
{
    switch_class = class_new(gensym("switch"),
			     (t_newmethod)switch_new,
			     (t_method)switch_free,
			     sizeof(t_switch), 0,
			     A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(switch_class, switch_float);
    class_addbang(switch_class, switch_bang);
    switch_proxy_class = class_new(gensym("_switch_proxy"), 0, 0,
				   sizeof(t_switch_proxy),
				   CLASS_PD | CLASS_NOINLET, 0);
    class_addfloat(switch_proxy_class, switch_proxy_float);
    class_addbang(switch_proxy_class, switch_proxy_bang);
    class_addsymbol(switch_proxy_class, switch_proxy_symbol);
    class_addpointer(switch_proxy_class, switch_proxy_pointer);
    class_addlist(switch_proxy_class, switch_proxy_list);
    class_addanything(switch_proxy_class, switch_proxy_anything);
}
