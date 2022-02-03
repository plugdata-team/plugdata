/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER revisit buffer handling (maxsize, reentrancy) */
/* LATER rethink handling of symbols */

/* CHECKED: if 'n' argument is given, then store whole messages, instead of
   distributing atoms across successive slots.  This is a new, buddy-like
   behaviour. */
/* CHECKED: without 'n' argument, 'symbol test' is parsed as two symbol atoms,
   but 'float 1' (or 'list 1') as a single float. */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define BONDO_MINSLOTS  2
#define BONDO_INISIZE   4  /* LATER rethink (useful only in multiatom mode) */

typedef struct _bondo
{
    t_object    x_ob;
    t_float     x_delay;
    int         x_multiatom;
    int         x_nslots;
    int         x_nproxies;  /* as requested (and allocated) */
    t_pd      **x_proxies;
    t_outlet  **x_outs;
    t_clock    *x_clock;
} t_bondo;

typedef struct _bondo_proxy
{
    t_object     p_ob;
    t_bondo     *p_master;
    int          p_id;
    t_symbol    *p_selector;
    t_float      p_float;
    t_symbol    *p_symbol;
    t_gpointer  *p_pointer;
    int          p_size;    /* as allocated */
    int          p_natoms;  /* as used */
    t_atom      *p_message;
    t_atom       p_messini[BONDO_INISIZE];
} t_bondo_proxy;

static t_class *bondo_class;
static t_class *bondo_proxy_class;

static void bondo_doit(t_bondo *x)
{
    t_bondo_proxy **p = (t_bondo_proxy **)x->x_proxies;
    int i = x->x_nslots;
    p = (t_bondo_proxy **)x->x_proxies;
    i = x->x_nslots;
    while (i--)
    {
	t_symbol *s = p[i]->p_selector;
	/* LATER consider complaining about extra arguments (CHECKED) */
	if (s == &s_bang)
	    outlet_bang(x->x_outs[i]);
	else if (s == &s_float)
	    outlet_float(x->x_outs[i], p[i]->p_float);
	else if (s == &s_symbol && p[i]->p_symbol)
	{
            
	    outlet_symbol(x->x_outs[i], p[i]->p_symbol);
	    /* LATER rethink */
            /* old code
	    if (x->x_multiatom)
		outlet_symbol(x->x_outs[i], p[i]->p_symbol);
	    else
		outlet_anything(x->x_outs[i], p[i]->p_symbol, 0, 0);
            */
	}
	else if (s == &s_pointer)
	{
	    /* LATER */
	}
	else if (s == &s_list)
	    outlet_list(x->x_outs[i], s, p[i]->p_natoms, p[i]->p_message);
	else if (s)  /* CHECKED: a slot may be inactive (in multiatom mode) */
	    outlet_anything(x->x_outs[i], s, p[i]->p_natoms, p[i]->p_message);
        //added for 1-elt anythings that are symbols
        else if(!s && (p[i]->p_symbol != &s_ || p[i]->p_symbol != NULL) && p[i]->p_natoms == 0)
	    outlet_anything(x->x_outs[i], p[i]->p_symbol, 0, 0);
    }
}

static void bondo_arm(t_bondo *x)
{
    if (x->x_delay <= 0) bondo_doit(x);
    else clock_delay(x->x_clock, x->x_delay);
}

static void bondo_proxy_bang(t_bondo_proxy *x)
{
    bondo_arm(x->p_master);  /* CHECKED: bang in any inlet works in this way */
}

static void bondo_proxy_dofloat(t_bondo_proxy *x, t_float f, int doit)
{
    x->p_selector = &s_float;
    x->p_float = f;
    x->p_natoms = 0;  /* defensive */
    if (doit) bondo_arm(x->p_master);
}

static void bondo_proxy_float(t_bondo_proxy *x, t_float f)
{
    bondo_proxy_dofloat(x, f, 1);
}

//when we don't want a selector for symbol - DK
static void bondo_proxy_donosymbol(t_bondo_proxy *x, t_symbol *s, int doit)
{

    x->p_selector = NULL;
    x->p_symbol = s;
    x->p_natoms = 0;  /* defensive */
    if (doit) bondo_arm(x->p_master);

}

static void bondo_proxy_dosymbol(t_bondo_proxy *x, t_symbol *s, int doit)
{
    x->p_selector = &s_symbol;
    x->p_symbol = s;
    x->p_natoms = 0;  /* defensive */
    if (doit) bondo_arm(x->p_master);
}

static void bondo_proxy_symbol(t_bondo_proxy *x, t_symbol *s)
{
    bondo_proxy_dosymbol(x, s, 1);
}

static void bondo_proxy_dopointer(t_bondo_proxy *x, t_gpointer *gp, int doit)
{
    x->p_selector = &s_pointer;
    x->p_pointer = gp;
    x->p_natoms = 0;  /* defensive */
    if (doit) bondo_arm(x->p_master);
}

static void bondo_proxy_pointer(t_bondo_proxy *x, t_gpointer *gp)
{
    bondo_proxy_dopointer(x, gp, 1);
}

