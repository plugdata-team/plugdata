// Porres 2016-2018

#include "m_pd.h"
#include <string.h>

static t_class *detect_class;

typedef struct _detect{
    t_object  x_obj;
    t_float   x_count;
    t_float   x_total;
    t_float   x_lastin;
    t_float   x_sr;
    t_int     x_mode;
    t_outlet *x_outlet;
}t_detect;

static t_int *detect_perform(t_int *w){
    t_detect *x = (t_detect *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float lastin = x->x_lastin;
    t_float count = x->x_count;
    t_float total = x->x_total;
    while(n--){
        t_float input = *in++;
        if(input > 0 && lastin <= 0){
            total = count;
            count = 1;
        }
        else
            count++;
        t_float output = total;
        if(x->x_mode == 1)
            output = output * 1000 / x->x_sr;
        else if(x->x_mode == 2)
            output = x->x_sr / output;
        else if(x->x_mode == 3)
            output = output * 60 / x->x_sr;
        *out++ = output;
        lastin = input;
    }
    x->x_count = count;
    x->x_total = total;
    x->x_lastin = lastin;
    return(w + 5);
}

static void detect_dsp(t_detect *x, t_signal **sp){
    x->x_sr = (t_float)sp[0]->s_sr;
    dsp_add(detect_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *detect_free(t_detect *x){
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *detect_new(t_symbol *s, int ac, t_atom *av){
    t_detect *x = (t_detect *)pd_new(detect_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    x->x_count = x->x_total = x->x_lastin = x->x_mode = 0;
    x->x_sr = sys_getsr();
    if(ac == 1){
        if(av->a_type == A_SYMBOL){
            t_symbol *curarg = s; // get rid of warning
            curarg = atom_getsymbolarg(0, ac, av);
            if(!strcmp(curarg->s_name, "samps"))
                x->x_mode = 0;
            else if(!strcmp(curarg->s_name, "ms"))
                x->x_mode = 1;
            else if(!strcmp(curarg->s_name, "hz"))
                x->x_mode = 2;
            else if(!strcmp(curarg->s_name, "bpm"))
                x->x_mode = 3;
            else
                goto errstate;
        }
        else
            goto errstate;
    }
    else if(ac > 1)
        goto errstate;
    return(x);
errstate:
    pd_error(x, "[detect~]: improper args");
    return NULL;
}

void detect_tilde_setup(void){
    detect_class = class_new(gensym("detect~"), (t_newmethod)detect_new,
        (t_method)detect_free, sizeof(t_detect), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(detect_class, nullfn, gensym("signal"), 0);
    class_addmethod(detect_class, (t_method) detect_dsp, gensym("dsp"), A_CANT, 0);
}
