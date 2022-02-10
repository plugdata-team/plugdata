// Porres 2016

#include "m_pd.h"
#include "math.h"

static t_class *changed_class;

typedef struct _changed
{
    t_object x_obj;
    t_float  x_x1m1;
    t_float  x_thresh;
    t_inlet  *x_tresh_inlet;
} t_changed;

static t_int *changed_perform(t_int *w)
{
    t_changed *x = (t_changed *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    t_float x1m1 = x->x_x1m1;
    while (nblock--)
    {
    float x1 = *in1++;
    float th = *in2++; // threshold
    if (th < 0)
        {
        *out++ = fabs(x1 - x1m1) > 0;
        }
    else
        {
        *out++ = fabs(x1 - x1m1) > th;
        }
    x1m1 = x1;
    }
    x->x_x1m1 = x1m1;
    return (w + 6);
}

static void changed_dsp(t_changed *x, t_signal **sp)
{
    dsp_add(changed_perform, 5, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *changed_free(t_changed *x)
{
    inlet_free(x->x_tresh_inlet);
    return (void *)x;
}

static void *changed_new(t_floatarg f)
{
    t_changed *x = (t_changed *)pd_new(changed_class);
    if(f < 0) f = 0;
    x->x_thresh = f;
    x->x_x1m1 = 0;
    x->x_tresh_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_tresh_inlet, f);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

void changed_tilde_setup(void)
{
    changed_class = class_new(gensym("changed~"),
        (t_newmethod)changed_new,
        changed_free,
        sizeof(t_changed),
        CLASS_DEFAULT,
        A_DEFFLOAT,
        0);
        class_addmethod(changed_class, nullfn, gensym("signal"), 0);
        class_addmethod(changed_class, (t_method)changed_dsp, gensym("dsp"), A_CANT, 0);
}

