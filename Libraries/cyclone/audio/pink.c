/* Copyright (c) 2003 krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* Filter: Paul Kellet's from music-dsp.  This is the most popular one,
   used in jMax, Csound, etc.  LATER compare to McCartney's (sc).
   Rng: noise~ code from d_osc.c. */

// Max has a dummy signal in (needed for begin~)

#include "m_pd.h"
#include <common/api.h>

/* more like 0.085 in c74's */
#define PINK_GAIN  .105

typedef struct _pink
{
    t_object  x_obj;
    t_inlet  *pink;
    int       x_state;
    float     x_b0;
    float     x_b1;
    float     x_b2;
    float     x_b3;
    float     x_b4;
    float     x_b5;
    float     x_b6;
} t_pink;

static t_class *pink_class;

static t_int *pink_perform(t_int *w)
{
    t_pink *x = (t_pink *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    int state = x->x_state;
    float b0 = x->x_b0;
    float b1 = x->x_b1;
    float b2 = x->x_b2;
    float b3 = x->x_b3;
    float b4 = x->x_b4;
    float b5 = x->x_b5;
    float b6 = x->x_b6;
    while (nblock--)
    {
    	float white = ((float)((state & 0x7fffffff) - 0x40000000)) *
    	    (float)(1.0 / 0x40000000);
    	state = state * 435898247 + 382842987;
	b0 = 0.99886 * b0 + white * 0.0555179;
	b1 = 0.99332 * b1 + white * 0.0750759;
	b2 = 0.96900 * b2 + white * 0.1538520;
	b3 = 0.86650 * b3 + white * 0.3104856;
	b4 = 0.55000 * b4 + white * 0.5329522;
	b5 = -0.7616 * b5 - white * 0.0168980;
	*out++ = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362)
	    * PINK_GAIN;
	b6 = white * 0.115926;
    }
    x->x_state = state;
    x->x_b0 = b0;
    x->x_b1 = b1;
    x->x_b2 = b2;
    x->x_b3 = b3;
    x->x_b4 = b4;
    x->x_b5 = b5;
    x->x_b6 = b6;
    return (w + 5);
}

static void pink_dsp(t_pink *x, t_signal **sp)
{
    dsp_add(pink_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *pink_new(void)
{
    t_pink *x = (t_pink *)pd_new(pink_class);
    /* borrowed from d_osc.c, LATER rethink */
    static int init = 307;
    x->x_state = (init *= 1319);
    /* all coefs set to zero */
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void pink_tilde_setup(void)
{
    pink_class = class_new(gensym("pink~"),
			   (t_newmethod)pink_new, 0,
			   sizeof(t_pink), 0, 0);
    class_addmethod(pink_class, nullfn, gensym("signal"), 0);
    class_addmethod(pink_class, (t_method) pink_dsp, gensym("dsp"), A_CANT, 0);
}
