/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKED (it cannot be emulated): creating [s/r <pv-symbol>] prints
   "error:send/receive:<pv-symbol>:already exists"
*/

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "g_canvas.h"
#include "common/grow.h"

#define PV_INISIZE   32  /* LATER rethink */
#define PV_MAXSIZE  256

typedef struct _pvfamily
{
    t_symbol          *f_selector;
    t_float            f_float;
    t_symbol          *f_symbol;
    t_gpointer        *f_pointer;
    int                f_size;    /* as allocated */
    int                f_natoms;  /* as used */
    t_atom            *f_message;
    t_atom             f_messini[PV_INISIZE];
    t_glist           *f_glist;   /* root glist of a family */
    t_symbol          *f_name;
    struct _pvfamily  *f_next;
} t_pvfamily;

typedef struct _pvlist
{
    t_pd         l_pd;
    int          l_refcount;
    t_symbol    *l_name;
    t_pvfamily  *l_pvlist;
} t_pvlist;

typedef struct _pv
{
    t_object     x_ob;
    t_glist     *x_glist;
    t_symbol    *x_name;
    t_pvfamily  *x_family;
} t_pv;

static t_class *pv_class;
static t_class *pvlist_class;

//dummy functions
static void pvlist_bang(t_pvlist *pl){}
static void pvlist_float(t_pvlist *pl, t_float f){}
static void pvlist_symbol(t_pvlist *pl, t_symbol *s){}
static void pvlist_pointer(t_pvlist *pl, t_gpointer *gp){}
static void pvlist_anything(t_pvlist *pl, t_symbol *s, int argc, t_atom * argv){}
static void pvlist_list(t_pvlist *pl, t_symbol *s, int argc, t_atom * argv){}


static void pvlist_decrement(t_pvlist *pl)
{
    if (!--pl->l_refcount)
    {
	pd_unbind(&pl->l_pd, pl->l_name);
	pd_free(&pl->l_pd);
    }
}

static t_pvlist *pv_getlist(t_symbol *s, int create)
{
    t_pvlist *pl = (t_pvlist *)pd_findbyclass(s, pvlist_class);
    if (pl)
    {
	if (create) pl->l_refcount++;
    }
    else
    {
	if (create)
	{
	    pl = (t_pvlist *)pd_new(pvlist_class);
	    pl->l_refcount = 1;
	    pl->l_name = s;
	    pl->l_pvlist = 0;
	    pd_bind(&pl->l_pd, s);
	}
	else
        post("bug [pv]: pv_getlist");
    }
    return (pl);
}

static t_pvfamily *pv_newfamily(t_pvlist *pvlist)
{
    t_pvfamily *pf = (t_pvfamily *)getbytes(sizeof(*pf));
    pf->f_name = pvlist->l_name;
    pf->f_next = pvlist->l_pvlist;
    pvlist->l_pvlist = pf;
    pf->f_selector = 0;
    pf->f_float = 0;
    pf->f_symbol = 0;
    pf->f_pointer = 0;
    pf->f_size = PV_INISIZE;
    pf->f_natoms = 0;
    pf->f_message = pf->f_messini;
    return (pf);
}

static void pvfamily_free(t_pvfamily *pf)
{
    if (pf->f_message != pf->f_messini)
	freebytes(pf->f_message, pf->f_size * sizeof(*pf->f_message));
    freebytes(pf, sizeof(*pf));
}

static void pv_update(t_glist *glist, t_pvfamily *pf)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
	    pv_update((t_glist *)g, pf);
	else if (pd_class(&g->g_pd) == pv_class
		 && ((t_pv *)g)->x_name == pf->f_name)
	    ((t_pv *)g)->x_family = pf;
}

static t_pvfamily *pvfamily_reusable;

static void pv_breakup(t_pvlist *pvlist, t_glist *glist)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
    {
	if (pd_class(&g->g_pd) == pv_class
	    && ((t_pv *)g)->x_name == pvlist->l_name)
	{
	    t_pvfamily *pf;
	    if (!(pf = pvfamily_reusable))
		pf = pv_newfamily(pvlist);
	    else pvfamily_reusable = 0;
	    pf->f_glist = glist;
	    pv_update(glist, pf);
	    /* LATER keep current value */
	    return;
	}
    }
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
	    pv_breakup(pvlist, (t_glist *)g);
}

/* join all families of a 'pvlist' rooted in any subglist of a 'glist' */
static t_pvfamily *pv_joinup(t_pvlist *pvlist, t_glist *glist)
{
    t_pvfamily *result = 0;
    t_pvfamily *pf, *pfprev, *pfnext;
    for (pfprev = 0, pf = pvlist->l_pvlist; pf; pfprev = pf, pf = pfnext)
    {
	t_glist *gl;
	pfnext = pf->f_next;
	for (gl = pf->f_glist; gl; gl = gl->gl_owner)
	{
	    if (gl == glist)
	    {
		if (result)
		{
		    pvfamily_free(pf);
		    if (pfprev)
			pfprev->f_next = pfnext;
		    else
			pvlist->l_pvlist = pfnext;
		    pf = pfprev;
		}
		else result = pf;
		break;
	    }
	}
    }
    return (result);
}

