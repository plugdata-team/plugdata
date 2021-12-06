// Porres 2018

#include "m_pd.h"
#include <math.h>

static t_class *rint_class;

typedef struct _rint{
	t_object    x_obj;
    t_int       x_bytes;
    t_float     x_f;
    t_atom     *x_at;
}t_rint;

static void rint_bang(t_rint *x){
    outlet_float(x->x_obj.ob_outlet, rint(x->x_f));
}

static void rint_float(t_rint *x, t_float f){
    outlet_float(x->x_obj.ob_outlet, rint(x->x_f = f));
}

static void rint_list(t_rint *x, t_symbol *s, int argc, t_atom *argv){
    int old_bytes = x->x_bytes, i;
    x->x_bytes = argc*sizeof(t_atom);
    x->x_at = (t_atom *)t_resizebytes(x->x_at, old_bytes, x->x_bytes);
	for(i = 0; i < argc; i++) // get output list
		SETFLOAT(x->x_at+i, rint(atom_getfloatarg(i, argc, argv)));
	outlet_list(x->x_obj.ob_outlet, &s_list, argc, x->x_at);
}

static void rint_free(t_rint *x){
    t_freebytes(x->x_at, x->x_bytes);
}

static void *rint_new(t_floatarg f){
    t_rint *x = (t_rint *)pd_new(rint_class);
    x->x_f = f;
    floatinlet_new(&x->x_obj, &x->x_f);
    outlet_new(&x->x_obj, 0);
    x->x_bytes = sizeof(t_atom);
    x->x_at = (t_atom *)getbytes(x->x_bytes);
    return(x);
}

void rint_setup(void){
	rint_class = class_new(gensym("rint"), (t_newmethod)rint_new,
            (t_method)rint_free, sizeof(t_rint), 0, A_DEFFLOAT, 0);
	class_addfloat(rint_class, (t_method)rint_float);
    class_addlist(rint_class, (t_method)rint_list);
    class_addbang(rint_class, (t_method)rint_bang);
}
