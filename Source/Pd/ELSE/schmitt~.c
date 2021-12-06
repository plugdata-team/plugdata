// Porres 2018

#include "m_pd.h"

typedef struct _schmitt{
    t_object x_obj;
    t_float  x_last;
    t_inlet  *x_lolet;
    t_inlet  *x_hilet;
} t_schmitt;

static t_class *schmitt_class;

static t_int *schmitt_perform(t_int *w){
    t_schmitt *x = (t_schmitt *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float last = x->x_last;
    while (nblock--){
        float in = *in1++;
        float lo = *in2++;
        float hi = *in3++;
        *out++ = last = (in > lo && (in >= hi || last));
    }
    x->x_last = last;
    return (w + 7);
}

static void schmitt_dsp(t_schmitt *x, t_signal **sp){
    dsp_add(schmitt_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *schmitt_free(t_schmitt *x){
    inlet_free(x->x_lolet);
    inlet_free(x->x_hilet);
    return (void *)x;
}

static void *schmitt_new(t_symbol *s, int argc, t_atom *argv){
    t_schmitt *x = (t_schmitt *) pd_new(schmitt_class);
    t_float thlo = 0, thhi = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0,argc,argv);
            switch(argnum){
                case 0:
                    thlo = argval;
                    break;
                case 1:
                    thhi = argval;
                    break;
                default:
                    break;
            };
            argc--;
            argv++;
            argnum++;
        }
        else{
            goto errstate;
        };
    };
    x->x_lolet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_lolet, thlo);
    x->x_hilet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_hilet, thhi);
    outlet_new((t_object *)x, &s_signal);
    x->x_last = 0;
    return (x);
errstate:
    pd_error(x, "schmitt~: improper args");
    return NULL;
}

void schmitt_tilde_setup(void){
    schmitt_class = class_new(gensym("schmitt~"), (t_newmethod)schmitt_new,
        (t_method)schmitt_free, sizeof(t_schmitt), 0, A_GIMME, 0);
    class_addmethod(schmitt_class, nullfn, gensym("signal"), 0);
    class_addmethod(schmitt_class, (t_method)schmitt_dsp, gensym("dsp"), A_CANT, 0);
}
