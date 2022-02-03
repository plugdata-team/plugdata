/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKED refman's rubbish: 1st outlet used only if there is a match,
   unchanged messages go through the 2nd outlet (no bang there),
   3rd argument ignored (no single-replacement mode). */

/*NOTE: as of 2016, has 3rd argt single-replacement mode! 
    Not tested with what auxmatch/auxrepl are guarding against
    (match/repl being changed during substitution process, mb?)
    but replaceonce can't change for now so it should be fine, but 
    if it somehow can in the future, look into this - DXK
    */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define SUBSTITUTE_INISIZE   32  /* LATER rethink */
#define SUBSTITUTE_MAXSIZE  256

typedef struct _substitute
{
    t_object   x_ob;
    t_pd      *x_proxy;
    t_atom     x_match;
    t_atom     x_repl;
    int        x_size;    /* as allocated */
    t_atom    *x_message;
    t_atom     x_messini[SUBSTITUTE_INISIZE];
    int        x_entered;
    t_atom     x_auxmatch;
    t_atom     x_auxrepl;
    t_outlet  *x_passout;
    int        x_replaceonce;
} t_substitute;

typedef struct _substitute_proxy
{
    t_object       p_ob;
    t_atom        *p_match;  /* pointing to parent's (aux)match */
    t_atom        *p_repl;
} t_substitute_proxy;

static t_class *substitute_class;
static t_class *substitute_proxy_class;

/* LATER rethink */
static void substitute_dooutput(t_substitute *x,
				t_symbol *s, int ac, t_atom *av, int pass)
{
    t_outlet *out = (pass ? x->x_passout : ((t_object *)x)->ob_outlet);
    if (s == &s_float)
    {
	if (ac > 1)
	    outlet_list(out, &s_list, ac, av);
	else
	    outlet_float(out, av->a_w.w_float);
    }
    else if (s == &s_bang && !ac)  /* CHECKED */
	outlet_bang(out);
    else if (s == &s_symbol && ac == 1 && av->a_type == A_SYMBOL)
	outlet_symbol(out, av->a_w.w_symbol);
    else if (s)
	outlet_anything(out, s, ac, av);
    else if (ac)
	outlet_list(out, &s_list, ac, av);
}

static int substitute_check(t_substitute *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_repl.a_type == A_NULL)
	return (-2);
    /* see substitute_proxy_validate() for possible types and values */
    if (x->x_match.a_type == A_FLOAT)
    {
	t_float f = x->x_match.a_w.w_float;
	int i;
	for (i = 0; i < ac; i++, av++)
	    if (av->a_type == A_FLOAT && av->a_w.w_float == f)
		return (i);
    }
    else if (x->x_match.a_type == A_SYMBOL)
    {
	/* match symbol is validated -- never null */
	t_symbol *match = x->x_match.a_w.w_symbol;
	int i;
	if (s == match)
	    return (-1);
	for (i = 0; i < ac; i++, av++)
	    if (av->a_type == A_SYMBOL && av->a_w.w_symbol == match)
		return (i);
    }
    return (-2);
}

static void substitute_doit(t_substitute *x,
			    t_symbol *s, int ac, t_atom *av, int startndx,
                            int replaceonce, int replaced)
{
    int cnt = ac - startndx;
    if (cnt > 0)
    {
	t_atom *ap = av + startndx;
	if (x->x_match.a_type == A_FLOAT)
	{
	    t_float f = x->x_match.a_w.w_float;
	    while (cnt--)
	    {
                //if were are replacing once and have replaced already, no need to replace anymore!
		if(replaceonce && replaced) break;
                if (ap->a_type == A_FLOAT && ap->a_w.w_float == f){ 
		    *ap = x->x_repl;
                    replaced = 1;
                };
		ap++;
	    }
	}
	else if (x->x_match.a_type == A_SYMBOL)
	{
	    t_symbol *match = x->x_match.a_w.w_symbol;
	    while (cnt--)
	    {
                //if were are replacing once and have replaced already, no need to replace anymore!
		if(replaceonce && replaced) break;
		if (ap->a_type == A_SYMBOL && ap->a_w.w_symbol == match){
		    *ap = x->x_repl;
                    replaced = 1;
                };
		ap++;
	    }
	}
    }
    substitute_dooutput(x, s, ac, av, 0);
}

