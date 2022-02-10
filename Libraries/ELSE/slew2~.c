// porres 2021

#include "m_pd.h"

typedef struct _slew2{
    t_object  x_obj;
    t_float   x_last;
    t_float   x_in;
    t_float   x_sr_rec;
    t_inlet  *x_downlet;
    t_inlet  *x_uplet;
}t_slew2;

static t_class *slew2_class;

static t_int *slew2_perform(t_int *w){
    t_slew2 *x = (t_slew2 *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float last = x->x_last;
    while(n--){
    	float f = *in1++;
        float up = *in2++ * x->x_sr_rec;
        float down = *in3++ * -x->x_sr_rec;
        float delta = f - last;
        if(delta > 0){ // up
            if(up >= 0 && delta > up)
                f = last + up;
        }
        else{ // down
            if(down <= 0 && delta < down)
                f = last + down;
        }
        *out++ = last = f;
    }
    x->x_last = last;
    return(w+7);
}

static void slew2_dsp(t_slew2 *x, t_signal **sp){
    x->x_sr_rec = 1 / sp[0]->s_sr;
    dsp_add(slew2_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
        sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void slew2_set(t_slew2 *x, t_floatarg f){
    x->x_last = f;
}

static void *slew2_free(t_slew2 *x){
    inlet_free(x->x_downlet);
    inlet_free(x->x_uplet);
    return(void *)x;
}

static void *slew2_new(t_symbol *s, t_floatarg f1, t_floatarg f2){
    s = NULL;
    t_slew2 *x = (t_slew2 *)pd_new(slew2_class);
    t_float up = f1;
    t_float down = f2;
    x->x_uplet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_uplet, up);
    x->x_downlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_downlet, down);
    outlet_new((t_object *)x, &s_signal);
    x->x_last = 0;
    return(x);
}

void slew2_tilde_setup(void){
    slew2_class = class_new(gensym("slew2~"), (t_newmethod)slew2_new,
        (t_method)slew2_free, sizeof(t_slew2), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(slew2_class, t_slew2, x_in);
    class_addmethod(slew2_class, (t_method)slew2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(slew2_class, (t_method)slew2_set, gensym("set"), A_DEFFLOAT, 0);
}
