// Porres 2017

#include <math.h>
#include "m_pd.h"

static t_class *henon_class;

typedef struct _henon
{
    t_object  x_obj;
    int x_val;
    t_float x_sr;
    double  x_a;
    double  x_b;
    double  x_y_nm1;
    double  x_y_nm2;
    double  x_init_y1;
    double  x_init_y2;
    double  x_phase;
    t_float  x_freq;
    t_outlet *x_outlet;
} t_henon;


static void henon_coeffs(t_henon *x, t_floatarg f1, t_floatarg f2)
{
    x->x_a = f1;
    x->x_b = f2;
}

static void henon_list(t_henon *x, t_symbol *s, int argc, t_atom * argv)
{
    if (argc != 2)
        {
        pd_error(x, "henon~: list size needs to be = 2");
        }
    else{
        int argnum = 0; // current argument
        while(argc)
        {
            if(argv -> a_type != A_FLOAT)
                {
                pd_error(x, "henon~: list needs to only contain floats");
                }
            else
                {
                t_float curf = atom_getfloatarg(0, argc, argv);
                switch(argnum)
                    {
                    case 0:
                        x->x_y_nm1 = curf;
                        break;
                    case 1:
                        x->x_y_nm2 = curf;
                        break;
                    };
                argnum++;
                };
            argc--;
            argv++;
        };
    }
}


static t_int *henon_perform(t_int *w)
{
    t_henon *x = (t_henon *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    double y_nm1 = x->x_y_nm1;
    double y_nm2 = x->x_y_nm2;
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
            output = 1 - (a * y_nm1*y_nm1) + (b * y_nm2);
            if((output > 1.5) || (output < -1.5))
                {
                output = 0;
                y_nm2 = 0; // x->x_init_y2;
                y_nm1 = 0; // x->x_init_y1;
                }
            else if((output < 1.5) && (output > -1.5))
                {
                y_nm2 = y_nm1;
                y_nm1 = output;
                }
            }
        *out++ = y_nm2;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_y_nm1 = y_nm1;
    x->x_y_nm2 = y_nm2;
    return (w + 5);
}


static void henon_dsp(t_henon *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(henon_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *henon_free(t_henon *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *henon_new(t_symbol *s, int ac, t_atom *av)
{
    t_henon *x = (t_henon *)pd_new(henon_class);
    x->x_sr = sys_getsr();
    t_float hz = x->x_sr * 0.5, a = 1.4, b = 0.3, y_nm1 = 0, y_nm2 = 0; // default parameters
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
                    y_nm1 = av->a_w.w_float;
                    ac--; av++;
                    if (ac && av->a_type == A_FLOAT)
                        y_nm2 = av->a_w.w_float;
    }
    if(hz >= 0) x->x_phase = 1;
    x->x_freq  = hz;
    x->x_a = a;
    x->x_b = b;
    x->x_y_nm1 = x->x_init_y1 = y_nm1;
    x->x_y_nm2 = x->x_init_y2 = y_nm2;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void henon_tilde_setup(void)
{
    henon_class = class_new(gensym("henon~"),
        (t_newmethod)henon_new, (t_method)henon_free,
        sizeof(t_henon), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(henon_class, t_henon, x_freq);
    class_addlist(henon_class, henon_list);
    class_addmethod(henon_class, (t_method)henon_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(henon_class, (t_method)henon_coeffs, gensym("coeffs"), A_DEFFLOAT, A_DEFFLOAT, 0);
}