/* Called normally with either 'create' or 'destroy' set to 1,
   but it might be called with both set to 0 for testing. */
static t_pvfamily *pv_getfamily(t_glist *glist, t_symbol *s,
				int create, int destroy)
{
    t_pvlist *pl = pv_getlist(s, create);
    if (pl)
    {
	if (destroy)
	{
	    t_pvfamily *pf, *mypf;
	    t_glist *gl;
	    for (mypf = pl->l_pvlist; mypf; mypf = mypf->f_next)
		if (mypf->f_glist == glist)
		    break;
	    /* mypf is not null iff we are invoked via a [pv] in root */
	    /* now check if there is a family rooted in a super-branch */
	    for (gl = glist->gl_owner; gl; gl = gl->gl_owner)
	    {
		for (pf = pl->l_pvlist; pf; pf = pf->f_next)
		{
		    if (pf->f_glist == gl)
		    {
			if (mypf)
                post("bug [pv]: pv_getfamily 1: %s in %s",
					mypf->f_name->s_name,
					mypf->f_glist->gl_name->s_name);
			else
			    return (0);
		    }
		}
	    }
	    if (mypf)
	    {
		pvfamily_reusable = mypf;
		pv_breakup(pl, glist);
		if (pvfamily_reusable == mypf)
		{
		    pvfamily_reusable = 0;
		    if (pl->l_pvlist == mypf)
			pl->l_pvlist = mypf->f_next;
		    else
		    {
			for (pf = pl->l_pvlist; pf; pf = pf->f_next)
			{
			    if (pf->f_next == mypf)
			    {
				pf->f_next = mypf->f_next;
				break;
			    }
			}
			if (!pf)
                post("bug [pv]: pv_getfamily 2");
		    }
		    pvfamily_free(mypf);
		}
	    }
        else
            post("bug [pv]: pv_getfamily 3");
	    pvlist_decrement(pl);
	}
	else
	{
	    t_pvfamily *pf;
	    t_glist *gl;
	    for (gl = glist; gl; gl = gl->gl_owner)
		for (pf = pl->l_pvlist; pf; pf = pf->f_next)
		    if (pf->f_glist == gl)
			return (pf);
	    if (create)
	    {
		if (!(pf = pv_joinup(pl, glist)))
		    pf = pv_newfamily(pl);
		pf->f_glist = glist;
		pv_update(glist, pf);
		return (pf);
	    }
	    else
            post("bug [pv]: pv_getfamily 4");
	}
    }
    else
        post("bug [pv]: pv_getfamily 5");
    return (0);
}

static t_pvfamily *pv_checkfamily(t_pv *x)
{
    if (!x->x_family)
    {
        post("bug [pv]: pv_checkfamily");
        x->x_family = pv_getfamily(x->x_glist, x->x_name, 0, 0);
    }
    return (x->x_family);
}

static void pv_bang(t_pv *x)
{
    t_pvfamily *pf = pv_checkfamily(x);
    if (pf)
    {
	t_symbol *s = pf->f_selector;
	if (s == &s_bang)
	    outlet_bang(((t_object *)x)->ob_outlet);
	else if (s == &s_float)
	    outlet_float(((t_object *)x)->ob_outlet, pf->f_float);
	else if (s == &s_symbol && pf->f_symbol)
	    outlet_symbol(((t_object *)x)->ob_outlet, pf->f_symbol);
	else if (s == &s_pointer)
	{
	    /* LATER */
	}
	else if (s == &s_list)
	    outlet_list(((t_object *)x)->ob_outlet,
			s, pf->f_natoms, pf->f_message);
	else if (s)
	    outlet_anything(((t_object *)x)->ob_outlet,
			    s, pf->f_natoms, pf->f_message);
    }
}

static void pv_float(t_pv *x, t_float f)
{
    t_pvfamily *pf = pv_checkfamily(x);
    if (pf)
    {
	pf->f_selector = &s_float;
	pf->f_float = f;
	pf->f_natoms = 0;  /* defensive */
    }
}

static void pv_symbol(t_pv *x, t_symbol *s)
{
    t_pvfamily *pf = pv_checkfamily(x);
    if (pf)
    {
	pf->f_selector = &s_symbol;
	pf->f_symbol = s;
	pf->f_natoms = 0;  /* defensive */
    }
}

static void pv_pointer(t_pv *x, t_gpointer *gp)
{
    t_pvfamily *pf = pv_checkfamily(x);
    if (pf)
    {
	pf->f_selector = &s_pointer;
	pf->f_pointer = gp;
	pf->f_natoms = 0;  /* defensive */
    }
}

