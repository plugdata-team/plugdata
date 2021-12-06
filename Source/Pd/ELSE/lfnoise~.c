// Porres 2017

#include "m_pd.h"

static t_class *lfnoise_class;

typedef struct _lfnoise
{
    t_object   x_obj;
    t_float    x_freq;
    int        x_val;
    double     x_phase;
    t_float    x_ynp1;
    t_float    x_yn;
    t_int      x_interp;
    t_inlet   *x_inlet_sync;
    t_outlet  *x_outlet;
    float      x_sr;
} t_lfnoise;

static void lfnoise_interp(t_lfnoise *x, t_floatarg f)
{
    x->x_interp = (int)f != 0;
}

static void lfnoise_seed(t_lfnoise *x, t_floatarg f)
{
    int val = (int)f * 1319;
    x->x_ynp1 = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
    val = val * 435898247 + 382842987;
    x->x_val = val;
}

static t_int *lfnoise_perform(t_int *w)
{
    t_lfnoise *x = (t_lfnoise *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_sample *)(w[5]);
    int val = x->x_val;
    double phase = x->x_phase;
    t_float ynp1 = x->x_ynp1;
    t_float yn = x->x_yn;
    t_int interp = x->x_interp;
    double sr = x->x_sr;
    while (nblock--)
        {
        float hz = *in1++;
        float sync = *in2++;
        double phase_step = hz / sr;
// clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        t_float random;
        if (hz >= 0)
            {
            if (sync == 1)
                phase = 1;
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
            if (sync == 1)
                phase = 0;
            if (phase <= 0.) // update
                {
                random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
                val = val * 435898247 + 382842987;
                phase = phase + 1;
                yn = ynp1;
                ynp1 = random; // next random value
                }
            }
        if (interp)
            {
            if (hz >= 0)
                *out++ = yn + (ynp1 - yn) * (phase);
            else
                *out++ = yn + (ynp1 - yn) * (1 - phase);
            }
        else
            *out++ = yn;

        phase += phase_step;
        }
    x->x_val = val;
    x->x_phase = phase;
    x->x_ynp1 = ynp1; // next random value
    x->x_yn = yn; // current output
    return (w + 6);
}

static void lfnoise_dsp(t_lfnoise *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(lfnoise_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *lfnoise_free(t_lfnoise *x)
{
    inlet_free(x->x_inlet_sync);
    outlet_free(x->x_outlet);
    return (void *)x;
}


static void *lfnoise_new(t_symbol *s, int ac, t_atom *av)
{
    t_lfnoise *x = (t_lfnoise *)pd_new(lfnoise_class);
// default seed
    static int init_seed = 234599;
    init_seed *= 1319;
// default parameters
    t_float hz = 0;
    t_int seed = init_seed, interp = 0;
    
    int argnum = 0; // argument number
    while(ac)
    {
        if(av -> a_type != A_FLOAT) goto errstate;
        else
        {
            t_float curf = atom_getfloatarg(0, ac, av);
            switch(argnum)
            {
                case 0:
                    hz = curf;
                    break;
                case 1:
                    interp = (int)curf;
                    break;
                case 2:
                    seed = (int)curf * 1319;
                    break;
            };
            argnum++;
        };
        ac--;
        av++;
    };
    if (hz >= 0) x->x_phase = 1.;
    x->x_freq = hz;
    x->x_interp = interp != 0;
    
// get 1st output
    x->x_ynp1 = (((float)((seed & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000));
    x->x_val = seed * 435898247 + 382842987;
// in/out
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
// done
    return (x);
    errstate:
        pd_error(x, "lfnoise~: arguments needs to only contain floats");
        return NULL;
}

void lfnoise_tilde_setup(void)
{
    lfnoise_class = class_new(gensym("lfnoise~"),
        (t_newmethod)lfnoise_new,
        (t_method)lfnoise_free,
        sizeof(t_lfnoise),
        CLASS_DEFAULT,
        A_GIMME,
        0);
    CLASS_MAINSIGNALIN(lfnoise_class, t_lfnoise, x_freq);
    class_addmethod(lfnoise_class, (t_method)lfnoise_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(lfnoise_class, (t_method)lfnoise_interp, gensym("interp"), A_DEFFLOAT, 0);
    class_addmethod(lfnoise_class, (t_method)lfnoise_seed, gensym("seed"), A_DEFFLOAT, 0);
}
