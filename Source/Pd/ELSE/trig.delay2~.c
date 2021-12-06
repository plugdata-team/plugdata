// Porres 2017

#include "m_pd.h"
#include <math.h>

static t_class *trig_delay2_class;

typedef struct _trig_delay2{
    t_object  x_obj;
    t_int     x_on;
    t_inlet  *x_del_let;
    t_float   x_count;
    t_float   x_sr_khz;
} t_trig_delay2;

static t_int *trig_delay2_perform(t_int *w){
    t_trig_delay2 *x = (t_trig_delay2 *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *del_in = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float count = x->x_count;
    t_float sr_khz = x->x_sr_khz;
    while (nblock--){
        t_float input = *in++;
        t_float del_ms = *del_in++;
        t_int samps = (int)roundf(del_ms * sr_khz);
        if(count > samps)
            x->x_on = 0;
        if (input == 1 && !x->x_on){
            x->x_on = 1;
            count = 0;
            }
        if(x->x_on){
            *out++ = count == samps;
            count += 1;
            }
        else
            *out++ = 0;
        }
    x->x_count = count;
    return (w + 6);
}

static void trig_delay2_dsp(t_trig_delay2 *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(trig_delay2_perform, 5, x, sp[0]->s_n,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *trig_delay2_new(t_floatarg f1){
    t_trig_delay2 *x = (t_trig_delay2 *)pd_new(trig_delay2_class);
    x->x_count = x->x_on = 0;
    x->x_sr_khz = sys_getsr() * 0.001;
    x->x_del_let = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_del_let, f1);
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static void *trig_delay2_free(t_trig_delay2 *x){
    inlet_free(x->x_del_let);
    return (void *)x;
}

void setup_trig0x2edelay2_tilde(void){
    trig_delay2_class = class_new(gensym("trig.delay2~"), (t_newmethod)trig_delay2_new,
        (t_method)trig_delay2_free, sizeof(t_trig_delay2), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(trig_delay2_class, nullfn, gensym("signal"), 0);
    class_addmethod(trig_delay2_class, (t_method) trig_delay2_dsp, gensym("dsp"), A_CANT, 0);
}
