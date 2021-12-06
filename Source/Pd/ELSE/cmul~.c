#include "m_pd.h"

static t_class *cmul_class;

typedef struct _cmul{
    t_object  x_obj;
    t_float   x_in;
}t_cmul;

static t_int *cmul_perform(t_int *w){
    t_cmul *x = (t_cmul *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *in4 = (t_float *)(w[6]);
    t_float *out1 = (t_float *)(w[7]);
    t_float *out2 = (t_float *)(w[8]);
    while(n--){
        float re1 = *in1++;
        float im1 = *in2++;
        float re2 = *in3++;
        float im2 = *in4++;
        *out1++ = re1 * re2 - im1 * im2; // re
        *out2++ = im1 * re2 + re1 * im2; // im
    };
    return(w + 9);
}

static void cmul_dsp(t_cmul *x, t_signal **sp){
    dsp_add(cmul_perform, 8, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec);
}

static void *cmul_new(void){
    t_cmul *x = (t_cmul *)pd_new(cmul_class);
    inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

void cmul_tilde_setup(void){
    cmul_class = class_new(gensym("cmul~"), (t_newmethod)cmul_new, 0, sizeof(t_cmul), 0, 0);
    CLASS_MAINSIGNALIN(cmul_class, t_cmul, x_in);
    class_addmethod(cmul_class, (t_method)cmul_dsp, gensym("dsp"), A_CANT, 0);
}
