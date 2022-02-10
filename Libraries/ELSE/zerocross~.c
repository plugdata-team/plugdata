// Porres 2016

#include "m_pd.h"
#include "math.h"


static t_class *zerocross_class;

typedef struct _zerocross
{
    t_object x_obj;
    t_outlet *x_outlet_dsp_0;
    t_outlet *x_outlet_dsp_1;
    t_outlet *x_outlet_dsp_2;
    t_float   x_lastout;
    t_float   x_init;
} t_zerocross;

static t_int *zerocross_perform(t_int *w)
{
    t_zerocross *x = (t_zerocross *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out1 = (t_float *)(w[4]);
    t_float *out2 = (t_float *)(w[5]);
    t_float *out3 = (t_float *)(w[6]);
    t_float lastout = x->x_lastout;
    while (nblock--)
    {
        float input = *in++;
        if (x->x_init)
            {
            x->x_init = *out1++ = *out2++ = *out3++ = 0;
            }
        else
            {
            *out1++ = (input > 0 && lastout <= 0);
            *out2++ = (input < 0 && lastout >= 0);
            *out3++ = (input > 0 && lastout <= 0) || (input < 0 && lastout >= 0);
            }
        lastout = input;
    }
    x->x_lastout = lastout;
    return (w + 7);
}

static void zerocross_dsp(t_zerocross *x, t_signal **sp)
{
    dsp_add(zerocross_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *zerocross_free(t_zerocross *x)
{
    outlet_free(x->x_outlet_dsp_0);
    outlet_free(x->x_outlet_dsp_1);
    outlet_free(x->x_outlet_dsp_2);
    return (void *)x;
}

static void *zerocross_new(t_floatarg f)
{
    t_zerocross *x = (t_zerocross *)pd_new(zerocross_class);
    x->x_lastout = 0;
    x->x_init = 1;
    x->x_outlet_dsp_0 = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet_dsp_1 = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet_dsp_2 = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void zerocross_tilde_setup(void)
{
    zerocross_class = class_new(gensym("zerocross~"),
        (t_newmethod)zerocross_new, (t_method)zerocross_free,
        sizeof(t_zerocross), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(zerocross_class, nullfn, gensym("signal"), 0);
    class_addmethod(zerocross_class, (t_method)zerocross_dsp, gensym("dsp"), A_CANT, 0);
    
}
