// Porres 2018

#include "m_pd.h"
#include <math.h>
#include <string.h>

static t_class *trunc_class;

typedef struct _trunc{
	t_object    x_obj;
    t_int       x_bytes;
    t_atom       *x_at;
}t_trunc;

static void trunc_float(t_trunc *x, t_float f){
    outlet_float(x->x_obj.ob_outlet, trunc(f));
}

static void trunc_list(t_trunc *x, t_symbol *s, int argc, t_atom *argv){
    int old_bytes = x->x_bytes;
    x->x_bytes = argc*sizeof(t_atom);
    x->x_at = (t_atom *)t_resizebytes(x->x_at, old_bytes, x->x_bytes);
    for(int i = 0; i < argc; i++){ // get output list
        t_float f = atom_getfloatarg(i, argc, argv);
		SETFLOAT(x->x_at+i, trunc(f));
    }
	outlet_list(x->x_obj.ob_outlet, &s_list, argc, x->x_at);
}
                 
void trunc_free(t_trunc *x){
    t_freebytes(x->x_at, x->x_bytes);
}

static void *trunc_new(void){
    t_trunc *x = (t_trunc *)pd_new(trunc_class);
    outlet_new(&x->x_obj, 0);
    x->x_bytes = sizeof(t_atom);
    x->x_at = (t_atom *)getbytes(x->x_bytes);
    return(x);
}

void trunc_setup(void){
	trunc_class = class_new(gensym("trunc"), (t_newmethod)trunc_new,
        (t_method)trunc_free, sizeof(t_trunc), 0, 0);
	class_addfloat(trunc_class, (t_method)trunc_float);
	class_addlist(trunc_class, (t_method)trunc_list);
}
