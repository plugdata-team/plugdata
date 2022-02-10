// Porres 2017

#include "m_pd.h"
#include "math.h"

static t_class *wrap2_class;

typedef struct _wrap2{
    t_object   x_obj;
    t_inlet   *x_low_let;
    t_inlet   *x_high_let;
    t_outlet  *x_outlet;
}t_wrap2;

static t_int *wrap2_perform(t_int *w){
    t_wrap2 *x = (t_wrap2 *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_sample *)(w[6]);
    while(n--){
        t_float output;
        float input = *in1++;
        float in_low = *in2++;
        float in_high = *in3++;
        float low = in_low;
        float high = in_high;
        if(low > high){
            low = in_high;
            high = in_low;
        }
        float range = high - low;
        if(low == high)
            output = low;
        else{
            if(input < low){
                output = input;
                while(output < low)
                    output += range;
            }
            else
                output = fmod(input - low, range) + low;
        }
        *out++ = output;
    }
    return (w + 7);
}

static void wrap2_dsp(t_wrap2 *x, t_signal **sp){
    dsp_add(wrap2_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *wrap2_free(t_wrap2 *x){
    inlet_free(x->x_low_let);
    inlet_free(x->x_high_let);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *wrap2_new(t_symbol *s, int ac, t_atom *av){
    t_wrap2 *x = (t_wrap2 *)pd_new(wrap2_class);
    t_symbol *dummy = s;
    dummy = NULL;
/////////////////////////////////////////////////////////////////////////////////////
    float init_low, low, init_high, high;
    if(ac){
        if(ac == 1){
            if(av -> a_type == A_FLOAT){
                low = init_low = 0;
                high = init_high = atom_getfloat(av);
            }
            else
                goto errstate;
        }
        else if(ac == 2){
            int argnum = 0;
            while(ac){
                if(av->a_type != A_FLOAT)
                    goto errstate;
                else{
                    t_float curf = atom_getfloatarg(0, ac, av);
                    switch(argnum){
                        case 0:
                            low = init_low = curf;
                            break;
                        case 1:
                            high = init_high = curf;
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
        low = init_low = -1;
        high = init_high = 1;
    }
    if(low > high){
        low = init_high;
        high = init_low;
    }
/////////////////////////////////////////////////////////////////////////////////////
    x->x_low_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_low_let, low);
    x->x_high_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_high_let, high);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
    errstate:
        pd_error(x, "[wrap2~]: improper args");
        return NULL;
}

void wrap2_tilde_setup(void){
    wrap2_class = class_new(gensym("wrap2~"), (t_newmethod)wrap2_new,
        (t_method)wrap2_free, sizeof(t_wrap2), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(wrap2_class, nullfn, gensym("signal"), 0);
    class_addmethod(wrap2_class, (t_method)wrap2_dsp, gensym("dsp"), A_CANT, 0);
}
