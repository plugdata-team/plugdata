
#include "m_pd.h"
#include <common/api.h>


typedef struct _typeroute
{
    t_object    x_obj;
    t_float     x_input;
    t_outlet   *x_bangout;
} t_typeroute;

static t_class *typeroute_class;

static void typeroute_bang(t_typeroute *x)
{
    outlet_bang(x->x_bangout);
}

static t_int *typeroute_perform(t_int *w)
{
    t_typeroute *x = (t_typeroute *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
   while (nblock--)
        {
            *out++ = *in++;
        }
    return (w + 5);
}

static void typeroute_dsp(t_typeroute *x, t_signal **sp)
{
    dsp_add(typeroute_perform, 4, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec);
}


static void *typeroute_new(void)
{
    t_typeroute *x = (t_typeroute *)pd_new(typeroute_class);
    outlet_new((t_object *)x, &s_signal);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    return (x);
}

CYCLONE_OBJ_API void typeroute_tilde_setup(void)
{
    typeroute_class = class_new(gensym("typeroute~"),
			    (t_newmethod)typeroute_new,
			    0,
                sizeof(t_typeroute),
                CLASS_DEFAULT,
                0);
    class_addmethod(typeroute_class, nullfn, gensym("signal"), 0);
    class_addmethod(typeroute_class, (t_method) typeroute_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(typeroute_class, typeroute_bang);
}
