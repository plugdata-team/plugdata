// Porres 2017

#include "m_pd.h"
#include <math.h>

#define PI 3.14159265358979323846

typedef struct _lop2{
    t_object    x_obj;
    t_int       x_n;
    t_inlet    *x_inlet_freq;
    t_outlet   *x_out;
    t_float     x_nyq;
    double      x_xnm1;
    double      x_ynm1;
    double      x_f;
    double      x_reson;
    double      x_a0;
    double      x_a1;
    double      x_b1;
}t_lop2;

static t_class *lop2_class;

static void update_coeffs(t_lop2 *x, double f){
    x->x_f = f;
    double omega = f * PI/x->x_nyq;
    if(omega < 0)
        omega = 0;
    if(omega > 2){
        x->x_a0 = 1;
        x->x_a1 = x->x_b1 = 0;
    }
    else{
        x->x_a0 = x->x_a1 = omega * 0.5;
        x->x_b1 = 1 - omega;
    }
}

static t_int *lop2_perform(t_int *w){
    t_lop2 *x = (t_lop2 *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    double xnm1 = x->x_xnm1;
    double ynm1 = x->x_ynm1;
    while (nblock--){
        double xn = *in1++, f = *in2++, yn;
        if(f < 0)
            f = 0;
        else{
            if(f != x->x_f)
                update_coeffs(x, (double)f);
            yn = x->x_a0 * xn + x->x_a1 * xnm1 + x->x_b1 * ynm1;
            *out++ = yn;
            xnm1 = xn;
            ynm1 = yn;
        }
    }
    x->x_xnm1 = xnm1;
    x->x_ynm1 = ynm1;
    return(w+6);
}

static void lop2_dsp(t_lop2 *x, t_signal **sp){
    t_float nyq = sp[0]->s_sr / 2;
    if(nyq != x->x_nyq){
        x->x_nyq = nyq;
        update_coeffs(x, x->x_f);
    }
    dsp_add(lop2_perform, 5, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec);
}

static void lop2_clear(t_lop2 *x){
    x->x_xnm1 = x->x_ynm1 = 0.;
}

static void *lop2_new(t_floatarg f){
    t_lop2 *x = (t_lop2 *)pd_new(lop2_class);
    double freq = f < 0 ? 0 : (double)f;
    x->x_nyq = sys_getsr()/2;
    update_coeffs(x, (double)freq);
    x->x_inlet_freq = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq, freq);
    x->x_out = outlet_new((t_object *)x, &s_signal);
    return(x);
}

void lop2_tilde_setup(void){
    lop2_class = class_new(gensym("lop2~"), (t_newmethod)lop2_new, 0,
        sizeof(t_lop2), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(lop2_class, (t_method)lop2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(lop2_class, nullfn, gensym("signal"), 0);
    class_addmethod(lop2_class, (t_method)lop2_clear, gensym("clear"), 0);
}
