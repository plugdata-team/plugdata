/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _xnotein
{
    t_object       x_ob;
    int            x_omni;
    unsigned char  x_ready;
    unsigned char  x_status;
    unsigned char  x_channel;
    unsigned char  x_pitch;
    t_outlet      *x_velout;
    t_outlet      *x_flagout;
    t_outlet      *x_chanout;
} t_xnotein;

static t_class *xnotein_class;

static void xnotein_clear(t_xnotein *x)
{
    x->x_status = 0;
    x->x_ready = 0;
}

static void xnotein_float(t_xnotein *x, t_float f)
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
		    xnotein_clear(x);
	    }
	    else if (status == 0x80 || status == 0x90)
	    {
		unsigned char channel = bval & 0x0F;
		if (x->x_omni)
		    x->x_channel = channel;
		x->x_status = (x->x_channel == channel ? status : 0);
		x->x_ready = 0;
	    }
	    else xnotein_clear(x);
	}
	else if (x->x_ready)
	{
	    int flag = (x->x_status == 0x90 && bval);
	    if (x->x_omni)
		outlet_float(x->x_chanout, x->x_channel + 1);
	    outlet_float(x->x_flagout, flag);
	    outlet_float(x->x_velout, bval);
	    outlet_float(((t_object *)x)->ob_outlet, x->x_pitch);
	    x->x_ready = 0;
	}
	else if (x->x_status)
	{
	    x->x_pitch = bval;
	    x->x_ready = 1;
	}
    }
    else xnotein_clear(x);
}

static void *xnotein_new(t_floatarg f)
{
    int channel = (int)f;  /* CHECKME */
    t_xnotein *x = (t_xnotein *)pd_new(xnotein_class);
    outlet_new((t_object *)x, &s_float);
    x->x_velout = outlet_new((t_object *)x, &s_float);
    x->x_flagout = outlet_new((t_object *)x, &s_float);
    if (x->x_omni = (channel == 0))  /* CHECKME */
	x->x_chanout = outlet_new((t_object *)x, &s_float);
    else
	x->x_channel = (unsigned char)--channel;  /* CHECKME */
    xnotein_clear(x);
    return (x);
}

CYCLONE_OBJ_API void xnotein_setup(void)
{
    xnotein_class = class_new(gensym("xnotein"), 
			      (t_newmethod)xnotein_new, 0,
			      sizeof(t_xnotein), 0,
			      A_DEFFLOAT, 0);
    class_addfloat(xnotein_class, xnotein_float);
    /* CHECKME autocasting lists to floats */
}
