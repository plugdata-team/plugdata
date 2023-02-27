// Porres 2016

#include "m_pd.h"
#include <math.h>
#include "magic.h"

static t_class *imp2_class;

typedef struct _imp2
{
    t_object x_obj;
    double  x_phase;
    double  x_last_phase_offset;
    t_float  x_last_output;
    t_float  x_freq;
    t_inlet  *x_inlet_width;
    t_inlet  *x_inlet_phase;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet;
    t_float x_sr;
// MAGIC:
    t_glist *x_glist; // object list
    t_float *x_signalscalar; // right inlet's float field
    int x_hasfeeders; // right inlet connection flag
    t_float  x_phase_sync_float; // float from magic
} t_imp2;

static t_int *imp2_perform_magic(t_int *w){
    t_imp2 *x = (t_imp2 *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // width
    t_float *in3 = (t_float *)(w[5]); // sync
    t_float *in4 = (t_float *)(w[6]); // phase
    t_float *out = (t_float *)(w[7]);
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    double last_output  = x->x_last_output;
    double output;
    while (nblock--){
        double hz = *in1++;
        double width = *in2++;
        width = width > 1. ? 1. : width < 0. ? 0. : width; // clipped
        double trig = *in3++;
        double phase_offset = *in4++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        double phase_dev = phase_offset - last_phase_offset;
        if (phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)
        if (hz >= 0){
            if (trig > 0 && trig <= 1)
                phase = trig;
            else{
                phase = phase + phase_dev;
                if (phase <= 0)
                    phase += 1.; // wrap deviated phase
            }
            if (phase >= 1){
                output = 1; // 1st sample is always 1
                phase -= 1; // wrapped phase
            }
            else if (phase + phase_step >= 1)
                output = -1; // last sample is always -1
            else
                output = phase <= width ? 1 : -1;
        }
        else{
            if (trig > 0 && trig < 1)
                phase = trig;
            else if (trig == 1)
                phase = 0;
            else{
                phase += phase_dev;
                if (phase >= 1)
                    phase -= 1.; // wrap deviated phase
            }
            if (phase <= 0){
                output = -1; // 1st sample is always -1
                phase += 1; // wrapped phase
            }
            else if (phase + phase_step <= 0)
                output = 1; // last sample is always 1
            else
                output = phase <= width ? 1 : -1;
        }
        *out++ = (output > last_output ? 1. : (output < last_output ? -1. : 0.));
        phase += phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
        last_output = output; // last output
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    x->x_last_output = last_output;
    return (w + 8);
}

static t_int *imp2_perform(t_int *w){
    t_imp2 *x = (t_imp2 *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // width
    t_float *in3 = (t_float *)(w[5]); // sync
    t_float *in4 = (t_float *)(w[6]); // phase
    t_float *out = (t_float *)(w[7]);
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    double last_output  = x->x_last_output;
    double output;
    while (nblock--){
        double hz = *in1++;
        double width = *in2++;
        width = width > 1. ? 1. : width < 0. ? 0. : width; // clipped
        double trig = *in3++;
        double phase_offset = *in4++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        double phase_dev = phase_offset - last_phase_offset;
        if (phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)
        if (hz >= 0){
            if (trig > 0 && trig <= 1)
                phase = trig;
            else{
                phase = phase + phase_dev;
                if (phase <= 0)
                    phase += 1.; // wrap deviated phase
            }
            if (phase >= 1){
                output = 1; // 1st sample is always 1
                phase -= 1; // wrapped phase
            }
            else if (phase + phase_step >= 1)
                output = -1; // last sample is always -1
            else
                output = phase <= width ? 1 : -1;
        }
        else{
            if (trig > 0 && trig < 1)
                phase = trig;
            else if (trig == 1)
                phase = 0;
            else{
                phase += phase_dev;
                if (phase >= 1)
                    phase -= 1.; // wrap deviated phase
            }
            if (phase <= 0){
                output = -1; // 1st sample is always -1
                phase += 1; // wrapped phase
            }
            else if (phase + phase_step <= 0)
                output = 1; // last sample is always 1
            else
                output = phase <= width ? 1 : -1;
            }
        *out++ = (output > last_output ? 1. : (output < last_output ? -1. : 0.));
        phase += phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
        last_output = output; // last output
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    x->x_last_output = last_output;
    return (w + 8);
}


static void imp2_dsp(t_imp2 *x, t_signal **sp){
    x->x_hasfeeders = else_magic_inlet_connection((t_object *)x, x->x_glist, 2, &s_signal); // magic feeder flag
    x->x_sr = sp[0]->s_sr;
    if (x->x_hasfeeders){
        dsp_add(imp2_perform, 7, x, sp[0]->s_n,
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
    else{
        dsp_add(imp2_perform, 7, x, sp[0]->s_n,
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
}

static void *imp2_free(t_imp2 *x)
{
    inlet_free(x->x_inlet_width);
    inlet_free(x->x_inlet_phase);
    inlet_free(x->x_inlet_sync);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *imp2_new(t_symbol *s, int ac, t_atom *av)
{
    t_imp2 *x = (t_imp2 *)pd_new(imp2_class);
    t_float f1 = 0, f2 = 0.5, f3 = 0;
    if (ac && av->a_type == A_FLOAT)
    {
        f1 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            f2 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            f3 = av->a_w.w_float;
    }
    
    t_float init_freq = f1;
    
    t_float init_width = f2;
    init_width < 0 ? 0 : init_width >= 1 ? 0 : init_width; // clipping width input
    
    t_float init_phase = f3;
    init_phase < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    if (init_phase == 0 && init_freq > 0)
        x->x_phase = 1.;
    
    x->x_last_phase_offset = 0;
    x->x_freq = init_freq;
    x->x_sr = sys_getsr(); // sample rate

    x->x_inlet_width = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_width, init_width);
    
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
    
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_phase, init_phase);
    
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    
// Magic
    x->x_glist = canvas_getcurrent();
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 2);
    
    return (x);
}

void imp2_tilde_setup(void)
{
    imp2_class = class_new(gensym("imp2~"),
        (t_newmethod)imp2_new, (t_method)imp2_free,
        sizeof(t_imp2), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(imp2_class, t_imp2, x_freq);
    class_addmethod(imp2_class, (t_method)imp2_dsp, gensym("dsp"), A_CANT, 0);
    class_sethelpsymbol(imp2_class, gensym("impulse2~"));
}
