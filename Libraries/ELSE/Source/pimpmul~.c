// Porres 2021

#include "m_pd.h"
#include "math.h"

typedef struct _pimpmul{
    t_object    x_obj;
    t_inlet    *x_inlet_rate;
    t_outlet   *x_out1;
    t_outlet   *x_out2;
    double      x_last_in;
    double      x_last_out;
    double      x_last_rate;
}t_pimpmul;

static t_class *pimpmul_class;

static t_int *pimpmul_perform(t_int *w){
    t_pimpmul *x = (t_pimpmul *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out1 = (t_float *)(w[5]);
    t_float *out2 = (t_float *)(w[6]);
    double last_in = x->x_last_in;
    double last_rate = x->x_last_rate;
    double last_out = x->x_last_out;
    double output;
    while(n--){
        double input = (double)*in1++;
        double rate = (double)*in2++;
        double delta = (input - last_in);
        if(fabs(delta) >= 0.5){
            if(delta < 0)
                delta += 1;
            else
                delta -= 1;
        }
        if(rate != last_rate){
            *out1++ = output = fmod(input * rate, 1);
            *out2++ = 0;
        }
        else{
            output = fmod(delta * rate + last_out, 1);
            if(output < 0)
                output += 1;
            *out1++ = output;
            *out2++ = fabs(output - last_out) > 0.5;
        }
        last_in = input;
        last_out = output;
        last_rate = rate;
    }
    x->x_last_in = last_in;
    x->x_last_out = last_out;
    x->x_last_rate = last_rate;
    return(w+7);
}

static void pimpmul_dsp(t_pimpmul *x, t_signal **sp){
    dsp_add(pimpmul_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
        sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *pimpmul_new(t_floatarg f){
    t_pimpmul *x = (t_pimpmul *)pd_new(pimpmul_class);
    x->x_inlet_rate = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_rate, f);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    x->x_last_in = x->x_last_out = x->x_last_rate = 0;
    return(x);
}

void pimpmul_tilde_setup(void){
    pimpmul_class = class_new(gensym("pimpmul~"), (t_newmethod)pimpmul_new, 0,
        sizeof(t_pimpmul), 0, A_DEFFLOAT, 0);
    class_addmethod(pimpmul_class, (t_method)pimpmul_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(pimpmul_class, nullfn, gensym("signal"), 0);
}
