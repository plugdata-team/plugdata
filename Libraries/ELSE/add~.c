// Porres 2016

#include "m_pd.h"

static t_class *add_class;

typedef struct _add{
    t_object  x_obj;
    t_float   x_sum;
    t_float   x_in;
    t_float   x_start;
    t_inlet  *x_triglet;
    t_outlet *x_outlet;
} t_add;

static void add_bang(t_add *x){
    x->x_sum = x->x_start;
}

static void add_set(t_add *x, t_floatarg f){
    x->x_start = f;
}

static t_int *add_perform(t_int *w){
    t_add *x = (t_add *)(w[1]);
    int n = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float sum = x->x_sum;
    t_float start = x->x_start;
    while(n--){
        t_float in = *in1++;
        t_float trig = *in2++;
        if(trig == 1)
            *out++ = sum = (start += in);
        else
            *out++ = (sum += in);
    }
    x->x_sum = sum; // next
    return (w + 6);
}

static void add_dsp(t_add *x, t_signal **sp){
    dsp_add(add_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *add_free(t_add *x){
    inlet_free(x->x_triglet);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *add_new(t_floatarg f){
    t_add *x = (t_add *)pd_new(add_class);
    x->x_sum = x->x_start = f;
    x->x_triglet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void add_tilde_setup(void){
    add_class = class_new(gensym("add~"),
        (t_newmethod)add_new, (t_method)add_free,
        sizeof(t_add), CLASS_DEFAULT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(add_class, t_add, x_in);
    class_addmethod(add_class, (t_method) add_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(add_class, (t_method)add_set, gensym("set"), A_FLOAT, 0);
    class_addbang(add_class,(t_method)add_bang);
}
