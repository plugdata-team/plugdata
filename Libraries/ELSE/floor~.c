// Porres 2017

#include "m_pd.h"
#include <math.h>

static t_class *floor_class;

typedef struct _floor{
    t_object x_obj;
    t_outlet *x_outlet;
}t_floor;

void *floor_new(void);
static t_int * floor_perform(t_int *w);
static void floor_dsp(t_floor *x, t_signal **sp);

static t_int * floor_perform(t_int *w){
    t_floor *x = (t_floor *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    while(n--)
        *out++ = floor(*in++);
    return (w + 5);
}

static void floor_dsp(t_floor *x, t_signal **sp){
    dsp_add(floor_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *floor_new(void){
  t_floor *x = (t_floor *)pd_new(floor_class);
  x->x_outlet = outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void floor_tilde_setup(void) {
    floor_class = class_new(gensym("floor~"),
            (t_newmethod) floor_new, 0, sizeof(t_floor), 0, 0);
    class_addmethod(floor_class, nullfn, gensym("signal"), 0);
    class_addmethod(floor_class, (t_method) floor_dsp, gensym("dsp"), A_CANT, 0);
}
