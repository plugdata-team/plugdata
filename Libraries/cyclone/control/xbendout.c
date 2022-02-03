/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _xbendout
{
    t_object  x_ob;
    t_float   x_channel;
    int       x_value;
} t_xbendout;

static t_class *xbendout_class;

static void xbendout_dooutput(t_xbendout *x)
{
    int value = x->x_value;
    int channel = (int)x->x_channel;  /* CHECKME */
    if (value >= 0 &&  /* CHECKME */
	value <= 16383 &&  /* CHECKME */
	channel > 0)  /* CHECKME */
    {
	outlet_float(((t_object *)x)->ob_outlet, 224 + ((channel-1) & 0x0F));
	outlet_float(((t_object *)x)->ob_outlet, value & 0x7F);
	outlet_float(((t_object *)x)->ob_outlet, value >> 7);
    }
}

static void xbendout_float(t_xbendout *x, t_float f)
{
    x->x_value = (int)f;  /* CHECKME */
    xbendout_dooutput(x);
}

static void *xbendout_new(t_floatarg f)
{
    t_xbendout *x = (t_xbendout *)pd_new(xbendout_class);
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    x->x_channel = ((int)f > 0 ? f : 1);  /* CHECKME */
    x->x_value = 8192;  /* CHECKME if not -1 */
    return (x);
}

CYCLONE_OBJ_API void xbendout_setup(void)
{
    xbendout_class = class_new(gensym("xbendout"), 
			       (t_newmethod)xbendout_new, 0,
			       sizeof(t_xbendout), 0,
			       A_DEFFLOAT, 0);
    class_addbang(xbendout_class, xbendout_dooutput);
    class_addfloat(xbendout_class, xbendout_float);
}
