// Porres 2016

#include "m_pd.h"
#include "math.h"

static t_class *toggleff_class;

typedef struct _toggleff
{
    t_object x_obj;
    t_float  x_xm1;
    t_float  x_ym1;
    t_float  x_init;
} t_toggleff;

static t_int *toggleff_perform(t_int *w)
{
    t_toggleff *x = (t_toggleff *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    t_float xm1 = x->x_xm1;
    t_float ym1 = x->x_ym1;
    while (nblock--)
    {
    float x0 = *in++;
        if (x->x_init) {
            *out++ = ym1 = 1;
            x->x_init = 0;
        }
        else {
            t_int cond = x0 > 0 && xm1 <= 0;
            if (cond) ym1 = fmod(cond + ym1, 2);
            *out++ = ym1;
            }
        xm1 = x0;
    }
    x->x_xm1 = xm1;
    x->x_ym1 = ym1;
    return (w + 5);
}

static void toggleff_dsp(t_toggleff *x, t_signal **sp)
{
    dsp_add(toggleff_perform, 4, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec);
}

static void *toggleff_new(t_floatarg f)
{
    t_toggleff *x = (t_toggleff *)pd_new(toggleff_class);
    x->x_init = f != 0;
    x->x_ym1 = 0;
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

void toggleff_tilde_setup(void)
{
    toggleff_class = class_new(gensym("toggleff~"),
        (t_newmethod)toggleff_new,
        0,
        sizeof(t_toggleff),
        CLASS_DEFAULT,
        A_DEFFLOAT,
        0);
        class_addmethod(toggleff_class, nullfn, gensym("signal"), 0);
        class_addmethod(toggleff_class, (t_method)toggleff_dsp, gensym("dsp"), A_CANT, 0);
}

