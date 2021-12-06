// Porres 2017

#include "m_pd.h"

static t_class *randi_class;

typedef struct _randi{
    t_object         x_obj;
    unsigned int     x_val;
    t_int            x_randi;
    t_float          x_lastin;
    t_int            x_trig_bang;
    t_inlet         *x_low_let;
    t_inlet         *x_high_let;
    t_outlet        *x_outlet;
} t_randi;

static void randi_seed(t_randi *x, t_floatarg f){
    x->x_val = f;
}

static void randi_bang(t_randi *x){
    x->x_trig_bang = 1;
}

static t_int *randi_perform(t_int *w){
    t_randi *x = (t_randi *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_sample *)(w[6]);
    unsigned int val = x->x_val;
    t_int randi = x->x_randi;
    t_float lastin = x->x_lastin;
    while (nblock--){
        float trig = *in1++;
        float input2 = *in2++;
        float input3 = *in3++;
        float out_low = (int)input2; // Output LOW
        float out_high = (int)input3 + 1; // Output HIGH
        if (out_low > out_high){
            int temp = out_low;
            out_low = out_high;
            out_high = temp;
        }
        int range = out_high - out_low; // range
        if (range == 0)
            randi = out_low;
        else{
            t_int trigger = 0;
            if(x->x_trig_bang){
                trigger = 1;
                x->x_trig_bang = 0;
            }
            else
                trigger = (trig > 0 && lastin <= 0) || (trig < 0 && lastin >= 0);
            if(trigger){ // update
                randi = ((double)range) * ((double)val) * (1./4294967296.);
                val = val * 472940017 + 832416023;
                if(randi > range)
                    randi = range;
                randi += out_low;
            }
            
        }
        *out++ = randi;
        lastin = trig;
        }
    x->x_val = val;
    x->x_randi = randi; // current output
    x->x_lastin = lastin; // last input
    return (w + 7);
}

static void randi_dsp(t_randi *x, t_signal **sp){
    dsp_add(randi_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *randi_free(t_randi *x){
    inlet_free(x->x_low_let);
    inlet_free(x->x_high_let);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *randi_new(t_symbol *s, int ac, t_atom *av){
    t_randi *x = (t_randi *)pd_new(randi_class);
    t_symbol *dummy = s;
    dummy = NULL;
/////////////////////////////////////////////////////////////////////////////////////
    float low;
    float high;
    if(ac){
        if(ac == 1){
            if(av -> a_type == A_FLOAT){
                low = 0;
                high = atom_getfloat(av);
                }
            else goto errstate;
            }
        else if(ac == 2){
            int argnum = 0;
            while(ac){
                if(av -> a_type != A_FLOAT)
                    goto errstate;
                else{
                    t_float curf = atom_getfloatarg(0, ac, av);
                    switch(argnum){
                        case 0:
                            low = curf;
                            break;
                        case 1:
                            high = curf;
                            break;
                        default:
                            break;
                        };
                    };
                    argnum++;
                    ac--;
                    av++;
                };
            }
        else goto errstate;
        }
    else{
        low = 0;
        high = 1;
        }
/////////////////////////////////////////////////////////////////////////////////////
    static unsigned int random_seed = 1489853723;
    random_seed = random_seed * 435898247 + 938284287;
    t_int seed = random_seed & 0x7fffffff;
    x->x_val = seed; // load seed value
    x->x_lastin = 0;
    x->x_low_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_low_let, low);
    x->x_high_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_high_let, high);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
    errstate:
        pd_error(x, "randi~: improper args");
        return NULL;
}

void setup_rand0x2ei_tilde(void){
    randi_class = class_new(gensym("rand.i~"), (t_newmethod)randi_new,
        (t_method)randi_free, sizeof(t_randi), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(randi_class, nullfn, gensym("signal"), 0);
    class_addmethod(randi_class, (t_method)randi_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(randi_class, (t_method)randi_seed, gensym("seed"), A_DEFFLOAT, 0);
    class_addbang(randi_class, (t_method)randi_bang);
}
