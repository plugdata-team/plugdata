// porres 2021

#include "m_pd.h"
#include "math.h"

static t_class *power_tilde_class;

typedef struct _power_tilde{
    t_object    x_obj;
    t_inlet    *x_inlet_v;
}t_power_tilde;

static t_int *power_tilde_perform(t_int *w){
    t_sample *in1 = (t_sample *)(w[2]);
    t_sample *in2 = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    while(n--){
        if(*in1 >= 0)
            *out++ = pow(*in1++, *in2++);
        else
            *out++ = -pow(-*in1++, *in2++);  
    }
    return(w+6);
}

static void power_tilde_dsp(t_power_tilde *x, t_signal **sp){
    dsp_add(power_tilde_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void * power_tilde_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_power_tilde *x = (t_power_tilde *) pd_new(power_tilde_class);
    t_float v = ac ? atom_getfloatarg(0, ac, av) : 1;
    x->x_inlet_v = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_v, v);
    outlet_new((t_object *)x, &s_signal);
    return(void *)x;
}

void power_tilde_setup(void){
    power_tilde_class = class_new(gensym("power~"), (t_newmethod)power_tilde_new,
        0, sizeof(t_power_tilde), 0, A_GIMME, 0);
    class_addmethod(power_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(power_tilde_class, (t_method)power_tilde_dsp, gensym("dsp"), A_CANT, 0);
}
