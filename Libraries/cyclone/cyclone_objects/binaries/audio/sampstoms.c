/* Copyright (c) 2003 krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _sampstoms
{
    t_object x_obj;
    float      x_rcpksr;
    float      x_f;
    t_outlet  *x_floatout;
} t_sampstoms;


static t_class *sampstoms_class;

static void sampstoms_float(t_sampstoms *x, t_float f)
{
	x->x_f = f;
    outlet_float(x->x_floatout, f * x->x_rcpksr);
}

static t_int *sampstoms_perform(t_int *w)
{
    t_sampstoms *x = (t_sampstoms *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    float rcpksr = x->x_rcpksr;
    while (nblock--) *out++ = *in++ * rcpksr;
    return (w + 5);
}

static void sampstoms_dsp(t_sampstoms *x, t_signal **sp)
{
    x->x_rcpksr = 1000. / sp[0]->s_sr;
    dsp_add(sampstoms_perform, 4, x,
	    sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *sampstoms_new(void)
{
    t_sampstoms *x = (t_sampstoms *)pd_new(sampstoms_class);
    x->x_rcpksr = 1000. / sys_getsr();  /* LATER rethink */
    outlet_new((t_object *)x, &s_signal);
    x->x_floatout = outlet_new((t_object *)x, &s_float);
    x->x_f = 0;
    return (x);
}

CYCLONE_OBJ_API void sampstoms_tilde_setup(void)
{
    sampstoms_class = class_new(gensym("sampstoms~"),
        (t_newmethod)sampstoms_new, 0, sizeof(t_sampstoms), CLASS_DEFAULT, 0);
    class_addmethod(sampstoms_class,
                    (t_method)sampstoms_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(sampstoms_class, t_sampstoms, x_f);
    class_addfloat(sampstoms_class,
                   (t_method)sampstoms_float);
}