static void substitute_anything(t_substitute *x,
				t_symbol *s, int ac, t_atom *av)
{
    int matchndx = substitute_check(x, s, ac, av);
    if (matchndx < -1)
        //if replace type is null or no match found
	substitute_dooutput(x, s, ac, av, 1);
    else
    {
        int replaceonce = x->x_replaceonce;
        int replaced = 0; //if we've replaced something, useful for replaceonce

	int reentered = x->x_entered;
	int prealloc = !reentered;
	int ntotal = ac;
	t_atom *buf;
	t_substitute_proxy *proxy = (t_substitute_proxy *)x->x_proxy;
	x->x_entered = 1;
        //while method is working, point to aux versions so if they change, won't effect
        //currently undergoing replacement scheme
	proxy->p_match = &x->x_auxmatch;
	proxy->p_repl = &x->x_auxrepl;
	if (s == &s_) s = 0;
	if (matchndx == -1)
	{
            //if match type is a symbol AND is the selector 
	    if (x->x_repl.a_type == A_FLOAT)
	    {
                //since we're replacing the selector and it can't be a float,
                //need to add it to the list
		ntotal++;
                //if there's a rest to it, the selector needs to be a list selector
		if (ac) s = &s_list;
                //else there's no "rest of it", can be a float selector
		else s = &s_float;

                replaced = 1;
	    }
	    else if (x->x_repl.a_type == A_SYMBOL)
	    {
                //just replace the selector if the replace type is the symbol
                //set matchndx to 0 to avoid the if in the message contructor clause below
		s = x->x_repl.a_w.w_symbol;
		matchndx = 0;

                replaced = 1;
	    }
	}
	else if (matchndx == 0
		 && (!s || s == &s_list || s == &s_float)
		 && av->a_type == A_FLOAT
		 && x->x_repl.a_type == A_SYMBOL)
	{
            //match index is at the beginning of a list (or singleton)
            //incoming list[0] is a float, being replaced by a symbol
            //just make the selector the replacement
	    s = x->x_repl.a_w.w_symbol;
	    ac--;
	    av++;
	    ntotal = ac;
	}
	if (prealloc && ac > x->x_size)
	{
            //if bigger that maximum size, don't bother allocating to x_message
	    if (ntotal > SUBSTITUTE_MAXSIZE)
		prealloc = 0;
	    else
		x->x_message = grow_nodata(&ntotal, &x->x_size, x->x_message,
					   SUBSTITUTE_INISIZE, x->x_messini,
					   sizeof(*x->x_message));
	}

        //if allocated, just point stack pointer to struct's x_message
        //else just point stack pointer to new allocated memory
	if (prealloc) buf = x->x_message;
	else
	    /* LATER consider using the stack if ntotal <= MAXSTACK */
	    buf = getbytes(ntotal * sizeof(*buf));
	if (buf)
	{
	    int ncopy = ntotal;
	    t_atom *bp = buf;
	    if (matchndx == -1)
	    {
                //only hit if x_repl is a float because of the earlier clause
		SETFLOAT(bp++, x->x_repl.a_w.w_float);
		ncopy--;
	    }
            //copy over incoming list to allocated buf (or x message), do the substitution
	    if (ncopy)
		memcpy(bp, av, ncopy * sizeof(*buf));
	    substitute_doit(x, s, ntotal, buf, matchndx, replaceonce, replaced);
            
            //free newly allocated memory if using it
	    if (buf != x->x_message)
		freebytes(buf, ntotal * sizeof(*buf));
	}
	if (!reentered)
	{
	    x->x_entered = 0;
	    if (x->x_auxmatch.a_type != A_NULL)
	    {
		x->x_match = x->x_auxmatch;
		x->x_auxmatch.a_type = A_NULL;
	    }
	    if (x->x_auxrepl.a_type != A_NULL)
	    {
		x->x_repl = x->x_auxrepl;
		x->x_auxrepl.a_type = A_NULL;
	    }
	    proxy->p_match = &x->x_match;
	    proxy->p_repl = &x->x_repl;
	}
    }
}

static void substitute_bang(t_substitute *x)
{
    substitute_anything(x, &s_bang, 0, 0);
}

static void substitute_float(t_substitute *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    substitute_anything(x, 0, 1, &at);
}

/* CHECKED (but LATER rethink) */
static void substitute_symbol(t_substitute *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    substitute_anything(x, &s_symbol, 1, &at);
}

/* LATER gpointer */

static void substitute_list(t_substitute *x, t_symbol *s, int ac, t_atom *av)
{
    substitute_anything(x, 0, ac, av);
}

