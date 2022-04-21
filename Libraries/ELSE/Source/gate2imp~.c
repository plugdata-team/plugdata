// Porres 2017

#include "m_pd.h"

static t_class *gate2imp_class;

typedef struct _gate2imp{
    t_object x_obj;
    t_float  x_lastin;
    t_float  x_in;
} t_gate2imp;


static t_int * gate2imp_perform(t_int *w){
    t_gate2imp *x = (t_gate2imp *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float lastin = x->x_lastin;
    while(n--){
        float input = *in++;
        if(lastin == 0 && input != 0)
            *out++ = input;
        else
            *out++ = 0;
        lastin = input;
    }
    x->x_lastin = lastin;
    return (w + 5);
}

static void gate2imp_dsp(t_gate2imp *x, t_signal **sp){
  dsp_add(gate2imp_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *gate2imp_new(void){
  t_gate2imp *x = (t_gate2imp *)pd_new(gate2imp_class);
  x->x_lastin = 0;
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void gate2imp_tilde_setup(void){
  gate2imp_class = class_new(gensym("gate2imp~"),
    (t_newmethod) gate2imp_new, 0, sizeof (t_gate2imp), CLASS_DEFAULT, 0);
  CLASS_MAINSIGNALIN(gate2imp_class, t_gate2imp, x_in);
  class_addmethod(gate2imp_class, (t_method) gate2imp_dsp, gensym("dsp"), A_CANT, 0);
}
