// Porres 2018

#include "m_pd.h"
#include <string.h>

static t_class *coin_class;

typedef struct _coin{
    t_object   x_obj;
    int        x_val;
    t_float    x_random;
    t_float    x_out_of;
    t_float    x_lastin;
    t_inlet   *x_n_let;
    t_outlet  *x_outlet1;
    t_outlet  *x_outlet2;
}t_coin;

static void coin_float(t_coin *x, t_float f){
    if(f > 0)
        x->x_out_of = f;
}

static void coin_seed(t_coin *x, t_floatarg f){
    x->x_val = (int)f * 1319;
}

static t_int *coin_perform(t_int *w){
    t_coin *x = (t_coin *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out1 = (t_sample *)(w[5]);
    t_float *out2 = (t_sample *)(w[6]);
    int val = x->x_val;
    t_float random = x->x_random;
    t_float lastin = x->x_lastin;
    while (nblock--){
        float trig = *in1++;
        float n = *in2++;
        t_float prob = n * 100/x->x_out_of;
        if(prob < 0){
            prob = 0;
        }
        if(prob > 100){
            prob = 100;
        }
        if (trig != 0 && lastin == 0){
            if (prob == 0){
                *out1++ = 0;
                *out2++ = trig;
            }
            else if (prob == 100){
                *out1++ = trig;
                *out1++ = 0;
            }
            else { // toss coin
                random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
                random = ((random + 1) / 2) * 100; // 0-100
                val = val * 435898247 + 382842987;
                if (random < prob){
                    *out1++ = trig;
                    *out2++ = 0;
                }
                else{
                    *out1++ = 0;
                    *out2++ = trig;
                }
            }
        }
        else{
            *out1++ = 0;
            *out2++ = 0;
        }
        lastin = trig;
    }
    x->x_val = val;
    x->x_random = random; // current output
    x->x_lastin = lastin; // last input
    return(w + 7);
}

static void coin_dsp(t_coin *x, t_signal **sp){
    dsp_add(coin_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *coin_free(t_coin *x){
    inlet_free(x->x_n_let);
    outlet_free(x->x_outlet1);
    outlet_free(x->x_outlet2);
    return (void *)x;
}

static void *coin_new(t_symbol *s, int ac, t_atom *av){
    t_coin *x = (t_coin *)pd_new(coin_class);
    t_symbol *dummy = s;
    dummy = NULL;
// default
    static int init_seed = 234599;
    init_seed *= 1319;
    t_int seed = init_seed;
    t_float init_n = 50;
    x->x_out_of = 100;
/////////////////////////////////////////////////////////////////////////////////////
int argnum = 0;
while(ac){
    if(av->a_type == A_SYMBOL){
        t_symbol *cursym = atom_getsymbolarg(0, ac, av);
        if(!strcmp(cursym->s_name, "-seed")){
            if(ac == 2 && (av+1)->a_type == A_FLOAT){
                t_float curfloat = atom_getfloatarg(1, ac, av);
                init_seed = curfloat;
                ac -= 2;
                av += 2;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    }
    else{
        t_float curf = atom_getfloatarg(0, ac, av);
        switch(argnum){
            case 0:
                init_n = curf;
                break;
            case 1:
                if(curf > 0)
                    x->x_out_of = curf;
            break;
            default:
            break;
            };
        };
    argnum++;
    ac--;
    av++;
    };
/////////////////////////////////////////////////////////////////////////////////////
    x->x_val = seed; // load seed value
    x->x_lastin = 0;
    if (init_n < 0)
        init_n = 0;
    if (init_n > 100)
        init_n = 100;
//
    x->x_n_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_n_let, init_n);
    x->x_outlet1 = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet2 = outlet_new(&x->x_obj, &s_signal);
    return (x);
    errstate:
        pd_error(x, "[coin~]: improper args");
        return NULL;
}

void coin_tilde_setup(void){
    coin_class = class_new(gensym("coin~"), (t_newmethod)coin_new,
        (t_method)coin_free, sizeof(t_coin), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(coin_class, nullfn, gensym("signal"), 0);
    class_addfloat(coin_class, coin_float);
    class_addmethod(coin_class, (t_method)coin_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(coin_class, (t_method)coin_seed, gensym("seed"), A_DEFFLOAT, 0);
}
