/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

/* LATER find a better way to synchronize with xbendin,
   while avoiding the c74's bug... */

typedef struct _xbendin2
{
    t_object       x_ob;
    int            x_omni;
    unsigned char  x_ready;
    unsigned char  x_status;
    unsigned char  x_channel;
    unsigned char  x_lsb;
    t_outlet      *x_lsbout;
    t_outlet      *x_chanout;
} t_xbendin2;

static t_class *xbendin2_class;

static void xbendin2_clear(t_xbendin2 *x)
{
    x->x_status = 0;
    x->x_ready = 0;
}

static void xbendin2_float(t_xbendin2 *x, t_float f)
{
    int ival = (int)f;  /* CHECKME */
    if (ival < 0)
    {
	/* CHECKME */
	return;
    }
    if (ival < 256)  /* CHECKME */
    {
	unsigned char bval = ival;
	if (bval & 0x80)
	{
	    unsigned char status = bval & 0xF0;
	    if (status == 0xF0)
	    {
		/* CHECKME */
		if (bval < 0xF8)
		    xbendin2_clear(x);
	    }
	    else if (status == 0xE0)
	    {
		unsigned char channel = bval & 0x0F;
		if (x->x_omni)
		    x->x_channel = channel;
		x->x_status = (x->x_channel == channel);
		x->x_ready = 0;
	    }
	    else xbendin2_clear(x);
	}
	else if (x->x_ready)
	{
	    if (x->x_omni)
		outlet_float(x->x_chanout, x->x_channel + 1);
	    outlet_float(x->x_lsbout, x->x_lsb);
	    outlet_float(((t_object *)x)->ob_outlet, bval);
	    x->x_ready = 0;
	}
	else if (x->x_status)
	{
	    x->x_lsb = bval;
	    x->x_ready = 1;
	}
    }
    else xbendin2_clear(x);
}

static void *xbendin2_new(t_floatarg f)
{
    int channel = (int)f;  /* CHECKME */
    t_xbendin2 *x = (t_xbendin2 *)pd_new(xbendin2_class);
    outlet_new((t_object *)x, &s_float);
    x->x_lsbout = outlet_new((t_object *)x, &s_float);
    if (x->x_omni = (channel == 0))  /* CHECKME */
	x->x_chanout = outlet_new((t_object *)x, &s_float);
    else
	x->x_channel = (unsigned char)--channel;  /* CHECKME */
    xbendin2_clear(x);
    return (x);
}

CYCLONE_OBJ_API void xbendin2_setup(void)
{
    xbendin2_class = class_new(gensym("xbendin2"), 
			       (t_newmethod)xbendin2_new, 0,
			       sizeof(t_xbendin2), 0,
			       A_DEFFLOAT, 0);
    class_addfloat(xbendin2_class, xbendin2_float);
    /* CHECKME autocasting lists to floats */
}
