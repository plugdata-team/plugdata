// Porres 2017-2018

#include "m_pd.h"

static t_class *status_class;

typedef struct _status{
    t_object x_obj;
    t_float  x_lastin;
    t_float  x_in;
} t_status;

static t_int * status_perform(t_int *w){
    t_status *x = (t_status *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out1 = (t_float *)(w[4]);
    t_float *out2 = (t_float *)(w[5]);
    t_float lastin = x->x_lastin;
    while(n--){
        float input = *in++;
        *out1++ = lastin == 0 && input != 0;
        *out2++ = lastin != 0 && input == 0;
        lastin = input;
    }
    x->x_lastin = lastin;
    return (w + 6);
}

static void status_dsp(t_status *x, t_signal **sp){
  dsp_add(status_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

void *status_new(t_floatarg f){
  t_status *x = (t_status *)pd_new(status_class);
  x->x_lastin = f != 0;
  outlet_new(&x->x_obj, &s_signal);
  outlet_new((t_object *)x, &s_signal);
  return (void *)x;
}

void status_tilde_setup(void){
  status_class = class_new(gensym("status~"),
    (t_newmethod) status_new, 0, sizeof (t_status), CLASS_DEFAULT, A_DEFFLOAT, 0);
  CLASS_MAINSIGNALIN(status_class, t_status, x_in);
  class_addmethod(status_class, (t_method) status_dsp, gensym("dsp"), A_CANT, 0);
}
