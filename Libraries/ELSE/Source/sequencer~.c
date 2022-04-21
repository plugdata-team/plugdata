// Porres 2017-2022

#include "m_pd.h"
#include <stdlib.h>

static t_class *sequencer_class;

#define MAXLEN 1024

typedef struct _sequencer{
    t_object    x_obj;
    float      *x_sequencer;
    float       x_lastin;
    float       x_lastout;
    int         x_length;
    int         x_index;
    int         x_bang;
}t_sequencer;

static void sequencer_bang(t_sequencer *x){
    x->x_bang = 1;
}

static void sequencer_set(t_sequencer *x, t_symbol *s, int ac, t_atom * av){
    s = NULL;
    x->x_index = 0;
    if(!ac){
        x->x_sequencer = (float*)malloc(MAXLEN * sizeof(float));
        x->x_length = 1;
        x->x_sequencer[0] = 1;
    }
    else{
        x->x_sequencer = (float *)malloc(MAXLEN * sizeof(float));
        x->x_length = ac;
        for(int i = 0; i < ac; i++)
            x->x_sequencer[i] = atom_getfloatarg(i, ac, av);
    }
}

void sequencer_goto(t_sequencer *x, t_floatarg f){
    int i = (int)f - 1;
    x->x_index = i < 0 ? 0 : i;
}

t_int *sequencer_perform(t_int *w){
    t_sequencer *x = (t_sequencer *) (w[1]);
    float *inlet = (t_float *) (w[2]);
    float *out1 = (t_float *) (w[3]);
    float *out2 = (t_float *) (w[4]);
    int n = (int) w[5];
    t_float lastin = x->x_lastin;
    t_float seq = x->x_lastout;
    while(n--){
        float input = *inlet++;
        float done = 0;
        if((input != 0 && lastin == 0) || x->x_bang){ // trigger
            seq = x->x_sequencer[x->x_index];
            x->x_index++;
            if(x->x_index >= x->x_length){
                x->x_index = 0;
                done = 1;
            }
            x->x_bang = 0;
        }
        *out1++ = seq;
        *out2++ = done;
        lastin = input;
    }
    x->x_lastin = lastin;
    x->x_lastout = seq;
    return(w+6);
}

void sequencer_dsp(t_sequencer *x, t_signal **sp){
    dsp_add(sequencer_perform, 5, x, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void *sequencer_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_sequencer *x = (t_sequencer *)pd_new(sequencer_class);
    if(!ac){
        x->x_sequencer = (float *) malloc(MAXLEN * sizeof(float));
        x->x_length = 1;
        x->x_sequencer[0] = 1;
    }
    else{
        x->x_sequencer = (float*)malloc(MAXLEN * sizeof(float));
        x->x_length = ac;
        for(int i = 0; i < ac; i++)
            x->x_sequencer[i] = atom_getfloatarg(i, ac, av);
    }
    x->x_index = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    return(x);
}

void sequencer_tilde_setup(void){
    sequencer_class = class_new(gensym("sequencer~"), (t_newmethod)sequencer_new, 0,
        sizeof(t_sequencer), 0, A_GIMME, 0);
    class_addbang(sequencer_class, (t_method)sequencer_bang);
    class_addmethod(sequencer_class, nullfn, gensym("signal"), 0);
    class_addmethod(sequencer_class,(t_method)sequencer_dsp,gensym("dsp"), A_CANT, 0);
    class_addmethod(sequencer_class,(t_method)sequencer_goto,gensym("goto"),A_FLOAT,0);
    class_addmethod(sequencer_class,(t_method)sequencer_set, gensym("set"),A_GIMME,0);
}
