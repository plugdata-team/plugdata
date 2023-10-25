/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is a modified version of Joseph A. Sarlo's code.
   The most important changes are listed in "pd-lib-notes.txt" file.  */

#include "m_pd.h"
#include <common/api.h>

//#define CYCLE_USEEVENTNO

#define CYCLE_MINOUTS       1
#define CYCLE_C74MAXOUTS  128
#define CYCLE_DEFOUTS       1

typedef struct _cycle
{
    t_object    x_ob;
    int         x_eventmode;
#ifdef CYCLE_USEEVENTNO
    int         x_lastevent;
#else
    double      x_lastevent;
#endif
    int         x_index;
    int         x_nouts;
    t_outlet  **x_outs;
} t_cycle;

static t_class *cycle_class;

static int cycle_isnextevent(t_cycle *x)
{
#ifdef CYCLE_USEEVENTNO
    int nextevent = sys_geteventno();
#else
    double nextevent = clock_getlogicaltime();
#endif
    if (x->x_lastevent == nextevent)
	return (0);
    else
    {
	x->x_lastevent = nextevent;
	return (1);
    }
}

static void cycle_bang(t_cycle *x)
{
    /* CHECKED: bangs ignored (but message 'bang' is an error -- why?) */
}

static void cycle_float(t_cycle *x, t_float f)
{
    if ((x->x_eventmode && cycle_isnextevent(x)) || x->x_index >= x->x_nouts)
	x->x_index = 0;
    outlet_float(x->x_outs[x->x_index++], f);
}

static void cycle_symbol(t_cycle *x, t_symbol *s)
{
    if ((x->x_eventmode && cycle_isnextevent(x)) || x->x_index >= x->x_nouts)
	x->x_index = 0;
    outlet_symbol(x->x_outs[x->x_index++], s);
}

/* LATER gpointer */

static void cycle_list(t_cycle *x, t_symbol *s, int ac, t_atom *av)
{
    if ((x->x_eventmode && cycle_isnextevent(x)) || x->x_index >= x->x_nouts)
	x->x_index = 0;
    while (ac--)
    {
	if (av->a_type == A_FLOAT)
	    outlet_float(x->x_outs[x->x_index], av->a_w.w_float);
	else if (av->a_type == A_SYMBOL)
	    outlet_anything(x->x_outs[x->x_index], av->a_w.w_symbol, 0, 0);
	av++;
	if (++(x->x_index) >= x->x_nouts) x->x_index = 0;
    }
}

static void cycle_anything(t_cycle *x, t_symbol *s, int ac, t_atom *av)
{
    if (s && s != &s)
    {
       if(ac > 1)  cycle_symbol(x, s);  /* CHECKED */
       else
       {
         t_atom list[1];
         SETSYMBOL(&list[0], s);
         cycle_list(x, 0, 1, list);
       };
    }
    cycle_list(x, 0, ac, av);
}

static void cycle_set(t_cycle *x, t_floatarg f)
{
    int i = (int)f;
    if (i >= 0 && i < x->x_nouts) x->x_index = i;
}

static void cycle_thresh(t_cycle *x, t_floatarg f)
{
    if (x->x_eventmode = (f != 0))
#ifdef CYCLE_USEEVENTNO
	x->x_lastevent = sys_geteventno();
#else
	x->x_lastevent = clock_getlogicaltime();
#endif
}

static void cycle_free(t_cycle *x)
{
    if (x->x_outs)
	freebytes(x->x_outs, x->x_nouts * sizeof(*x->x_outs));
}

static void *cycle_new(t_floatarg f1, t_floatarg f2)
{
    t_cycle *x;
    int i, nouts = (int)f1;
    t_outlet **outs;
    if(nouts < CYCLE_MINOUTS)
        nouts = CYCLE_DEFOUTS;
    if(nouts > CYCLE_C74MAXOUTS){
        post("cycle: %d is a lot of outlets", nouts);
        post("cycle: perhaps you were trying to make an oscillator?", nouts);
        nouts = CYCLE_C74MAXOUTS;
    }
    if (!(outs = (t_outlet **)getbytes(nouts * sizeof(*outs))))
	return (0);
    x = (t_cycle *)pd_new(cycle_class);
    x->x_nouts = nouts;
    x->x_outs = outs;
    x->x_index = 0;
    for (i = 0; i < nouts; i++)
        x->x_outs[i] = outlet_new((t_object *)x, &s_anything);
    cycle_thresh(x, f2);
    return (x);
}

CYCLONE_OBJ_API void cycle_setup(void){
    cycle_class = class_new(gensym("cycle"),
			    (t_newmethod)cycle_new,
			    (t_method)cycle_free,
			    sizeof(t_cycle), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addbang(cycle_class, cycle_bang);
    class_addfloat(cycle_class, cycle_float);
    class_addsymbol(cycle_class, cycle_symbol);
    class_addlist(cycle_class, cycle_list);
    class_addanything(cycle_class, cycle_anything);
    class_addmethod(cycle_class, (t_method)cycle_set,
		    gensym("set"), A_FLOAT, 0);  /* CHECKED: arg required */
    class_addmethod(cycle_class, (t_method)cycle_thresh,
		    gensym("thresh"), A_FLOAT, 0);
}
