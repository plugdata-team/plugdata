// Porres 2017

#include "m_pd.h"
#include "random.h"

static t_class *stepnoise_class;

typedef struct _stepnoise{
    t_object        x_obj;
    t_float         x_freq;
    t_random_state  x_rstate;
    double          x_phase;
    t_float         x_val;
    float           x_sr;
    int             x_id;
}t_stepnoise;

static void stepnoise_seed(t_stepnoise *x, t_symbol *s, int ac, t_atom *av){
    x->x_phase = 0;
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    x->x_val = (t_float)(random_frand(s1, s2, s3));
}

static t_int *stepnoise_perform(t_int *w){
    t_stepnoise *x = (t_stepnoise *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *out = (t_sample *)(w[4]);
    double phase = x->x_phase;
    t_float random = x->x_val;
    double sr = x->x_sr;
    while(nblock--){
        uint32_t *s1 = &x->x_rstate.s1;
        uint32_t *s2 = &x->x_rstate.s2;
        uint32_t *s3 = &x->x_rstate.s3;
        float hz = *in1++;
        double phase_step = hz / sr;
// clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        if(hz >= 0){
            if(phase >= 1.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase - 1;
            }
        }
        else{
            if(phase <= 0.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase + 1;
            }
        }
        *out++ = random;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_val = random;
    return(w+5);
}

static void stepnoise_dsp(t_stepnoise *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(stepnoise_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *stepnoise_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_stepnoise *x = (t_stepnoise *)pd_new(stepnoise_class);
    x->x_id = random_get_id();
    stepnoise_seed(x, s, 0, NULL);
// default parameters
    t_float hz = 0;
    if(ac){
        if(av->a_type == A_SYMBOL){
            if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
                t_atom at[1];
                SETFLOAT(at, atom_getfloat(av+1));
                ac-=2, av+=2;
                stepnoise_seed(x, s, 1, at);
            }
            else
                goto errstate;
        }
        else if(av->a_type == A_FLOAT)
           hz = atom_getfloat(av);
    }
    if(hz >= 0)
        x->x_phase = 1.;
    x->x_freq = hz;
    outlet_new(&x->x_obj, &s_signal);
    return(x);
errstate:
    pd_error(x, "[stepnoise~]: improper args");
    return(NULL);
}

void stepnoise_tilde_setup(void){
    stepnoise_class = class_new(gensym("stepnoise~"), (t_newmethod)stepnoise_new,
        0, sizeof(t_stepnoise), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(stepnoise_class, t_stepnoise, x_freq);
    class_addmethod(stepnoise_class, (t_method)stepnoise_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(stepnoise_class, (t_method)stepnoise_seed, gensym("seed"), A_GIMME, 0);
}
