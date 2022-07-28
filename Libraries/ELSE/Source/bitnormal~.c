// based on the code by Matt Barber for cyclone's bitnormal~
 
#include "m_pd.h"

#define NAN_V   0x7FFFFFFFul
#define POS_INF 0x7F800000ul
#define NEG_INF 0xFF800000ul

typedef struct _bitnormal{
    t_object    x_obj;
    t_inlet    *bitnormal;
    t_outlet   *x_outlet;
}t_bitnormal;

static t_class *bitnormal_class;

union bit_check{
    uint32_t    uif_uint32;
    t_float     uif_float;
};

typedef union _isdenorm{
    t_float     f;
    uint32_t    ui;
}t_isdenorm;

static inline int check_denorm(t_float f){
    t_isdenorm mask;
    mask.f = f;
    return((mask.ui & POS_INF) == 0);
}

int check_isnan(t_float in){
    union bit_check input_u;
    input_u.uif_float = in;
    return(((input_u.uif_uint32 & POS_INF) == POS_INF) && (input_u.uif_uint32 & NAN_V));
}

int check_isinf(t_float in){
    union bit_check input_u;
    input_u.uif_float = in;
    return(input_u.uif_uint32 == POS_INF || input_u.uif_uint32 == NEG_INF);
}

static t_int *bitnormal_perform(t_int *w){
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while(nblock--){
        float f = *in++;
        if(check_isnan(f) || check_isinf(f) || check_denorm(f))
            f = 0;
        *out++ = f;
    }
    return(w+4);
}

static void bitnormal_dsp(t_bitnormal *x, t_signal **sp){
    x = NULL;
    dsp_add(bitnormal_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *bitnormal_new(void){
    t_bitnormal *x = (t_bitnormal *)pd_new(bitnormal_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return(x);
}

void bitnormal_tilde_setup(void){
    bitnormal_class = class_new(gensym("bitnormal~"), (t_newmethod)bitnormal_new, 0,
        sizeof(t_bitnormal), CLASS_DEFAULT, 0);
    class_addmethod(bitnormal_class, nullfn, gensym("signal"), 0);
    class_addmethod(bitnormal_class, (t_method) bitnormal_dsp, gensym("dsp"), A_CANT, 0);
}
