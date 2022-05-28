// Porres 2017

#include "m_pd.h"
#include <math.h>

#define PI 3.14159265358979323846
#define HALF_LOG2 log(2)/2

typedef struct _allpass_2nd{
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
    double      x_b1;
    double      x_b2;
}t_allpass_2nd;

static t_class *allpass_2nd_class;

static void update_coeffs(t_allpass_2nd *x, double f, double reson){
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
        x->x_a0 = 1, x->x_a1 = x->x_b1 = x->x_b2 = 0;
    else{
        double alphaQ = sin(omega) / (2*q);
        double cos_w = cos(omega);
        double b0 = alphaQ + 1;
        x->x_a0 = (1 - alphaQ) / b0;
        x->x_a1 = -2*cos_w / b0;
        x->x_b1 = -x->x_a1;
        x->x_b2 = (alphaQ - 1) / b0;
    }
}

static t_int *allpass_2nd_perform(t_int *w){
    t_allpass_2nd *x = (t_allpass_2nd *)(w[1]);
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
    while(nblock--){
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
            yn = x->x_a0 * xn + x->x_a1 * xnm1 + xnm2 + x->x_b1 * ynm1 + x->x_b2 * ynm2;
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

static void allpass_2nd_dsp(t_allpass_2nd *x, t_signal **sp){
    t_float nyq = sp[0]->s_sr / 2;
    if(nyq != x->x_nyq){
        x->x_nyq = nyq;
        update_coeffs(x, x->x_f, x->x_reson);
    }
    dsp_add(allpass_2nd_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void allpass_2nd_clear(t_allpass_2nd *x){
    x->x_xnm1 = x->x_xnm2 = x->x_ynm1 = x->x_ynm2 = 0.;
}

static void allpass_2nd_bypass(t_allpass_2nd *x, t_floatarg f){
    x->x_bypass = (int)(f != 0);
}

static void allpass_2nd_bw(t_allpass_2nd *x){
    x->x_bw = 1;
    update_coeffs(x, x->x_f, x->x_reson);
}

static void allpass_2nd_q(t_allpass_2nd *x){
    x->x_bw = 0;
    update_coeffs(x, x->x_f, x->x_reson);
}

static void *allpass_2nd_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_allpass_2nd *x = (t_allpass_2nd *)pd_new(allpass_2nd_class);
    float freq = 0.000001;
    float reson = 1;
    int bw = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT){
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
            argnum++, argc--, argv++;
        }
        else if(argv->a_type == A_SYMBOL && !argnum){
            t_symbol *sym = atom_getsymbolarg(0, argc, argv);
            if(sym == gensym("-bw")){
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
    return(x);
    errstate:
        pd_error(x, "[allpass.2nd~]: improper args");
        return(NULL);
}

void setup_allpass0x2e2nd_tilde(void){
    allpass_2nd_class = class_new(gensym("allpass.2nd~"), (t_newmethod)allpass_2nd_new, 0,
        sizeof(t_allpass_2nd), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(allpass_2nd_class, (t_method)allpass_2nd_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(allpass_2nd_class, nullfn, gensym("signal"), 0);
    class_addmethod(allpass_2nd_class, (t_method)allpass_2nd_clear, gensym("clear"), 0);
    class_addmethod(allpass_2nd_class, (t_method)allpass_2nd_bypass, gensym("bypass"), A_DEFFLOAT, 0);
    class_addmethod(allpass_2nd_class, (t_method)allpass_2nd_bw, gensym("bw"), 0);
    class_addmethod(allpass_2nd_class, (t_method)allpass_2nd_q, gensym("q"), 0);
}
