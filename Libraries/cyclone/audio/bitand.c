/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

//#include <math.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h"

// EXTERN t_float *obj_findsignalscalar(t_object *x, int m);


typedef struct _bitand
{
    t_object  x_obj;
    t_inlet  *x_rightinlet;
    t_glist  *x_glist;
    int32_t   x_mask;
    int       x_mode;
    int       x_convert1;
    t_float  *x_signalscalar;
    
} t_bitand;

static t_class *bitand_class;

static void bitand_intmask(t_bitand *x, t_floatarg f)
{
	x->x_mask = (int32_t)f;
	pd_float(x->x_rightinlet, (t_float)x->x_mask);
}

static t_int *bitand_perform(t_int *w)
{
    t_bitand *x = (t_bitand *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_i32_fl left, right, result;
    switch (x->x_mode)
    {
        case 0:	while (nblock--)  // treat inputs as float
        {
        left.if_float = *in1++;
        right.if_float = *in2++;
        result.if_int32 = left.if_int32 & right.if_int32;
        if (BITWISE_ISDENORM(result.if_float))
        	*out++ = 0;
        else
            *out++ = result.if_float;
        }
        break;
        case 1: while (nblock--) // convert inputs to int
    	{ 
    	int32_t i = ((int32_t)*in1++) & ((int32_t)*in2++);
    	*out++ = (t_float)i;
    	}
        break;
        case 2: while (nblock--) // right input as int
        {
        left.if_float = *in1++;
        result.if_int32 = left.if_int32 & ((int32_t)*in2++);
        if (BITWISE_ISDENORM(result.if_float))
        	*out++ = 0;
        else
            *out++ = result.if_float;
        }
        break;
        case 3: while (nblock--) // left input as int
        {
        right.if_float = *in2++;
        int32_t i = ((int32_t)*in1++) & right.if_int32;
        *out++ = (t_float)i;
        }
	break;
    }
    return (w + 6);
}

static t_int *bitand_perform_noin2(t_int *w)
{ // LATER think about performance
    t_bitand *x = (t_bitand *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_i32_fl left, result;
    int32_t mask = x->x_mask;
    t_float inmask = *x->x_signalscalar;
    if (!magic_isnan(inmask) && mask != (int32_t)inmask)
    {
    	bitand_intmask(x, inmask);
    }
    if (x->x_convert1)
    while (nblock--)
        { int32_t i = ((int32_t)*in++) & mask;
        *out++ = (t_float)i;
        }
    else while (nblock--)
        { 
          left.if_float = *in++;
          result.if_int32 = left.if_int32 & mask;
          if (BITWISE_ISDENORM(result.if_float))
        	*out++ = 0;
          else
            *out++ = result.if_float;
        }
    return (w + 5);
}

static void bitand_dsp(t_bitand *x, t_signal **sp)
{
    if (magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal))
	dsp_add(bitand_perform, 5, x, // use mask from 2nd input signal
        sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    else  // use mask set by 'bits' message or argument
	dsp_add(bitand_perform_noin2, 4, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec);
}

static void bitand_bits(t_bitand *x, t_symbol *s, int ac, t_atom *av)
{
	t_float f;
	magic_setnan(&f);
    x->x_mask = bitwise_getbitmask(ac, av);
    pd_float(x->x_rightinlet, f);
}

static void bitand_mode(t_bitand *x, t_floatarg f)
{
    int i = (int)f;
    x->x_mode = i < 0 ? 0 : i > 3 ? 3 : i;
    x->x_convert1 = (x->x_mode == 1 || x->x_mode == 3);
}

static void *bitand_new(t_floatarg f1, t_floatarg f2)
{
    t_bitand *x = (t_bitand *)pd_new(bitand_class);
    x->x_glist = canvas_getcurrent();
    x->x_rightinlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    x->x_signalscalar = obj_findsignalscalar(x, 1);
    bitand_intmask(x, f1);
    bitand_mode(x, f2);
    return (x);
}

CYCLONE_OBJ_API void bitand_tilde_setup(void)
{
    bitand_class = class_new(gensym("bitand~"), (t_newmethod)bitand_new, 0,
        sizeof(t_bitand), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(bitand_class, nullfn, gensym("signal"), 0);
    class_addmethod(bitand_class, (t_method) bitand_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(bitand_class, (t_method)bitand_bits, gensym("bits"), A_GIMME, 0);
    class_addmethod(bitand_class, (t_method)bitand_mode, gensym("mode"), A_FLOAT, 0);
}
