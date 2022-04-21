// Porres 2017

#include "m_pd.h"
#include <math.h>

#define LOG001 log(0.001)

typedef struct _lag2{
    t_object    x_obj;
    t_float     x_in;
    t_inlet    *x_inlet_ms_up;
    t_inlet    *x_inlet_ms_down;
    t_outlet   *x_out;
    t_float     x_sr_khz;
    double      x_ynm1;
    int         x_reset;
}t_lag2;

static t_class *lag2_class;

static t_int *lag2_perform(t_int *w){
    t_lag2 *x = (t_lag2 *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    double ynm1 = x->x_ynm1;
    t_float sr_khz = x->x_sr_khz;
    while(n--){
        double xn = *in1++;
        double ms_up = *in2++, ms_down = *in3++;
        double a, yn;
        if(x->x_reset){ // reset
            x->x_reset = 0;
            *out++ = yn = xn;
        }
        else{
            if(xn >= ynm1)
                a = ms_up > 0 ? exp(LOG001 / (ms_up * sr_khz)) : 0;
            else
                a = ms_down > 0 ? exp(LOG001 / (ms_down * sr_khz)) : 0;
            if(a == 0)
                *out++ = yn = xn;
            else
                *out++ = yn = xn + a*(ynm1 - xn);
            ynm1 = yn;
        }
    }
    x->x_ynm1 = (PD_BIGORSMALL(ynm1) ? 0. : ynm1);
    return(w+7);
}

static void lag2_dsp(t_lag2 *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(lag2_perform, 6, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[3]->s_vec);
}

static void lag2_reset(t_lag2 *x){
    x->x_reset = 1;
}

static void *lag2_new(t_floatarg f1, t_floatarg f2){
    t_lag2 *x = (t_lag2 *)pd_new(lag2_class);
    x->x_reset = 0;
    x->x_inlet_ms_up = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_ms_up, f1);
    x->x_inlet_ms_down = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_ms_down, f2);
    x->x_out = outlet_new((t_object *)x, &s_signal);
    return(x);
}

void lag2_tilde_setup(void){
    lag2_class = class_new(gensym("lag2~"), (t_newmethod)lag2_new, 0,
            sizeof(t_lag2), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(lag2_class, t_lag2, x_in);
    class_addmethod(lag2_class, (t_method)lag2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(lag2_class, (t_method)lag2_reset, gensym("reset"), 0);
}
