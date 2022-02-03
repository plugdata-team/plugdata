// Porres 2016

#include "m_pd.h"
#include <common/api.h>
#include <math.h>

#define SR_DEF 1.f
#define BIT_DEF 24.f

static t_class *degrade_class;


typedef struct _degrade
{
    t_object  x_obj;
    t_float   x_phase;
    t_float   x_lastout;
    t_float   sr_val;
    t_float   bit_val;
    t_inlet  *x_ratio_inlet;
    t_inlet  *x_bitdepth_inlet;
    t_outlet *x_outlet;
} t_degrade;


static t_int *degrade_perform(t_int *w)
{
    t_degrade *x = (t_degrade *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float phase = x->x_phase;
    t_float lastout = x->x_lastout;
    while (nblock--)
    {
        float input = *in1++;
        float bit = *in3++;
        float steps = pow(2, (bit > 24. ? 24. : bit < 1. ? 1. : bit)) / 2;
        if (input > 0) input = ((int)(input * steps)) / steps;
        else input = (((int)(input * steps)) / steps) - (1 / steps);
        float phase_step = *in2++;
        if (phase_step > 1.) phase_step = 1.;
        if (phase_step < 0.) phase_step = 0.;
        if (phase >= 1.) lastout = input;
        *out++ = lastout;
        phase = fmod(phase, 1.) + phase_step;
    }
    x->x_phase = phase;
    x->x_lastout = lastout;
    return (w + 7);
}


static void degrade_dsp(t_degrade *x, t_signal **sp)
{
    dsp_add(degrade_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *degrade_free(t_degrade *x){
    inlet_free(x->x_ratio_inlet);
    inlet_free(x->x_bitdepth_inlet);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *degrade_new(t_symbol *s, int argc, t_atom *argv){ //two optional args: sr, bits
    t_degrade *x = (t_degrade *)pd_new(degrade_class);
    x->sr_val = SR_DEF;
    x->bit_val = BIT_DEF;
    switch(argc){
        default:
        case 2:
            x->bit_val = atom_getfloat(argv+1);
            (x->bit_val > 24. ? 24. : (x->bit_val < 1. ? 1. : x->bit_val));
        case 1:
            x->sr_val = atom_getfloat(argv);
            (x->sr_val > 1. ? 1. : (x->sr_val < 0. ? 0. : x->sr_val));
            break;
        case 0:
            break;
    }
    x->x_ratio_inlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    x->x_bitdepth_inlet =  inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    pd_float( (t_pd *) x->x_ratio_inlet, x->sr_val);
    pd_float( (t_pd *) x->x_bitdepth_inlet, x->bit_val);
    x->x_outlet =  outlet_new(&x->x_obj, gensym("signal"));
    x->x_lastout = 0;
    x->x_phase = 0;
    return (x);
}

CYCLONE_OBJ_API void degrade_tilde_setup(void)
{
    degrade_class = class_new(gensym("degrade~"), (t_newmethod)degrade_new, (t_method)degrade_free,
            sizeof(t_degrade), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(degrade_class, nullfn, gensym("signal"), 0);
    class_addmethod(degrade_class, (t_method)degrade_dsp, gensym("dsp"), A_CANT, 0);
}