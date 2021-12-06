// Porres 2017

#include <math.h>
#include "m_pd.h"

static t_class *latoocarfian_class;

typedef struct _latoocarfian
{
    t_object  x_obj;
    int x_val;
    double  x_xn;
    double  x_yn;
    t_float  x_sr;
    double  x_a;
    double  x_b;
    double  x_c;
    double  x_d;
    double  x_lastout;
    double  x_phase;
    t_float  x_freq;
    t_outlet *x_outlet;
} t_latoocarfian;


static void latoocarfian_coeffs(t_latoocarfian *x, t_symbol *s, int argc, t_atom * argv)
{
    if (argc != 4)
        {
        pd_error(x, "latoocarfian~: 'coeffs' needs a list of 4 floats as arguments");
        }
    else
        {
        int argnum = 0; // current argument
        while(argc)
            {
            if(argv -> a_type != A_FLOAT)
                {
                pd_error(x, "latoocarfian~: 'coeffs' arguments needs to only contain floats");
                }
            else
                {
                t_float curf = atom_getfloatarg(0, argc, argv);
                switch(argnum)
                    {
                    case 0:
                    x->x_a = curf;
                    break;
                    case 1:
                    x->x_b = curf;
                    break;
                    case 2:
                    x->x_c = curf;
                    break;
                    case 3:
                    x->x_d = curf;
                    break;
                    };
                    argnum++;
                };
            argc--;
            argv++;
            };
        }
}

static void latoocarfian_list(t_latoocarfian *x, t_symbol *s, int argc, t_atom * argv)
{
    if (argc > 2)
        {
        pd_error(x, "latoocarfian~: list size needs to be = 2");
        }
    else
        {
        int argnum = 0; // current argument
        while(argc)
            {
            if(argv -> a_type != A_FLOAT)
                {
                pd_error(x, "latoocarfian~: list needs to only contain floats");
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

static t_int *latoocarfian_perform(t_int *w)
{
    t_latoocarfian *x = (t_latoocarfian *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    double yn = x->x_yn;
    double xn = x->x_xn;
    double a = x->x_a;
    double b = x->x_b;
    double c = x->x_c;
    double d = x->x_d;
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
        if (trig) // update
            {
            output = sin(yn * b) + c * sin(lastout * b);
            yn = sin(lastout * a) + d * sin(yn * a);
            lastout = output;
            }
        else output = lastout; // last output
        *out++ = output;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_lastout = lastout;
    x->x_yn = yn;
    return (w + 5);
}


static void latoocarfian_dsp(t_latoocarfian *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(latoocarfian_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *latoocarfian_free(t_latoocarfian *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}


static void *latoocarfian_new(t_symbol *s, int ac, t_atom *av)
{
    t_latoocarfian *x = (t_latoocarfian *)pd_new(latoocarfian_class);
    x->x_sr = sys_getsr();
// default parameters
    t_float hz = x->x_sr * 0.5, a = 1, b = 3, c = 0.5, d = 0.5, lastout = 0.5, yn = 0.5;

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
                        a = curf;
                        break;
                    case 2:
                        b = curf;
                        break;
                    case 3:
                        c = curf;
                        break;
                    case 4:
                        d = curf;
                        break;
                    case 5:
                        lastout = curf;
                        break;
                    case 6:
                        yn = curf;
                        break;
                };
                argnum++;
            };
            ac--;
            av++;
        };
    if(hz >= 0) x->x_phase = 1;
    x->x_freq  = hz;
    x->x_yn = yn;
    x->x_lastout = lastout;
    x->x_a = a;
    x->x_b = b;
    x->x_c = c;
    x->x_d = d;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
    errstate:
        pd_error(x, "latoocarfian~: arguments needs to only contain floats");
        return NULL;
}

void latoocarfian_tilde_setup(void)
{
    latoocarfian_class = class_new(gensym("latoocarfian~"),
        (t_newmethod)latoocarfian_new, (t_method)latoocarfian_free,
        sizeof(t_latoocarfian), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(latoocarfian_class, t_latoocarfian, x_freq);
    class_addlist(latoocarfian_class, latoocarfian_list);
    class_addmethod(latoocarfian_class, (t_method)latoocarfian_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(latoocarfian_class, (t_method)latoocarfian_coeffs, gensym("coeffs"), A_GIMME, 0);
}
