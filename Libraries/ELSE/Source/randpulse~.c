// Porres 2016

#include "m_pd.h"
#include "math.h"
#include "random.h"

static t_class *randpulse_class;

typedef struct _randpulse{
    t_object        x_obj;
    double          x_phase;
    t_random_state  x_rstate;
    t_float         x_random;
    t_float         x_freq;
    t_inlet        *x_inlet_width;
    t_inlet        *x_inlet_sync;
    t_float         x_sr;
    int             x_id;
}t_randpulse;

static void randpulse_seed(t_randpulse *x, t_symbol *s, int ac, t_atom *av){
    x->x_phase = x->x_freq >= 0 ? 1.0 : 0.0;
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static t_int *randpulse_perform(t_int *w){
    t_randpulse *x = (t_randpulse *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq
    t_float *in2 = (t_float *)(w[4]); // width
    t_float *in3 = (t_float *)(w[5]); // sync
    t_float *out = (t_float *)(w[6]);
    double phase = x->x_phase;
    float random = x->x_random;
    double sr = x->x_sr;
    double output;
    while(nblock--){
        uint32_t *s1 = &x->x_rstate.s1;
        uint32_t *s2 = &x->x_rstate.s2;
        uint32_t *s3 = &x->x_rstate.s3;
        double hz = *in1++;
        double width = *in2++;
        width = width > 1. ? 1. : width < 0. ? 0. : width; // clipped
        double sync = *in3++;
        double phase_step = hz / sr; // phase_step
        phase_step = phase_step > 0.5 ? 0.5 : phase_step < -0.5 ? -0.5 : phase_step; // clipped to nyq
        if(hz >= 0){
            if(sync == 1)
                phase = 1;
            if(phase >= 1.){  // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase - 1;
                output = random; // first is always random
            }
            else if (phase + phase_step >= 1)
                output = 0; // last sample is always 0
            else
                output = phase <= width ? random : 0;
        }
        else{
            if(sync == 1)
                phase = 0;
            if(phase <= 0.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase + 1;
                output = 0;
            }
            else if(phase + phase_step <= 0)
                output = random; // last sample is always 1
            else
                output = phase <= width ? random : 0;
        }
        *out++ = output;
        phase += phase_step; // next phase
    }
    x->x_phase = phase;
    x->x_random = random;
    return(w+7);
}

static void randpulse_dsp(t_randpulse *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(randpulse_perform, 6, x, sp[0]->s_n,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *randpulse_free(t_randpulse *x){
    inlet_free(x->x_inlet_width);
    inlet_free(x->x_inlet_sync);
    return(void *)x;
}

static void *randpulse_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_randpulse *x = (t_randpulse *)pd_new(randpulse_class);
    x->x_id = random_get_id();
    randpulse_seed(x, s, 0, NULL);
    t_float init_freq = 0, init_width = 0.5;//, f3 = init_seed;
    if(av->a_type == A_SYMBOL){
        if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
            t_atom at[1];
            SETFLOAT(at, atom_getfloat(av+1));
            ac-=2, av+=2;
            randpulse_seed(x, s, 1, at);
        }
    }
    if(ac && av->a_type == A_FLOAT){
        init_freq = av->a_w.w_float;
        ac--; av++;
        if(ac && av->a_type == A_FLOAT){
            init_width = av->a_w.w_float;
            ac--; av++;
        }
    }
    init_width = init_width < 0 ? 0 : init_width >= 1 ? 0 : init_width; // clipping width input
    if(init_freq > 0)
        x->x_phase = 1.;
    x->x_freq = init_freq;
    x->x_sr = sys_getsr(); // sample rate
    x->x_inlet_width = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_width, init_width);
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_sync, 0);
    outlet_new(&x->x_obj, &s_signal);
    return(x);
}

void randpulse_tilde_setup(void){
    randpulse_class = class_new(gensym("randpulse~"), (t_newmethod)randpulse_new,
        (t_method)randpulse_free, sizeof(t_randpulse), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(randpulse_class, t_randpulse, x_freq);
    class_addmethod(randpulse_class, (t_method)randpulse_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(randpulse_class, (t_method)randpulse_seed, gensym("seed"), A_GIMME, 0);
}
