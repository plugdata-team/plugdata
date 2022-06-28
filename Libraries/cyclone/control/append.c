/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define APPEND_INISIZE     32  /* LATER rethink */
#define APPEND_MAXSIZE    4096

typedef struct _append
{
    t_object  x_ob;
    int       x_size;    /* as allocated */
    int       x_natoms;  /* as used */
    t_atom   *x_message;
    t_atom   *x_messbuf;
    t_atom    x_messini[APPEND_INISIZE];
    int       x_entered;
    int       x_auxsize;
    t_atom   *x_auxbuf;
} t_append;

static t_class *append_class;

static int append_iscompatible = 0;  /* FIXME per-object */

/* Usually a preallocation method is used, except in special cases of:
   1) reentrant output request, or 2) an output request which would cause
   resizing to more than MAXSIZE (no such limit for a 'set' message).
   In both special cases, a temporary output buffer is allocated.
   A separately preallocated output buffer is not used, thus avoiding
   memcpying of the stored message (a small performance gain when the
   preallocation method is chosen).  Instead, self-invoked 'set'
   messages are postponed, using an auxiliary buffer. */

/* Any Append's output, except bangout, goes through
   outlet_anything() -> typedmess(), LATER rethink */

static void append_setnatoms(t_append *x, int natoms)
{
    x->x_message = x->x_messbuf + x->x_size - natoms;
    x->x_natoms = natoms;
}

static void append_bangout(t_outlet *outp, int ac, t_atom *av){
    if(ac){
        if (av->a_type == A_SYMBOL)
            outlet_anything(outp, av->a_w.w_symbol, ac-1, av+1);
        else if (av->a_type == A_POINTER){
            if (ac == 1)
                outlet_pointer(outp, av->a_w.w_gpointer);
            else
                outlet_list(outp, &s_list, ac, av);
        }
        else if (av->a_type == A_FLOAT){
            if (ac == 1)
                outlet_float(outp, av->a_w.w_float);
            else
                outlet_list(outp, &s_list, ac, av);
        }
        else
            post("bug [append]: append_bangout");
    }
    else
        outlet_bang(outp);
}

static void append_anything(t_append *x, t_symbol *s, int ac, t_atom *av)
{
    int reentered = x->x_entered;
    int prealloc = !reentered;
    int ntotal = x->x_natoms + ac;
    t_atom *buf;
    x->x_entered = 1;
    if (prealloc && ntotal > x->x_size)
    {
	if (ntotal > APPEND_MAXSIZE)
	    prealloc = 0;
	else
	{
	    int nrequested = ntotal;
	    x->x_messbuf = grow_withtail(&nrequested, &x->x_natoms,
					 (char **)&x->x_message,
					 &x->x_size, x->x_messbuf,
					 APPEND_INISIZE, x->x_messini,
					 sizeof(*x->x_message));
	    prealloc = (nrequested == ntotal);
	}
    }
    if (prealloc)
    {
	buf = x->x_message - ac;
	if (ac)
	    memcpy(buf, av, ac * sizeof(*buf));
	if (s)
	    outlet_anything(((t_object *)x)->ob_outlet, s, ntotal, buf);
	else
	    append_bangout(((t_object *)x)->ob_outlet, ntotal, buf);
    }
    else
    {
	/* LATER consider using the stack if ntotal <= MAXSTACK */
	if (buf = getbytes(ntotal * sizeof(*buf)))
	{
	    if (ac)
		memcpy(buf, av, ac * sizeof(*buf));
	    if (x->x_natoms)
		memcpy(buf + ac, x->x_message, x->x_natoms * sizeof(*buf));
	    if (s)
		outlet_anything(((t_object *)x)->ob_outlet, s, ntotal, buf);
	    else
		append_bangout(((t_object *)x)->ob_outlet, ntotal, buf);
	    freebytes(buf, ntotal * sizeof(*buf));
	}
    }
    if (!reentered)
    {
	x->x_entered = 0;
	if (x->x_auxbuf)
	{
	    if (x->x_auxsize <= x->x_size)
	    {
		append_setnatoms(x, x->x_auxsize / 2);
		memcpy(x->x_message, x->x_auxbuf + x->x_natoms,
		       x->x_natoms * sizeof(*x->x_message));
		freebytes(x->x_auxbuf, x->x_auxsize * sizeof(*x->x_auxbuf));
	    }
	    else
	    {
		if (x->x_messbuf != x->x_messini)
		    freebytes(x->x_messbuf, x->x_size * sizeof(*x->x_messbuf));
		x->x_size = x->x_auxsize;
		x->x_messbuf = x->x_auxbuf;
		append_setnatoms(x, x->x_auxsize / 2);
	    }
	    x->x_auxbuf = 0;
	}
    }
}