/* CHECKED: the slots fire in right-to-left order,
   but they trigger only once (refman error) */
static void bondo_distribute(t_bondo *x, int startid,
			     t_symbol *s, int ac, t_atom *av, int doit)
{
    t_atom *ap = av;
    t_bondo_proxy **pp;
    int id = startid + ac;
    if (s) id++;
    if (id > x->x_nslots)
	id = x->x_nslots;
    ap += id - startid;
    pp = (t_bondo_proxy **)(x->x_proxies + id);
    if (s) ap--;
    while (ap-- > av)
    {
	pp--;
	if (ap->a_type == A_FLOAT)
	    bondo_proxy_dofloat(*pp, ap->a_w.w_float, 0);
	else if (ap->a_type == A_SYMBOL)
	    bondo_proxy_donosymbol(*pp, ap->a_w.w_symbol, 0);
	else if (ap->a_type == A_POINTER)
	    bondo_proxy_dopointer(*pp, ap->a_w.w_gpointer, 0);
    }
    if (s)
	bondo_proxy_donosymbol((t_bondo_proxy *)x->x_proxies[startid], s, 0);
    if (doit) bondo_arm(x);
}

static void bondo_proxy_domultiatom(t_bondo_proxy *x,
				    int ac, t_atom *av, int doit)
{
    if (ac > x->p_size)
    {
	/* LATER consider using BONDO_MAXSIZE (and warning if exceeded) */
	x->p_message = grow_nodata(&ac, &x->p_size, x->p_message,
				   BONDO_INISIZE, x->p_messini,
				   sizeof(*x->p_message));
    }
    x->p_natoms = ac;
    memcpy(x->p_message, av, ac * sizeof(*x->p_message));
    if (doit) bondo_arm(x->p_master);
}

static void bondo_proxy_dolist(t_bondo_proxy *x, int ac, t_atom *av, int doit)
{
    if (x->p_master->x_multiatom)
    {
	x->p_selector = &s_list;
	bondo_proxy_domultiatom(x, ac, av, doit);
    }
    else bondo_distribute(x->p_master, x->p_id, 0, ac, av, doit);
}

static void bondo_proxy_list(t_bondo_proxy *x,
			     t_symbol *s, int ac, t_atom *av)
{
    bondo_proxy_dolist(x, ac, av, 1);
}

static void bondo_proxy_doanything(t_bondo_proxy *x,
				   t_symbol *s, int ac, t_atom *av, int doit)
{
    if (x->p_master->x_multiatom)
    {
	/* LATER rethink and CHECKME */
	if (s == &s_symbol)
	{
	    if (ac && av->a_type == A_SYMBOL)
		bondo_proxy_dosymbol(x, av->a_w.w_symbol, doit);
	    else
		bondo_proxy_dosymbol(x, &s_symbol, doit);
	}
	else
	{
            x->p_selector = s;
	    bondo_proxy_domultiatom(x, ac, av, doit);
	}
    }
    else bondo_distribute(x->p_master, x->p_id, s, ac, av, doit);
}

static void bondo_proxy_anything(t_bondo_proxy *x,
				 t_symbol *s, int ac, t_atom *av)
{
    bondo_proxy_doanything(x, s, ac, av, 1);
}

static void bondo_proxy_set(t_bondo_proxy *x,
			    t_symbol *s, int ac, t_atom *av)
{
    if (ac)
    {
	if (av->a_type == A_FLOAT)
	{
	    if (ac > 1)
		bondo_proxy_dolist(x, ac, av, 0);
	    else
		bondo_proxy_dofloat(x, av->a_w.w_float, 0);
	}
	else if (av->a_type == A_SYMBOL)
	    /* CHECKED: no tests for 'set float ...' and 'set list...' --
	       the parsing is made in an output routine */
	    bondo_proxy_doanything(x, av->a_w.w_symbol, ac-1, av+1, 0);
	else if (av->a_type == A_POINTER)
	    bondo_proxy_dopointer(x, av->a_w.w_gpointer, 0);
    }
    /* CHECKED: 'set' without arguments makes a slot inactive,
       if multiatom, but is ignored, if !multiatom */
    else if (x->p_master->x_multiatom) x->p_selector = 0;
}

static void bondo_bang(t_bondo *x)
{
    bondo_proxy_bang((t_bondo_proxy *)x->x_proxies[0]);
}

static void bondo_float(t_bondo *x, t_float f)
{
    bondo_proxy_dofloat((t_bondo_proxy *)x->x_proxies[0], f, 1);
}

static void bondo_symbol(t_bondo *x, t_symbol *s)
{
    bondo_proxy_dosymbol((t_bondo_proxy *)x->x_proxies[0], s, 1);
}

static void bondo_pointer(t_bondo *x, t_gpointer *gp)
{
    bondo_proxy_dopointer((t_bondo_proxy *)x->x_proxies[0], gp, 1);
}

static void bondo_list(t_bondo *x, t_symbol *s, int ac, t_atom *av)
{
    bondo_proxy_dolist((t_bondo_proxy *)x->x_proxies[0], ac, av, 1);
}

