// Porres 2017

#include <math.h>
#include "m_pd.h"

#define TWOPI 6.283185307179586
#define PI 3.141592653589793
#define RECPI 0.3183098861837907

static t_class *standard_class;

typedef struct _standard
{
    t_object  x_obj;
    int x_val;
    double  x_xn;
    double  x_yn;
    t_float  x_sr;
    double  x_k;
    double  x_lastout;
    double  x_phase;
    t_float  x_freq;
    t_outlet *x_outlet;
} t_standard;


static void standard_k(t_standard *x, t_float f)
{
    x->x_k = f;
}

static void standard_list(t_standard *x, t_symbol *s, int argc, t_atom * argv)
{
    if (argc > 2)
        {
        pd_error(x, "standard~: list size needs to be = 2");
        }
    else{
        int argnum = 0; // current argument
        while(argc)
        {
            if(argv -> a_type != A_FLOAT)
                {
                pd_error(x, "standard~: list needs to only contain floats");
                }
            else
                {
                t_float curf = atom_getfloatarg(0, argc, argv);
                switch(argnum)
                    {
                    case 0:
                        x->x_xn = curf;
                        break;
                    case 1:
                        x->x_yn = curf;
                        break;
                    };
                argnum++;
                };
            argc--;
            argv++;
        };
    }
}



static t_int *standard_perform(t_int *w)
{
    t_standard *x = (t_standard *)(w[1]);
    int nblock = (t_int)(w[2]);
    int *vp = (int *)(w[3]);
    t_float *in = (t_float *)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    int val = *vp; // MUST FALL
    double yn = x->x_yn;
    double xn = x->x_xn;
    double k = x->x_k;
    double lastout = x->x_lastout;
    double phase = x->x_phase;
    double sr = x->x_sr;
    while (nblock--)
    {
        t_float hz = *in++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step; // clipped phase_step
        int trig;
        t_float output;
        if (hz >= 0)
            {
            trig = phase >= 1.;
            if (phase >= 1.) phase = phase - 1;
            }
        else
            {
            trig = (phase <= 0.);
            if (phase <= 0.) phase = phase + 1.;
            }
        if(trig){ // update
            yn = fmod(yn + k * sin(xn), TWOPI);
            xn = fmod(xn + yn, TWOPI);
            if (xn < 0)
                xn = xn + TWOPI;
            output = lastout = (xn - PI) * RECPI;
        }
        else output = lastout; // last output
        *out++ = output;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_lastout = lastout;
    x->x_yn = yn;
    x->x_xn = xn;
    return (w + 6);
}


static void standard_dsp(t_standard *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(standard_perform, 5, x, sp[0]->s_n, &x->x_val, sp[0]->s_vec, sp[1]->s_vec);
}

static void *standard_free(t_standard *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}


static void *standard_new(t_symbol *s, int ac, t_atom *av)
{
    t_standard *x = (t_standard *)pd_new(standard_class);
    x->x_sr = sys_getsr();
    t_float hz = x->x_sr * 0.5, k = 1, xn = 0.5, yn = 0; // default parameters
    if (ac && av->a_type == A_FLOAT)
    {
        hz = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            k = av->a_w.w_float;
            ac--; av++;
            if (ac && av->a_type == A_FLOAT)
                xn = av->a_w.w_float;
                ac--; av++;
                if (ac && av->a_type == A_FLOAT)
                    yn = av->a_w.w_float;
    }
    if(hz >= 0) x->x_phase = 1;
    x->x_freq  = hz;
    x->x_yn = yn;
    x->x_xn = xn;
    x->x_k = k;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void standard_tilde_setup(void)
{
    standard_class = class_new(gensym("standard~"),
        (t_newmethod)standard_new, (t_method)standard_free,
        sizeof(t_standard), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(standard_class, t_standard, x_freq);
    class_addlist(standard_class, standard_list);
    class_addmethod(standard_class, (t_method)standard_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(standard_class, (t_method)standard_k, gensym("k"), A_DEFFLOAT, 0);
}
