// Porres 2017

#include "m_pd.h"
#include "random.h"

static t_class *rampnoise_class;

typedef struct _rampnoise{
    t_object        x_obj;
    t_float         x_freq;
    t_random_state  x_rstate;
    double          x_phase;
    t_float         x_ynp1;
    t_float         x_yn;
    float           x_sr;
}t_rampnoise;

static unsigned int instanc_n = 0;

static void rampnoise_seed(t_rampnoise *x, t_symbol *s, int ac, t_atom *av){
    x->x_phase = 0;
    random_init(&x->x_rstate, get_seed(s, ac, av, ++instanc_n));
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    x->x_yn = (t_float)(random_frand(s1, s2, s3));
    x->x_ynp1 = (t_float)(random_frand(s1, s2, s3));
}

static t_int *rampnoise_perform(t_int *w){
    t_rampnoise *x = (t_rampnoise *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *out = (t_sample *)(w[4]);
    double phase = x->x_phase;
    t_float ynp1 = x->x_ynp1;
    t_float yn = x->x_yn;
    double sr = x->x_sr;
    while(nblock--){
        uint32_t *s1 = &x->x_rstate.s1;
        uint32_t *s2 = &x->x_rstate.s2;
        uint32_t *s3 = &x->x_rstate.s3;
        float hz = *in1++;
        double phase_step = hz / sr;
// clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        t_float random;
        if(hz >= 0){
            if(phase >= 1.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase - 1;
                yn = ynp1;
                ynp1 = random; // next random value
            }
        }
        else{
            if(phase <= 0.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase + 1;
                yn = ynp1;
                ynp1 = random; // next random value
            }
        }
        if(hz >= 0)
            *out++ = yn + (ynp1 - yn) * (phase);
        else
            *out++ = yn + (ynp1 - yn) * (1 - phase);
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_ynp1 = ynp1; // next random value
    x->x_yn = yn; // current output
    return(w+5);
}

static void rampnoise_dsp(t_rampnoise *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(rampnoise_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *rampnoise_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_rampnoise *x = (t_rampnoise *)pd_new(rampnoise_class);
    rampnoise_seed(x, s, 0, NULL);
// default parameters
    t_float hz = 0;
    int numargs = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
                t_atom at[1];
                SETFLOAT(at, atom_getfloat(av+1));
                ac-=2, av+=2;
                rampnoise_seed(x, s, 1, at);
            }
            else
                goto errstate;
        }
        else{
            switch(numargs){
                case 0: hz = atom_getfloat(av);
                    numargs++;
                    ac--;
                    av++;
                    break;
                default:
                    ac--;
                    av++;
                    break;
            };
        }
    }
    if(hz >= 0)
        x->x_phase = 1.;
    x->x_freq = hz;
// get 1st output
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    x->x_ynp1 = (t_float)(random_frand(s1, s2, s3));
// in/out
    outlet_new(&x->x_obj, &s_signal);
// done
    return(x);
errstate:
    pd_error(x, "[rampnoise~]: improper args");
    return(NULL);
}

void rampnoise_tilde_setup(void){
    rampnoise_class = class_new(gensym("rampnoise~"), (t_newmethod)rampnoise_new,
        0, sizeof(t_rampnoise), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(rampnoise_class, t_rampnoise, x_freq);
    class_addmethod(rampnoise_class, (t_method)rampnoise_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(rampnoise_class, (t_method)rampnoise_seed, gensym("seed"), A_GIMME, 0);
}
