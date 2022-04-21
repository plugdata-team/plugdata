// Porres 2017

#include "m_pd.h"

static t_class *above_class;

typedef struct _above{
    t_object x_obj;
    t_inlet  *x_thresh_inlet;
    t_float  x_in;
    t_float  x_lastin;
}t_above;

static t_int *above_perform(t_int *w){
    t_above *x = (t_above *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out1 = (t_float *)(w[5]);
    t_float *out2 = (t_float *)(w[6]);
    t_float lastin = x->x_lastin;
    while(n--){
        float input = *in1++;
        float threshold = *in2++;
        *out1++ = input > threshold && lastin <= threshold;
        *out2++ = input <= threshold && lastin > threshold;
        lastin = input;
    }
    x->x_lastin = lastin;
    return(w+7);
}

static void above_dsp(t_above *x, t_signal **sp){
    dsp_add(above_perform, 6, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *above_free(t_above *x){
    inlet_free(x->x_thresh_inlet);
    return (void *)x;
}

static void *above_new(t_floatarg f){
    t_above *x = (t_above *)pd_new(above_class);
    x->x_lastin = 0;
    x->x_thresh_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_thresh_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return(x);
}

void above_tilde_setup(void){
    above_class = class_new(gensym("above~"), (t_newmethod)above_new, (t_method)above_free,
        sizeof(t_above), CLASS_DEFAULT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(above_class, t_above, x_in);
    class_addmethod(above_class, (t_method)above_dsp, gensym("dsp"), A_CANT, 0);
}
