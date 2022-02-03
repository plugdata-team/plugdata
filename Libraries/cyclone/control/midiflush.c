/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <string.h>
#include "m_pd.h"
#include <common/api.h>

#define MIDIFLUSH_NCHANNELS    16
#define MIDIFLUSH_NPITCHES    128
#define MIDIFLUSH_VOIDPITCH  0xFF

typedef struct _midiflush
{
    t_object       x_ob;
    unsigned char  x_status;
    unsigned char  x_channel;
    unsigned char  x_pitch;
    unsigned char  x_notes[MIDIFLUSH_NCHANNELS][MIDIFLUSH_NPITCHES];
} t_midiflush;

static t_class *midiflush_class;

static void midiflush_float(t_midiflush *x, t_float f)
{
    int ival = (int)f;
    if (ival >= 0 && ival < 256)
    {
	unsigned char bval = ival;
	outlet_float(((t_object *)x)->ob_outlet, bval);
	if (bval & 0x80)
	{
	    x->x_status = bval & 0xF0;
	    if (x->x_status == 0x80 || x->x_status == 0x90)
		x->x_channel = bval & 0x0F;
	    else
		x->x_status = 0;
	}
	else if (x->x_status)
	{
	    if (x->x_pitch == MIDIFLUSH_VOIDPITCH)
	    {
		x->x_pitch = bval;
		return;
	    }
	    else if (x->x_status == 0x90 && bval)
	    {
		x->x_notes[x->x_channel][x->x_pitch]++;
	    }
	    else
	    {
		x->x_notes[x->x_channel][x->x_pitch]--;
	    }
	}
    }
    x->x_pitch = MIDIFLUSH_VOIDPITCH;
}

static void midiflush_bang(t_midiflush *x)
{
    int chn, pch;
    for (chn = 0; chn < MIDIFLUSH_NCHANNELS; chn++)
    {
	for (pch = 0; pch < MIDIFLUSH_NPITCHES; pch++)
	{
	    int status = 0x090 | chn;
	    while (x->x_notes[chn][pch])
	    {
		outlet_float(((t_object *)x)->ob_outlet, status);
		outlet_float(((t_object *)x)->ob_outlet, pch);
		outlet_float(((t_object *)x)->ob_outlet, 0);
		x->x_notes[chn][pch]--;
	    }
	}
    }
}

static void midiflush_clear(t_midiflush *x)
{
    memset(x->x_notes, 0, sizeof(x->x_notes));
}

static void *midiflush_new(void)
{
    t_midiflush *x = (t_midiflush *)pd_new(midiflush_class);
    x->x_status = 0;  /* `not a note' */
    x->x_pitch = MIDIFLUSH_VOIDPITCH;
    midiflush_clear(x);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void midiflush_setup(void)
{
    midiflush_class = class_new(gensym("midiflush"), 
				(t_newmethod)midiflush_new,
				0,  /* CHECKED: no flushout */
				sizeof(t_midiflush), 0, 0);
    class_addfloat(midiflush_class, midiflush_float);
    class_addbang(midiflush_class, midiflush_bang);
    class_addmethod(midiflush_class, (t_method)midiflush_clear,
		    gensym("clear"), 0);
}
