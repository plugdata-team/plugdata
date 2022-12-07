/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include "g_canvas.h"
#include "m_imp.h"

// LATER fragilize (for some reason)

typedef struct _universal
{
    t_object  x_ob;
    t_glist  *x_glist;
    t_int     x_descend;
} t_universal;

static t_class *universal_class;

static void universal_dobang(t_glist *glist, int descend, t_symbol *cname)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd)->c_name == cname)  /* LATER rethink */
	    pd_bang(&g->g_pd);
    if (descend)
	for (g = glist->gl_list; g; g = g->g_next)
	    if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
		universal_dobang((t_glist *)g, descend, cname);
}

static void universal_dofloat(t_glist *glist, int descend, t_symbol *cname,
			      t_float f)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd)->c_name == cname)  /* LATER rethink */
	    pd_float(&g->g_pd, f);
    if (descend)
	for (g = glist->gl_list; g; g = g->g_next)
	    if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
		universal_dofloat((t_glist *)g, descend, cname, f);
}

static void universal_dosymbol(t_glist *glist, int descend, t_symbol *cname,
			       t_symbol *s)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd)->c_name == cname)  /* LATER rethink */
	    pd_symbol(&g->g_pd, s);
    if (descend)
	for (g = glist->gl_list; g; g = g->g_next)
	    if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
		universal_dosymbol((t_glist *)g, descend, cname, s);
}

static void universal_dopointer(t_glist *glist, int descend, t_symbol *cname,
				t_gpointer *gp)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd)->c_name == cname)  /* LATER rethink */
	    pd_pointer(&g->g_pd, gp);
    if (descend)
	for (g = glist->gl_list; g; g = g->g_next)
	    if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
		universal_dopointer((t_glist *)g, descend, cname, gp);
}

static void universal_dolist(t_glist *glist, int descend, t_symbol *cname,
			     int ac, t_atom *av)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd)->c_name == cname)  /* LATER rethink */
	    pd_list(&g->g_pd, &s_list, ac, av);
    if (descend)
	for (g = glist->gl_list; g; g = g->g_next)
	    if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
		universal_dolist((t_glist *)g, descend, cname, ac, av);
}

static void universal_doanything(t_glist *glist, int descend, t_symbol *cname,
				 t_symbol *s, int ac, t_atom *av)
{
    t_gobj *g;
    for (g = glist->gl_list; g; g = g->g_next)
	if (pd_class(&g->g_pd)->c_name == cname)  /* LATER rethink */
	    typedmess(&g->g_pd, s, ac, av);
    if (descend)
	for (g = glist->gl_list; g; g = g->g_next)
	    if (pd_class(&g->g_pd) == canvas_class)  /* LATER rethink */
		universal_doanything((t_glist *)g, descend, cname, s, ac, av);
}

/* LATER rethink type-checking -- it is borrowed from typedmess().
   Anyway, do it once, before traversal, bypassing the generic mechanism
   performed for every object. */
static void universal_anything(t_universal *x, t_symbol *s, int ac, t_atom *av)
{
    /* CHECKED selector without arguments ignored with no complaints */
    if (x->x_glist && s && ac)
    {
	if (av->a_type == A_FLOAT)
	{
	    if (ac > 1)
		universal_dolist(x->x_glist, x->x_descend, s, ac, av);
	    else
		universal_dofloat(x->x_glist, x->x_descend, s, av->a_w.w_float);
	}
	else if (av->a_type == A_SYMBOL)
	{
	    if (av->a_w.w_symbol == &s_bang)
		universal_dobang(x->x_glist, x->x_descend, s);
	    else if (av->a_w.w_symbol == &s_float)
	    {
		if (ac == 1)
		    universal_dofloat(x->x_glist, x->x_descend, s, 0.);
		else if (av[1].a_type == A_FLOAT)
		    universal_dofloat(x->x_glist, x->x_descend, s,
				      av[1].a_w.w_float);
		else
            pd_error(x, "universal: bad argument for message 'float'");
	    }
	    else if (av->a_w.w_symbol == &s_symbol)
		universal_dosymbol(x->x_glist, x->x_descend, s,
				   (ac > 1 && av[1].a_type == A_SYMBOL ?
				    av[1].a_w.w_symbol : &s_));
	    else if (av->a_w.w_symbol == &s_list)
		universal_dolist(x->x_glist, x->x_descend, s, ac - 1, av + 1);
	    else
		universal_doanything(x->x_glist, x->x_descend, s,
				     av->a_w.w_symbol, ac - 1, av + 1);
	}
	if (av->a_type == A_POINTER)
	    universal_dopointer(x->x_glist, x->x_descend, s,
				av->a_w.w_gpointer);
    }
}

static void universal_send(t_universal *x, t_symbol *s, int ac, t_atom *av)
{
    if (ac && av->a_type == A_SYMBOL)
	universal_anything(x, av->a_w.w_symbol, ac - 1, av + 1);
    /* CHECKED: else ignored without complaints */
}

static void *universal_new(t_floatarg f)
{
    t_universal *x = (t_universal *)pd_new(universal_class);
    x->x_glist = canvas_getcurrent();
    x->x_descend = ((int)f != 0);  /* CHECKED */
    return (x);
}

CYCLONE_OBJ_API void universal_setup(void)
{
    universal_class = class_new(gensym("universal"),
			      (t_newmethod)universal_new, 0,
			      sizeof(t_universal), 0, A_DEFFLOAT, 0);
    class_addanything(universal_class, universal_anything);
    /* CHECKED: 'send', not 'sendmessage' */
    class_addmethod(universal_class, (t_method)universal_send,
		    gensym("send"), A_GIMME, 0);
}
