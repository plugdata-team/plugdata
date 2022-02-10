// Porres 2017

#include "m_pd.h"
#include "math.h"
#include "magic.h"

#define TWOPI (3.14159265358979323846 * 2)

static t_class *fbsine_class;

typedef struct _fbsine
{
    t_object x_obj;
    double  x_phase;
    float  x_yn_m1;
    float  x_yn_m2;
    float  x_last_phase_offset;
    t_float  x_freq;
    t_int    x_filter;
    t_inlet  *x_inlet_fb;
    t_inlet  *x_inlet_phase;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet;
    t_float x_sr;
// MAGIC:
    t_glist *x_glist; // object list
    t_float *x_signalscalar; // right inlet's float field
    int x_hasfeeders; // right inlet connection flag
    t_float  x_phase_sync_float; // float from magic
} t_fbsine;

static void fbsine_filter(t_fbsine *x, t_floatarg f)
{
    x->x_filter = f != 0;
}

static t_int *fbsine_perform_magic(t_int *w)
{
    t_fbsine *x = (t_fbsine *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // fb
    t_float *in3 = (t_float *)(w[5]); // sync
    t_float *in4 = (t_float *)(w[6]); // phase
    t_float *out = (t_float *)(w[7]);
// Magic Start
    t_float *scalar = x->x_signalscalar;
    if (!magic_isnan(*x->x_signalscalar)){
        t_float input_phase = fmod(*scalar, 1);
        if (input_phase < 0)
            input_phase += 1;
        x->x_phase = input_phase;
        magic_setnan(x->x_signalscalar);
    }
// Magic End
    float yn_m1 = x->x_yn_m1;
    float yn_m2 = x->x_yn_m2;
    double phase = x->x_phase;
    float last_phase_offset = x->x_last_phase_offset;
    float sr = x->x_sr;
    while (nblock--){
        float hz = *in1++;
        float index = *in2++;
        float phase_offset = *in4++;
        float fback;
        float output;
        float phase_dev = phase_offset - last_phase_offset;
        phase += (double)phase_dev;
        if(x->x_filter)
            fback = ((yn_m1 + yn_m2) * 0.5) * index; // filter
        else
            fback = yn_m1 * index; // no filter
        float radians = (phase + fback) * TWOPI;
        output = sin(radians);
        *out++ = output;
        phase += (double)(hz / sr); // next phase
        last_phase_offset = phase_offset; // last phase offset
        yn_m2 = yn_m1;
        yn_m1 = output;
    }
    x->x_yn_m1 = yn_m1; // last out
    x->x_yn_m2 = yn_m2; // last out
    x->x_phase = fmod(phase, 1); // next wrapped phase
    x->x_last_phase_offset = last_phase_offset;
    return (w + 8);
}

static t_int *fbsine_perform(t_int *w){
    t_fbsine *x = (t_fbsine *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // fb
    t_float *in3 = (t_float *)(w[5]); // sync
    t_float *in4 = (t_float *)(w[6]); // phase
    t_float *out = (t_float *)(w[7]);
    float yn_m1 = x->x_yn_m1;
    float yn_m2 = x->x_yn_m2;
    double phase = x->x_phase;
    float last_phase_offset = x->x_last_phase_offset;
    float sr = x->x_sr;
    while (nblock--){
        float hz = *in1++;
        float index = *in2++;
        float trig = *in3++;
        float phase_offset = *in4++;
        float fback;
        float output;
        float phase_dev = phase_offset - last_phase_offset;
        phase += (double)phase_dev;
        if (trig > 0 && trig <= 1)
            phase = trig;
        if(x->x_filter)
            fback = ((yn_m1 + yn_m2) * 0.5) * index; // filter
        else
            fback = yn_m1 * index; // no filter
        float radians = (phase + fback) * TWOPI;
        output = sin(radians);
        *out++ = output;
        phase += (double)(hz / sr); // next phase
        last_phase_offset = phase_offset; // last phase offset
        yn_m2 = yn_m1;
        yn_m1 = output;
    }
    x->x_yn_m1 = yn_m1; // last out
    x->x_yn_m2 = yn_m2; // last out
    x->x_phase = fmod(phase, 1); // next wrapped phase
    x->x_last_phase_offset = last_phase_offset;
    return (w + 8);
}

static void fbsine_dsp(t_fbsine *x, t_signal **sp){
    x->x_hasfeeders = magic_inlet_connection((t_object *)x, x->x_glist, 2, &s_signal); // magic feeder flag
    x->x_sr = sp[0]->s_sr;
    if (x->x_hasfeeders){
        dsp_add(fbsine_perform, 7, x, sp[0]->s_n,
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
    else{
        dsp_add(fbsine_perform_magic, 7, x, sp[0]->s_n,
                sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
    }
}


static void *fbsine_free(t_fbsine *x)
{
    inlet_free(x->x_inlet_fb);
    inlet_free(x->x_inlet_phase);
    inlet_free(x->x_inlet_sync);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *fbsine_new(t_symbol *s, int ac, t_atom *av)
{
    t_fbsine *x = (t_fbsine *)pd_new(fbsine_class);
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
    t_float init_freq = f1;
    t_float init_fb = f2;
    t_float init_phase = f3;
    x->x_filter = f4 != 0;
    init_phase < 0 ? 0 : init_phase >= 1 ? 0 : init_phase; // clipping phase input
    x->x_last_phase_offset = 0;
    x->x_freq = init_freq;
    x->x_inlet_fb = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_fb, init_fb);
    x->x_inlet_phase = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_phase, init_phase);
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_sync, 0);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
// Magic
    x->x_glist = canvas_getcurrent();
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 2);
    return (x);
}

void fbsine_tilde_setup(void)
{
    fbsine_class = class_new(gensym("fbsine~"),
        (t_newmethod)fbsine_new, (t_method)fbsine_free,
        sizeof(t_fbsine), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(fbsine_class, t_fbsine, x_freq);
    class_addmethod(fbsine_class, (t_method)fbsine_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(fbsine_class, (t_method)fbsine_filter, gensym("filter"), A_FLOAT, 0);
}
