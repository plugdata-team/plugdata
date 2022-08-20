// Porres 2017

#include "m_pd.h"
#include "random.h"

static t_class *randf_class;

typedef struct _randf{
    t_object        x_obj;
    t_random_state  x_rstate;
    t_float         x_randf;
    t_float         x_lastin;
    t_int           x_trig_bang;
    t_inlet        *x_low_let;
    t_inlet        *x_high_let;
}t_randf;

static unsigned int instanc_n = 0;

static void randf_seed(t_randf *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, ++instanc_n));
}

static void randf_bang(t_randf *x){
    x->x_trig_bang = 1;
}

static t_int *randf_perform(t_int *w){
    t_randf *x = (t_randf *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_sample *)(w[6]);
    t_float randf = x->x_randf;
    t_float lastin = x->x_lastin;
    while(n--){
        float trig = *in1++;
        float out_low = *in2++;
        float out_high = *in3++;
        if(out_low > out_high){
            float temp = out_low;
            out_low = out_high;
            out_high = temp;
        }
        float range = out_high - out_low; // range
        if(range == 0)
            randf = out_low;
        else{
            t_int trigger = 0;
            if(x->x_trig_bang){
                trigger = 1;
                x->x_trig_bang = 0;
            }
            else
                trigger = (trig > 0 && lastin <= 0) || (trig < 0 && lastin >= 0);
            if(trigger){ // update
                uint32_t *s1 = &x->x_rstate.s1;
                uint32_t *s2 = &x->x_rstate.s2;
                uint32_t *s3 = &x->x_rstate.s3;
                t_float noise = (t_float)(random_frand(s1, s2, s3)) * 0.5 + 0.5;
                randf = (noise * range) + out_low;
            }
        }
        *out++ = randf;
        lastin = trig;
    }
    x->x_randf = randf;   // current output
    x->x_lastin = lastin; // last input
    return(w+7);
}

static void randf_dsp(t_randf *x, t_signal **sp){
    dsp_add(randf_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *randf_free(t_randf *x){
    inlet_free(x->x_low_let);
    inlet_free(x->x_high_let);
    return(void *)x;
}

static void *randf_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_randf *x = (t_randf *)pd_new(randf_class);
    randf_seed(x, s, 0, NULL);
    float low = -1, high = 1;
    if(ac){
        if(av->a_type == A_SYMBOL){
            if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
                t_atom at[1];
                SETFLOAT(at, atom_getfloat(av+1));
                ac-=2, av+=2;
                randf_seed(x, s, 1, at);
            }
            else
                goto errstate;
        }
        if(ac == 1)
            high = atom_getfloat(av);
        else if(ac >= 2){
            low = atom_getfloatarg(0, ac, av);
            high = atom_getfloatarg(1, ac, av);
        }
        else
            goto errstate;
    }
    x->x_lastin = 0;
    x->x_low_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_low_let, low);
    x->x_high_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_high_let, high);
    outlet_new(&x->x_obj, &s_signal);
    return(x);
errstate:
        pd_error(x, "[rand.f~]: improper args");
        return(NULL);
}

void setup_rand0x2ef_tilde(void){
    randf_class = class_new(gensym("rand.f~"), (t_newmethod)randf_new,
        (t_method)randf_free, sizeof(t_randf), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(randf_class, nullfn, gensym("signal"), 0);
    class_addmethod(randf_class, (t_method)randf_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(randf_class, (t_method)randf_seed, gensym("seed"), A_GIMME, 0);
    class_addbang(randf_class, (t_method)randf_bang);
}
