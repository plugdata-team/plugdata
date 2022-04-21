// Porres 2016

#include "m_pd.h"
#include "math.h"
#include "magic.h"

static t_class *pimp_class;

typedef struct _pimp
{
    t_object x_obj;
    double  x_phase;
    double  x_last_phase_offset;
    t_float  x_freq;
    t_inlet  *x_inlet_phase;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet_dsp_0;
    t_outlet *x_outlet_dsp_1;
    t_float x_sr;
// MAGIC:
    int x_posfreq; // positive frequency flag
    t_glist *x_glist; // object list
    t_float *x_signalscalar; // right inlet's float field
    int x_hasfeeders; // right inlet connection flag
    t_float  x_phase_sync_float; // float from magic
} t_pimp;


static t_int *pimp_perform_magic(t_int *w){
    t_pimp *x = (t_pimp *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // sync
    t_float *in3 = (t_float *)(w[5]); // phase
    t_float *out1 = (t_float *)(w[6]);
    t_float *out2 = (t_float *)(w[7]);
    int posfreq = x->x_posfreq;
// Magic Start
    t_float *scalar = x->x_signalscalar;
    if (!magic_isnan(*x->x_signalscalar)) {
        t_float input_phase = fmod(*scalar, 1);
        if (input_phase < 0)
            input_phase += 1;
        if(input_phase == 0 && x->x_posfreq)
            input_phase = 1;
        x->x_phase = input_phase;
    magic_setnan(x->x_signalscalar);
    }
// Magic End 
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    while (nblock--){
        double hz = *in1++;
        posfreq = hz >= 0;
        double phase_offset = *in3++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step; // clipped phase_step
        double phase_dev = phase_offset - last_phase_offset;
        if (phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)
        if (hz >= 0){
            phase = phase + phase_dev;
            if (phase_dev != 0 && phase <= 0)
                phase = phase + 1.; // wrap deviated phase
            *out2++ = phase >= 1.;
            if (phase >= 1.)
                phase = phase - 1; // wrapped phase
        }
        else{
            phase = phase + phase_dev;
            if (phase >= 1)
                phase = phase - 1.; // wrap deviated phase
            *out2++ = phase <= 0.;
            if (phase <= 0.)
                phase = phase + 1.; // wrapped phase
        }
        *out1++ = phase; // wrapped phase
        phase += phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
    }
    x->x_posfreq = posfreq;
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    return (w + 8);
}


static t_int *pimp_perform(t_int *w){
    t_pimp *x = (t_pimp *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // sync
    t_float *in3 = (t_float *)(w[5]); // phase
    t_float *out1 = (t_float *)(w[6]);
    t_float *out2 = (t_float *)(w[7]);
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    while (nblock--){
        double hz = *in1++;
        double trig = *in2++;
        double phase_offset = *in3++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step; // clipped phase_step
        double phase_dev = phase_offset - last_phase_offset;
        if (phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)
        if (hz >= 0){
            if (trig > 0 && trig <= 1)
                phase = trig;
            else{
                phase = phase + phase_dev;
                if (phase_dev != 0 && phase <= 0)
                    phase = phase + 1.; // wrap deviated phase
            }
            *out2++ = phase >= 1.;
            if (phase >= 1.)
                phase = phase - 1; // wrapped phase
        }
        else{
            if (trig > 0 && trig < 1)
                phase = trig;
            else if (trig == 1)
                phase = 0;
            else{
                phase = phase + phase_dev;
                if (phase >= 1)
                    phase = phase - 1.; // wrap deviated phase
            }
            *out2++ = phase <= 0.;
            if (phase <= 0.)
                phase = phase + 1.; // wrapped phase
        }
        *out1++ = phase; // wrapped phase
        phase += phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    return (w + 8);
}

static void pimp_dsp(t_pimp *x, t_signal **sp){
    x->x_hasfeeders = magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal); // magic feeder flag
    x->x_sr = sp[0]->s_sr;
    if (x->x_hasfeeders){
        dsp_add(pimp_perform, 7, x, sp[0]->s_n,
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
    else{
        dsp_add(pimp_perform_magic, 7, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
}

static void *pimp_free(t_pimp *x)
{
    inlet_free(x->x_inlet_sync);
    inlet_free(x->x_inlet_phase);
    outlet_free(x->x_outlet_dsp_0);
    outlet_free(x->x_outlet_dsp_1);
    return (void *)x;
}

static void *pimp_new(t_floatarg f1, t_floatarg f2){
    t_pimp *x = (t_pimp *)pd_new(pimp_class);
    t_float init_freq = f1;
    t_float init_phase = f2;
    init_phase < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    if (init_freq >= 0)
        x->x_posfreq = 1;
    if (init_phase == 0 && x->x_posfreq)
        x->x_phase = 1.;
    x->x_last_phase_offset = 0;
    x->x_freq = init_freq;
    x->x_sr = sys_getsr(); // sample rate
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_sync, 0);
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_phase, init_phase);
    x->x_outlet_dsp_0 = outlet_new(&x->x_obj, &s_signal);
// Magic
    x->x_glist = canvas_getcurrent();
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 1);
    x->x_outlet_dsp_1 = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void pimp_tilde_setup(void){
    pimp_class = class_new(gensym("pimp~"),
        (t_newmethod)pimp_new, (t_method)pimp_free,
        sizeof(t_pimp), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(pimp_class, t_pimp, x_freq);
    class_addmethod(pimp_class, (t_method)pimp_dsp, gensym("dsp"), A_CANT, 0);
}
