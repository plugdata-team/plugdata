// porres 2023

#include "m_pd.h"

/*
static t_class *nchs_tilde_class;

typedef struct _nchs{
    t_object x_obj;
    int      x_nchans;
}t_nchs;

static void nchs_bang(t_nchs *x){
    outlet_float(x->x_obj.ob_outlet, x->x_nchans);
}

static void nchs_tilde_dsp(t_nchs *x, t_signal **sp){
    x->x_nchans = sp[0]->s_nchans;
    nchs_bang(x);
}

static void *nchs_tilde_new(void){
    t_nchs *x = (t_nchs *)pd_new(nchs_tilde_class);
    x->x_nchans = 0;
    outlet_new(&x->x_obj, &s_float);
    return(x);
}

void nchs_tilde_setup(void){
    nchs_tilde_class = class_new(gensym("nchs~"),
        (t_newmethod)nchs_tilde_new, 0, sizeof(t_nchs), 0, 0);
    class_setdspflags(nchs_tilde_class, CLASS_MULTICHANNEL);
    class_addmethod(nchs_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(nchs_tilde_class, (t_method)nchs_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(nchs_tilde_class, (t_method)nchs_bang);
}
*/