static int substitute_atomvalidate(t_atom *ap)
{
    return (ap->a_type == A_FLOAT
	    || (ap->a_type == A_SYMBOL
		&& ap->a_w.w_symbol && ap->a_w.w_symbol != &s_));
}

/* CHECKED: 'set' is ignored, single '<atom>' does not modify a replacement */
static void substitute_proxy_anything(t_substitute_proxy *x,
				      t_symbol *s, int ac, t_atom *av)
{
    if (s == &s_) s = 0;
    if (s)
    {
	SETSYMBOL(x->p_match, s);
	if (ac && substitute_atomvalidate(av))
	    *x->p_repl = *av;
    }
    else if (ac && substitute_atomvalidate(av))
    {
	*x->p_match = *av++;
	if (ac > 1 && substitute_atomvalidate(av))
	    *x->p_repl = *av;
    }
}

static void substitute_proxy_bang(t_substitute_proxy *x)
{
    SETSYMBOL(x->p_match, &s_bang);
}

static void substitute_proxy_float(t_substitute_proxy *x, t_float f)
{
    SETFLOAT(x->p_match, f);
}

/* CHECKED (but LATER rethink) */
static void substitute_proxy_symbol(t_substitute_proxy *x, t_symbol *s)
{
    SETSYMBOL(x->p_match, &s_symbol);
    SETSYMBOL(x->p_repl, s);
}

/* LATER gpointer */

static void substitute_proxy_list(t_substitute_proxy *x,
				  t_symbol *s, int ac, t_atom *av)
{
    substitute_proxy_anything(x, 0, ac, av);
}

static void substitute_free(t_substitute *x)
{
    if (x->x_proxy) pd_free(x->x_proxy);
    if (x->x_message != x->x_messini)
	freebytes(x->x_message, x->x_size * sizeof(*x->x_message));
}

static void *substitute_new(t_symbol *s, int ac, t_atom *av)
{
    t_substitute *x = 0;
    t_substitute_proxy *proxy =
	(t_substitute_proxy *)pd_new(substitute_proxy_class);
    if (proxy)
    {
	x = (t_substitute *)pd_new(substitute_class);
	proxy->p_match = &x->x_match;
	proxy->p_repl = &x->x_repl;
	x->x_proxy = (t_pd *)proxy;
	x->x_size = SUBSTITUTE_INISIZE;
	x->x_message = x->x_messini;
	x->x_entered = 0;
        //if there are more than 2 args passed, then we are in replace once mode
        x->x_replaceonce = ac >= 3 ? 1 : 0; 
	/* CHECKED: everything is to be passed unchanged, until both are set */
	/* CHECKED: max crashes if a match has been set, but not a replacement,
	   and there is a match */
	x->x_match.a_type = x->x_repl.a_type = A_NULL;
	x->x_auxmatch.a_type = x->x_auxrepl.a_type = A_NULL;
	inlet_new((t_object *)x, (t_pd *)proxy, 0, 0);
	outlet_new((t_object *)x, &s_anything);
	/* CHECKED (refman error: 'a bang is sent') */
	x->x_passout = outlet_new((t_object *)x, &s_anything);
	substitute_proxy_anything(proxy, 0, ac, av);
    }
    return (x);
}

CYCLONE_OBJ_API void substitute_setup(void)
{
    substitute_class = class_new(gensym("substitute"),
			      (t_newmethod)substitute_new,
			      (t_method)substitute_free,
			      sizeof(t_substitute), 0,
			      A_GIMME, 0);
    class_addbang(substitute_class, substitute_bang);
    class_addfloat(substitute_class, substitute_float);
    class_addsymbol(substitute_class, substitute_symbol);
    class_addlist(substitute_class, substitute_list);
    class_addanything(substitute_class, substitute_anything);
    substitute_proxy_class = class_new(gensym("_substitute_proxy"), 0, 0,
				       sizeof(t_substitute_proxy),
				       CLASS_PD | CLASS_NOINLET, 0);
    class_addbang(substitute_proxy_class, substitute_proxy_bang);
    class_addfloat(substitute_proxy_class, substitute_proxy_float);
    class_addsymbol(substitute_proxy_class, substitute_proxy_symbol);
    class_addlist(substitute_proxy_class, substitute_proxy_list);
    class_addanything(substitute_proxy_class, substitute_proxy_anything);
    class_addmethod(substitute_proxy_class, (t_method)substitute_proxy_list,
		    gensym("set"), A_GIMME, 0);
}
