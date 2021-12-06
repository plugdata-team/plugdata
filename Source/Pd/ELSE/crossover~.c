// Porres 2016-2018

#include "m_pd.h"
#include <math.h>

#define PI 3.14159265358979323846
#define PHI ((PI/2) * (2./3))
#define TWO_COS_PHI (2 * cosf(PHI))
#define TWO_SIN_PHI (2 * sinf(PHI))

typedef struct _crossover{
    t_object    x_obj;
    t_inlet    *x_inlet_freq;  
    t_outlet   *x_out1;
    t_outlet   *x_out2;
    t_float     x_nyq;
    t_float     x_last_f;
    t_float     x_lo1_x1;
    t_float     x_lo1_y1;
    t_float     x_lo2_x1;
    t_float     x_lo2_x2;
    t_float     x_lo2_y1;
    t_float     x_lo2_y2;
    t_float     x_hi1_x1;
    t_float     x_hi1_y1;
    t_float     x_hi2_x1;
    t_float     x_hi2_x2;
    t_float     x_hi2_y1;
    t_float     x_hi2_y2;
}t_crossover;

static t_class *crossover_class;

void crossover_clear(t_crossover *x){
    x->x_lo1_x1 = x->x_lo2_x1 = x->x_lo2_x2 =
    x->x_lo1_y1 = x->x_lo2_y1 = x->x_lo2_y2 = 0.;
}

static t_int *crossover_perform(t_int *w){
    t_crossover *x = (t_crossover *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out1 = (t_float *)(w[5]);
    t_float *out2 = (t_float *)(w[6]);
    t_float lo1_x1 = x->x_lo1_x1; // 1st order zero LOWPASS
    t_float lo1_y1 = x->x_lo1_y1; // 1st order pole LOWPASS
    t_float lo2_x1 = x->x_lo2_x1; // 2nd order zero1 LOWPASS
    t_float lo2_x2 = x->x_lo2_x2; // 2nd order zero2 LOWPASS
    t_float lo2_y1 = x->x_lo2_y1; // 2nd order pole1 LOWPASS
    t_float lo2_y2 = x->x_lo2_y2; // 2nd order pole2 LOWPASS
    t_float hi1_x1 = x->x_hi1_x1; // 1st order zero HIGHPASS
    t_float hi1_y1 = x->x_hi1_y1; // 1st order pole HIGHPASS
    t_float hi2_x1 = x->x_hi2_x1; // 2nd order zero1 HIGHPASS
    t_float hi2_x2 = x->x_hi2_x2; // 2nd order zero2 HIGHPASS
    t_float hi2_y1 = x->x_hi2_y1; // 2nd order pole1 HIGHPASS
    t_float hi2_y2 = x->x_hi2_y2; // 2nd order pole2 HIGHPASS
    t_float nyq = x->x_nyq;
    t_float last_f = x->x_last_f;
    while(nblock--){
        float lo2_yn, lo1_yn, lo2_xn, hi2_yn, hi1_yn, hi2_xn, lo1_xn, hi1_xn, f = *in2++;
        lo1_xn = hi1_xn = *in1++;
        if (f < 1) f = last_f;
        if (f > nyq) f = nyq;
        float r = tanf((f/nyq) * (PI/2));
        last_f = f;
        // poles:
        float c1 = (1 - r*r) / (1 + r*r + 2*r);
        float re = (1 - r*r) / (1 + r*r + r*TWO_COS_PHI);
        float im = r*TWO_SIN_PHI / (1 + r*r + r*TWO_COS_PHI);
        float c2 = 2*re;
        float c3 = -pow(hypotf(re, im), 2);
        
        // start of lowpass:
        float lg1 = fabsf(1 - c1) * 0.5; // gain
        lo2_xn = lo1_yn = lg1*lo1_xn + lg1*lo1_x1 + c1*lo1_y1; // 1sr order section
        lo1_x1 = lo1_xn;
        lo1_y1 = lo1_yn;
        float lg2 = (pow(re-1, 2) + pow(im, 2)) * 0.25; // gain
        lo2_yn = lg2*lo2_xn + 2*lg2*lo2_x1 + lg2*lo2_x2 + c2*lo2_y1 + c3*lo2_y2; // 2nd order section
        lo2_x2 = lo2_x1;
        lo2_x1 = lo2_xn;
        lo2_y2 = lo2_y1;
        lo2_y1 = lo2_yn;
        *out1++ = lo2_yn; // LOWPASS OUTPUT
        
        // start of highpass:
        float hg1 = (c1 + 1) * 0.5; // gain
        hi2_xn = hi1_yn = hg1*lo1_xn + hg1*lo1_x1 + c1*lo1_y1; // 1sr order section
        hi1_x1 = hi1_xn;
        hi1_y1 = hi1_yn;
        float hg2 = (pow(re+1, 2) + pow(im, 2)) * 0.25; // gain
        hi2_yn = hg2*hi2_xn - 2*hg2*hi2_x1 + hg2*hi2_x2 + c2*hi2_y1 + c3*hi2_y2; // 2nd order section
        hi2_x2 = hi2_x1;
        hi2_x1 = hi2_xn;
        hi2_y2 = hi2_y1;
        hi2_y1 = hi2_yn;
        *out2++ = hi2_yn; // HIGHPASS OUTPUT
    }
    x->x_lo1_x1 = lo1_x1;
    x->x_lo1_y1 = lo1_y1;
    x->x_lo2_x1 = lo2_x1;
    x->x_lo2_x2 = lo2_x2;
    x->x_lo2_y1 = lo2_y1;
    x->x_lo2_y2 = lo2_y2;
    x->x_hi1_x1 = hi1_x1;
    x->x_hi1_y1 = hi1_y1;
    x->x_hi2_x1 = hi2_x1;
    x->x_hi2_x2 = hi2_x2;
    x->x_hi2_y1 = hi2_y1;
    x->x_hi2_y2 = hi2_y2;
    x->x_last_f = last_f;
    return (w + 7);
}


static void crossover_dsp(t_crossover *x, t_signal **sp){
    x->x_nyq = sp[0]->s_sr / 2;
    dsp_add(crossover_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *crossover_new(t_floatarg f){
    t_crossover *x = (t_crossover *)pd_new(crossover_class);
    x->x_inlet_freq = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq, f);
    x->x_out1 = outlet_new((t_object *)x, &s_signal);
    x->x_out2 = outlet_new((t_object *)x, &s_signal);
    x->x_lo1_x1 = 0;
    x->x_lo1_y1 = 0;
    x->x_lo2_x1 = 0;
    x->x_lo2_x2 = 0;
    x->x_lo2_y1 = 0;
    x->x_lo2_y2 = 0;
    x->x_hi1_x1 = 0;
    x->x_hi1_y1 = 0;
    x->x_hi2_x1 = 0;
    x->x_hi2_x2 = 0;
    x->x_hi2_y1 = 0;
    x->x_hi2_y2 = 0;
    x->x_last_f = 1000;
    return (x);
}

void crossover_tilde_setup(void){
    crossover_class = class_new(gensym("crossover~"), (t_newmethod)crossover_new, 0,
        sizeof(t_crossover), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(crossover_class, (t_method)crossover_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(crossover_class, nullfn, gensym("signal"), 0);
    class_addmethod(crossover_class, (t_method) crossover_clear, gensym("clear"), 0);
}
