/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* LATER: 'click' method */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>
#include "signal/cybuf.h"
#define PEEK_MAXCHANNELS  64  /* LATER implement arsic resizing feature */
#define PEEK_TICK_TIME  2  //

typedef struct _peek
{
    t_object    x_obj;
    t_cybuf   *x_cybuf;
    int       x_channum;
    int       x_clipmode;
    int       x_pokemode;
    t_float   x_value;
    t_clock  *x_clock;
    double    x_clocklasttick;
    int       x_clockset;

    t_inlet   *x_vallet;
    t_inlet   *x_chanlet;
    t_outlet  *x_outlet;
} t_peek;

static t_class *peek_class;

static void peek_tick(t_peek *x)
{
    cybuf_redraw(x->x_cybuf);  /* LATER redraw only dirty channel(s!) */
    x->x_clockset = 0;
    x->x_clocklasttick = clock_getlogicaltime();
}

static void peek_set(t_peek *x, t_symbol *s)
{
    cybuf_setarray(x->x_cybuf, s);
}

#define peek_doclip(f)  (f < -1. ? -1. : (f > 1. ? 1. : f))

/* CHECKED refman's error: ``if the number received in the left inlet
   specifies a sample index that does not exist in the buffer~ object's
   currently allocated memory, nothing happens.''  This is plainly wrong,
   at least for max/msp 4.0.7 bundle: the index is clipped (just like
   in tabread/tabwrite).   As a kind of an experiment, lets make this
   the refman's way... */
static void peek_float(t_peek *x, t_float f)
{
    t_cybuf * c = x->x_cybuf;
    t_word *vp = c->c_vectors[0];
    //second arg is to allow error posting
    cybuf_validate(c, 1);  /* LATER rethink (efficiency, and complaining) */
    if (vp)
    {
	int ndx = (int)f;
	if (ndx >= 0 && ndx < c->c_npts)
	{
	    if (x->x_pokemode)
	    {
		double timesince;
		t_float val = x->x_value;
		vp[ndx].w_float = (x->x_clipmode ? peek_doclip(val) : val);
		x->x_pokemode = 0;
		timesince = clock_gettimesince(x->x_clocklasttick);
		if (timesince > PEEK_TICK_TIME) peek_tick(x);
		else if (!x->x_clockset)
		{
		    clock_delay(x->x_clock, PEEK_TICK_TIME - timesince);
		    x->x_clockset = 1;
		}
	    }
	    /* CHECKED: output not clipped */
	    else outlet_float(x->x_outlet, vp[ndx].w_float);
	}
    }
}

static void peek_value(t_peek *x, t_floatarg f)
{
    x->x_value = f;
    x->x_pokemode = 1;
    /* CHECKED: poke-mode is reset only after receiving left inlet input
       -- it is kept across 'channel', 'clip', and 'set' inputs. */
}

static void peek_channel(t_peek *x, t_floatarg f)
{
    int ch = f < 1 ? 1 : (f > CYBUF_MAXCHANS ? CYBUF_MAXCHANS : (int) f);
    x->x_channum = ch;
    cybuf_getchannel(x->x_cybuf, ch, 1);
}

static void peek_clip(t_peek *x, t_floatarg f)
{
    x->x_clipmode = ((int)f != 0);
}

static void peek_free(t_peek *x)
{
    if (x->x_clock) clock_free(x->x_clock);
    inlet_free(x->x_vallet);
    inlet_free(x->x_chanlet);
    outlet_free(x->x_outlet);
    cybuf_free(x->x_cybuf);
}

static void *peek_new(t_symbol *s, t_floatarg f1, t_floatarg f2)
{
    //should default to 1?
    int ch = (f1 > 0 ? (int)f1 : 1);
    t_peek *x = (t_peek *)pd_new(peek_class);
    x->x_cybuf = cybuf_init((t_class *)x, s, 1, (ch > CYBUF_MAXCHANS ?  CYBUF_MAXCHANS: ch));
    if (x->x_cybuf)
    {
	if (ch > PEEK_MAXCHANNELS) ch = PEEK_MAXCHANNELS;
        x->x_channum = ch;
	/* CHECKED (refman's error) clipping is disabled by default */
	x->x_clipmode = ((int)f2 != 0);
	x->x_pokemode = 0;
	x->x_vallet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("value"));
	x->x_chanlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("channel"));
	x->x_outlet = outlet_new(&x->x_obj, &s_float);
	x->x_clock = clock_new(x, (t_method)peek_tick);
	x->x_clocklasttick = clock_getlogicaltime();
	x->x_clockset = 0;
    }
    return (x);
}

CYCLONE_OBJ_API void peek_tilde_setup(void)
{
    peek_class = class_new(gensym("peek~"),
			   (t_newmethod)peek_new,
			   (t_method)peek_free,
			   sizeof(t_peek), 0,
			   A_DEFSYM, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(peek_class, peek_float);
    class_addmethod(peek_class, (t_method)peek_set,
		    gensym("set"), A_SYMBOL, 0);
    class_addmethod(peek_class, (t_method)peek_value,
		    gensym("value"), A_FLOAT, 0);
    class_addmethod(peek_class, (t_method)peek_channel,
		    gensym("channel"), A_FLOAT, 0);
    class_addmethod(peek_class, (t_method)peek_clip,
		    gensym("clip"), A_FLOAT, 0);
}
