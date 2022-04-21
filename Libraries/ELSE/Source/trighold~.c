// Porres 2017

#include "m_pd.h"

static t_class *trighold_class;

typedef struct _trighold{
    t_object x_obj;
    t_float  x_lastin;
    t_float  x_trig;
    t_float  x_in;
} t_trighold;


static t_int * trighold_perform(t_int *w){
    t_trighold *x = (t_trighold *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float lastin = x->x_lastin;
    t_float trig = x->x_trig;
    while(n--){
        float input = *in++;
        if(lastin == 0 && input != 0)
            trig = input;
        *out++ = trig;
        lastin = input;
    }
    x->x_lastin = lastin;
    x->x_trig = trig;
    return (w + 5);
}

static void trighold_dsp(t_trighold *x, t_signal **sp){
  dsp_add(trighold_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *trighold_new(void){
  t_trighold *x = (t_trighold *)pd_new(trighold_class);
  x->x_lastin = 0;
  outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void trighold_tilde_setup(void){
  trighold_class = class_new(gensym("trighold~"),
    (t_newmethod) trighold_new, 0, sizeof (t_trighold), CLASS_DEFAULT, 0);
  CLASS_MAINSIGNALIN(trighold_class, t_trighold, x_in);
  class_addmethod(trighold_class, (t_method) trighold_dsp, gensym("dsp"), A_CANT, 0);
}