static void bondo_anything(t_bondo *x, t_symbol *s, int ac, t_atom *av)
{
    bondo_proxy_doanything((t_bondo_proxy *)x->x_proxies[0], s, ac, av, 1);
}

static void bondo_set(t_bondo *x, t_symbol *s, int ac, t_atom *av)
{
    bondo_proxy_set((t_bondo_proxy *)x->x_proxies[0], s, ac, av);
}

static void bondo_free(t_bondo *x)
{
    if (x->x_clock) clock_free(x->x_clock);
    if (x->x_proxies)
    {
	int i = x->x_nslots;
	while (i--)
	{
	    t_bondo_proxy *y = (t_bondo_proxy *)x->x_proxies[i];
	    if (y->p_message != y->p_messini)
		freebytes(y->p_message, y->p_size * sizeof(*y->p_message));
	    pd_free((t_pd *)y);
	}
	freebytes(x->x_proxies, x->x_nproxies * sizeof(*x->x_proxies));
    }
    if (x->x_outs)
	freebytes(x->x_outs, x->x_nslots * sizeof(*x->x_outs));
}

static void *bondo_new(t_symbol *s, int ac, t_atom *av)
{
    t_bondo *x;
    int i, nslots, nproxies = BONDO_MINSLOTS;
    int multiatom = 0;
    t_float delay = 0;
    t_pd **proxies;
    t_outlet **outs;
    i = 0;
    while (ac--)
    {
	/* CHECKED: no warnings */
	if (av->a_type == A_FLOAT)
	{
	    if (i == 0)
		nproxies = (int)av->a_w.w_float;
	    else if (i == 1)
		delay = av->a_w.w_float;
	    i++;
	}
	else if (av->a_type == A_SYMBOL)
	{
	    if (av->a_w.w_symbol == gensym("n")) multiatom = 1;
	    /* CHECKED: 'n' has to be the last argument given;
	       the arguments after any symbol are silently ignored --
	       LATER decide if we should comply and break here */
	}
	av++;
    }
    if (nproxies < BONDO_MINSLOTS)
	nproxies = BONDO_MINSLOTS;
    if (!(proxies = (t_pd **)getbytes(nproxies * sizeof(*proxies))))
	return (0);
    for (nslots = 0; nslots < nproxies; nslots++)
	if (!(proxies[nslots] = pd_new(bondo_proxy_class))) break;
    if (nslots < BONDO_MINSLOTS
	|| !(outs = (t_outlet **)getbytes(nslots * sizeof(*outs))))
    {
	i = nslots;
	while (i--) pd_free(proxies[i]);
	freebytes(proxies, nproxies * sizeof(*proxies));
	return (0);
    }
    x = (t_bondo *)pd_new(bondo_class);
    x->x_delay = delay;
    x->x_multiatom = multiatom;
    x->x_nslots = nslots;
    x->x_nproxies = nproxies;
    x->x_proxies = proxies;
    x->x_outs = outs;
    for (i = 0; i < nslots; i++)
    {
	t_bondo_proxy *y = (t_bondo_proxy *)proxies[i];
	y->p_master = x;
	y->p_id = i;
	y->p_selector = &s_float;  /* CHECKED: it is so in multiatom mode too */
	y->p_float = 0;
	y->p_symbol = 0;
	y->p_pointer = 0;
	y->p_size = BONDO_INISIZE;
	y->p_natoms = 0;
	y->p_message = y->p_messini;
	if (i) inlet_new((t_object *)x, (t_pd *)y, 0, 0);
	x->x_outs[i] = outlet_new((t_object *)x, &s_anything);
    }
    x->x_clock = clock_new(x, (t_method)bondo_doit);
    return (x);
}

CYCLONE_OBJ_API void bondo_setup(void)
{
    bondo_class = class_new(gensym("bondo"),
			    (t_newmethod)bondo_new,
			    (t_method)bondo_free,
			    sizeof(t_bondo), 0, A_GIMME, 0);
    class_addbang(bondo_class, bondo_bang);
    class_addfloat(bondo_class, bondo_float);
    class_addsymbol(bondo_class, bondo_symbol);
    class_addpointer(bondo_class, bondo_pointer);
    class_addlist(bondo_class, bondo_list);
    class_addanything(bondo_class, bondo_anything);
    class_addmethod(bondo_class, (t_method)bondo_set,
		    gensym("set"), A_GIMME, 0);
    bondo_proxy_class = class_new(gensym("_bondo_proxy"), 0, 0,
				  sizeof(t_bondo_proxy),
				  CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(bondo_proxy_class, bondo_proxy_bang);
    class_addfloat(bondo_proxy_class, bondo_proxy_float);
    class_addsymbol(bondo_proxy_class, bondo_proxy_symbol);
    class_addpointer(bondo_proxy_class, bondo_proxy_pointer);
    class_addlist(bondo_proxy_class, bondo_proxy_list);
    class_addanything(bondo_proxy_class, bondo_proxy_anything);
    class_addmethod(bondo_proxy_class, (t_method)bondo_proxy_set,
		    gensym("set"), A_GIMME, 0);
}
