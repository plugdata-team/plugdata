// Porres 2017

#include "m_pd.h"
#include <math.h>

#define PI 3.14159265358979323846
#define HALF_LOG2 log(2)/2

typedef struct _lowpass{
    t_object    x_obj;
    t_int       x_n;
    t_inlet    *x_inlet_freq;
    t_inlet    *x_inlet_q;
    t_outlet   *x_out;
    t_float     x_nyq;
    int         x_bypass;
    int         x_bw;
    double      x_xnm1;
    double      x_xnm2;
    double      x_ynm1;
    double      x_ynm2;
    double      x_f;
    double      x_reson;
    double      x_a0;
    double      x_a1;
    double      x_a2;
    double      x_b1;
    double      x_b2;
}t_lowpass;

static t_class *lowpass_class;

static void update_coeffs(t_lowpass *x, double f, double reson){
    x->x_f = f;
    x->x_reson = reson;
    double q;
    double omega = f * PI/x->x_nyq;
    if(x->x_bw){ // reson is bw in octaves
         if(reson < 0.000001)
             reson = 0.000001;
         q = 1 / (2 * sinh(HALF_LOG2 * reson * omega/sin(omega)));
    }
    else
        q = reson;
    if(q < 0.000001) // force bypass
        x->x_a0 = 1, x->x_a2 = x->x_b1 = x->x_b2 = 0;
    else{
        double alphaQ = sin(omega) / (2*q);
        double cos_w = cos(omega);
        double b0 = alphaQ + 1;
        x->x_a0 = (1 - cos_w) / (2 * b0);
        x->x_a1 = (1 - cos_w) / b0;
        x->x_a2 = x->x_a0;
        x->x_b1 = 2*cos_w / b0;
        x->x_b2 = (alphaQ - 1) / b0;
    }
}

static t_int *lowpass_perform(t_int *w){
    t_lowpass *x = (t_lowpass *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    double xnm1 = x->x_xnm1;
    double xnm2 = x->x_xnm2;
    double ynm1 = x->x_ynm1;
    double ynm2 = x->x_ynm2;
    t_float nyq = x->x_nyq;
    while (nblock--){
        double xn = *in1++, f = *in2++, reson = *in3++, yn;
        if(f < 0.000001)
            f = 0.000001;
        if(f > nyq - 0.000001)
            f = nyq - 0.000001;
        if(x->x_bypass)
            *out++ = xn;
        else{
            if(f != x->x_f || reson != x->x_reson)
                update_coeffs(x, (double)f, (double)reson);
            yn = x->x_a0 * xn + x->x_a1 * xnm1 + x->x_a2 * xnm2 + x->x_b1 * ynm1 + x->x_b2 * ynm2;
            *out++ = yn;
            xnm2 = xnm1;
            xnm1 = xn;
            ynm2 = ynm1;
            ynm1 = yn;
        }
    }
    x->x_xnm1 = xnm1;
    x->x_xnm2 = xnm2;
    x->x_ynm1 = ynm1;
    x->x_ynm2 = ynm2;
    return(w+7);
}

static void lowpass_dsp(t_lowpass *x, t_signal **sp){
    t_float nyq = sp[0]->s_sr / 2;
    if(nyq != x->x_nyq){
        x->x_nyq = nyq;
        update_coeffs(x, x->x_f, x->x_reson);
    }
    dsp_add(lowpass_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void lowpass_clear(t_lowpass *x){
    x->x_xnm1 = x->x_xnm2 = x->x_ynm1 = x->x_ynm2 = 0.;
}

static void lowpass_bypass(t_lowpass *x, t_floatarg f){
    x->x_bypass = (int)(f != 0);
}

static void lowpass_bw(t_lowpass *x){
    x->x_bw = 1;
    update_coeffs(x, x->x_f, x->x_reson);
}

static void lowpass_q(t_lowpass *x){
    x->x_bw = 0;
    update_coeffs(x, x->x_f, x->x_reson);
}

static void *lowpass_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_lowpass *x = (t_lowpass *)pd_new(lowpass_class);
    float freq = 0.000001;
    float reson = 1;
    float bw = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    freq = argval;
                    break;
                case 1:
                    reson = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--, argv++;
        }
        else if(argv->a_type == A_SYMBOL && !argnum){
            if(atom_getsymbolarg(0, argc, argv) == gensym("-bw")){
                bw = 1;
                argc--, argv++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    };
    x->x_bw = bw;
    x->x_nyq = sys_getsr()/2;
    update_coeffs(x, (double)freq, (double)reson);
    x->x_inlet_freq = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq, freq);
    x->x_inlet_q = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_q, reson);
    x->x_out = outlet_new((t_object *)x, &s_signal);
    return (x);
errstate:
    pd_error(x, "[lowpass~]: improper args");
    return(NULL);
}

void lowpass_tilde_setup(void){
    lowpass_class = class_new(gensym("lowpass~"), (t_newmethod)lowpass_new, 0,
        sizeof(t_lowpass), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(lowpass_class, (t_method)lowpass_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(lowpass_class, nullfn, gensym("signal"), 0);
    class_addmethod(lowpass_class, (t_method)lowpass_clear, gensym("clear"), 0);
    class_addmethod(lowpass_class, (t_method)lowpass_bypass, gensym("bypass"), A_DEFFLOAT, 0);
    class_addmethod(lowpass_class, (t_method)lowpass_bw, gensym("bw"), 0);
    class_addmethod(lowpass_class, (t_method)lowpass_q, gensym("q"), 0);
}
