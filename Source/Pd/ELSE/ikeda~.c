// Porres 2017

#include <math.h>
#include "m_pd.h"

static t_class *ikeda_class;

typedef struct _ikeda
{
    t_object  x_obj;
    int x_val;
    t_float  x_yn1;
    t_float  x_yn2;
    t_float  x_sr;
    double  x_phase;
    float  x_u;
    t_float  x_freq;
    t_outlet *x_outlet;
} t_ikeda;

static void ikeda_u(t_ikeda *x, t_float f)
{
    x->x_u = f;
}

static void ikeda_clear(t_ikeda *x)
{
    x->x_yn1 = x->x_yn2 = 0;
}

static void ikeda_list(t_ikeda *x, t_symbol *s, int argc, t_atom * argv)
{
    if (argc > 2)
        {
        pd_error(x, "ikeda~: list size needs to be = 2");
        }
    else{
        int argnum = 0; // current argument
        while(argc)
        {
            if(argv -> a_type != A_FLOAT)
                {
                pd_error(x, "ikeda~: list needs to only contain floats");
                }
            else
                {
                t_float curf = atom_getfloatarg(0, argc, argv);
                switch(argnum)
                    {
                    case 0:
                        x->x_yn1 = curf;
                        break;
                    case 1:
                        x->x_yn2 = curf;
                        break;
                    };
                argnum++;
                };
            argc--;
            argv++;
        };
    }
}

static t_int *ikeda_perform(t_int *w)
{
    t_ikeda *x = (t_ikeda *)(w[1]);
    int nblock = (t_int)(w[2]);
    int *vp = (int *)(w[3]);
    t_float *in = (t_float *)(w[4]);
    t_sample *output_1 = (t_sample *)(w[5]);
    t_sample *output_2 = (t_sample *)(w[6]);
    t_float yn1 = x->x_yn1;
    t_float yn2 = x->x_yn2;
    double phase = x->x_phase;
    t_float sr = x->x_sr;
    t_float u = x->x_u;
    while (nblock--)
    {
        t_float hz = *in++;
        double phase_step = (double)(hz / sr); // phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step; // clipped phase_step
        int trig;
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
            float t = 0.4 - 6/(1 + pow(yn1, 2) + pow(yn2, 2));
            yn1 = 1 + u * (yn1 * cos(t) - yn2 * sin(t));
            yn2 = u * (yn1 * sin(t) + yn2 * cos(t));
            }
        *output_1++ = yn1;
        *output_2++ = yn2;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_yn1 = yn1;
    x->x_yn2 = yn2;
    return (w + 7);
}

static void ikeda_dsp(t_ikeda *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(ikeda_perform, 6, x, sp[0]->s_n, &x->x_val, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *ikeda_new(t_symbol *s, int ac, t_atom *av)
{
    t_ikeda *x = (t_ikeda *)pd_new(ikeda_class);
    x->x_sr = sys_getsr();
    t_float hz = x->x_sr * 0.5, init_u = 0.75, y1 = 0, y2 = 0; // default parameters
    if (ac && av->a_type == A_FLOAT){
        hz = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            init_u = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            y1 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            y2 = av->a_w.w_float;
    }
    if(hz >= 0) x->x_phase = 1;
    x->x_freq  = hz;
    x->x_u = init_u;
    x->x_yn1 = y1;
    x->x_yn2 = y2;
    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void ikeda_tilde_setup(void)
{
    ikeda_class = class_new(gensym("ikeda~"),
        (t_newmethod)ikeda_new, 0,
        sizeof(t_ikeda), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(ikeda_class, t_ikeda, x_freq);
    class_addlist(ikeda_class, ikeda_list);
    class_addmethod(ikeda_class, (t_method)ikeda_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(ikeda_class, (t_method)ikeda_u, gensym("u"), A_DEFFLOAT, 0);
    class_addmethod(ikeda_class, (t_method)ikeda_clear, gensym("clear"), 0);
}
