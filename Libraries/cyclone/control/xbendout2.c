/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _xbendout2
{
    t_object  x_ob;
    t_float   x_channel;
    t_float   x_lsb;
    int       x_msb;
} t_xbendout2;

static t_class *xbendout2_class;

static void xbendout2_dooutput(t_xbendout2 *x)
{
    int msb = x->x_msb;
    int lsb = (int)x->x_lsb;  /* CHECKME */
    int channel = (int)x->x_channel;  /* CHECKME */
    if (msb >= 0 &&  /* CHECKME */
	msb <= 127 &&  /* CHECKME */
	lsb >= 0 &&  /* CHECKME */
	lsb <= 127 &&  /* CHECKME */
	channel > 0)  /* CHECKME */
    {
	outlet_float(((t_object *)x)->ob_outlet, 224 + ((channel-1) & 0x0F));
	outlet_float(((t_object *)x)->ob_outlet, lsb);
	outlet_float(((t_object *)x)->ob_outlet, msb);
    }
}

static void xbendout2_float(t_xbendout2 *x, t_float f)
{
    x->x_msb = (int)f;  /* CHECKME */
    xbendout2_dooutput(x);
}

static void *xbendout2_new(t_floatarg f)
{
    t_xbendout2 *x = (t_xbendout2 *)pd_new(xbendout2_class);
    floatinlet_new((t_object *)x, &x->x_lsb);
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    x->x_channel = ((int)f > 0 ? f : 1);  /* CHECKME */
    x->x_lsb = 0;
    x->x_msb = 64;  /* CHECKME if not -1 */
    return (x);
}

CYCLONE_OBJ_API void xbendout2_setup(void)
{
    xbendout2_class = class_new(gensym("xbendout2"), 
				(t_newmethod)xbendout2_new, 0,
				sizeof(t_xbendout2), 0,
				A_DEFFLOAT, 0);
    class_addbang(xbendout2_class, xbendout2_dooutput);
    class_addfloat(xbendout2_class, xbendout2_float);
}
