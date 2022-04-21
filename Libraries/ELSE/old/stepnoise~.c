// Porres 2017

#include "m_pd.h"

static t_class *stepnoise_class;

typedef struct _stepnoise
{
    t_object   x_obj;
    t_float    x_freq;
    int        x_val;
    double     x_phase;
    t_float    x_ynp1;
    t_float    x_yn;
    t_outlet  *x_outlet;
    float      x_sr;
} t_stepnoise;

static void stepnoise_seed(t_stepnoise *x, t_floatarg f)
{
    int val = (int)f * 1319;
    x->x_ynp1 = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
    val = val * 435898247 + 382842987;
    x->x_val = val;
}

static t_int *stepnoise_perform(t_int *w)
{
    t_stepnoise *x = (t_stepnoise *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *out = (t_sample *)(w[4]);
    int val = x->x_val;
    double phase = x->x_phase;
    t_float ynp1 = x->x_ynp1;
    t_float yn = x->x_yn;
    double sr = x->x_sr;
    while (nblock--)
        {
        float hz = *in1++;
        double phase_step = hz / sr;
// clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        t_float random;
        if (hz >= 0)
            {
            if (phase >= 1.) // update
                {
                random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
                val = val * 435898247 + 382842987;
                phase = phase - 1;
                yn = ynp1;
                ynp1 = random; // next random value
                }
            }
        else 
            {
            if (phase <= 0.) // update
                {
                random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
                val = val * 435898247 + 382842987;
                phase = phase + 1;
                yn = ynp1;
                ynp1 = random; // next random value
                }
            }
        *out++ = yn;
        phase += phase_step;
        }
    x->x_val = val;
    x->x_phase = phase;
    x->x_ynp1 = ynp1; // next random value
    x->x_yn = yn; // current output
    return (w + 5);
}

static void stepnoise_dsp(t_stepnoise *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(stepnoise_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *stepnoise_free(t_stepnoise *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *stepnoise_new(t_floatarg f)
{
    t_stepnoise *x = (t_stepnoise *)pd_new(stepnoise_class);
    static int seed = 234599;
    seed *= 1319;
    if (f >= 0)
        x->x_phase = 1.;
    x->x_freq = f;
// get 1st output
    x->x_ynp1 = (((float)((seed & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000));
    x->x_val = seed * 435898247 + 382842987;
//
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void stepnoise_tilde_setup(void)
{
    stepnoise_class = class_new(gensym("stepnoise~"), (t_newmethod)stepnoise_new,
        (t_method)stepnoise_free, sizeof(t_stepnoise), CLASS_DEFAULT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(stepnoise_class, t_stepnoise, x_freq);
    class_addmethod(stepnoise_class, (t_method)stepnoise_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(stepnoise_class, (t_method)stepnoise_seed, gensym("seed"), A_DEFFLOAT, 0);
}
