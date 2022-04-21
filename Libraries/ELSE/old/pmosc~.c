// Porres 2017

#include "m_pd.h"
#include "math.h"

#define TWOPI (3.14159265358979323846 * 2)

static t_class *pmosc_class;

typedef struct _pmosc
{
    t_object x_obj;
    double  x_phase1;
    double  x_phase2;
    t_float  x_carrier;
    t_inlet  *x_inlet_mod;
    t_inlet  *x_inlet_index;
    t_inlet  *x_inlet_phase;
    t_outlet *x_outlet;
    t_float x_sr;
} t_pmosc;

static t_int *pmosc_perform(t_int *w)
{
    t_pmosc *x = (t_pmosc *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // carrier
    t_float *in2 = (t_float *)(w[4]); // mod
    t_float *in3 = (t_float *)(w[5]); // index
    t_float *in4 = (t_float *)(w[6]); // phase
    t_float *out = (t_float *)(w[7]);
    double phase1 = x->x_phase1; // carrier
    double phase2 = x->x_phase2; // modulator
    float sr = x->x_sr;
    while (nblock--){
        float carrier = *in1++;
        float mod = *in2++;
        float index = *in3++;
        float phase_offset = *in4++;
        float modulator = sin((phase2 + phase_offset) * TWOPI) * index;
        *out++ = sin((phase1 + modulator) * TWOPI);
        phase1 += (double)(carrier / sr); // next phase
        phase2 += (double)(mod / sr); // next phase
    }
    x->x_phase1 = fmod(phase1, 1); // next wrapped phase
    x->x_phase2 = fmod(phase2, 1); // next wrapped phase
    return (w + 8);
}

static void pmosc_dsp(t_pmosc *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(pmosc_perform, 7, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void *pmosc_free(t_pmosc *x)
{
    inlet_free(x->x_inlet_mod);
    inlet_free(x->x_inlet_index);
    inlet_free(x->x_inlet_phase);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *pmosc_new(t_symbol *s, int ac, t_atom *av)
{
    t_pmosc *x = (t_pmosc *)pd_new(pmosc_class);
    t_float f1 = 0, f2 = 0, f3 = 0, f4 = 0;
    if (ac && av->a_type == A_FLOAT){
        f1 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT){
            f2 = av->a_w.w_float;
            ac--; av++;
            if (ac && av->a_type == A_FLOAT){
                f3 = av->a_w.w_float;
                ac--; av++;
                if (ac && av->a_type == A_FLOAT){
                    f4 = av->a_w.w_float;
                    ac--; av++;
                }
            }
        }
    }
    x->x_carrier = f1;
    t_float init_mod = f2;
    t_float init_index = f3;
    t_float init_phase = f4;
    init_phase < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    x->x_inlet_mod = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_mod, init_mod);
    x->x_inlet_index = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_index, init_index);
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_phase, init_phase);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void pmosc_tilde_setup(void)
{
    pmosc_class = class_new(gensym("pmosc~"),
        (t_newmethod)pmosc_new, (t_method)pmosc_free,
        sizeof(t_pmosc), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(pmosc_class, t_pmosc, x_carrier);
    class_addmethod(pmosc_class, (t_method)pmosc_dsp, gensym("dsp"), A_CANT, 0);
}
