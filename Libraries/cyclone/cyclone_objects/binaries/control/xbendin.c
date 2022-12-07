/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _xbendin
{
    t_object       x_ob;
    int            x_omni;
    unsigned char  x_ready;
    unsigned char  x_status;
    unsigned char  x_channel;
    unsigned char  x_lsb;
    t_outlet      *x_chanout;
} t_xbendin;

static t_class *xbendin_class;

static void xbendin_clear(t_xbendin *x)
{
    x->x_status = 0;
    x->x_ready = 0;
}

static void xbendin_float(t_xbendin *x, t_float f)
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
		    xbendin_clear(x);
	    }
	    else if (status == 0xE0)
	    {
		unsigned char channel = bval & 0x0F;
		if (x->x_omni)
		    x->x_channel = channel;
		x->x_status = (x->x_channel == channel);
		x->x_ready = 0;
	    }
	    else xbendin_clear(x);
	}
	else if (x->x_ready)
	{
	    if (x->x_omni)
		outlet_float(x->x_chanout, x->x_channel + 1);
	    outlet_float(((t_object *)x)->ob_outlet, (bval << 7) + x->x_lsb);
	    x->x_ready = 0;
	}
	else if (x->x_status)
	{
	    x->x_lsb = bval;
	    x->x_ready = 1;
	}
    }
    else xbendin_clear(x);
}

static void *xbendin_new(t_floatarg f)
{
    int channel = (int)f;  /* CHECKME */
    t_xbendin *x = (t_xbendin *)pd_new(xbendin_class);
    outlet_new((t_object *)x, &s_float);
    if (x->x_omni = (channel == 0))  /* CHECKME */
	x->x_chanout = outlet_new((t_object *)x, &s_float);
    else
	x->x_channel = (unsigned char)--channel;  /* CHECKME */
    xbendin_clear(x);
    return (x);
}

CYCLONE_OBJ_API void xbendin_setup(void)
{
    xbendin_class = class_new(gensym("xbendin"), 
			      (t_newmethod)xbendin_new, 0,
			      sizeof(t_xbendin), 0,
			      A_DEFFLOAT, 0);
    class_addfloat(xbendin_class, xbendin_float);
    /* CHECKME autocasting lists to floats */
}
