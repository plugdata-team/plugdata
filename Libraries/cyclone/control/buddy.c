/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER compare with buddy.c from max sdk */
/* LATER revisit buffer handling (maxsize, reentrancy) */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define BUDDY_MINSLOTS  2
#define BUDDY_INISIZE   4  /* LATER rethink */

typedef struct _buddy
{
    t_object    x_ob;
    int         x_nslots;
    int         x_nproxies;  /* as requested (and allocated) */
    t_pd      **x_proxies;
    t_outlet  **x_outs;
} t_buddy;

typedef struct _buddy_proxy
{
    t_object     p_ob;
    t_buddy     *p_master;
    t_symbol    *p_selector;
    t_float      p_float;
    t_symbol    *p_symbol;
    t_gpointer  *p_pointer;
    int          p_size;    /* as allocated */
    int          p_natoms;  /* as used */
    t_atom      *p_message;
    t_atom       p_messini[BUDDY_INISIZE];
} t_buddy_proxy;

static t_class *buddy_class;
static t_class *buddy_proxy_class;

static void buddy_clear(t_buddy *x)
{
    t_buddy_proxy **p = (t_buddy_proxy **)x->x_proxies;
    int i = x->x_nslots;
    while (i--)
    {
	(*p)->p_selector = 0;
	(*p++)->p_natoms = 0;  /* defensive */
    }
}

static void buddy_check(t_buddy *x)
{
    t_buddy_proxy **p = (t_buddy_proxy **)x->x_proxies;
    int i = x->x_nslots;
    while (i--)
	if (!(*p++)->p_selector)
	    return;
    p = (t_buddy_proxy **)x->x_proxies;
    i = x->x_nslots;
    while (i--)
    {
	t_symbol *s = p[i]->p_selector;
	if (s == &s_bang)
	    outlet_bang(x->x_outs[i]);
	else if (s == &s_float)
	    outlet_float(x->x_outs[i], p[i]->p_float);
	else if (s == &s_symbol && p[i]->p_symbol)
	    outlet_symbol(x->x_outs[i], p[i]->p_symbol);
	else if (s == &s_pointer)
	{
	    /* LATER */
	}
	else if (s == &s_list)
	    outlet_list(x->x_outs[i], s, p[i]->p_natoms, p[i]->p_message);
	else if (s)
	    outlet_anything(x->x_outs[i], s, p[i]->p_natoms, p[i]->p_message);
    }
    buddy_clear(x);
}

static void buddy_proxy_float(t_buddy_proxy *x, t_float f)
{
    x->p_selector = &s_float;
    x->p_float = f;
    x->p_natoms = 0;  /* defensive */
    buddy_check(x->p_master);
}

static void buddy_proxy_bang(t_buddy_proxy *x)
{

    buddy_proxy_float(x, 0);
    //x->p_selector = &s_bang;
    //x->p_natoms = 0;  /* defensive */
    //buddy_check(x->p_master);
    
}

static void buddy_proxy_symbol(t_buddy_proxy *x, t_symbol *s)
{
    x->p_selector = &s_symbol;
    x->p_symbol = s;
    x->p_natoms = 0;  /* defensive */
    buddy_check(x->p_master);
}

static void buddy_proxy_pointer(t_buddy_proxy *x, t_gpointer *gp)
{
    x->p_selector = &s_pointer;
    x->p_pointer = gp;
    x->p_natoms = 0;  /* defensive */
    buddy_check(x->p_master);
}

static void buddy_proxy_domessage(t_buddy_proxy *x, int ac, t_atom *av)
{
    if (ac > x->p_size)
    {
	/* LATER consider using BUDDY_MAXSIZE (and warning if exceeded) */
	x->p_message = grow_nodata(&ac, &x->p_size, x->p_message,
				   BUDDY_INISIZE, x->p_messini,
				   sizeof(*x->p_message));
    }
    x->p_natoms = ac;
    memcpy(x->p_message, av, ac * sizeof(*x->p_message));
    buddy_check(x->p_master);
}

static void buddy_proxy_list(t_buddy_proxy *x,
			     t_symbol *s, int ac, t_atom *av)
{
    x->p_selector = &s_list;  /* LATER rethink */
    buddy_proxy_domessage(x, ac, av);
}

static void buddy_proxy_anything(t_buddy_proxy *x,
				 t_symbol *s, int ac, t_atom *av)
{
    x->p_selector = s;  /* LATER rethink */
    buddy_proxy_domessage(x, ac, av);
}

