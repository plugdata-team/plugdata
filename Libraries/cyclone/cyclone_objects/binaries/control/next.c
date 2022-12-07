/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

//#define NEXT_USEEVENTNO

typedef struct _next
{
    t_object   x_ob;
#ifdef NEXT_USEEVENTNO
    int        x_lastevent;
#else
    double     x_lastevent;
#endif
    t_outlet  *x_out2;
} t_next;

static t_class *next_class;

/* CHECKME first call, CHECKME if the outlets are not swapped */
static void next_anything(t_next *x, t_symbol *s, int ac, t_atom *av)
{
#ifdef NEXT_USEEVENTNO
    int nextevent = sys_geteventno();
#else
    double nextevent = clock_getlogicaltime();
#endif
    if (x->x_lastevent == nextevent)
	outlet_bang(x->x_out2);
    else
    {
	x->x_lastevent = nextevent;
	outlet_bang(((t_object *)x)->ob_outlet);
    }
}

static void *next_new(void)
{
    t_next *x = (t_next *)pd_new(next_class);
    outlet_new((t_object *)x, &s_bang);
    x->x_out2 = outlet_new((t_object *)x, &s_bang);
#ifdef NEXT_USEEVENTNO
	x->x_lastevent = sys_geteventno();
#else
	x->x_lastevent = clock_getlogicaltime();
#endif
    return (x);
}

CYCLONE_OBJ_API void next_setup(void)
{
    next_class = class_new(gensym("next"),
			   (t_newmethod)next_new, 0,
			   sizeof(t_next), 0, 0);
    class_addanything(next_class, next_anything);
}
