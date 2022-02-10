// porres 2018

#include "m_pd.h"
#include "math.h"

static t_class *rint_class;

typedef struct _rint{
    t_object x_obj;
}t_rint;

static t_int * rint_perform(t_int *w){
    t_rint *x = (t_rint *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    for(int i = 0 ; i < n ; i++)
        out[i] = rint(in[i]);
    return (w + 5);
}

static void rint_dsp(t_rint *x, t_signal **sp){
   dsp_add(rint_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void * rint_new(void){
    t_rint *x = (t_rint *) pd_new(rint_class);
    outlet_new((t_object *)x, &s_signal);
    return (void *) x;
}

void rint_tilde_setup(void) {
    rint_class = class_new(gensym("rint~"),
        (t_newmethod) rint_new, 0, sizeof(t_rint), 0, 0);
    class_addmethod(rint_class, nullfn, gensym("signal"), 0);
    class_addmethod(rint_class, (t_method)rint_dsp, gensym("dsp"), A_CANT, 0);
}


