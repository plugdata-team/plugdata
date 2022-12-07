/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define PREPEND_INISIZE   32  /* LATER rethink */
#define PREPEND_MAXSIZE  4096

typedef struct _prepend{
    t_object   x_ob;
    t_symbol  *x_selector;
    int        x_size;    /* as allocated */
    int        x_natoms;  /* as used */
    t_atom    *x_message;
    t_atom     x_messini[PREPEND_INISIZE];
    int        x_entered;
    int        x_auxsize;
    t_atom    *x_auxbuf;
}t_prepend;

static t_class *prepend_class;

/* Usually a preallocation method is used, except in special cases of:
   1) reentrant output request, or 2) an output request which would cause
   resizing to more than MAXSIZE (no such limit for a 'set' message).
   In both special cases, a temporary output buffer is allocated.
   A separately preallocated output buffer is not used, thus avoiding
   memcpying of the stored message (a small performance gain when the
   preallocation method is chosen).  Instead, self-invoked 'set'
   messages are postponed, using an auxiliary buffer. */

/* called only from prepend_doanything() */
static void prepend_dooutput(t_prepend *x, int ac, t_atom *av){
    if (x->x_selector == &s_float){
        if(ac > 1)
            outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, av);
        else
            outlet_float(((t_object *)x)->ob_outlet, av->a_w.w_float);
    }
    else if
        (x->x_selector == &s_list)
	outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, av);
    else  // x->x_selector guaranteed non-empty
        outlet_anything(((t_object *)x)->ob_outlet, x->x_selector, ac, av);
}

static void prepend_doanything(t_prepend *x, t_symbol *s, int ac, t_atom *av)
{
    int reentered = x->x_entered;
    x->x_entered = 1;
    if (s == &s_)
	s = 0;
    if (s || x->x_natoms)
    {
	int prealloc = !reentered;
	int ntotal = x->x_natoms + ac;
	t_atom *buf;
	if (s)
	    ntotal++;
	if (prealloc && ntotal > x->x_size)
	{
	    if (ntotal > PREPEND_MAXSIZE)
		prealloc = 0;
	    else
	    {
		int nrequested = ntotal;
		x->x_message = grow_withdata(&nrequested, &x->x_natoms,
					     &x->x_size, x->x_message,
					     PREPEND_INISIZE, x->x_messini,
					     sizeof(*x->x_message));
		prealloc = (nrequested == ntotal);
	    }
	}
	if (prealloc)
	{
	    buf = x->x_message + x->x_natoms;
	    if (s)
	    {
		SETSYMBOL(buf, s);
		buf++;
	    }
	    if (ac)
		memcpy(buf, av, ac * sizeof(*buf));
	    prepend_dooutput(x, ntotal, x->x_message);
	}
	else
	{
	    /* LATER consider using the stack if ntotal <= MAXSTACK */
	    if (buf = getbytes(ntotal * sizeof(*buf)))
	    {
		t_atom *bp = buf + x->x_natoms;
		if (x->x_natoms)
		    memcpy(buf, x->x_message, x->x_natoms * sizeof(*buf));
		if (s)
		{
		    SETSYMBOL(bp, s);
		    bp++;
		}
		if (ac)
		    memcpy(bp, av, ac * sizeof(*bp));
		prepend_dooutput(x, ntotal, buf);
		freebytes(buf, ntotal * sizeof(*buf));
	    }
	}
    }
    else prepend_dooutput(x, ac, av);
    if (!reentered)
    {
	x->x_entered = 0;
	if (x->x_auxbuf)
	{
	    if (x->x_auxsize <= x->x_size)
	    {
		x->x_natoms = x->x_auxsize / 2;
		memcpy(x->x_message, x->x_auxbuf,
		       x->x_natoms * sizeof(*x->x_message));
		freebytes(x->x_auxbuf, x->x_auxsize * sizeof(*x->x_auxbuf));
	    }
	    else
	    {
		if (x->x_message != x->x_messini)
		    freebytes(x->x_message, x->x_size * sizeof(*x->x_message));
		x->x_size = x->x_auxsize;
		x->x_message = x->x_auxbuf;
		x->x_natoms = x->x_auxsize / 2;
	    }
	    x->x_auxbuf = 0;
	}
    }
}

static void prepend_bang(t_prepend *x){
    if(x->x_selector){
            prepend_doanything(x, &s_bang, 0, 0);
    }
    else
        outlet_bang(((t_object *)x)->ob_outlet);
}

