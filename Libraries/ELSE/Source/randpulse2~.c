// Porres 2018

#include "m_pd.h"
#include "random.h"

static t_class *randpulse2_class;

typedef struct _randpulse2{
    t_object   x_obj;
    t_float    x_freq;
    t_random_state x_rstate;
    int        x_rand;
    double     x_phase;
    t_float    x_ynp1;
    t_float    x_yn;
    t_outlet  *x_outlet;
    float      x_sr;
    float      x_lastout;
    float      x_output;
    int        x_id;
}t_randpulse2;

static void randpulse2_rand(t_randpulse2 *x, t_float f){
    x->x_rand = f != 0;
}

static void randpulse2_seed(t_randpulse2 *x, t_symbol *s, int ac, t_atom *av){
    x->x_output = 0;
    x->x_phase = x->x_freq >= 0;
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    x->x_yn = (t_float)(random_frand(s1, s2, s3));
    x->x_ynp1 = (t_float)(random_frand(s1, s2, s3));
}

static t_int *randpulse2_perform(t_int *w){
    t_randpulse2 *x = (t_randpulse2 *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *out = (t_sample *)(w[4]);
    t_float ynp1 = x->x_ynp1;
    t_float yn = x->x_yn;
    t_float lastout = x->x_lastout, output = x->x_output;
    double phase = x->x_phase, sr = x->x_sr;
    while(n--){
        uint32_t *s1 = &x->x_rstate.s1;
        uint32_t *s2 = &x->x_rstate.s2;
        uint32_t *s3 = &x->x_rstate.s3;
        float hz = *in1++;
        float amp;
        double phase_step = hz / sr;
// clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        t_float random;
        if(hz >= 0){
            if (phase >= 1.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase - 1;
                yn = ynp1;
                ynp1 = random; // next random value
            }
            amp = (yn + (ynp1 - yn) * (phase));
        }
        else{
            if (phase <= 0.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase + 1;
                yn = ynp1;
                ynp1 = random; // next random value
            }
            amp = (yn + (ynp1 - yn) * (1 - phase));
        }
        if(amp > 0 && lastout == 0){
            if(x->x_rand)
                output =  (t_float)(random_frand(s1, s2, s3));
            else
                output = 1;
        }
        else if(amp < 0)
            output = 0;
        *out++ = output;
        phase += phase_step;
        lastout = output;
    }
    x->x_output = output;
    x->x_lastout = lastout;
    x->x_phase = phase;
    x->x_ynp1 = ynp1; // next random value
    x->x_yn = yn; // current output
    return(w+5);
}

static void randpulse2_dsp(t_randpulse2 *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(randpulse2_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *randpulse2_free(t_randpulse2 *x){
    outlet_free(x->x_outlet);
    return(void *)x;
}


static void *randpulse2_new(t_symbol *s, int ac, t_atom *av){
    t_randpulse2 *x = (t_randpulse2 *)pd_new(randpulse2_class);
    s = NULL;
    x->x_id = random_get_id();
// default seed
    randpulse2_seed(x, s, 0, NULL);
    x->x_lastout = x->x_output = 0.;
    t_float hz = 0;
/////////////////////////////////////////////////////////////////////////////////////
    if(av->a_type == A_SYMBOL){
        if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
            t_atom at[1];
            SETFLOAT(at, atom_getfloat(av+1));
            ac-=2, av+=2;
            randpulse2_seed(x, s, 1, at);
        }
    }
    if(ac <= 2){
        int numargs = 0;
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                switch(numargs){
                    case 0: hz = atom_getfloatarg(0, ac, av);
                        numargs++;
                        ac--;
                        av++;
                        break;
                    case 1: x->x_rand  = atom_getfloatarg(0, ac, av) != 0;
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
            else
                goto errstate;
        };
    }
    else if(ac > 3)
        goto errstate;
/////////////////////////////////////////////////////////////////////////////////////
    if(hz >= 0)
        x->x_phase = 1.;
    x->x_freq = hz;
// get 1st output
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    x->x_ynp1 = (t_float)(random_frand(s1, s2, s3));
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return(x);
errstate:
    pd_error(x, "[randpulse2~]: improper args");
    return NULL;
}

void randpulse2_tilde_setup(void){
    randpulse2_class = class_new(gensym("randpulse2~"), (t_newmethod)randpulse2_new,
        (t_method)randpulse2_free, sizeof(t_randpulse2), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(randpulse2_class, t_randpulse2, x_freq);
    class_addmethod(randpulse2_class, (t_method)randpulse2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(randpulse2_class, (t_method)randpulse2_seed, gensym("seed"), A_GIMME, 0);
    class_addmethod(randpulse2_class, (t_method)randpulse2_rand, gensym("rand"), A_DEFFLOAT, 0);
}