static void buddy_bang(t_buddy *x)
{
    buddy_proxy_bang((t_buddy_proxy *)x->x_proxies[0]);
}

static void buddy_float(t_buddy *x, t_float f)
{
    buddy_proxy_float((t_buddy_proxy *)x->x_proxies[0], f);
}

static void buddy_symbol(t_buddy *x, t_symbol *s)
{
    buddy_proxy_symbol((t_buddy_proxy *)x->x_proxies[0], s);
}

static void buddy_pointer(t_buddy *x, t_gpointer *gp)
{
    buddy_proxy_pointer((t_buddy_proxy *)x->x_proxies[0], gp);
}

static void buddy_list(t_buddy *x, t_symbol *s, int ac, t_atom *av)
{
    buddy_proxy_list((t_buddy_proxy *)x->x_proxies[0], s, ac, av);
}

static void buddy_anything(t_buddy *x, t_symbol *s, int ac, t_atom *av)
{
    buddy_proxy_anything((t_buddy_proxy *)x->x_proxies[0], s, ac, av);
}

static void buddy_free(t_buddy *x)
{
    if (x->x_proxies)
    {
	int i = x->x_nslots;
	while (i--)
	{
	    t_buddy_proxy *y = (t_buddy_proxy *)x->x_proxies[i];
	    if (y->p_message != y->p_messini)
		freebytes(y->p_message, y->p_size * sizeof(*y->p_message));
	    pd_free((t_pd *)y);
	}
	freebytes(x->x_proxies, x->x_nproxies * sizeof(*x->x_proxies));
    }
    if (x->x_outs)
	freebytes(x->x_outs, x->x_nslots * sizeof(*x->x_outs));
}

static void *buddy_new(t_floatarg f)
{
    t_buddy *x;
    int i, nslots, nproxies = (int)f;
    t_pd **proxies;
    t_outlet **outs;
    if (nproxies < BUDDY_MINSLOTS)
	nproxies = BUDDY_MINSLOTS;
    if (!(proxies = (t_pd **)getbytes(nproxies * sizeof(*proxies))))
	return (0);
    for (nslots = 0; nslots < nproxies; nslots++)
	if (!(proxies[nslots] = pd_new(buddy_proxy_class))) break;
    if (nslots < BUDDY_MINSLOTS
	|| !(outs = (t_outlet **)getbytes(nslots * sizeof(*outs))))
    {
	int i = nslots;
	while (i--) pd_free(proxies[i]);
	freebytes(proxies, nproxies * sizeof(*proxies));
	return (0);
    }
    x = (t_buddy *)pd_new(buddy_class);
    x->x_nslots = nslots;
    x->x_nproxies = nproxies;
    x->x_proxies = proxies;
    x->x_outs = outs;
    for (i = 0; i < nslots; i++)
    {
	t_buddy_proxy *y = (t_buddy_proxy *)proxies[i];
	y->p_master = x;
	y->p_selector = 0;
	y->p_float = 0;
	y->p_symbol = 0;
	y->p_pointer = 0;
	y->p_size = BUDDY_INISIZE;
	y->p_natoms = 0;
	y->p_message = y->p_messini;
	if (i) inlet_new((t_object *)x, (t_pd *)y, 0, 0);
	x->x_outs[i] = outlet_new((t_object *)x, &s_anything);
    }
    return (x);
}

CYCLONE_OBJ_API void buddy_setup(void)
{
    buddy_class = class_new(gensym("buddy"),
			    (t_newmethod)buddy_new,
			    (t_method)buddy_free,
			    sizeof(t_buddy), 0, A_DEFFLOAT, 0);
    class_addbang(buddy_class, buddy_bang);
    class_addfloat(buddy_class, buddy_float);
    class_addsymbol(buddy_class, buddy_symbol);
    class_addpointer(buddy_class, buddy_pointer);
    class_addlist(buddy_class, buddy_list);
    class_addanything(buddy_class, buddy_anything);
    class_addmethod(buddy_class, (t_method)buddy_clear, gensym("clear"), 0);
    buddy_proxy_class = class_new(gensym("_buddy_proxy"), 0, 0,
				  sizeof(t_buddy_proxy),
				  CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(buddy_proxy_class, buddy_proxy_bang);
    class_addfloat(buddy_proxy_class, buddy_proxy_float);
    class_addsymbol(buddy_proxy_class, buddy_proxy_symbol);
    class_addpointer(buddy_proxy_class, buddy_proxy_pointer);
    class_addlist(buddy_proxy_class, buddy_proxy_list);
    class_addanything(buddy_proxy_class, buddy_proxy_anything);
}
