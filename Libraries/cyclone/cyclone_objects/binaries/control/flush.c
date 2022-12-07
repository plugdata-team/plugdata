/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>

#define FLUSH_NPITCHES  128

typedef struct _flush
{
    t_object       x_ob;
    t_float        x_velocity;
    unsigned char  x_pitches[FLUSH_NPITCHES];  /* CHECKED */
    t_outlet      *x_voutlet;
} t_flush;

static t_class *flush_class;

static void flush_float(t_flush *x, t_float f)
{
    int pitch = (int)f;
    if (pitch >= 0 && pitch < FLUSH_NPITCHES)
    {
	outlet_float(x->x_voutlet, x->x_velocity);
	outlet_float(((t_object *)x)->ob_outlet, pitch);
	if (x->x_velocity != 0)
	{
	    x->x_pitches[pitch]++;  /* CHECKED (lame) */
	}
	else if (x->x_pitches[pitch])
	{
	    x->x_pitches[pitch]--;  /* CHECKED (lame) */
	}
    }
}

static void flush_bang(t_flush *x)
{
    int i;
    unsigned char *pp;
    for (i = 0, pp = x->x_pitches; i < FLUSH_NPITCHES; i++, pp++)
    {
	while (*pp)
	{
	    outlet_float(x->x_voutlet, 0);
	    outlet_float(((t_object *)x)->ob_outlet, i);
	    (*pp)--;
	}
    }
}

static void flush_clear(t_flush *x)
{
    memset(x->x_pitches, 0, sizeof(x->x_pitches));
}

static void *flush_new(void)
{
    t_flush *x = (t_flush *)pd_new(flush_class);
    x->x_velocity = 0;
    flush_clear(x);
    floatinlet_new((t_object *)x, &x->x_velocity);
    outlet_new((t_object *)x, &s_float);
    x->x_voutlet = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void flush_setup(void)
{
    flush_class = class_new(gensym("flush"), 
			    (t_newmethod)flush_new,
			    0,  /* CHECKED: no flushout */
			    sizeof(t_flush), 0, 0);
    class_addfloat(flush_class, flush_float);
    class_addbang(flush_class, flush_bang);
    class_addmethod(flush_class, (t_method)flush_clear,
		    gensym("clear"), 0);
}
