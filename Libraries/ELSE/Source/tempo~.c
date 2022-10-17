// Porres 2017

#include "m_pd.h"
#include "math.h"
#include "random.h"

typedef struct _tempo{
    t_object       x_obj;
    t_random_state x_rstate;
    double         x_phase;
    int            x_val;
    t_inlet       *x_inlet_tempo;
    t_inlet       *x_inlet_swing;
    t_inlet       *x_inlet_sync;
    t_float        x_sr;
    t_float        x_gate;
    t_float        x_mul;
    t_float        x_current_mul;
    t_float        x_deviation;
    t_float        x_last_gate;
    t_float        x_last_sync;
    t_float        x_swing;
    t_float        x_mode;
    int            x_id;
}t_tempo;

static t_class *tempo_class;

static void tempo_seed(t_tempo *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
    x->x_deviation = x->x_phase = 1;
}

static void tempo_bang(t_tempo *x){
    x->x_phase = 1;
}

static t_int *tempo_perform(t_int *w){
    t_tempo *x = (t_tempo *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // gate
    t_float *in2 = (t_float *)(w[4]); // tempo
    t_float *in3 = (t_float *)(w[5]); // swing
    t_float *in4 = (t_float *)(w[6]); // sync
    t_float *out = (t_float *)(w[7]);
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    float last_gate = x->x_last_gate; // last gate state
    float last_sync = x->x_last_sync; // last gate state
    double phase = x->x_phase;
    double sr = x->x_sr;
    int val = x->x_val;
    float deviation = x->x_deviation;
    while(n--){
        float gate = *in1++;
        double tempo = *in2++;
        float swing = *in3++;
        float sync = *in4++;
        float ratio = (swing/100.) + 1;
        if(tempo < 0)
            tempo = 0;
        else
            tempo *= deviation;
        double t = (double)tempo;
        if(x->x_mode == 0)
            t /= 60.;
        else if(x->x_mode == 1)
            t = 1000. / t;
        double hz = (t * (double)x->x_current_mul);
        double phase_step = hz / sr; // phase_step
        if(phase_step > 1)
            phase_step = 1;
        if(gate != 0){ // if on
            if(last_gate == 0)
                phase = 1;
            if(sync != 0 && last_sync == 0)
                phase = 1;
            *out++ = phase >= 1.;
            if(phase >= 1.){
                phase = phase - 1; // wrapped phase
// update deviation/mul
                x->x_current_mul = x->x_mul;
                t_float random = (t_float)(random_frand(s1, s2, s3));
                x->x_deviation = exp(log(ratio) * random);
            }
            phase += phase_step; // next phase
        }
        else
            *out++ = 0; // resets phase too
        last_gate = gate;
        last_sync = sync;
        deviation = x->x_deviation;
    }
    x->x_phase = phase;
    x->x_last_gate = last_gate;
    x->x_last_sync = last_sync;
    x->x_val = val;
    return(w+8);
}

static void tempo_dsp(t_tempo *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(tempo_perform, 7, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void tempo_mul(t_tempo *x, t_floatarg f){
    x->x_mul = f <= 0 ? 0.0000000000000001 : f;
}

static void tempo_bpm(t_tempo *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac){
        pd_float((t_pd *)x->x_inlet_tempo, atom_getfloatarg(0, ac, av));
        if(ac == 2)
            pd_float((t_pd *)x->x_inlet_swing, atom_getfloatarg(1, ac, av));
    }
    x->x_mode = 0;
}

static void tempo_ms(t_tempo *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac){
        pd_float((t_pd *)x->x_inlet_tempo, atom_getfloatarg(0, ac, av));
        if(ac == 2)
            pd_float((t_pd *)x->x_inlet_swing, atom_getfloatarg(1, ac, av));
    }
    x->x_mode = 1;
}

static void tempo_hz(t_tempo *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac){
        pd_float((t_pd *)x->x_inlet_tempo, atom_getfloatarg(0, ac, av));
        if(ac == 2)
            pd_float((t_pd *)x->x_inlet_swing, atom_getfloatarg(1, ac, av));
    }
    x->x_mode = 2;
}

static void *tempo_free(t_tempo *x){
    inlet_free(x->x_inlet_tempo);
    inlet_free(x->x_inlet_swing);
    inlet_free(x->x_inlet_sync);
    return(void *)x;
}

static void *tempo_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_tempo *x = (t_tempo *)pd_new(tempo_class);
    x->x_id = random_get_id();
    tempo_seed(x, s, 0, NULL);
    t_float init_swing = 0, init_tempo = 0;
    t_float on = 0, mode = 0, mul = 1;
    int flag_no_more = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(ac > 0){
        if(av->a_type == A_FLOAT){
            flag_no_more = 1;
            t_float aval = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    init_tempo = aval;
                    break;
                case 1:
                    init_swing = aval;
                    break;
                default:
                    break;
            };
            argnum++;
            ac--;
            av++;
        }
        else if(av->a_type == A_SYMBOL){
            if(flag_no_more)
                goto errstate;
            t_symbol *curarg = atom_getsymbol(av);
            if(curarg == gensym("-on")){
                on = 1;
                ac--;
                av++;
            }
            else if(curarg == gensym("-ms")){
                mode = 1;
                ac--;
                av++;
            }
            else if(curarg == gensym("-hz")){
                mode = 2;
                ac--;
                av++;
            }
            else if(curarg == gensym("-mul")){
                if((av+1)->a_type == A_FLOAT){
                    mul = atom_getfloatarg(1, ac, av);
                    ac-=2;
                    av+=2;
                }
                else
                    goto errstate;
            }
            else if(curarg == gensym("-seed")){
                if((av+1)->a_type == A_FLOAT){
                    t_atom at[1];
                    SETFLOAT(at, atom_getfloat(av+1));
                    ac-=2, av+=2;
                    tempo_seed(x, s, 1, at);
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
    };
/////////////////////////////////////////////////////////////////////////////////////
    if(init_tempo < 0)
        init_tempo = 0;
    if(init_swing < 0)
        init_swing = 0;
    if(mul < 1)
        mul = 1;
    x->x_mul = mul;
    x->x_mode = mode;
    x->x_last_sync = 0;
    x->x_last_gate = 0;
    x->x_swing = init_swing;
    x->x_gate = on;
    x->x_sr = sys_getsr(); // sample rate
    x->x_phase = 1;
    x->x_inlet_tempo = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_tempo, init_tempo);
    x->x_inlet_swing = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_swing, init_swing);
    x->x_inlet_sync = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    return(x);
errstate:
    pd_error(x, "[tempo~]: improper args");
    return(NULL);
}

void tempo_tilde_setup(void){
    tempo_class = class_new(gensym("tempo~"),(t_newmethod)tempo_new, (t_method)tempo_free,
        sizeof(t_tempo), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(tempo_class, t_tempo, x_gate);
    class_addbang(tempo_class, tempo_bang);
    class_addmethod(tempo_class, (t_method)tempo_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(tempo_class, (t_method)tempo_ms, gensym("ms"), A_GIMME, 0);
    class_addmethod(tempo_class, (t_method)tempo_hz, gensym("hz"), A_GIMME, 0);
    class_addmethod(tempo_class, (t_method)tempo_bpm, gensym("bpm"), A_GIMME, 0);
    class_addmethod(tempo_class, (t_method)tempo_mul, gensym("mul"), A_DEFFLOAT, 0);
    class_addmethod(tempo_class, (t_method)tempo_seed, gensym("seed"), A_GIMME, 0);
}
