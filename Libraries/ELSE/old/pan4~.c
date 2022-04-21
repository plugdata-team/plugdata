// Porres 2016

#include "m_pd.h"
#include <math.h>

#define HALF_PI (3.14159265358979323846 * 0.5)

static t_class *pan4_class;

typedef struct _pan4
{
    t_object x_obj;
    t_inlet  *x_xpan_inlet;
    t_inlet  *x_ypan_inlet;
} t_pan4;

static t_int *pan4_perform(t_int *w)
{
    t_pan4 *x = (t_pan4 *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // in
    t_float *in2 = (t_float *)(w[4]); // xpan
    t_float *in3 = (t_float *)(w[5]); // xpan
    t_float *out1 = (t_float *)(w[6]); // BL
    t_float *out2 = (t_float *)(w[7]); // FL
    t_float *out3 = (t_float *)(w[8]); // BL
    t_float *out4 = (t_float *)(w[9]); // FL
    while (nblock--)
    {
        float in = *in1++;
        float xpan = *in2++;
        float ypan = *in3++;
        if (xpan < -1) xpan = -1;
        if (xpan > 1) xpan = 1;
        if (ypan < -1) ypan = -1;
        if (ypan > 1) ypan = 1;
        xpan = (xpan + 1) * 0.5;
        ypan = (ypan + 1) * 0.5;
        float back = in * (ypan == 1 ? 0 : cos(ypan * HALF_PI));
        float front = in * sin(ypan * HALF_PI);
        *out1++ = back * (xpan == 1 ? 0 : cos(xpan * HALF_PI)); // Left Back
        *out2++ = front * (xpan == 1 ? 0 : cos(xpan * HALF_PI));// Left Front
        *out3++ = front * sin(xpan * HALF_PI);                  // Right Front
        *out4++ = back * sin(xpan * HALF_PI);                   // Right Back

    }
    return (w + 10);
}

static void pan4_dsp(t_pan4 *x, t_signal **sp)
{
    dsp_add(pan4_perform, 9, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
            sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec);
}

static void *pan4_free(t_pan4 *x)
{
    inlet_free(x->x_xpan_inlet);
    inlet_free(x->x_ypan_inlet);
    return (void *)x;
}

static void *pan4_new(t_floatarg f1, t_floatarg f2)
{
    t_pan4 *x = (t_pan4 *)pd_new(pan4_class);
    if(f1 < -1) f1 = -1;
    if(f1 > 1) f1 = 1;
    if(f2 < -1) f2 = -1;
    if(f2 > 1) f2 = 1;
    x->x_xpan_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_xpan_inlet, f1);
    x->x_ypan_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_ypan_inlet, f2);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

void pan4_tilde_setup(void)
{
    pan4_class = class_new(gensym("pan4~"), (t_newmethod)pan4_new,
        (t_method)pan4_free, sizeof(t_pan4), CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
        class_addmethod(pan4_class, nullfn, gensym("signal"), 0);
        class_addmethod(pan4_class, (t_method)pan4_dsp, gensym("dsp"), A_CANT, 0);
}

