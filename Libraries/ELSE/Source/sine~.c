// Porres 2017

#include "m_pd.h"
#include "math.h"
#include "magic.h"

#define TWOPI (3.14159265358979323846 * 2)

static t_class *sine_class;

typedef struct _sine{
    t_object x_obj;
    double  x_phase;
    double  x_last_phase_offset;
    t_int midi;
    t_int soft;
    t_float  x_freq;
    t_inlet  *x_inlet_phase;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet;
    t_float x_sr;
// MAGIC:
    t_glist *x_glist; // object list
    t_float *x_signalscalar; // right inlet's float field
    int x_hasfeeders; // right inlet connection flag
    t_float  x_phase_sync_float; // float from magic
}t_sine;

static t_int *sine_perform(t_int *w){
    t_sine *x = (t_sine *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in3 = (t_float *)(w[5]); // phase
    t_float *out = (t_float *)(w[6]);
// Magic Start
    t_float *scalar = x->x_signalscalar;
    if(!else_magic_isnan(*x->x_signalscalar)){
        t_float input_phase = fmod(*scalar, 1);
        if(input_phase < 0)
            input_phase += 1;
        x->x_phase = input_phase;
        else_magic_setnan(x->x_signalscalar);
    }
// Magic End
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    while(nblock--){
        double hz = *in1++;
        if(x->midi)
            hz = pow(2, (hz - 69)/12) * 440;
        double phase_offset = *in3++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        double phase_dev = phase_offset - last_phase_offset;
        if(phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)
        phase = phase + phase_dev;
        if(phase <= 0)
            phase = phase + 1.; // wrap deviated phase
        if(phase >= 1)
            phase = phase - 1.; // wrap deviated phase
        *out++ = sin(phase * TWOPI);
        phase = phase + phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    return(w+7);
}

static t_int *sine_perform_sig(t_int *w){
    t_sine *x = (t_sine *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // sync
    t_float *in3 = (t_float *)(w[5]); // phase
    t_float *out = (t_float *)(w[6]);
    double phase = x->x_phase;
    double last_phase_offset = x->x_last_phase_offset;
    double sr = x->x_sr;
    while(nblock--){
        double hz = *in1++;
        if(x->midi)
            hz = pow(2, (hz - 69)/12) * 440;
        t_float trig = *in2++;
        double phase_offset = *in3++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        double phase_dev = phase_offset - last_phase_offset;
        if(phase_dev >= 1 || phase_dev <= -1)
            phase_dev = fmod(phase_dev, 1); // fmod(phase_dev)
        if(x->soft)
            phase_step *= (x->soft);
        if(trig > 0 && trig <= 1){
            if(x->soft)
                x->soft = x->soft == 1 ? -1 : 1;
            else
                phase = trig;
        }
        else{
            phase = phase + phase_dev;
            if(phase <= 0)
                phase = phase + 1.; // wrap deviated phase
            if(phase >= 1)
                phase = phase - 1.; // wrap deviated phase
        }
        *out++ = sin(phase * TWOPI);
        phase = phase + phase_step; // next phase
        last_phase_offset = phase_offset; // last phase offset
    }
    x->x_phase = phase;
    x->x_last_phase_offset = last_phase_offset;
    return(w+7);
}

static void sine_dsp(t_sine *x, t_signal **sp){
    x->x_hasfeeders = else_magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal); // magic feeder flag
    x->x_sr = sp[0]->s_sr;
    if(x->x_hasfeeders){
        dsp_add(sine_perform_sig, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
    }
    else
        dsp_add(sine_perform, 6, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void sine_midi(t_sine *x, t_floatarg f){
    x->midi = (int)(f != 0);
}

static void sine_soft(t_sine *x, t_floatarg f){
    x->soft = (int)(f != 0);
}

static void *sine_free(t_sine *x){
    inlet_free(x->x_inlet_sync);
    inlet_free(x->x_inlet_phase);
    outlet_free(x->x_outlet);
    return(void *)x;
}

static void *sine_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_sine *x = (t_sine *)pd_new(sine_class);
    t_float f1 = 0, f2 = 0;
    x->midi = x->soft = 0;
    while(ac && av->a_type == A_SYMBOL){
        if(atom_getsymbol(av) == gensym("-midi"))
            x->midi = 1;
        else if(atom_getsymbol(av) == gensym("-soft"))
            x->soft = 1;
        ac--, av++;
    }
    if(ac && av->a_type == A_FLOAT){
        f1 = av->a_w.w_float;
        ac--, av++;
        if(ac && av->a_type == A_FLOAT){
            f2 = av->a_w.w_float;
            ac--, av++;
        }
    }
    t_float init_freq = f1;
    t_float init_phase = f2;
    init_phase = init_phase  < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    if(init_phase == 0 && init_freq > 0)
        x->x_phase = 1.;
    else
        x->x_phase = init_phase;
    x->x_last_phase_offset = 0;
    x->x_freq = init_freq;
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_sync, 0);
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_phase, init_phase);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
// Magic
    x->x_glist = canvas_getcurrent();
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 1);
    return(x);
}

void sine_tilde_setup(void){
    sine_class = class_new(gensym("sine~"), (t_newmethod)sine_new, (t_method)sine_free,
        sizeof(t_sine), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(sine_class, t_sine, x_freq);
    class_addmethod(sine_class, (t_method)sine_soft, gensym("soft"), A_DEFFLOAT, 0);
    class_addmethod(sine_class, (t_method)sine_midi, gensym("midi"), A_DEFFLOAT, 0);
    class_addmethod(sine_class, (t_method)sine_dsp, gensym("dsp"), A_CANT, 0);
}
