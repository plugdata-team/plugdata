// Porres 2019

#include "m_pd.h"
#include <stdlib.h>

static t_class *sequencer_class;

#define MAXLEN 4096

typedef struct _sequencer{
    t_object    x_obj;
    float       x_lastin;
    float       x_output;
    int         x_bang;
    int         x_index;
    int         x_length;
    float      *x_seq;
}t_sequencer;

static void sequencer_bang(t_sequencer *x){
    x->x_bang = 1;
}

void sequencer_goto(t_sequencer *x, t_floatarg f){
    int l = x->x_length;
    x->x_index = (int)(f < 1 ? 1 : f > l ? l : f) - 1;
}

static void sequencer_set(t_sequencer *x, t_symbol *s, int ac, t_atom * av){
    s = NULL;
    if(ac){
        x->x_seq = (float *)malloc(MAXLEN * sizeof(float));
        x->x_length = ac;
        for(int i = 0; i < ac; i++)
            x->x_seq[i] = atom_getfloatarg(i, ac, av);
        x->x_index = 0;
    }
}

t_int *sequencer_perform(t_int *w){
    t_sequencer *x = (t_sequencer *) (w[1]);
    float *in = (t_float *) (w[2]);
    float *out = (t_float *) (w[3]);
    int nblock = (int) w[4];
    t_float lastin = x->x_lastin;
    int index = x->x_index;
    float output = x->x_output;
    while(nblock--){
        float input = *in++;
        if((input != 0 && lastin == 0) || x->x_bang){ // trigger
            if(index >= x->x_length)
                index = 0;
            x->x_bang = 0;
            output = x->x_seq[index];
            index++;
        }
        *out++ = output;
        lastin = input;
    }
    x->x_output = output;
    x->x_lastin = lastin;
    x->x_index = index;
    return(w+5);
}

void sequencer_dsp(t_sequencer *x, t_signal **sp){
    dsp_add(sequencer_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void *sequencer_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_sequencer *x = (t_sequencer *)pd_new(sequencer_class);
    x->x_index = 0;
    x->x_output = 0;
    x->x_seq = (float *)malloc(MAXLEN * sizeof(float));
    if(ac == 0){
        x->x_length = 1;
        x->x_seq[0] = 0;
    }
    else{
        x->x_length = ac;
        for(int i = 0; i < ac; i++)
            x->x_seq[i] = atom_getfloatarg(i, ac, av);
    }
    x->x_index = 0;
    outlet_new(&x->x_obj, gensym("signal"));
    return(x);
}

void sequencer_tilde_setup(void){
    sequencer_class = class_new(gensym("sequencer~"), (t_newmethod)sequencer_new,
            0, sizeof(t_sequencer), 0, A_GIMME, 0);
    class_addmethod(sequencer_class, nullfn, gensym("signal"), 0);
    class_addmethod(sequencer_class,(t_method)sequencer_dsp,gensym("dsp"), A_CANT, 0);
    class_addmethod(sequencer_class,(t_method)sequencer_set, gensym("set"), A_GIMME, 0);
    class_addmethod(sequencer_class,(t_method)sequencer_goto, gensym("goto"), A_FLOAT, 0);
    class_addbang(sequencer_class, (t_method)sequencer_bang);
}
