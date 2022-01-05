// Porres 2017

#include "m_pd.h"
#include <math.h>

static t_class *pulsediv_class;

typedef struct _pulsediv{
    t_object  x_obj;
    t_float   x_div;
    t_float   x_count;
    t_float   x_start;
    t_float   x_lastin;
    t_int     x_mod;
    t_inlet  *x_triglet;
    t_outlet *x_outlet;
}t_pulsediv;

static void pulsediv_div(t_pulsediv *x, t_floatarg f){
    x->x_div = f < 1 ? 1 : f;
}

static void pulsediv_start(t_pulsediv *x, t_floatarg f){
    x->x_start = f;
}

static t_int *pulsediv_perform(t_int *w){
    t_pulsediv *x = (t_pulsediv *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float lastin = x->x_lastin;
    t_float start = x->x_start;
    t_float div = x->x_div;
    t_float count = x->x_count;
    while (nblock--){
        t_float in = *in1++;
        t_float trig = *in2++;
        t_float pulse;
        if(trig > 0)
            count = start;
        count += (pulse = (in > 0 && lastin <= 0));
        if (count >= 0)
            count = fmod(count, div);
        *out++ = pulse && count == 0;
        lastin = in;
    }
    x->x_lastin = lastin;
    x->x_count = count;
    return (w + 6);
}

static void pulsediv_dsp(t_pulsediv *x, t_signal **sp){
    dsp_add(pulsediv_perform, 5, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *pulsediv_free(t_pulsediv *x){
    inlet_free(x->x_triglet);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *pulsediv_new(t_floatarg f1, t_floatarg f2){
    t_pulsediv *x = (t_pulsediv *)pd_new(pulsediv_class);
    x->x_lastin = 1;
    x->x_div = f1 < 1 ? 1 : f1;
    x->x_start = x->x_count = f2 - 1;
    x->x_triglet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void pulsediv_tilde_setup(void){
    pulsediv_class = class_new(gensym("pulsediv~"),
        (t_newmethod)pulsediv_new, (t_method)pulsediv_free,
        sizeof(t_pulsediv), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(pulsediv_class, nullfn, gensym("signal"), 0);
    class_addmethod(pulsediv_class, (t_method) pulsediv_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(pulsediv_class, (t_method)pulsediv_div, gensym("div"), A_DEFFLOAT, 0);
    class_addmethod(pulsediv_class, (t_method)pulsediv_start, gensym("start"), A_DEFFLOAT, 0);
}
