// Porres 2017

#include "m_pd.h"

static t_class *dust_class;

typedef struct _dust{
    t_object   x_obj;
    int        x_val;
    t_float    x_sample_dur;
    t_float    x_density;
    t_float    x_lastout;
    t_outlet  *x_outlet;
} t_dust;

static void dust_seed(t_dust *x, t_floatarg f){
    x->x_val = (int)f * 1319;
}

static t_int *dust_perform(t_int *w){
    t_dust *x = (t_dust *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *out = (t_sample *)(w[4]);
    int val = x->x_val;
    t_float lastout = x->x_lastout;
    while(nblock--){
        t_float output;
        t_float thresh;
        t_float scale;
        t_float density = *in1++;
        thresh = density * x->x_sample_dur;
        scale = thresh > 0 ? 1./thresh : 0;
        output = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
        output = (output + 1) / 2;
        output = output < thresh ? output * scale : 0;
        if(output != 0 && lastout != 0)
            output = 0;
        *out++ = lastout = output;
        val = val * 435898247 + 382842987;
    }
    x->x_val = val;
    x->x_lastout = lastout;
    return (w + 5);
}

static void dust_dsp(t_dust *x, t_signal **sp){
    x->x_sample_dur = 1./sp[0]->s_sr;
    dsp_add(dust_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *dust_free(t_dust *x){
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *dust_new(t_floatarg f){
    t_dust *x = (t_dust *)pd_new(dust_class);
    x->x_lastout = 0;
    x->x_density = f;
    static int init_seed = 74599;
    init_seed *= 1319;
    t_int seed = init_seed;
    x->x_val = seed; // load seed value
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return(x);
}

void dust_tilde_setup(void){
    dust_class = class_new(gensym("dust~"), (t_newmethod)dust_new,
            (t_method)dust_free, sizeof(t_dust), CLASS_DEFAULT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(dust_class, t_dust, x_density);
    class_addmethod(dust_class, (t_method)dust_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(dust_class, (t_method)dust_seed, gensym("seed"), A_DEFFLOAT, 0);
}
