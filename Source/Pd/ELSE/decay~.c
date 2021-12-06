// Porres 2017

#include "m_pd.h"
#include <math.h>

#define LOG001 log(0.001)

typedef struct _decay{
    t_object    x_obj;
    t_inlet    *x_inlet_ms;
    t_outlet   *x_out;
    t_float     x_sr_khz;
    t_int       x_flag;
    double      x_ynm1;
    double      x_f;
}t_decay;

static t_class *decay_class;

static void decay_bang(t_decay *x){
    x->x_flag = 1;
}

static void decay_float(t_decay *x, t_float f){
    x->x_f = (double)f;
    x->x_flag = 1;
}

static t_int *decay_perform(t_int *w){
    t_decay *x = (t_decay *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    double ynm1 = x->x_ynm1;
    t_float sr_khz = x->x_sr_khz;
    while(n--){
        double xn = *in1++, ms = *in2++, a, yn;
        if(x->x_flag){
            xn = x->x_f;
            x->x_flag = 0;
        }
        if(ms <= 0)
            *out++ = xn;
        else{
            a = exp(LOG001 / (ms * sr_khz));
            yn = xn + a * ynm1;
            *out++ = yn;
            ynm1 = yn;
        }
    }
    x->x_ynm1 = ynm1;
    return(w + 6);
}

static void decay_dsp(t_decay *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(decay_perform, 5, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec);
}

static void decay_clear(t_decay *x){
    x->x_ynm1 = 0.;
}

static void *decay_new(t_symbol *s, int argc, t_atom *argv){
    t_decay *x = (t_decay *)pd_new(decay_class);
    float ms = 1000;
    x->x_f = 1.;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    ms = argval;

                default:
                    break;
            };
            argnum++;
            argc--;
            argv++;
        }
        else
            goto errstate;
    };
/////////////////////////////////////////////////////////////////////////////////////
    x->x_inlet_ms = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_ms, ms);
    x->x_out = outlet_new((t_object *)x, &s_signal);
    return(x);
    errstate:
        pd_error(x, "[decay~]: improper args");
        return NULL;
}

void decay_tilde_setup(void){
    decay_class = class_new(gensym("decay~"), (t_newmethod)decay_new, 0,
        sizeof(t_decay), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(decay_class, (t_method)decay_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(decay_class, nullfn, gensym("signal"), 0);
    class_addmethod(decay_class, (t_method)decay_clear, gensym("clear"), 0);
    class_addfloat(decay_class, (t_method)decay_float);
    class_addbang(decay_class, (t_method)decay_bang);
}
