// Porres 2016

#include "m_pd.h"
#include <string.h>

static t_class *sh_class;

typedef struct _sh{
    t_object    x_obj;
    t_inlet     *x_trig_inlet;
    t_float     x_f;
    t_float     x_lastout;
    t_float     x_last_trig;
    t_float     x_trig_bang;
    t_float     x_thresh;
    t_int         x_mode;
} t_sh;

static void sh_thresh(t_sh *x, t_floatarg f){
    x->x_thresh = f;
}

static void sh_gate(t_sh *x){
    x->x_mode = 0;
}

static void sh_trigger(t_sh *x){
    x->x_mode = 1;
}

static void sh_set(t_sh *x, t_floatarg f){
    x->x_lastout = f;
}

static void sh_bang(t_sh *x){
    x->x_trig_bang = 1;
}

static t_int *sh_perform(t_int *w){
    t_sh *x = (t_sh *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float output = x->x_lastout;
    t_float last_trig = x->x_last_trig;
    while (nblock--){
        float input = *in1++;
        float trigger = *in2++;
        if (x->x_trig_bang){
            output = input;
            x->x_trig_bang = 0;
        }
        if (!x->x_mode && trigger > x->x_thresh)
            output = input;
        if (x->x_mode && trigger > x->x_thresh && last_trig <= x->x_thresh)
            output = input;
        *out++ = output;
        last_trig = trigger;
    }
    x->x_lastout = output;
    x->x_last_trig = last_trig;
    return (w + 6);
}

static void sh_dsp(t_sh *x, t_signal **sp){
    dsp_add(sh_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *sh_free(t_sh *x){
    inlet_free(x->x_trig_inlet);
    return (void *)x;
}

static void *sh_new(t_symbol *s, int argc, t_atom *argv){
    t_sh *x = (t_sh *)pd_new(sh_class);
    t_symbol *dummy = s;
    dummy = NULL;
    float init_thresh = 0;
    float init_value = 0;
    int init_mode = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    init_thresh = argval;
                    break;
                case 1:
                    init_value = argval;
                    break;
                case 2:
                    init_mode = argval != 0;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--;
            argv++;
        }
        else if(argv->a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(curarg->s_name, "-tr")){
                init_mode = 1;
                argc--;
                argv++;
            }
            else
                goto errstate;
        }
    };
/////////////////////////////////////////////////////////////////////////////////////
    x->x_trig_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    x->x_thresh = init_thresh;
    x->x_lastout = init_value;
    x->x_mode = init_mode;
    return (x);
    errstate:
        pd_error(x, "sh~: improper args");
        return NULL;
}

void sh_tilde_setup(void){
    sh_class = class_new(gensym("sh~"), (t_newmethod)sh_new, (t_method)sh_free,
        sizeof(t_sh), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(sh_class, t_sh, x_f);
    class_addmethod(sh_class, (t_method)sh_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(sh_class, (t_method)sh_set, gensym("set"), A_DEFFLOAT, 0);
    class_addmethod(sh_class, (t_method)sh_thresh, gensym("thresh"), A_DEFFLOAT, 0);
    class_addmethod(sh_class, (t_method)sh_trigger, gensym("trigger"), 0);
    class_addmethod(sh_class, (t_method)sh_gate, gensym("gate"), 0);
    class_addbang(sh_class,(t_method)sh_bang);
}

