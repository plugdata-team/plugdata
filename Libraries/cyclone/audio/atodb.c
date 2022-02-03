/*
 Copyright (c) 2016 Marco Matteo Markidis
 mm.markidis@gmail.com
 
 For information on usage and redistribution, and for a DISCLAIMER OF ALL
 WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 
 Made while listening:
 Evan Parker Electro-Acoustic Ensemble -- Hasselt
 */

#include "m_pd.h"
#include <common/api.h>
#include <math.h>

static t_class *atodb_class;

typedef struct _atodb{
  t_object x_obj;
  t_inlet *x_inlet;
  t_outlet *x_outlet;
}t_atodb;

static t_int * atodb_perform(t_int *w){
    t_atodb *x = (t_atodb *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while(n--){
        t_float output = (20*log10(*in++));
        if(output < -999)
            output = -999;
        *out++ = output;
    }
    return(w+5);
}

static void atodb_dsp(t_atodb *x, t_signal **sp){
  dsp_add(atodb_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *atodb_new(void){
  t_atodb *x = (t_atodb *)pd_new(atodb_class);
  x->x_outlet = outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

CYCLONE_OBJ_API void atodb_tilde_setup(void) {
  atodb_class = class_new(gensym("atodb~"),
       (t_newmethod) atodb_new, 0, sizeof (t_atodb), CLASS_DEFAULT, 0);
  class_addmethod(atodb_class, nullfn, gensym("signal"), 0);
  class_addmethod(atodb_class, (t_method) atodb_dsp, gensym("dsp"), A_CANT, 0);
}
