// Porres 2017

#include "m_pd.h"
#include "random.h"

static t_class *lfnoise_class;

typedef struct _lfnoise{
    t_object        x_obj;
    t_float         x_freq;
    t_random_state  x_rstate;
    double          x_phase;
    t_float         x_ynp1;
    t_float         x_yn;
    t_int           x_interp;
    t_inlet        *x_inlet_sync;
    float           x_sr;
    int             x_id;
}t_lfnoise;

static void lfnoise_interp(t_lfnoise *x, t_floatarg f){
    x->x_interp = (int)f != 0;
}

static void lfnoise_seed(t_lfnoise *x, t_symbol *s, int ac, t_atom *av){
    x->x_phase = 0;
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    x->x_yn = (t_float)(random_frand(s1, s2, s3));
    x->x_ynp1 = (t_float)(random_frand(s1, s2, s3));
}

static t_int *lfnoise_perform(t_int *w){
    t_lfnoise *x = (t_lfnoise *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_sample *)(w[5]);
    double phase = x->x_phase;
    t_float ynp1 = x->x_ynp1;
    t_float yn = x->x_yn;
    t_int interp = x->x_interp;
    double sr = x->x_sr;
    while(nblock--){
        uint32_t *s1 = &x->x_rstate.s1;
        uint32_t *s2 = &x->x_rstate.s2;
        uint32_t *s3 = &x->x_rstate.s3;
        float hz = *in1++;
        float sync = *in2++;
        double phase_step = hz / sr;
// clipped phase_step
        phase_step = phase_step > 1 ? 1. : phase_step < -1 ? -1 : phase_step;
        t_float random;
        if(hz >= 0){
            if (sync == 1)
                phase = 1;
            if(phase >= 1.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase - 1;
                yn = ynp1;
                ynp1 = random; // next random value
            }
        }
        else{
            if(sync == 1)
                phase = 0;
            if(phase <= 0.){ // update
                random = (t_float)(random_frand(s1, s2, s3));
                phase = phase + 1;
                yn = ynp1;
                ynp1 = random; // next random value
            }
        }
        if(interp){
            if(hz >= 0)
                *out++ = yn + (ynp1 - yn) * (phase);
            else
                *out++ = yn + (ynp1 - yn) * (1 - phase);
        }
        else
            *out++ = yn;
        phase += phase_step;
    }
    x->x_phase = phase;
    x->x_ynp1 = ynp1; // next random value
    x->x_yn = yn; // current output
    return(w+6);
}

static void lfnoise_dsp(t_lfnoise *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(lfnoise_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *lfnoise_free(t_lfnoise *x){
    inlet_free(x->x_inlet_sync);
    return(void *)x;
}


static void *lfnoise_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_lfnoise *x = (t_lfnoise *)pd_new(lfnoise_class);
    x->x_id = random_get_id();
    lfnoise_seed(x, s, 0, NULL);
// default parameters
    t_float hz = 0;
    t_int interp = 0;
    int numargs = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
                t_atom at[1];
                SETFLOAT(at, atom_getfloat(av+1));
                ac-=2, av+=2;
                lfnoise_seed(x, s, 1, at);
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
                case 1: interp = atom_getfloat(av) != 0;
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
    x->x_interp = interp != 0;
// get 1st output
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    x->x_ynp1 = (t_float)(random_frand(s1, s2, s3));
// in/out
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    pd_float((t_pd *)x->x_inlet_sync, 0);
// done
    return(x);
errstate:
    pd_error(x, "[lfnoise~]: improper args");
    return(NULL);
}

void lfnoise_tilde_setup(void){
    lfnoise_class = class_new(gensym("lfnoise~"), (t_newmethod)lfnoise_new,
        (t_method)lfnoise_free, sizeof(t_lfnoise), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(lfnoise_class, t_lfnoise, x_freq);
    class_addmethod(lfnoise_class, (t_method)lfnoise_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(lfnoise_class, (t_method)lfnoise_interp, gensym("interp"), A_DEFFLOAT, 0);
    class_addmethod(lfnoise_class, (t_method)lfnoise_seed, gensym("seed"), A_GIMME, 0);
}
