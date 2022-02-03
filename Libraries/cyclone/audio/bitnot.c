/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h"

//~ union i32_fl {
	//~ int32_t if_int32;
	//~ t_float if_float;
//~ };

typedef struct _bitnot {
    t_object x_obj;
    t_outlet *x_outlet;
    int    x_convert1;
} t_bitnot;

static t_class *bitnot_class;

void *bitnot_new(t_floatarg f);
static t_int * bitnot_perform(t_int *w);
static void bitnot_dsp(t_bitnot *x, t_signal **sp);
static void bitnot_mode(t_bitnot *x, t_floatarg f);
static void bitnot_float(t_bitnot *x, t_float f);


static t_int * bitnot_perform(t_int *w)
{   // LATER think about performance
    t_bitnot *x = (t_bitnot *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_i32_fl result;
    if (x->x_convert1) while (nblock--)
    {
        int32_t i = ~((int32_t)*in++);
        *out++ = (t_float)i;
    }
    else while (nblock--)
    {
    	result.if_float = *in++;
        result.if_int32 = ~result.if_int32;
        if (BITWISE_ISDENORM(result.if_float))
        	*out++ = 0;
        else
        	*out++ = result.if_float;
    }
    return (w + 5);
}

static void bitnot_dsp(t_bitnot *x, t_signal **sp)
{
    dsp_add(bitnot_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void bitnot_mode(t_bitnot *x, t_floatarg f)
{
    int i = (int)f;
    x->x_convert1 = (i > 0);
}

static void bitnot_float(t_bitnot *x, t_float f)
{
    int i = (int)f;
    x->x_convert1 = (i > 0);
}


void *bitnot_new(t_floatarg f)
{
    t_bitnot *x = (t_bitnot *)pd_new(bitnot_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    bitnot_mode(x, f);
    return (x);
}

CYCLONE_OBJ_API void bitnot_tilde_setup(void) {
    bitnot_class = class_new(gensym("bitnot~"), (t_newmethod) bitnot_new, 0,
        sizeof (t_bitnot), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(bitnot_class, nullfn, gensym("signal"), 0);
    class_addmethod(bitnot_class, (t_method) bitnot_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(bitnot_class, (t_method)bitnot_mode, gensym("mode"), A_FLOAT, 0);
    class_addfloat(bitnot_class, (t_method)bitnot_float);
}
