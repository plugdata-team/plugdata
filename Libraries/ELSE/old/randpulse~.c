// Porres 2016

#include "m_pd.h"
#include "math.h"


static t_class *randpulse_class;

typedef struct _randpulse
{
    t_object x_obj;
    int        x_val;
    double  x_phase;
    
    t_float  x_random;
    t_float  x_freq;
    t_inlet  *x_inlet_width;
    t_inlet  *x_inlet_sync;
    t_outlet *x_outlet;
    t_float x_sr;
} t_randpulse;


static void randpulse_seed(t_randpulse *x, t_floatarg f){
    int val = (int)f * 1319;
    val = val * 435898247 + 382842987;
    x->x_val = val;
}

static t_int *randpulse_perform(t_int *w)
{
    t_randpulse *x = (t_randpulse *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // width
    t_float *in3 = (t_float *)(w[5]); // sync
    t_float *out = (t_float *)(w[6]);
    double phase = x->x_phase;
    float random = x->x_random;
    int val = x->x_val;
    double sr = x->x_sr;
    double output;
    while (nblock--){
        double hz = *in1++;
        double width = *in2++;
        width = width > 1. ? 1. : width < 0. ? 0. : width; // clipped
        double sync = *in3++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        
        if (hz >= 0){
            if (sync == 1)
                phase = 1;
            if (phase >= 1.){  // update
                random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
                val = val * 435898247 + 382842987;
                phase = phase - 1;
                output = random; // first is always random
            }
            else if (phase + phase_step >= 1)
                output = 0; // last sample is always 0
            else
                output = phase <= width ? random : 0;
        }
        else{
            if (sync == 1)
                phase = 0;
            if (phase <= 0.){ // update
                random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
                val = val * 435898247 + 382842987;
                phase = phase + 1;
                output = 0;
            }
            else if (phase + phase_step <= 0)
                output = random; // last sample is always 1
            else
                output = phase <= width ? random : 0;
        }
        *out++ = output;
        phase += phase_step; // next phase
    }
    x->x_phase = phase;
    x->x_random = random;
    x->x_val = val;
    return (w + 7);
}

static void randpulse_dsp(t_randpulse *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(randpulse_perform, 6, x, sp[0]->s_n,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *randpulse_free(t_randpulse *x)
{
    inlet_free(x->x_inlet_width);
    inlet_free(x->x_inlet_sync);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *randpulse_new(t_symbol *s, int ac, t_atom *av)
{
    t_randpulse *x = (t_randpulse *)pd_new(randpulse_class);
    static int init_seed = 234599;
    init_seed *= 1319;
    t_float f1 = 0, f2 = 0.5, f3 = init_seed;
    if (ac && av->a_type == A_FLOAT){
        f1 = av->a_w.w_float;
        ac--; av++;
        if (ac && av->a_type == A_FLOAT)
            f2 = av->a_w.w_float;
            ac--; av++;
            if (ac && av->a_type == A_FLOAT)
                f3 = av->a_w.w_float;
                ac--; av++;
    }
    t_float init_freq = f1;
    t_float init_width = f2;
    
    init_width < 0 ? 0 : init_width >= 1 ? 0 : init_width; // clipping width input
   
    if (init_freq > 0)
        x->x_phase = 1.;

    t_int seed = f3;
    x->x_val = seed * 435898247 + 382842987;
    
    x->x_freq = init_freq;
    x->x_sr = sys_getsr(); // sample rate

    x->x_inlet_width = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_width, init_width);
    
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
    
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void randpulse_tilde_setup(void){
    randpulse_class = class_new(gensym("randpulse~"),
        (t_newmethod)randpulse_new, (t_method)randpulse_free,
        sizeof(t_randpulse), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(randpulse_class, t_randpulse, x_freq);
    class_addmethod(randpulse_class, (t_method)randpulse_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(randpulse_class, (t_method)randpulse_seed, gensym("seed"), A_DEFFLOAT, 0);
}
