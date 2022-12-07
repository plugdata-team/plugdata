/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is a modified version of Joseph A. Sarlo's code.
   The most important changes are listed in "pd-lib-notes.txt" file.  */

/* LATER compare with iter.c from max sdk */
/* LATER clean up buffer handling */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define ITER_INISIZE  8  /* LATER rethink */

typedef struct _iter
{
    t_object   x_ob;
    int        x_size;    /* as allocated */
    int        x_natoms;  /* as used */
    t_symbol  *x_selector;
    t_atom    *x_message;
    t_atom     x_messini[ITER_INISIZE];
} t_iter;

static t_class *iter_class;

static void iter_dobang(t_iter *x, t_symbol *s, int ac, t_atom *av){
    if(s && s != &s_)
	outlet_symbol(((t_object *)x)->ob_outlet, s);
    while(ac--){
        if(av->a_type == A_FLOAT)
            outlet_float(((t_object *)x)->ob_outlet, av->a_w.w_float);
        else if (av->a_type == A_SYMBOL)
            outlet_symbol(((t_object *)x)->ob_outlet, av->a_w.w_symbol);
        av++;
    }
}

static void iter_bang(t_iter *x)
{
    iter_dobang(x, x->x_selector, x->x_natoms, x->x_message);
}

static void iter_float(t_iter *x, t_float f)
{
    outlet_float(((t_object *)x)->ob_outlet, f);
    x->x_selector = 0;
    x->x_natoms = 1;
    SETFLOAT(x->x_message, f);
}

/* CHECKME */
static void iter_symbol(t_iter *x, t_symbol *s)
{
    outlet_symbol(((t_object *)x)->ob_outlet, s);
    x->x_selector = 0;
    x->x_natoms = 1;
    SETSYMBOL(x->x_message, s);
}

/* LATER gpointer */

static void iter_anything(t_iter *x, t_symbol *s, int ac, t_atom *av){
    x->x_selector = s;
    if(ac > x->x_size)
        x->x_message = grow_nodata(&ac, &x->x_size, x->x_message,
            ITER_INISIZE, x->x_messini, sizeof(*x->x_message));
    x->x_natoms = ac;
    memcpy(x->x_message, av, ac * sizeof(*x->x_message));
    iter_dobang(x, s, ac, av);
}

static void iter_list(t_iter *x, t_symbol *s, int ac, t_atom *av)
{
    iter_anything(x, 0, ac, av);
}

static void iter_free(t_iter *x)
{
    if (x->x_message != x->x_messini)
	freebytes(x->x_message, x->x_natoms * sizeof(*x->x_message));
}

static void *iter_new(void)
{
    t_iter *x = (t_iter *)pd_new(iter_class);
    x->x_size = ITER_INISIZE;
    x->x_natoms = 0;
    x->x_message = x->x_messini;
    outlet_new((t_object *)x, &s_anything);
    return (x);
}

CYCLONE_OBJ_API void iter_setup(void)
{
    iter_class = class_new(gensym("iter"),
			   (t_newmethod)iter_new,
			   (t_method)iter_free,
			   sizeof(t_iter), 0, 0);
    class_addbang(iter_class, iter_bang);
    class_addfloat(iter_class, iter_float);
    class_addsymbol(iter_class, iter_symbol);
    class_addlist(iter_class, iter_list);
    class_addanything(iter_class, iter_anything);
}
