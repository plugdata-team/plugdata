/* Copyright (c) 2003 krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _mstosamps{
    t_object   x_obj;
    float      x_sr_khz;
    float      x_f;
    t_outlet  *x_floatout;
}t_mstosamps;

static t_class *mstosamps_class;

static void mstosamps_float(t_mstosamps *x, t_float f){
    outlet_float(x->x_floatout, (x->x_f = f) * x->x_sr_khz);
}

static t_int *mstosamps_perform(t_int *w){
    t_mstosamps *x = (t_mstosamps *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while(n--)
        *out++ = *in++ * x->x_sr_khz;
    return(w+5);
}

static void mstosamps_dsp(t_mstosamps *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * .001;
    dsp_add(mstosamps_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *mstosamps_new(void){
    t_mstosamps *x = (t_mstosamps *)pd_new(mstosamps_class);
    x->x_sr_khz = sys_getsr() * .001;
    x->x_f = 0;
    outlet_new((t_object *)x, &s_signal);
    x->x_floatout = outlet_new((t_object *)x, &s_float);
    return(x);
}

CYCLONE_OBJ_API void mstosamps_tilde_setup(void){
    mstosamps_class = class_new(gensym("mstosamps~"),
            (t_newmethod)mstosamps_new, 0, sizeof(t_mstosamps), CLASS_DEFAULT, 0);
    class_addmethod(mstosamps_class, (t_method)mstosamps_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(mstosamps_class, t_mstosamps, x_f);
    class_addfloat(mstosamps_class, (t_method)mstosamps_float);
}
