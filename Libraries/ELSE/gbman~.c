// Porres 2017

#include <math.h>
#include "m_pd.h"

static t_class *gbman_class;

typedef struct _gbman
{
    t_object  x_obj;
    int x_val;
    t_float  x_y_nm1;
    t_float  x_y_nm2;
    t_float  x_sr;
    double  x_phase;
    t_float  x_freq;
    t_outlet *x_outlet;
} t_gbman;


static void gbman_list(t_gbman *x, t_symbol *s, int argc, t_atom * argv)
{
    if (argc > 2)
        {
        pd_error(x, "gbman~: list size needs to be = 2");
        }
    else{
        int argnum = 0; // current argument
        while(argc)
        {
            if(argv -> a_type != A_FLOAT)
                {
                pd_error(x, "gbman~: list needs to only contain floats");
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



static t_int *gbman_perform(t_int *w)
{
    t_gbman *x = (t_gbman *)(w[1]);
    int nblock = (t_int)(w[2]);
    int *vp = (int *)(w[3]);
    t_float *in = (t_float *)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    int val = *vp;
    t_float y_nm1 = x->x_y_nm1;
    t_float y_nm2 = x->x_y_nm2;
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
            output = 1 + fabsf(y_nm1) - y_nm2;
            y_nm2 = y_nm1;
            y_nm1 = output;
            }
        else output = y_nm1; // last output
        *out++ = output;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_y_nm1 = y_nm1;
    x->x_y_nm2 = y_nm2;
    return (w + 6);
}

static void gbman_dsp(t_gbman *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(gbman_perform, 5, x, sp[0]->s_n, &x->x_val, sp[0]->s_vec, sp[1]->s_vec);
}

static void *gbman_free(t_gbman *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}


static void *gbman_new(t_symbol *s, int ac, t_atom *av)
{
    t_gbman *x = (t_gbman *)pd_new(gbman_class);
    x->x_sr = sys_getsr();
    t_float hz = x->x_sr * 0.5, y1 = 1.2, y2 = 2.1; // default parameters
    if (ac && av->a_type == A_FLOAT)
    {
        hz = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            y1 = av->a_w.w_float;
            ac--; av++;
            if (ac && av->a_type == A_FLOAT)
                y2 = av->a_w.w_float;
    }
    if(hz >= 0) x->x_phase = 1;
    x->x_freq  = hz;
    x->x_y_nm1 = y1;
    x->x_y_nm2 = y2;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void gbman_tilde_setup(void)
{
    gbman_class = class_new(gensym("gbman~"),
        (t_newmethod)gbman_new, (t_method)gbman_free,
        sizeof(t_gbman), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(gbman_class, t_gbman, x_freq);
    class_addlist(gbman_class, gbman_list);
    class_addmethod(gbman_class, (t_method)gbman_dsp, gensym("dsp"), A_CANT, 0);
}