static void append_bang(t_append *x)
{
// ignore
}

static void append_float(t_append *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    append_anything(x, &s_list, 1, &at);  /* CHECKED: converted to list */
}

static void append_symbol(t_append *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    append_anything(x, &s_anything, 1, &at);
}

/* LATER gpointer */

static void append_doset(t_append *x, t_symbol *s, int ac, t_atom *av)
{
    int newsize = ac * 2;
    if (s)
	newsize += 2;
    if (newsize > 0)
    {
	if (x->x_entered)
	{
	    if (x->x_auxbuf)
	    {
		pd_error(x, "[append]: \'set\' message overridden");
		freebytes(x->x_auxbuf, x->x_auxsize * sizeof(*x->x_auxbuf));
		x->x_auxsize = 0;
	    }
	    if (x->x_auxbuf = getbytes(newsize * sizeof(*x->x_auxbuf)))
	    {
		t_atom *ap = x->x_auxbuf + ac;
		if (s)
		{
		    ap++;
		    SETSYMBOL(ap, s);
		    ap++;
		}
		if (ac)
		    memcpy(ap, av, ac * sizeof(*x->x_auxbuf));
		x->x_auxsize = newsize;
	    }
	}
	else
	{
	    t_atom *ap;
	    if (newsize > x->x_size)
	    {
		int sz = newsize;
		x->x_messbuf = grow_nodata(&sz, &x->x_size, x->x_messbuf,
					   APPEND_INISIZE, x->x_messini,
					   sizeof(*x->x_messbuf));
		if (sz != newsize)
		{
		    ac = sz / 2;  /* LATER rethink */
		    if (s)
			ac--;
		}
	    }
	    if (s)
	    {
		append_setnatoms(x, ac + 1);
		ap = x->x_message;
		SETSYMBOL(ap, s);
		ap++;
	    }
	    else
	    {
		append_setnatoms(x, ac);
		ap = x->x_message;
	    }
	    while (ac--) *ap++ = *av++;
	}
    }
}

static void append_set(t_append *x, t_symbol *s, int ac, t_atom *av)
{
	append_doset(x, 0, ac, av);
}

static void append_free(t_append *x){
    if (x->x_messbuf != x->x_messini)
	freebytes(x->x_messbuf, x->x_size * sizeof(*x->x_messbuf));
    if(x->x_auxbuf){
        post("bug [append]: append_free");  /* LATER rethink */
        freebytes(x->x_auxbuf, x->x_auxsize * sizeof(*x->x_auxbuf));
    }
}

static void *append_new(t_symbol *s, int ac, t_atom *av)
{
    t_append *x = (t_append *)pd_new(append_class);
    x->x_size = APPEND_INISIZE;
    x->x_natoms = 0;
    x->x_messbuf = x->x_messini;
    x->x_auxbuf = 0;
    x->x_entered = 0;
    append_setnatoms(x, 0);
    append_doset(x, 0, ac, av);
    outlet_new((t_object *)x, &s_anything);
    return (x);
}

CYCLONE_OBJ_API void append_setup(void)
{
    append_class = class_new(gensym("cyclone/append"),
        (t_newmethod)append_new, (t_method)append_free, sizeof(t_append), 0, A_GIMME, 0);
    class_addbang(append_class, append_bang);
    class_addfloat(append_class, append_float);
    class_addsymbol(append_class, append_symbol);
    class_addlist(append_class, append_anything);  /* LATER rethink */
    class_addanything(append_class, append_anything);
    class_addmethod(append_class, (t_method)append_set,
		    gensym("set"), A_GIMME, 0);
    class_sethelpsymbol(append_class, gensym("append"));
}


CYCLONE_OBJ_API void Append_setup(void)
{
    append_class = class_new(gensym("Append"),
        (t_newmethod)append_new, (t_method)append_free, sizeof(t_append), 0, A_GIMME, 0);
    class_addbang(append_class, append_bang);
    class_addfloat(append_class, append_float);
    class_addsymbol(append_class, append_symbol);
    class_addlist(append_class, append_anything);  /* LATER rethink */
    class_addanything(append_class, append_anything);
    class_addmethod(append_class, (t_method)append_set,
            gensym("set"), A_GIMME, 0);
    class_sethelpsymbol(append_class, gensym("append"));
    pd_error(append_class, "Cyclone: please use [append] instead of [Append] to suppress this error");
}
