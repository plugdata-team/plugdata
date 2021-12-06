// Porres 2016
 
#include "m_pd.h"
#include <math.h>

static t_class *cents2ratio_class;

typedef struct _cents2ratio{
  t_object      x_obj;
  t_outlet     *x_outlet;
  t_int         x_bytes;
  t_atom       *x_atom_list;
  t_float       x_f;
}t_cents2ratio;

static t_float convert(t_float f){
    return pow(2, (f/1200));
}

static void cents2ratio_float(t_cents2ratio *x, t_floatarg f){
  outlet_float(x->x_outlet, convert(x->x_f = f));
}

static void cents2ratio_list(t_cents2ratio *x, t_symbol *s, int argc, t_atom *argv){
    int old_bytes = x->x_bytes, i = 0;
    x->x_bytes = argc*sizeof(t_atom);
    x->x_atom_list = (t_atom *)t_resizebytes(x->x_atom_list, old_bytes, x->x_bytes);
    for(i = 0; i < argc; i++)
        SETFLOAT(x->x_atom_list+i, convert(atom_getfloatarg(i, argc, argv)));
    outlet_list(x->x_outlet, 0, argc, x->x_atom_list);
}

static void cents2ratio_set(t_cents2ratio *x, t_float f){
  x->x_f = f;
}

static void cents2ratio_bang(t_cents2ratio *x){
  outlet_float(x->x_outlet, convert(x->x_f));
}

static void *cents2ratio_new(t_floatarg f){
    t_cents2ratio *x = (t_cents2ratio *) pd_new(cents2ratio_class);
    x->x_f = f;
    x->x_outlet = outlet_new(&x->x_obj, 0);
    x->x_bytes = sizeof(t_atom);
    x->x_atom_list = (t_atom *)getbytes(x->x_bytes);
    return(x);
}

static void cents2ratio_free(t_cents2ratio *x){
  t_freebytes(x->x_atom_list, x->x_bytes);
}

void cents2ratio_setup(void){
  cents2ratio_class = class_new(gensym("cents2ratio"), (t_newmethod)cents2ratio_new,
			  (t_method)cents2ratio_free, sizeof(t_cents2ratio), 0, A_DEFFLOAT, 0);
  class_addfloat(cents2ratio_class, (t_method)cents2ratio_float);
  class_addlist(cents2ratio_class, (t_method)cents2ratio_list);
  class_addmethod(cents2ratio_class, (t_method)cents2ratio_set, gensym("set"), A_DEFFLOAT,0 );
  class_addbang(cents2ratio_class, (t_method)cents2ratio_bang);
}
