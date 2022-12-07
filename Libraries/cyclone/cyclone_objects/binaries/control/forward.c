/* Copyright (c) 1997-2002 Miller Puckette, krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _forward
{
    t_object   x_ob;
    t_symbol  *x_sym;
} t_forward;

static t_class *forward_class;

static void forward_bang(t_forward *x)
{
    if (x->x_sym->s_thing) pd_bang(x->x_sym->s_thing);
}

static void forward_float(t_forward *x, t_float f)
{
    if (x->x_sym->s_thing) pd_float(x->x_sym->s_thing, f);
}

static void forward_symbol(t_forward *x, t_symbol *s)
{
    if (x->x_sym->s_thing) pd_symbol(x->x_sym->s_thing, s);
}

static void forward_pointer(t_forward *x, t_gpointer *gp)
{
    if (x->x_sym->s_thing) pd_pointer(x->x_sym->s_thing, gp);
}

static void forward_list(t_forward *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_sym->s_thing) pd_list(x->x_sym->s_thing, s, ac, av);
}

static void forward_anything(t_forward *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_sym->s_thing) typedmess(x->x_sym->s_thing, s, ac, av);
}

static void forward_send(t_forward *x, t_symbol *s)
{
    /* CHECKED: 'send' without arguments erases destination */
    if (s) x->x_sym = s;
}

static void *forward_new(t_symbol *s)
{
    t_forward *x = (t_forward *)pd_new(forward_class);
    x->x_sym = s;
    return (x);
}

CYCLONE_OBJ_API void forward_setup(void)
{
    forward_class = class_new(gensym("forward"),
			      (t_newmethod)forward_new, 0,
			      sizeof(t_forward), 0, A_DEFSYM, 0);
    class_addbang(forward_class, forward_bang);
    class_addfloat(forward_class, forward_float);
    class_addsymbol(forward_class, forward_symbol);
    class_addpointer(forward_class, forward_pointer);
    class_addlist(forward_class, forward_list);
    class_addanything(forward_class, forward_anything);
    class_addmethod(forward_class, (t_method)forward_send,
		    gensym("send"), A_DEFSYM, 0);
}
