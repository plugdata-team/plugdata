/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h"

static t_class *bitshift_class;

//~ union i32_fl {
	//~ int32_t if_int32;
	//~ t_float if_float;
//~ };

typedef struct _bitshift {
    t_object x_obj;
    t_outlet *x_outlet;
    t_int      x_convert1;
    t_float  x_lshift;
    t_float  x_rshift;
} t_bitshift;

void *bitshift_new(t_floatarg f1, t_floatarg f2);
static t_int * bitshift_perform(t_int *w);
static void bitshift_dsp(t_bitshift *x, t_signal **sp);
static void bitshift_mode(t_bitshift *x, t_floatarg f);
static void bitshift_float(t_bitshift *x, t_float f);

static t_int * bitshift_perform(t_int *w)
{   // LATER think about performance
    t_bitshift *x = (t_bitshift *)(w[1]);
    t_int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_i32_fl result;
    if (x->x_lshift)
    {
        unsigned int shift = (int)x->x_lshift;
        if (x->x_convert1) while (nblock--)
        {
            int32_t i = ((int32_t)*in++ << shift);
            *out++ = (t_float)i;
        }
        else while (nblock--)
        {
        	result.if_float = *in++;
        	result.if_int32 = result.if_int32 << shift;
            if (BITWISE_ISDENORM(result.if_float))
        		*out++ = 0;
       		else
        		*out++ = result.if_float;
        }
    }
    else if (x->x_rshift)
    {
        unsigned int shift = (int)x->x_rshift;
        if (x->x_convert1) while (nblock--)
        {
            int32_t i = ((int32_t)*in++ >> shift);
            *out++ = (t_float)i;
        }
        else while (nblock--)
        {
            result.if_float = *in++;
        	result.if_int32 = result.if_int32 >> shift;
            if (BITWISE_ISDENORM(result.if_float))
        		*out++ = 0;
       		else
        		*out++ = result.if_float;
        }
    }
    else
        while (nblock--) *out++ = *in++;
    return (w + 5);
}

static void bitshift_dsp(t_bitshift *x, t_signal **sp)
{
    dsp_add(bitshift_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void bitshift_mode(t_bitshift *x, t_floatarg f)
{
    int i = (int)f;
    x->x_convert1 = (i > 0);
}

static void bitshift_float(t_bitshift *x, t_float f)
{
    int i = (int)f;
    x->x_convert1 = (i > 0);
}

static void bitshift_shift(t_bitshift *x, t_floatarg f)
 {
 x->x_lshift = x->x_rshift = 0;
 if (f > 0) x->x_lshift = f;
 else x->x_rshift = -f;
 }

void *bitshift_new(t_floatarg f1, t_floatarg f2)
{
    t_bitshift *x = (t_bitshift *)pd_new(bitshift_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    bitshift_shift(x, f1);
    bitshift_mode(x, f2);
    return (x);
}

CYCLONE_OBJ_API void bitshift_tilde_setup(void) { bitshift_class = class_new(gensym("bitshift~"),
        (t_newmethod) bitshift_new, 0, sizeof (t_bitshift), CLASS_DEFAULT,
        A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(bitshift_class, (t_method)bitshift_float);
    class_addmethod(bitshift_class, nullfn, gensym("signal"), 0);
    class_addmethod(bitshift_class, (t_method) bitshift_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(bitshift_class, (t_method)bitshift_mode, gensym("mode"), A_FLOAT, 0);
    class_addmethod(bitshift_class, (t_method)bitshift_shift, gensym("shift"), A_FLOAT, 0);
}
