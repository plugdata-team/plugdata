// porres 2018

#include "m_pd.h"
#include "math.h"

static t_class *trunc_tilde_class;

typedef struct _trunc_tilde{
    t_object x_obj;
}t_trunc_tilde;

static t_int *trunc_tilde_perform(t_int *w){
    t_trunc_tilde *x = (t_trunc_tilde *)(w[1]);
    t_int n = (trunc)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    for(t_int i = 0 ; i < n; i++)
        out[i] = trunc(in[i]);
    return(w+5);
}

static void trunc_tilde_dsp(t_trunc_tilde *x, t_signal **sp){
   dsp_add(trunc_tilde_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void * trunc_tilde_new(void){
    t_trunc_tilde *x = (t_trunc_tilde *) pd_new(trunc_tilde_class);
    outlet_new((t_object *)x, &s_signal);
    return(void *)x;
}

void trunc_tilde_setup(void) {
    trunc_tilde_class = class_new(gensym("trunc~"), (t_newmethod)trunc_tilde_new,
        0, sizeof(t_trunc_tilde), 0, 0);
    class_addmethod(trunc_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(trunc_tilde_class, (t_method)trunc_tilde_dsp, gensym("dsp"), A_CANT, 0);
}
