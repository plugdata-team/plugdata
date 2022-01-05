// Porres 2017

#include "m_pd.h"

static t_class *rampnoise_class;

typedef struct _rampnoise
{
    t_object   x_obj;
    t_float    x_freq;
    int        x_val;
    double     x_phase;
    t_float    x_ynp1;
    t_float    x_yn;
    t_outlet  *x_outlet;
    float      x_sr;
} t_rampnoise;

static void rampnoise_seed(t_rampnoise *x, t_floatarg f)
{
    int val = (int)f * 1319;
    x->x_ynp1 = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
    val = val * 435898247 + 382842987;
    x->x_val = val;
}

static t_int *rampnoise_perform(t_int *w)
{
    t_rampnoise *x = (t_rampnoise *)(w[1]);
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
            *out++ = yn + (ynp1 - yn) * (phase);
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
            *out++ = yn + (ynp1 - yn) * (1 - phase);
            }
        phase += phase_step;
        }
    x->x_val = val;
    x->x_phase = phase;
    x->x_ynp1 = ynp1; // next random value
    x->x_yn = yn; // current output
    return (w + 5);
}

static void rampnoise_dsp(t_rampnoise *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(rampnoise_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *rampnoise_free(t_rampnoise *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *rampnoise_new(t_floatarg f)
{
    t_rampnoise *x = (t_rampnoise *)pd_new(rampnoise_class);
// default seed
    static int seed = 234599;
    seed *= 1319;
    if (f >= 0)
        x->x_phase = 1.;
    x->x_freq = f;
// get 1st output
    x->x_ynp1 = (((float)((seed & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000));
    x->x_val = seed * 435898247 + 382842987;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void rampnoise_tilde_setup(void){
    rampnoise_class = class_new(gensym("rampnoise~"), (t_newmethod)rampnoise_new,
        (t_method)rampnoise_free, sizeof(t_rampnoise), CLASS_DEFAULT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(rampnoise_class, t_rampnoise, x_freq);
    class_addmethod(rampnoise_class, (t_method)rampnoise_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(rampnoise_class, (t_method)rampnoise_seed, gensym("seed"), A_DEFFLOAT, 0);
}
