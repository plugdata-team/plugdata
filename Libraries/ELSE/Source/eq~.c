// Porres 2017

#include "m_pd.h"
#include <math.h>

#define PI 3.14159265358979323846
#define HALF_LOG2 log(2) * 0.5

typedef struct _eq{
    t_object    x_obj;
    t_int       x_n;
    t_inlet    *x_inlet_freq;
    t_inlet    *x_inlet_q;
    t_inlet    *x_inlet_amp;
    t_outlet   *x_out;
    t_float     x_nyq;
    int         x_bw;
    int         x_bypass;
    double      x_xnm1;
    double      x_xnm2;
    double      x_ynm1;
    double      x_ynm2;
    double      x_f;
    double      x_reson;
    double      x_db;
    double      x_a0;
    double      x_a1;
    double      x_a2;
    double      x_b1;
    double      x_b2;
}t_eq;

static t_class *eq_class;

static void update_coeffs(t_eq *x, double f, double reson, double db){
    x->x_f = f;
    x->x_reson = reson;
    x->x_db = db;
    double q;
    double omega = f * PI/x->x_nyq;
    if(x->x_bw){ // reson is bw in octaves
         if(reson < 0.000001)
             reson = 0.000001;
         q = 1 / (2 * sinh(HALF_LOG2 * reson * omega/sin(omega)));
    }
    else
        q = reson;
    double amp = pow(10, db / 40);
    double alphaQ = sin(omega) / (2*q);
    double cos_w = cos(omega);
    double b0 = alphaQ/amp + 1;
    x->x_a0 = (1 + alphaQ*amp) / b0;
    x->x_a1 = -2*cos_w / b0;
    x->x_a2 = (1 - alphaQ*amp) / b0;
    x->x_b1 = -x->x_a1;
    x->x_b2 = (alphaQ/amp - 1) / b0;
}

static t_int *eq_perform(t_int *w){
    t_eq *x = (t_eq *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *in4 = (t_float *)(w[6]);
    t_float *out = (t_float *)(w[7]);
    double xnm1 = x->x_xnm1;
    double xnm2 = x->x_xnm2;
    double ynm1 = x->x_ynm1;
    double ynm2 = x->x_ynm2;
    t_float nyq = x->x_nyq;
    while(nblock--){
        double xn = *in1++, f = *in2++, reson = *in3++, db = *in4++, yn;
        if(x->x_bypass)
            *out++ = xn;
        else{
            if(f < 0.1)
                f = 0.1;
            if(f > nyq - 0.1)
                f = nyq - 0.1;
            if(reson < 0.000001)
                reson = 0.000001;
            if(f != x->x_f || reson != x->x_reson || db != x->x_db)
                update_coeffs(x, f, reson, db);
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
    return(w+8);
}

static void eq_dsp(t_eq *x, t_signal **sp){
    t_float nyq = sp[0]->s_sr / 2;
    if(nyq != x->x_nyq){
        x->x_nyq = nyq;
        update_coeffs(x, x->x_f, x->x_reson, x->x_db);
    }
    dsp_add(eq_perform, 7, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void eq_clear(t_eq *x){
    x->x_xnm1 = x->x_xnm2 = x->x_ynm1 = x->x_ynm2 = 0.;
}

static void eq_bypass(t_eq *x, t_floatarg f){
    x->x_bypass = (int)(f != 0);
}

static void eq_bw(t_eq *x){
    x->x_bw = 1;
    update_coeffs(x, x->x_f, x->x_reson, x->x_db);
}

static void eq_q(t_eq *x){
    x->x_bw = 0;
    update_coeffs(x, x->x_f, x->x_reson, x->x_db);
}

static void *eq_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_eq *x = (t_eq *)pd_new(eq_class);
    float freq = 0.1;
    float reson = 0;
    float db = 0;
    int bw = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    freq = argval;
                    break;
                case 1:
                    reson = argval;
                    break;
                case 2:
                    db = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--, argv++;
        }
        else if(argv -> a_type == A_SYMBOL && !argnum){
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
    update_coeffs(x, (double)freq, (double)reson, (double)db);
    x->x_inlet_freq = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq, freq);
    x->x_inlet_q = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_q, reson);
    x->x_inlet_amp = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_amp, db);
    x->x_out = outlet_new((t_object *)x, &s_signal);
    return(x);
errstate:
    pd_error(x, "[eq~]: improper args");
    return(NULL);
}

void eq_tilde_setup(void){
    eq_class = class_new(gensym("eq~"), (t_newmethod)eq_new, 0,
        sizeof(t_eq), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(eq_class, (t_method)eq_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(eq_class, nullfn, gensym("signal"), 0);
    class_addmethod(eq_class, (t_method)eq_clear, gensym("clear"), 0);
    class_addmethod(eq_class, (t_method)eq_bypass, gensym("bypass"), A_DEFFLOAT, 0);
    class_addmethod(eq_class, (t_method)eq_bw, gensym("bw"), 0);
    class_addmethod(eq_class, (t_method)eq_q, gensym("q"), 0);
}
