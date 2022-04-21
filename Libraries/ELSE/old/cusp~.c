// Porres 2017

#include <math.h>
#include "m_pd.h"

static t_class *cusp_class;

typedef struct _cusp
{
    t_object  x_obj;
    int x_val;
    t_float x_sr;
    double  x_a;
    double  x_b;
    double  x_yn;
    double  x_phase;
    t_float  x_freq;
    t_outlet *x_outlet;
} t_cusp;


static void cusp_list(t_cusp *x, t_symbol *s, int argc, t_atom * argv){
    int argnum = 0; // current argument
    while(argc)
    {
        if(argv -> a_type != A_FLOAT)
        {
            pd_error(x, "cusp~: list needs to only contain floats");
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
                    x->x_yn = curf;
                    break;
            };
            argnum++;
        };
        argc--;
        argv++;
    };
}


static t_int *cusp_perform(t_int *w)
{
    t_cusp *x = (t_cusp *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    double yn = x->x_yn;
    double a = x->x_a;
    double b = x->x_b;
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
            if (trig) phase = phase - 1;
            }
        else
            {
            trig = (phase <= 0.);
            if (trig) phase = phase + 1.;
            }
        if (trig) // update
            {
            yn = a - b * sqrt(fabs(yn));
            }
        *out++ = yn;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_yn = yn;
    return (w + 5);
}


static void cusp_dsp(t_cusp *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(cusp_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *cusp_free(t_cusp *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *cusp_new(t_symbol *s, int ac, t_atom *av)
{
    t_cusp *x = (t_cusp *)pd_new(cusp_class);
    x->x_sr = sys_getsr();
    t_float hz = x->x_sr * 0.5, a = 1, b = 1.9, yn = 0; // default parameters
    if (ac && av->a_type == A_FLOAT)
    {
        hz = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            a = av->a_w.w_float;
            ac--; av++;
            if (ac && av->a_type == A_FLOAT)
                b = av->a_w.w_float;
                ac--; av++;
                if (ac && av->a_type == A_FLOAT)
                    yn = av->a_w.w_float;
    }
    if(hz >= 0) x->x_phase = 1;
    x->x_freq  = hz;
    x->x_a = a;
    x->x_b = b;
    x->x_yn = yn;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void cusp_tilde_setup(void)
{
    cusp_class = class_new(gensym("cusp~"),
        (t_newmethod)cusp_new, (t_method)cusp_free,
        sizeof(t_cusp), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(cusp_class, t_cusp, x_freq);
    class_addlist(cusp_class, cusp_list);
    class_addmethod(cusp_class, (t_method)cusp_dsp, gensym("dsp"), A_CANT, 0);
}
