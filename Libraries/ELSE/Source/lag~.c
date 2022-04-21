// Porres 2019

#include "m_pd.h"
#include <math.h>

#define LOG001 log(0.001)

typedef struct _lag{
    t_object    x_obj;
    t_float     x_in;
    t_inlet    *x_inlet_ms;
    t_outlet   *x_out;
    t_float     x_sr_khz;
    double      x_ynm1;
    int         x_reset;
}t_lag;

static t_class *lag_class;

static t_int *lag_perform(t_int *w){
    t_lag *x = (t_lag *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float sr_khz = x->x_sr_khz;
    double ynm1 = x->x_ynm1;
    while(n--){
        double xn = *in1++, ms = *in2++;
        if(x->x_reset){ // reset
            x->x_reset = 0;
            *out++ = ynm1 = xn;
        }
        else{
            if(ms <= 0)
                *out++ = ynm1 = xn;
            else{
                double a = exp(LOG001 / (ms * sr_khz));
                *out++ = ynm1 = xn + a*(ynm1 - xn);
            }
        }
    }
    x->x_ynm1 = ynm1;
    return(w+6);
}

static void lag_dsp(t_lag *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(lag_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void lag_reset(t_lag *x){
    x->x_reset = 1;
}

static void *lag_new(t_floatarg f){
    t_lag *x = (t_lag *)pd_new(lag_class);
    x->x_reset = 0;
    x->x_inlet_ms = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_ms, f);
    x->x_out = outlet_new((t_object *)x, &s_signal);
    return(x);
}

void lag_tilde_setup(void){
    lag_class = class_new(gensym("lag~"), (t_newmethod)lag_new, 0,
            sizeof(t_lag), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(lag_class, t_lag, x_in);
    class_addmethod(lag_class, (t_method)lag_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(lag_class, (t_method)lag_reset, gensym("reset"), 0);
}
