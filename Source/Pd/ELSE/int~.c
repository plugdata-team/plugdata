// porres 2018

#include "m_pd.h"
#include <math.h>

static t_class *int_tilde_class;

typedef struct _int_tilde{
    t_object x_obj;
}t_int_tilde;

static t_int * int_tilde_perform(t_int *w){
    t_int_tilde *x = (t_int_tilde *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    for(int i = 0 ; i < n ; i++)
        out[i] = trunc(in[i]);
    return (w + 5);
}

static void int_tilde_dsp(t_int_tilde *x, t_signal **sp){
   dsp_add(int_tilde_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void * int_tilde_new(void){
    t_int_tilde *x = (t_int_tilde *) pd_new(int_tilde_class);
    outlet_new((t_object *)x, &s_signal);
    return (void *) x;
}

void int_tilde_setup(void) {
    int_tilde_class = class_new(gensym("int~"),
                (t_newmethod) int_tilde_new, 0, sizeof(t_int_tilde), 0, 0);
    class_addmethod(int_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(int_tilde_class, (t_method)int_tilde_dsp, gensym("dsp"), A_CANT, 0);
}


