// porres 2018

#include "m_pd.h"
#include "math.h"

static t_class *db2lin_tilde_class;

typedef struct _db2lin_tilde{
    t_object x_obj;
    t_float  x_min;
}t_db2lin_tilde;

static t_int *db2lin_tilde_perform(t_int *w){
    t_db2lin_tilde *x = (t_db2lin_tilde *)(w[1]);
    t_int n = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    for(t_int i = 0; i < n; i++){
        if(in[i] <= x->x_min)
            out[i] = 0;
        else
            out[i] = pow(10, in[i] * 0.05);
    }
    return(w+5);
}

static void db2lin_tilde_dsp(t_db2lin_tilde *x, t_signal **sp){
   dsp_add(db2lin_tilde_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void * db2lin_tilde_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_db2lin_tilde *x = (t_db2lin_tilde *) pd_new(db2lin_tilde_class);
    x->x_min = -100;
    if(ac){
        if(av->a_type == A_FLOAT)
            x->x_min = atom_getfloat(av);
        else if(atom_getsymbol(av) == gensym("-inf"))
            x->x_min = -INFINITY;
    }
    outlet_new((t_object *)x, &s_signal);
    return(void *)x;
}

void db2lin_tilde_setup(void) {
    db2lin_tilde_class = class_new(gensym("db2lin~"), (t_newmethod)db2lin_tilde_new,
        0, sizeof(t_db2lin_tilde), 0, A_GIMME, 0);
    class_addmethod(db2lin_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(db2lin_tilde_class, (t_method)db2lin_tilde_dsp, gensym("dsp"), A_CANT, 0);
}
