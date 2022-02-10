// Porres 2017

#include "m_pd.h"
#include "math.h"

static t_class *downsample_class;

typedef struct _downsample
{
    t_object x_obj;
    double  x_phase;
    t_float  x_yn;
    t_float  x_ynm1;
    t_float  x_interp;
    t_inlet  *x_inlet;
    float    x_sr;
} t_downsample;

static void downsample_interp(t_downsample *x, t_floatarg f)
{
    x->x_interp = f != 0;
}

static t_int *downsample_perform(t_int *w)
{
    t_downsample *x = (t_downsample *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    double phase = x->x_phase;
    t_float yn = x->x_yn;
    t_float ynm1 = x->x_ynm1;
    t_float interp = x->x_interp;
    double sr = x->x_sr;
    while (nblock--)
        {
        float input = *in1++;
        double hz = *in2++;
        double phase_step = hz / sr;
// clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        if (hz >= 0)
            {
            if (phase >= 1.) // update
                {
                phase = phase - 1;
                ynm1 = yn;
                yn = input;
                }
            }
        else 
            {
            if (phase <= 0.) // update
                {
                phase = phase + 1;
                ynm1 = yn;
                yn = input;
                }
            }
        
            if (interp)
                {
                if (hz >= 0)
                    *out++ = ynm1 + (yn - ynm1) * (phase);
                else
                    *out++ = ynm1 + (yn - ynm1) * (1 - phase);
                }
        else
            *out++ = yn;
            
        phase += phase_step;
        }
    x->x_phase = phase;
    x->x_yn = yn;
    x->x_ynm1 = ynm1;
    return (w + 6);
}

static void downsample_dsp(t_downsample *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(downsample_perform, 5, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *downsample_free(t_downsample *x)
{
    inlet_free(x->x_inlet);
    return (void *)x;
}

static void *downsample_new(t_symbol *s, int argc, t_atom *argv){
    t_downsample *x = (t_downsample *)pd_new(downsample_class);
    t_symbol *dummy = s;
    dummy = NULL;
    t_float init_freq = sys_getsr();
    x->x_interp = x->x_phase = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    init_freq = argval;
                    break;
                case 1:
                    x->x_interp = (argval != 0);
                    break;
                default:
                    break;
            };
            argnum++;
            argc--;
            argv++;
        }
        else
            goto errstate;
    }
/////////////////////////////////////////////////////////////////////////////////////
    if(init_freq >= 0)
        x->x_phase = 1;
    x->x_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet, init_freq);
    outlet_new((t_object *)x, &s_signal);
    return (x);
    errstate:
        pd_error(x, "[downsample~]: improper args");
        return NULL;
}


void downsample_tilde_setup(void)
{
    downsample_class = class_new(gensym("downsample~"),
        (t_newmethod)downsample_new,
        (t_method)downsample_free,
        sizeof(t_downsample),
        CLASS_DEFAULT,
        A_GIMME,
        0);
    class_addmethod(downsample_class, nullfn, gensym("signal"), 0);
    class_addmethod(downsample_class, (t_method)downsample_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(downsample_class, (t_method)downsample_interp, gensym("interp"), A_DEFFLOAT, 0);
}
