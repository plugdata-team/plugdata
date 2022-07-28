// Porres 2017-2022

#include "m_pd.h"
#include <stdlib.h>

static t_class *impseq_class;

#define MAXLEN 1024

typedef struct _impseq{
    t_object    x_obj;
    float      *x_impseq;
    float       x_lastin;
    int         x_length;
    int         x_index;
    int         x_bang;
}t_impseq;

static void impseq_list(t_impseq *x, t_symbol *s, int ac, t_atom * av){
    s = NULL;
    if(ac == 0){
        x->x_bang = 1;
        return;
    }
    if(x->x_length != ac){
        x->x_impseq = (float*)malloc(MAXLEN * sizeof(float));
        x->x_length = ac;
    }
    for(int i = 0; i < ac; i++)
        x->x_impseq[i] = atom_getfloatarg(i, ac, av);
    x->x_index = 0;
    x->x_bang = 1;
}

static void impseq_set(t_impseq *x, t_symbol *s, int ac, t_atom * av){
    s = NULL;
    x->x_index = 0;
    if(!ac){
        x->x_impseq = (float*)malloc(MAXLEN * sizeof(float));
        x->x_length = 1;
        x->x_impseq[0] = 1;
    }
    else{
        x->x_impseq = (float *)malloc(MAXLEN * sizeof(float));
        x->x_length = ac;
        for(int i = 0; i < ac; i++)
            x->x_impseq[i] = atom_getfloatarg(i, ac, av);
    }
}

void impseq_goto(t_impseq *x, t_floatarg f){
    int i = (int)f - 1;
    x->x_index = i < 0 ? 0 : i;
}

t_int *impseq_perform(t_int *w){
    t_impseq *x = (t_impseq *) (w[1]);
    float *inlet = (t_float *) (w[2]);
    float *out1 = (t_float *) (w[3]);
    float *out2 = (t_float *) (w[4]);
    int n = (int) w[5];
    t_float lastin = x->x_lastin;
    while(n--){
        float input = *inlet++;
        float imp = 0, done = 0;
        if((input != 0 && lastin == 0) || x->x_bang){ // trigger
            imp = x->x_impseq[x->x_index];
            x->x_index++;
            if(x->x_index >= x->x_length){
                x->x_index = 0;
                done = 1;
            }
            x->x_bang = 0;
        }
        *out1++ = imp;
        *out2++ = done;
        lastin = input;
    }
    x->x_lastin = lastin;
    return(w+6);
}

void impseq_dsp(t_impseq *x, t_signal **sp){
    dsp_add(impseq_perform, 5, x, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void *impseq_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_impseq *x = (t_impseq *)pd_new(impseq_class);
    if(!ac){
        x->x_impseq = (float *) malloc(MAXLEN * sizeof(float));
        x->x_length = 1;
        x->x_impseq[0] = 1;
    }
    else{
        x->x_impseq = (float*)malloc(MAXLEN * sizeof(float));
        x->x_length = ac;
        for(int i = 0; i < ac; i++)
            x->x_impseq[i] = atom_getfloatarg(i, ac, av);
    }
    x->x_index = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    return(x);
}

void impseq_tilde_setup(void){
    impseq_class = class_new(gensym("impseq~"), (t_newmethod)impseq_new, 0,
        sizeof(t_impseq), 0, A_GIMME, 0);
    class_addmethod(impseq_class, nullfn, gensym("signal"), 0);
    class_addlist(impseq_class, (t_method)impseq_list);
    class_addmethod(impseq_class,(t_method)impseq_dsp,gensym("dsp"), A_CANT, 0);
    class_addmethod(impseq_class,(t_method)impseq_goto,gensym("goto"),A_FLOAT,0);
    class_addmethod(impseq_class,(t_method)impseq_set, gensym("set"),A_GIMME,0);
}
