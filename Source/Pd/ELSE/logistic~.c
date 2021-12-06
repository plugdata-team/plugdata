// Porres 2017

#include "m_pd.h"

static t_class *logistic_class;

typedef struct _logistic
{
    t_object  x_obj;
    int x_val;
    t_float x_sr;
    double  x_p;
    double  x_yn;
    double  x_phase;
    t_float  x_freq;
    t_inlet  *x_p_inlet;
    t_outlet *x_outlet;
} t_logistic;

static t_int *logistic_perform(t_int *w)
{
    t_logistic *x = (t_logistic *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    double yn = x->x_yn;
    double phase = x->x_phase;
    double sr = x->x_sr;
    while (nblock--)
    {
        int trig;
        t_float hz = *in1++;
        t_float p = *in2++; // p
        if (p < 0) p = 0;
        if (p > 1) p = 1;
        p = p + 3;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step; // clipped phase_step
        t_float output;
        if (hz >= 0)
            {
            trig = phase >= 1.;
            if (trig) phase = phase - 1;
            }
        else
            {
            trig = (phase <= 0.);
            if (trig) phase = phase + 1.;
            }
        if (trig) // update
            {
            yn = p*yn * (1 - yn);
            }
        *out++ = yn * 2 - 1; // rescale
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_yn = yn;
    return (w + 6);
}

static void logistic_dsp(t_logistic *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(logistic_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *logistic_free(t_logistic *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *logistic_new(t_symbol *s, int ac, t_atom *av)
{
    t_logistic *x = (t_logistic *)pd_new(logistic_class);
    x->x_sr = sys_getsr();
    t_float hz = 0, p = 0.5; // default parameters
    if (ac && av->a_type == A_FLOAT)
        {
        hz = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            p = av->a_w.w_float;
        }
    if(hz >= 0) x->x_phase = 1;
    x->x_freq  = hz;
    x->x_p = p;
    x->x_yn = 0.5;
    x->x_p_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_p_inlet, p);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void logistic_tilde_setup(void)
{
    logistic_class = class_new(gensym("logistic~"),
        (t_newmethod)logistic_new, (t_method)logistic_free,
        sizeof(t_logistic), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(logistic_class, t_logistic, x_freq);
    class_addmethod(logistic_class, (t_method)logistic_dsp, gensym("dsp"), A_CANT, 0);
}