static void prepend_float(t_prepend *x, t_float f)
{
    if (x->x_selector)
    {
	t_atom at;
	SETFLOAT(&at, f);
	prepend_doanything(x, 0, 1, &at);
    }
    else outlet_float(((t_object *)x)->ob_outlet, f);
}

static void prepend_symbol(t_prepend *x, t_symbol *s)
{
    if (x->x_selector)
    {
	t_atom at;
	SETSYMBOL(&at, s);
	prepend_doanything(x, 0, 1, &at);
    }
    else outlet_symbol(((t_object *)x)->ob_outlet, s);
}

/* LATER gpointer */

static void prepend_list(t_prepend *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_selector)
	prepend_doanything(x, 0, ac, av);
    else
	outlet_list(((t_object *)x)->ob_outlet, s, ac, av);
}

static void prepend_anything(t_prepend *x, t_symbol *s, int ac, t_atom *av){
    if(x->x_selector)
        prepend_doanything(x, s, ac, av);
    else
        outlet_anything(((t_object *)x)->ob_outlet, s, ac, av);
}

static void prepend_doset(t_prepend *x, t_symbol *s, int ac, t_atom *av)
{
    if (s)
	x->x_selector = s;
    else if (ac)
    {
	if (av->a_type == A_SYMBOL)
	{
	    x->x_selector = av->a_w.w_symbol;
	    ac--; av++;
	}
	else if (av->a_type == A_FLOAT)
	    x->x_selector = (ac > 1 ? &s_list : &s_float);
	else
	    return;  /* LATER rethink */
    }
    else x->x_selector = 0;
    if (ac)
    {
	int newsize = ac * 2;
	if (x->x_entered)
	{
	    if (x->x_auxbuf)
	    {
		post("'set' message overridden");
		freebytes(x->x_auxbuf, x->x_auxsize * sizeof(*x->x_auxbuf));
		x->x_auxsize = 0;
	    }
	    if (x->x_auxbuf = getbytes(newsize * sizeof(*x->x_auxbuf)))
	    {
		memcpy(x->x_auxbuf, av, ac * sizeof(*x->x_auxbuf));
		x->x_auxsize = newsize;
	    }
	}
	else
	{
	    t_atom *ap;
	    if (newsize > x->x_size)
	    {
		int sz = newsize;
		x->x_message = grow_nodata(&sz, &x->x_size, x->x_message,
					   PREPEND_INISIZE, x->x_messini,
					   sizeof(*x->x_message));
		if (sz != newsize)
		    ac = sz / 2;  /* LATER rethink */
	    }
	    x->x_natoms = ac;
	    ap = x->x_message;
	    while (ac--) *ap++ = *av++;
	}
    }
    else x->x_natoms = 0;
}

static void prepend_set(t_prepend *x, t_symbol *s, int ac, t_atom *av)
{
    prepend_doset(x, 0, ac, av);
}

static void prepend_free(t_prepend *x)
{
    if (x->x_message != x->x_messini)
	freebytes(x->x_message, x->x_size * sizeof(*x->x_message));
    if (x->x_auxbuf)
    {
	freebytes(x->x_auxbuf, x->x_auxsize * sizeof(*x->x_auxbuf));
    }
}

static void *prepend_new(t_symbol *s, int ac, t_atom *av){
    t_prepend *x = (t_prepend *)pd_new(prepend_class);
    x->x_selector = 0;
    x->x_size = PREPEND_INISIZE;
    x->x_natoms = 0;
    x->x_message = x->x_messini;
    x->x_auxbuf = 0;
    x->x_entered = 0;
    if(ac){
        prepend_doset(x, 0, ac, av);
    }
    outlet_new((t_object *)x, &s_anything);
    return (x);
}

CYCLONE_OBJ_API void prepend_setup(void){
    prepend_class = class_new(gensym("prepend"), (t_newmethod)prepend_new,
        (t_method)prepend_free, sizeof(t_prepend), 0, A_GIMME, 0);
    class_addbang(prepend_class, prepend_bang);
    class_addfloat(prepend_class, prepend_float);
    class_addsymbol(prepend_class, prepend_symbol);
    class_addlist(prepend_class, prepend_list);
    class_addanything(prepend_class, prepend_anything);
    class_addmethod(prepend_class, (t_method)prepend_set, gensym("set"), A_GIMME, 0);
}
