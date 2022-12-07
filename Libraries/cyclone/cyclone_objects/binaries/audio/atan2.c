/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include <math.h>
#include "m_pd.h"
#include <common/api.h>


typedef struct _atan2 {
    t_object    x_obj;
    t_float     x_input;    //dummy var
    t_inlet    *x_inlet_y;  // main 1st inlet
    t_inlet    *x_inlet_x;  // 2nd inlet
    t_outlet   *x_outlet;
} t_atan2;

static t_class *atan2_class;

static t_int *atan2_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *in2 = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while (nblock--)
    {
	float f1 = *in1++;
	float f2 = *in2++;
	*out++ = atan2f(f1, f2);
    }
    return (w + 5);
}

static void atan2_dsp(t_atan2 *x, t_signal **sp)
{
    dsp_add(atan2_perform, 4, sp[0]->s_n,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *atan2_new(t_floatarg f) // arg = x (not documented in max)
{
    t_atan2 *x = (t_atan2 *)pd_new(atan2_class);
    x->x_inlet_x = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_x, f);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void atan2_tilde_setup(void)
{
    atan2_class = class_new(gensym("atan2~"), (t_newmethod)atan2_new, 0,
        sizeof(t_atan2), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(atan2_class, (t_method)atan2_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(atan2_class, t_atan2, x_input);
}
