/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _xnoteout
{
    t_object  x_ob;
    t_float   x_channel;
    t_float   x_flag;
    t_float   x_velocity;
    int       x_pitch;
} t_xnoteout;

static t_class *xnoteout_class;

static void xnoteout_dooutput(t_xnoteout *x)
{
    int status = ((int)x->x_flag ? 0x90 : 0x80);  /* CHECKME */
    int channel = (int)x->x_channel;  /* CHECKME */
    int pitch = x->x_pitch;
    int velocity = (int)x->x_velocity & 0x7F;  /* CHECKME */
    if (pitch >= 0 &&  /* CHECKME */
	pitch <= 127 &&  /* CHECKME */
	channel > 0)  /* CHECKME */
    {
	outlet_float(((t_object *)x)->ob_outlet, status + ((channel-1) & 0x0F));
	outlet_float(((t_object *)x)->ob_outlet, pitch);
	outlet_float(((t_object *)x)->ob_outlet, velocity);
    }
}

static void xnoteout_float(t_xnoteout *x, t_float f)
{
    x->x_pitch = (int)f;  /* CHECKME */
    xnoteout_dooutput(x);
}

static void *xnoteout_new(t_floatarg f)
{
    t_xnoteout *x = (t_xnoteout *)pd_new(xnoteout_class);
    floatinlet_new((t_object *)x, &x->x_velocity);
    floatinlet_new((t_object *)x, &x->x_flag);
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    x->x_channel = ((int)f > 0 ? f : 1);  /* CHECKME */
    x->x_flag = 0;  /* CHECKME */
    x->x_velocity = 0;  /* CHECKME */
    x->x_pitch = -1;  /* CHECKME */
    return (x);
}

CYCLONE_OBJ_API void xnoteout_setup(void)
{
    xnoteout_class = class_new(gensym("xnoteout"), 
			       (t_newmethod)xnoteout_new, 0,
			       sizeof(t_xnoteout), 0,
			       A_DEFFLOAT, 0);
    class_addbang(xnoteout_class, xnoteout_dooutput);
    class_addfloat(xnoteout_class, xnoteout_float);
}
