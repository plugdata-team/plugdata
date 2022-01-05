
#include "m_pd.h"
#include <math.h>

typedef struct _range{
    t_object    x_obj;
    t_inlet    *x_inlet;
    t_float     x_min;
    t_float     x_max;
}t_range;

static t_class *range_class;

static void range_reset(t_range *x){
    x->x_min = INFINITY;
    x->x_max = -INFINITY;
}

static t_int *range_perform(t_int *w){
    t_range *x = (t_range *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *outmin = (t_float *)(w[5]);
    t_float *outmax = (t_float *)(w[6]);
    t_float fmin = x->x_min;
    t_float fmax = x->x_max;
    while(n--){
        t_float f = *in1++;
        t_float reset = *in2++;
        if(reset != 0) // impulse
            fmin = fmax = f;
        else{
            if(f < fmin)
                fmin = f;
            if(f > fmax)
                fmax = f;
        }
        *outmin++ = fmin;
        *outmax++ = fmax;
    }
    x->x_min = fmin;
    x->x_max = fmax;
    return(w+7);
}

static void range_dsp(t_range *x, t_signal **sp){
    dsp_add(range_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *range_new(void){
    t_range *x = (t_range *)pd_new(range_class);
    inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    range_reset(x);
    return(x);
}

void range_tilde_setup(void){
    range_class = class_new(gensym("range~"),
        (t_newmethod)range_new, 0, sizeof(t_range), 0, 0);
    class_addmethod(range_class, nullfn, gensym("signal"), 0);
    class_addmethod(range_class, (t_method)range_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(range_class, (t_method)range_reset, gensym("reset"), 0);
}
