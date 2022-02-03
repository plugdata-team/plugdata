// Barber and Porres 2016

#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h" // for magic_isnan(), magic_isinf() and BITWISE_ISDENORM

typedef struct _bitsafe {
    t_object x_obj;
    t_inlet *bitsafe;
    t_outlet *x_outlet;
} t_bitsafe;

void *bitsafe_new(void);
static t_int * bitsafe_perform(t_int *w);
static void bitsafe_dsp(t_bitsafe *x, t_signal **sp);

static t_class *bitsafe_class;

static t_int *bitsafe_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in = (t_float *)(w[2]);
    t_float *out = (t_float *)(w[3]);
    while (nblock--)
    {
        float f = *in++;
        if(magic_isnan(f) || magic_isinf(f) || BITWISE_ISDENORM(f)) f = 0;
        *out++ = f;
    }
    return (w + 4);
}

static void bitsafe_dsp(t_bitsafe *x, t_signal **sp)
{
    dsp_add(bitsafe_perform, 3, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *bitsafe_new(void)
{
    t_bitsafe *x = (t_bitsafe *)pd_new(bitsafe_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void bitsafe_tilde_setup(void)
{
    bitsafe_class = class_new(gensym("bitsafe~"), (t_newmethod)bitsafe_new, 0,
        sizeof(t_bitsafe), CLASS_DEFAULT, 0);
    class_addmethod(bitsafe_class, nullfn, gensym("signal"), 0);
    class_addmethod(bitsafe_class, (t_method) bitsafe_dsp, gensym("dsp"), A_CANT, 0);
}
