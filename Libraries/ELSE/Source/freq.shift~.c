// Porres 2016

#include "m_pd.h"
#include "math.h"

#define TWO_PI (3.14159265358979323846 * 2.)

typedef struct _freqshift{
    t_object    x_obj;
    t_inlet    *x_inlet_freq;
    t_outlet   *x_out1;
    t_outlet   *x_out2;
    t_float     x_r1x1;
    t_float     x_r1x2;
    t_float     x_r1y1;
    t_float     x_r1y2;
    t_float     x_r2x1;
    t_float     x_r2x2;
    t_float     x_r2y1;
    t_float     x_r2y2;
    t_float     x_i1x1; // im
    t_float     x_i1x2;
    t_float     x_i1y1;
    t_float     x_i1y2;
    t_float     x_i2x1;
    t_float     x_i2x2;
    t_float     x_i2y1;
    t_float     x_i2y2;
    t_float     x_sr;
    t_float     x_phase;
}t_freqshift;

static t_class *freqshift_class;

void freqshift_clear(t_freqshift *x){
    x->x_r1x1 = x->x_r1x2 = x->x_r1y1 = x->x_r1y2 = 0.;
    x->x_r2x1 = x->x_r2x2 = x->x_r2y1 = x->x_r2y2 = 0.;
    x->x_i1x1 = x->x_i1x2 = x->x_i1y1 = x->x_i1y2 = 0.;
    x->x_i2x1 = x->x_i2x2 = x->x_i2y1 = x->x_i2y2 = 0.;
}

static t_int *freqshift_perform(t_int *w){
    t_freqshift *x = (t_freqshift *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out1 = (t_float *)(w[5]);
    t_float *out2 = (t_float *)(w[6]);
    t_float r1x1 = x->x_r1x1;
    t_float r1x2 = x->x_r1x2;
    t_float r1y1 = x->x_r1y1;
    t_float r1y2 = x->x_r1y2;
    t_float r2x1 = x->x_r2x1;
    t_float r2x2 = x->x_r2x2;
    t_float r2y1 = x->x_r2y1;
    t_float r2y2 = x->x_r2y2;
    t_float i1x1 = x->x_i1x1; // im
    t_float i1x2 = x->x_i1x2;
    t_float i1y1 = x->x_i1y1;
    t_float i1y2 = x->x_i1y2;
    t_float i2x1 = x->x_i2x1;
    t_float i2x2 = x->x_i2x2;
    t_float i2y1 = x->x_i2y1;
    t_float i2y2 = x->x_i2y2;
    t_float sr = x->x_sr;
    t_float phase = x->x_phase;
    while (nblock--){
        float r1xn, r1yn, r2xn, r2yn, i1xn, i1yn, i2xn, i2yn;
        r1xn = i1xn = *in1++;
        r1yn = -0.260502*r1xn + 0.02569*r1x1 + r1x2 - 0.02569*r1y1 + 0.260502*r1y2;
        r1x2 = r1x1;
        r1x1 = r1xn;
        r1y2 = r1y1;
        r1y1 = r1yn;
        r2xn = r1yn;
        r2yn = 0.870686*r2xn - 1.8685*r2x1 + r2x2 + 1.8685*r2y1 - 0.870686*r2y2;
        r2x2 = r2x1;
        r2x1 = r2xn;
        r2y2 = r2y1;
        r2y1 = r2yn;
        float re = r2yn;
       i1yn = 0.94657*i1xn - 1.94632*i1x1 + i1x2 + 1.94632*i1y1 - 0.94657*i1y2;
        i1x2 = i1x1;
        i1x1 = i1xn;
        i1y2 = i1y1;
        i1y1 = i1yn;
        i2xn = i1yn;
        i2yn = 0.06338*i2xn - 0.83774*i2x1 + i2x2 + 0.83774*i2y1 - 0.06338*i2y2;
        i2x2 = i2x1;
        i2x1 = i2xn;
        i2y2 = i2y1;
        i2y1 = i2yn;
        float im = i2yn;
        // complex sinusoid
        float f = *in2++;
        float phase_step = f / sr;
        if (phase_step > 1) phase_step = 1;
        else if (phase_step < -1) phase_step = -1;
        phase += phase_step;
        if(phase > 1) phase = phase - 1;
        else if (phase < 0) phase = phase + 1;
        float re_osc = cosf(phase * TWO_PI);
        float im_osc = sinf(phase * TWO_PI);
        *out1++ = re * re_osc - im * im_osc;
        *out2++ = re * re_osc + im * im_osc;
    }
    x->x_phase = phase;
    x->x_r1x1 = r1x1;
    x->x_r1x2 = r1x2;
    x->x_r1y1 = r1y1;
    x->x_r1y2 = r1y2;
    x->x_r2x1 = r2x1;
    x->x_r2x2 = r2x2;
    x->x_r2y1 = r2y1;
    x->x_r2y2 = r2y2;
    x->x_i1x1 = i1x1;
    x->x_i1x2 = i1x2;
    x->x_i1y1 = i1y1;
    x->x_i1y2 = i1y2;
    x->x_i2x1 = i2x1;
    x->x_i2x2 = i2x2;
    x->x_i2y1 = i2y1;
    x->x_i2y2 = i2y2;
    return (w + 7);
}

static void freqshift_dsp(t_freqshift *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(freqshift_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
        sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *freqshift_new(t_floatarg f){
    t_freqshift *x = (t_freqshift *)pd_new(freqshift_class);
    x->x_inlet_freq = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq, f);
    x->x_out1 = outlet_new((t_object *)x, &s_signal);
    x->x_out2 = outlet_new((t_object *)x, &s_signal);
    x->x_r1x1 = x->x_r1x2 = x->x_r1y1 = x->x_r1y2 = 0.;
    x->x_r2x1 = x->x_r2x2 = x->x_r2y1 = x->x_r2y2 = 0.;
    x->x_i1x1 = x->x_i1x2 = x->x_i1y1 = x->x_i1y2 = 0.;
    x->x_i2x1 = x->x_i2x2 = x->x_i2y1 = x->x_i2y2 = 0.;
    x->x_phase = 0.;
    return (x);
}

void setup_freq0x2eshift_tilde(void){
    freqshift_class = class_new(gensym("freq.shift~"), (t_newmethod)freqshift_new, 0,
        sizeof(t_freqshift), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(freqshift_class, (t_method)freqshift_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(freqshift_class, nullfn, gensym("signal"), 0);
    class_addmethod(freqshift_class, (t_method) freqshift_clear, gensym("clear"), 0);
}
