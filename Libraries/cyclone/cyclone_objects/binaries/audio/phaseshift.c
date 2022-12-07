// Porres 2016

#define _USE_MATH_DEFINES

#include "m_pd.h"
#include <common/api.h>
#include <math.h>

#define PI M_PI

typedef struct _phaseshift{
    t_object    x_obj;
    t_inlet    *x_inlet_freq;
    t_inlet    *x_inlet_q;
    t_outlet   *x_out;
    t_float     x_nyq;
    t_float     x_x1;
    t_float     x_x2;
    t_float     x_y1;
    t_float     x_y2;
    t_float     x_lastq;
}t_phaseshift;

static t_class *phaseshift_class;

void phaseshift_clear(t_phaseshift *x){
    x->x_x1 = x->x_x2 = x->x_y1 = x->x_y2 = 0.;
}

static t_int *phaseshift_perform(t_int *w){
    t_phaseshift *x = (t_phaseshift *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float x1 = x->x_x1;
    t_float x2 = x->x_x2;
    t_float y1 = x->x_y1;
    t_float y2 = x->x_y2;
    t_float nyq = x->x_nyq;
    t_float lastq = x->x_lastq;
    while (nblock--){
        float omega, alphaQ, b0, a0, a1, yn, xn = *in1++, f = *in2++, q = *in3++;
        if (q < 0) q = lastq;
        lastq = q;
        if (f < 0) f = 0;
        if (f > nyq) f = nyq;
        omega = f * PI/nyq;
        alphaQ = sinf(omega) / (2*q);
        b0 = alphaQ + 1;
        a0 = (1 - alphaQ) / b0;
        a1 = -2*cosf(omega) / b0;
        yn = a0*xn + a1*x1 + x2 - a1*y1 - a0*y2;
        x2 = x1;
        x1 = xn;
        y2 = y1;
        y1 = yn;
        *out++ = yn;
    }
    x->x_x1 = x1;
    x->x_x2 = x2;
    x->x_y1 = y1;
    x->x_y2 = y2;
    x->x_lastq = lastq;
    return (w + 7);
}

static void phaseshift_dsp(t_phaseshift *x, t_signal **sp){
    x->x_nyq = sp[0]->s_sr / 2;
    dsp_add(phaseshift_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
        sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *phaseshift_new(t_floatarg f1, t_floatarg f2){
    t_phaseshift *x = (t_phaseshift *)pd_new(phaseshift_class);
    x->x_inlet_freq = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq, f1);
    x->x_inlet_q = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_q, f2);
    x->x_out = outlet_new((t_object *)x, &s_signal);
    x->x_x1 = 0;
    x->x_y1 = 0;
    x->x_x2 = 0;
    x->x_y2 = 0;
    x->x_lastq = 1;
    return (x);
}

CYCLONE_OBJ_API void phaseshift_tilde_setup(void){
    phaseshift_class = class_new(gensym("phaseshift~"), (t_newmethod)phaseshift_new, 0,
        sizeof(t_phaseshift), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(phaseshift_class, (t_method)phaseshift_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(phaseshift_class, nullfn, gensym("signal"), 0);
    class_addmethod(phaseshift_class, (t_method) phaseshift_clear, gensym("clear"), 0);
}
