// Porres 2017

#include "m_pd.h"
#include <math.h>

#define LOG001 log(0.001)

typedef struct _decay2{
    t_object    x_obj;
    t_inlet    *x_inlet_attack;
    t_inlet    *x_inlet_decay;
    t_outlet   *x_out;
    t_float     x_sr_khz;
    t_int       x_flag;
    double      x_last1;
    double      x_last2;
    double      x_f;
}t_decay2;

static t_class *decay2_class;

static void decay2_bang(t_decay2 *x){
    x->x_flag = 1;
}

static void decay2_float(t_decay2 *x, t_float f){
    x->x_f = (double)f;
    x->x_flag = 1;
}

static t_int *decay2_perform(t_int *w){
    t_decay2 *x = (t_decay2 *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    double last1 = x->x_last1;
    double last2 = x->x_last2;
    t_float sr_khz = x->x_sr_khz;
    while(nblock--){
        double a1, a2, yn1, yn2, a, b, n, t, xn = *in1++, attack = *in2++, decay = *in3++;
        if(x->x_flag){
            xn = x->x_f;
            x->x_flag = 0;
        }
        if(attack <= 0)
            yn1 = 0;
        else{
            a1 = exp(LOG001 / (attack * sr_khz));
            yn1 = xn + a1 * last1;
            last1 = yn1;
            a = 1000 * log(1000) / attack;
        }
        if (decay <= 0)
            yn2 = xn;
        else{
            a2 = exp(LOG001 / (decay * sr_khz));
            yn2 = xn + a2 * last2;
            last2 = yn2;
            b = 1000 * log(1000) / decay;
        }
        if(attack > 0 && decay > 0){
            t = log(a/b) / (a-b);
            n = fabs(1/(exp(-b*t) - exp(-a*t)));
            *out++ = (yn2 - yn1) * n;
        }
        else
            *out++ = yn2 - yn1;
    }
    x->x_last1 = last1;
    x->x_last2 = last2;
    return (w + 7);
}

static void decay2_dsp(t_decay2 *x, t_signal **sp)
{
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(decay2_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void decay2_clear(t_decay2 *x)
{
    x->x_last1 = 0.;
    x->x_last2 = 0.;
}

static void *decay2_new(t_symbol *s, int argc, t_atom *argv){
    t_symbol *dummy = s;
    dummy = NULL;
    t_decay2 *x = (t_decay2 *)pd_new(decay2_class);
    float attack = 100;
    float decay = 1000;
    x->x_f = 1.;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0)
    {
        if(argv -> a_type == A_FLOAT)
        { //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum)
                {
                case 0:
                    attack = argval;
                case 1:
                    decay = argval;
                default:
                    break;
                };
            argnum++;
            argc--;
            argv++;
        }
        else
            {
                goto errstate;
            };
    };
/////////////////////////////////////////////////////////////////////////////////////
    x->x_inlet_attack = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_attack, attack);
    x->x_inlet_decay = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_decay, decay);
    x->x_out = outlet_new((t_object *)x, &s_signal);

    return (x);
    errstate:
        pd_error(x, "decay2~: improper args");
        return NULL;
}

void decay2_tilde_setup(void)
{
    decay2_class = class_new(gensym("decay2~"), (t_newmethod)decay2_new, 0,
        sizeof(t_decay2), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(decay2_class, (t_method)decay2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(decay2_class, nullfn, gensym("signal"), 0);
    class_addmethod(decay2_class, (t_method)decay2_clear, gensym("clear"), 0);
    class_addfloat(decay2_class, (t_method)decay2_float);
    class_addbang(decay2_class, (t_method)decay2_bang);
}
