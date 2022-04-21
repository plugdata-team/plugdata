// porres 2021

#include "m_pd.h"

typedef struct _slew{
    t_object  x_obj;
    t_float   x_last;
    t_float   x_in;
    t_float   x_sr_rec;
	t_inlet  *x_limit;
}t_slew;

static t_class *slew_class;

static t_int *slew_perform(t_int *w){
    t_slew *x = (t_slew *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float last = x->x_last;
    while(n--){
    	float f = *in1++;
        float delta = f - last;
        float lo = *in2++;
        if(lo < 0)
            *out++ = last = f;
        else{
            float hi = lo = lo * x->x_sr_rec;
            lo = -lo;
            if(delta < lo)
                f = last + lo;
            else if(delta > hi)
                f = last + hi;
            *out++ = last = f;
        }
    }
    x->x_last = last;
    return(w+6);
}

static void slew_dsp(t_slew *x, t_signal **sp){
    x->x_sr_rec = 1 / sp[0]->s_sr;
    dsp_add(slew_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void slew_set(t_slew *x, t_floatarg f){
    x->x_last = f;
}

static void *slew_free(t_slew *x){
    inlet_free(x->x_limit);
    return(void *)x;
}

static void *slew_new(t_symbol *s, t_floatarg f){
    s = NULL;
    t_slew *x = (t_slew *)pd_new(slew_class);
	x->x_limit = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
	pd_float((t_pd *)x->x_limit, f);
    outlet_new((t_object *)x, &s_signal);
    x->x_last = 0;
    return(x);
}

void slew_tilde_setup(void){
    slew_class = class_new(gensym("slew~"), (t_newmethod)slew_new,
        (t_method)slew_free, sizeof(t_slew), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(slew_class, t_slew, x_in);
    class_addmethod(slew_class, (t_method)slew_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(slew_class, (t_method)slew_set, gensym("set"), A_DEFFLOAT, 0);
}