static void pvfamily_domessage(t_pvfamily *pf, int ac, t_atom *av)
{
    if (ac > pf->f_size)
    {
	/* LATER consider using PV_MAXSIZE (and warning if exceeded) */
	pf->f_message = grow_nodata(&ac, &pf->f_size, pf->f_message,
				    PV_INISIZE, pf->f_messini,
				    sizeof(*pf->f_message));
    }
    pf->f_natoms = ac;
    memcpy(pf->f_message, av, ac * sizeof(*pf->f_message));
}

static void pv_list(t_pv *x, t_symbol *s, int ac, t_atom *av)
{
    t_pvfamily *pf = pv_checkfamily(x);
    if (pf)
    {
	pf->f_selector = &s_list;  /* LATER rethink */
	pvfamily_domessage(pf, ac, av);
    }
}

static void pv_anything(t_pv *x, t_symbol *s, int ac, t_atom *av)
{
    t_pvfamily *pf = pv_checkfamily(x);
    if (pf)
    {
	pf->f_selector = s;  /* LATER rethink */
	pvfamily_domessage(pf, ac, av);
    }
}

static void pv_objstatus(t_pv *x, t_glist *glist)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
    {
	if (g == (t_gobj *)x)
	    post("%lx (this object) owning patcher [%s]",
		 (unsigned long)g, glist->gl_name->s_name);
	else if (pd_class(&g->g_pd) == pv_class
		 && ((t_pv *)g)->x_name == x->x_name)
	    post("%lx owning patcher [%s]", (unsigned long)g, glist->gl_name->s_name);
    }
}

static void pv_status(t_pv *x)
{
    t_pvlist *pl = pv_getlist(x->x_name, 0);
    post("pv status: Tied to %s", x->x_name->s_name);
    if (pl)
    {
	t_pvfamily *pf;
	int fcount;
	for (pf = pl->l_pvlist, fcount = 1; pf; pf = pf->f_next, fcount++)
	{
	    t_glist *glist = pf->f_glist;
	    t_gobj *g;
	    post("Family %d:", fcount);
	    pv_objstatus(x, glist);
	    for (g = glist->gl_list; g; g = g->g_next)
		if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
		    pv_objstatus(x, (t_glist *)g);
	}
    }
}

static void pv_free(t_pv *x)
{
    pv_getfamily(x->x_glist, x->x_name, 0, 1);
}

static void *pv_new(t_symbol *s, int ac, t_atom *av){
    t_pv *x = 0;
    if (ac && av->a_type == A_SYMBOL)
        s = av->a_w.w_symbol;
    else{
        pd_error(x, "[pv]: missing or bad arguments");
        s = gensym("_cyclone-pv-default");
    }
    t_glist *gl = canvas_getcurrent();
    t_pvfamily *pf = pv_getfamily(gl, s, 1, 0);
    x = (t_pv *)pd_new(pv_class);
    x->x_glist = gl;
    x->x_name = s;
    x->x_family = pf;
    outlet_new((t_object *)x, &s_float);
    if(--ac){
        av++;
        if(av->a_type == A_SYMBOL){
            if (av->a_w.w_symbol == &s_symbol){
                if (ac > 1 && av[1].a_type == A_SYMBOL)
                    pv_symbol(x, av[1].a_w.w_symbol);
            }
            /* LATER rethink 'pv <name> bang' (now it is accepted) */
            else
                pv_anything(x, av->a_w.w_symbol, ac - 1, av + 1);
        }
        else if (av->a_type == A_FLOAT){
            if (ac > 1)
                pv_list(x, &s_list, ac, av);
            else
                pv_float(x, av->a_w.w_float);
        }
    }
    return(x);
}

CYCLONE_OBJ_API void pv_setup(void)
{
    pv_class = class_new(gensym("pv"),
			 (t_newmethod)pv_new,
			 (t_method)pv_free,
			 sizeof(t_pv), 0, A_GIMME, 0);
    class_addbang(pv_class, pv_bang);
    class_addfloat(pv_class, pv_float);
    class_addsymbol(pv_class, pv_symbol);
    class_addpointer(pv_class, pv_pointer);
    class_addlist(pv_class, pv_list);
    class_addanything(pv_class, pv_anything);
    class_addmethod(pv_class, (t_method)pv_status,
		    gensym("status"), 0);
    /* CHECKED: sending bang (or int, list, status, etc.) with '; <pv-symbol>'
       "error::doesn't understand bang" (class name is an empty string) */
    pvlist_class = class_new(&s_, 0, 0,
			     sizeof(t_pvlist), CLASS_PD, 0);
    class_addbang(pvlist_class, pvlist_bang);
    class_addfloat(pvlist_class, pvlist_float);
    class_addsymbol(pvlist_class, pvlist_symbol);
    class_addpointer(pvlist_class, pvlist_pointer);
    class_addlist(pvlist_class, pvlist_list);
    class_addanything(pvlist_class, pvlist_anything);

}
