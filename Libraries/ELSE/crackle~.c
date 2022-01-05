// Porres 2017

#include <math.h>
#include "m_pd.h"

static t_class *crackle_class;

typedef struct _crackle
{
    t_object  x_obj;
    t_float   x_p;
    double    x_y_nm1;
    double    x_y_nm2;
    t_outlet *x_outlet;
} t_crackle;

static void crackle_clear(t_crackle *x)
{
    x->x_y_nm1 = x->x_y_nm2 = 0;
}

static t_int *crackle_perform(t_int *w)
{
    t_crackle *x = (t_crackle *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float  *in = (t_float *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    double y_nm1 = x->x_y_nm1;
    double y_nm2 = x->x_y_nm2;
    while (nblock--)
    {
        t_float output;
        t_float p = *in++;
        if (p < 0) p = 0;
        if (p > 1) p = 1;
        p = p + 1;
        output = fabs(y_nm1 * p - y_nm2 - 0.05f);
       *out++ = output;
        y_nm2 = y_nm1;
        y_nm1 = output;
    }
    x->x_y_nm1 = y_nm1;
    x->x_y_nm2 = y_nm2;
    return (w + 5);
}

static void crackle_dsp(t_crackle *x, t_signal **sp)
{
    dsp_add(crackle_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *crackle_free(t_crackle *x)
{
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *crackle_new(t_symbol *s, int ac, t_atom *av)
{
    t_crackle *x = (t_crackle *)pd_new(crackle_class);
    t_float p = 0.5; // default chaos parameter
    if (ac && av->a_type == A_FLOAT)
        {
        p = av->a_w.w_float;
        }
    x->x_p = p;
    x->x_y_nm1 = x->x_y_nm2 = 0;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void crackle_tilde_setup(void)
{
    crackle_class = class_new(gensym("crackle~"),
        (t_newmethod)crackle_new, (t_method)crackle_free,
        sizeof(t_crackle), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(crackle_class, t_crackle, x_p);
    class_addmethod(crackle_class, (t_method)crackle_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(crackle_class, (t_method)crackle_clear, gensym("clear"), 0);
}
