/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER 'clock' method */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/grow.h"

#define SPEEDLIM_INISIZE   32  /* LATER rethink */
#define SPEEDLIM_MAXSIZE  256  /* not used */

typedef struct _speedlim
{
    t_object     x_ob;
    int          x_open;
    t_float      x_delta;
    t_symbol    *x_selector;
    t_float      x_float;
    t_symbol    *x_symbol;
    t_gpointer  *x_pointer;
    int          x_size;    /* as allocated */
    int          x_natoms;  /* as used */
    t_atom      *x_message;
    t_atom       x_messini[SPEEDLIM_INISIZE];
    int          x_entered;
    t_clock     *x_clock;
} t_speedlim;

static t_class *speedlim_class;

static void speedlim_dooutput(t_speedlim *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_open = 0;     /* so there will be no reentrant calls of dooutput */
    x->x_entered = 1;  /* this prevents a message from being overridden */
    clock_unset(x->x_clock);
    if (s == &s_bang)
	outlet_bang(((t_object *)x)->ob_outlet);
    else if (s == &s_float)
	outlet_float(((t_object *)x)->ob_outlet, x->x_float);
    else if (s == &s_symbol && x->x_symbol)
    {
	/* if x_symbol is null, then symbol &s_ is passed
	   by outlet_anything() -> typedmess() */
	outlet_symbol(((t_object *)x)->ob_outlet, x->x_symbol);
	x->x_symbol = 0;
    }
    else if (s == &s_pointer && x->x_pointer)
    {
	/* LATER */
	x->x_pointer = 0;
    }
    else if (s == &s_list)
	outlet_list(((t_object *)x)->ob_outlet, &s_list, ac, av);
    else if (s)
	outlet_anything(((t_object *)x)->ob_outlet, s, ac, av);
    x->x_selector = 0;
    x->x_natoms = 0;
    if (x->x_delta > 0)
	clock_delay(x->x_clock, x->x_delta);
    else
	x->x_open = 1;
    x->x_entered = 0;
}

static void speedlim_tick(t_speedlim *x)
{
    if (x->x_selector)
	speedlim_dooutput(x, x->x_selector, x->x_natoms, x->x_message);
    else
	x->x_open = 1;
}

static void speedlim_anything(t_speedlim *x, t_symbol *s, int ac, t_atom *av)
{
    if (x->x_open)
	speedlim_dooutput(x, s, ac, av);
    else if (s && s != &s_ && !x->x_entered)
    {
	if (ac > x->x_size)
	    /* MAXSIZE not used, not even a warning...
	       LATER consider clipping */
	    x->x_message = grow_nodata(&ac, &x->x_size, x->x_message,
				       SPEEDLIM_INISIZE, x->x_messini,
				       sizeof(*x->x_message));
	x->x_selector = s;
	x->x_natoms = ac;
	if (ac)
	    memcpy(x->x_message, av, ac * sizeof(*x->x_message));
    }
}

static void speedlim_bang(t_speedlim *x)
{
    x->x_selector = &s_bang;
    speedlim_anything(x, x->x_selector, 0, 0);
}

static void speedlim_float(t_speedlim *x, t_float f)
{
    x->x_selector = &s_float;
    x->x_float = f;
    speedlim_anything(x, x->x_selector, 0, 0);
}

static void speedlim_symbol(t_speedlim *x, t_symbol *s)
{
    x->x_selector = &s_symbol;
    x->x_symbol = s;
    speedlim_anything(x, x->x_selector, 0, 0);
}

/* LATER gpointer */

static void speedlim_list(t_speedlim *x, t_symbol *s, int ac, t_atom *av)
{
    x->x_selector = &s_list;
    speedlim_anything(x, x->x_selector, ac, av);
}

static void speedlim_ft1(t_speedlim *x, t_floatarg f)
{
    if (f < 0)
	f = 0;  /* redundant (and CHECKED) */
    x->x_delta = f;
    /* CHECKED: no rearming --
       if clock is set, then new delta value is not used until next tick */
}

static void speedlim_free(t_speedlim *x)
{
    if (x->x_message != x->x_messini)
	freebytes(x->x_message, x->x_size * sizeof(*x->x_message));
    if (x->x_clock)
	clock_free(x->x_clock);
}

static void *speedlim_new(t_floatarg f)
{
    t_speedlim *x = (t_speedlim *)pd_new(speedlim_class);
    x->x_open = 1;  /* CHECKED */
    x->x_delta = 0;
    x->x_selector = 0;
    x->x_float = 0;
    x->x_symbol = 0;
    x->x_pointer = 0;
    x->x_size = SPEEDLIM_INISIZE;
    x->x_natoms = 0;
    x->x_message = x->x_messini;
    x->x_entered = 0;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_anything);
    x->x_clock = clock_new(x, (t_method)speedlim_tick);
    speedlim_ft1(x, f);
    return (x);
}

CYCLONE_OBJ_API void speedlim_setup(void)
{
    speedlim_class = class_new(gensym("speedlim"),
			       (t_newmethod)speedlim_new,
			       (t_method)speedlim_free,
			       sizeof(t_speedlim), 0,
			       A_DEFFLOAT, 0);
    class_addbang(speedlim_class, speedlim_bang);
    class_addfloat(speedlim_class, speedlim_float);
    class_addsymbol(speedlim_class, speedlim_symbol);
    class_addlist(speedlim_class, speedlim_list);
    class_addanything(speedlim_class, speedlim_anything);
    class_addmethod(speedlim_class, (t_method)speedlim_ft1,
		    gensym("ft1"), A_FLOAT, 0);
}
