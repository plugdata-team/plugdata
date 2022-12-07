/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

#define SLIDE_DEFUP  1.
#define SLIDE_DEFDN  1.

typedef struct _slide
{
    t_object x_obj;
    t_inlet *slide;
    t_int    x_slide_up;
    t_int    x_slide_down;
    t_float  x_last;
} t_slide;

static t_class *slide_class;

static t_int *slide_perform(t_int *w)
{
    t_slide *x = (t_slide *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float last = x->x_last;
    while (nblock--)
    {
    float f = *in1++;
    float output;
	if (f >= last)
	{
	    if (x->x_slide_up > 1.)
        output = last + ((f - last) / x->x_slide_up);
	    else
		output = last = f;
	}
	else if (f < last)
	{
	    if (x->x_slide_down > 1)
        output = last + ((f - last) / x->x_slide_down);
	    else
		output = last = f;
	}
    if (output == last && output != f) output = f;
    *out++ = output;
    last = output; 
    }
    x->x_last = (PD_BIGORSMALL(last) ? 0. : last);
    return (w + 5);
}

static void slide_reset(t_slide *x)
{
    x->x_last = 0;
}

static void slide_slide_up(t_slide *x, t_floatarg f)
{
    int i = (int)f;
    if (i > 1)
    {
        x->x_slide_up = i;
    }
    else
    {
        x->x_slide_up = 0;
    }
}

static void slide_slide_down(t_slide *x, t_floatarg f)
{
    int i = (int)f;
    if (i > 1)
    {
        x->x_slide_down = i;
    }
    else
    {
        x->x_slide_down = 0;
    }
}

static void slide_dsp(t_slide *x, t_signal **sp)
{
    dsp_add(slide_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *slide_new(t_symbol *s, int ac, t_atom *av)
{
    t_slide *x = (t_slide *)pd_new(slide_class);
    float f1 = SLIDE_DEFUP;
    float f2 = SLIDE_DEFDN;
    if (ac && av->a_type == A_FLOAT)
    {
        f1 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            f2 = av->a_w.w_float;
    }
    slide_slide_up(x, f1);
    slide_slide_down(x, f2);
    x->x_last = 0.;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("slide_up"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("slide_down"));
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void slide_tilde_setup(void)
{
    slide_class = class_new(gensym("slide~"),
			    (t_newmethod)slide_new, 0,
			    sizeof(t_slide), 0, A_GIMME, 0);
    class_addmethod(slide_class, nullfn, gensym("signal"), 0);
    class_addmethod(slide_class, (t_method) slide_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(slide_class, (t_method)slide_reset, gensym("reset"), 0);
    class_addmethod(slide_class, (t_method)slide_slide_up, gensym("slide_up"), A_FLOAT, 0);
    class_addmethod(slide_class, (t_method)slide_slide_down, gensym("slide_down"), A_FLOAT, 0);
}
