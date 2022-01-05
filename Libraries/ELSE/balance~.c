// Porres 2016

#include "m_pd.h"
#include <math.h>

#define HALF_PI (3.14159265358979323846 * 0.5)

static t_class *balance_class;

typedef struct _balance
{
    t_object x_obj;
    t_inlet  *x_bal_inlet;
} t_balance;

static t_int *balance_perform(t_int *w)
{
    t_balance *x = (t_balance *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // in1
    t_float *in2 = (t_float *)(w[4]); // in2
    t_float *in3 = (t_float *)(w[5]); // bal
    t_float *out1 = (t_float *)(w[6]); // L
    t_float *out2 = (t_float *)(w[7]); // R
    while (nblock--)
    {
        float in_l = *in1++;
        float in_r = *in2++;
        float bal = *in3++;
        if (bal < -1) bal = -1;
        if (bal > 1) bal = 1;
        bal = (bal + 1) * 0.5;
        *out1++ = in_l * (bal == 1 ? 0 : cos(bal * HALF_PI));
        *out2++ = in_r * sin(bal * HALF_PI);
    }
    return (w + 8);
}

static void balance_dsp(t_balance *x, t_signal **sp)
{
    dsp_add(balance_perform, 7, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void *balance_free(t_balance *x)
{
    inlet_free(x->x_bal_inlet);
    return (void *)x;
}

static void *balance_new(t_floatarg f)
{
    t_balance *x = (t_balance *)pd_new(balance_class);
    if(f < -1) f = -1;
    if(f > 1) f = 1;
    inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    x->x_bal_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_bal_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

void balance_tilde_setup(void)
{
    balance_class = class_new(gensym("balance~"), (t_newmethod)balance_new,
        (t_method)balance_free, sizeof(t_balance), CLASS_DEFAULT, A_DEFFLOAT, 0);
        class_addmethod(balance_class, nullfn, gensym("signal"), A_CANT, 0);
        class_addmethod(balance_class, (t_method)balance_dsp, gensym("dsp"), A_CANT, 0);
}

