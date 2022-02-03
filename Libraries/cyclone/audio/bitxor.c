/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

//adding argument parsing and errstate - Derek Kwan 2016

#include <string.h>
//#include <math.h>
#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h"

#define PDCYBXORMASK 0 //default for bitmask
#define PDCYBXORMODE 0 //default for mode


//EXTERN t_float *obj_findsignalscalar(t_object *x, int m);

//~ union i32_fl {
    //~ int32_t if_int32;
    //~ t_float if_float;
//~ };

typedef struct _bitxor
{
    t_object  x_obj;
    t_inlet  *x_rightinlet;
    t_glist  *x_glist;
    int32_t   x_mask;
    int       x_mode;
    int       x_convert1;
    t_float  *x_signalscalar;
    
} t_bitxor;

static t_class *bitxor_class;

static void bitxor_intmask(t_bitxor *x, t_floatarg f)
{
    x->x_mask = (int32_t)f;
    pd_float(x->x_rightinlet, (t_float)x->x_mask);
}

static t_int *bitxor_perform(t_int *w)
{
    t_bitxor *x = (t_bitxor *)(w[1]);
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
            result.if_int32 = left.if_int32 ^ right.if_int32;
            if (BITWISE_ISDENORM(result.if_float))
                *out++ = 0;
            else
                *out++ = result.if_float;
        }
            break;
        case 1: while (nblock--) // convert inputs to int
        {
            int32_t i = ((int32_t)*in1++) ^ ((int32_t)*in2++);
            *out++ = (t_float)i;
        }
            break;
        case 2: while (nblock--) // right input as int
        {
            left.if_float = *in1++;
            result.if_int32 = left.if_int32 ^ ((int32_t)*in2++);
            if (BITWISE_ISDENORM(result.if_float))
                *out++ = 0;
            else
                *out++ = result.if_float;
        }
            break;
        case 3: while (nblock--) // left input as int
        {
            right.if_float = *in2++;
            int32_t i = ((int32_t)*in1++) ^ right.if_int32;
            *out++ = (t_float)i;
        }
            break;
    }
    return (w + 6);
}

static t_int *bitxor_perform_noin2(t_int *w)
{ // LATER think about performance
    t_bitxor *x = (t_bitxor *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_i32_fl left, result;
    int32_t mask = x->x_mask;
    t_float inmask = *x->x_signalscalar;
    if (!magic_isnan(inmask) && mask != (int32_t)inmask)
    {
        bitxor_intmask(x, inmask);
    }
    if (x->x_convert1)
        while (nblock--)
        { int32_t i = ((int32_t)*in++) ^ mask;
            *out++ = (t_float)i;
        }
    else while (nblock--)
    {
        left.if_float = *in++;
        result.if_int32 = left.if_int32 ^ mask;
        if (BITWISE_ISDENORM(result.if_float))
            *out++ = 0;
        else
            *out++ = result.if_float;
    }
    return (w + 5);
}

static void bitxor_dsp(t_bitxor *x, t_signal **sp)
{
    if (magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal))
        dsp_add(bitxor_perform, 5, x, // use mask from 2nd input sig
                sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
    else  // use mask set by 'bits' message or argument
        dsp_add(bitxor_perform_noin2, 4, x, sp[0]->s_n,
                sp[0]->s_vec, sp[1]->s_vec);
}

static void bitxor_bits(t_bitxor *x, t_symbol *s, int ac, t_atom *av)
{
	t_float f;
	magic_setnan(&f);
    x->x_mask = bitwise_getbitmask(ac, av);
    pd_float(x->x_rightinlet, f);
}

static void bitxor_mode(t_bitxor *x, t_floatarg f)
{
    int i = (int)f;
    x->x_mode = i < 0 ? 0 : i > 3 ? 3 : i;
    x->x_convert1 = (x->x_mode == 1 || x->x_mode == 3);
}

static void *bitxor_new(t_symbol *s, int argc, t_atom * argv)
{
    t_bitxor *x = (t_bitxor *)pd_new(bitxor_class);
    x->x_glist = canvas_getcurrent();
    x->x_rightinlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    x->x_signalscalar = obj_findsignalscalar(x, 1);

    //defaults
    t_float bitmask = PDCYBXORMASK;
    t_float opmode = PDCYBXORMODE;
    int curarg = 0; //current argument
    //argument parsing
    while(argc > 0){
        if(argv->a_type == A_FLOAT){
            t_float curfloat = atom_getfloatarg(0, argc, argv);
            switch(curarg){
                case 0:
                    bitmask = curfloat;
                    break;
                case 1:
                    opmode = curfloat;
                    break;
                default:
                    break;
            };
            //increment
            curarg++;
            argc--;
            argv++;
        }
        else if(argv->a_type == A_SYMBOL){
            if(argc >= 2){
                t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
                t_float curfloat = atom_getfloatarg(1, argc, argv);
                if(strcmp(cursym->s_name, "@mode") == 0){
                    opmode = curfloat;
                    //increment
                    curarg+=2;
                    argc-=2;
                    argv+=2;
                }
                else{
                    goto errstate;
                };
            }
            else{
                goto errstate;
            };
        }
        else{
            goto errstate;
        };
    };
    bitxor_intmask(x, bitmask);
    bitxor_mode(x, opmode);
    return (x);
    errstate:
	pd_error(x, "bitxor~: improper args");
        return NULL;

}

CYCLONE_OBJ_API void bitxor_tilde_setup(void)
{
    bitxor_class = class_new(gensym("bitxor~"), (t_newmethod)bitxor_new, 0,
                            sizeof(t_bitxor), 0, A_GIMME, 0);
    class_addmethod(bitxor_class, nullfn, gensym("signal"), 0);
    class_addmethod(bitxor_class, (t_method) bitxor_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(bitxor_class, (t_method)bitxor_bits, gensym("bits"), A_GIMME, 0);
    class_addmethod(bitxor_class, (t_method)bitxor_mode, gensym("mode"), A_FLOAT, 0);
}
