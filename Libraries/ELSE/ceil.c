// Porres 2016
 
#include "m_pd.h"
#include <math.h>

static t_class *ceil_class;

typedef struct _ceil
{
  t_object x_obj;
  t_outlet *float_outlet;
  t_int bytes;
  t_atom *output_list;
  t_float f;
} t_ceil;

static t_float convert(t_float f);

static void ceil_float(t_ceil *x, t_floatarg f)
{
  x->f = f;
  outlet_float(x->float_outlet, convert(f));
}

static  t_float convert(t_float f)
{
  return ceil(f);
}

static void ceil_list(t_ceil *x, t_symbol *s, int argc, t_atom *argv)
{
  int old_bytes = x->bytes, i = 0;
  x->bytes = argc*sizeof(t_atom);
  x->output_list = (t_atom *)t_resizebytes(x->output_list,old_bytes,x->bytes);
  for(i=0;i<argc;i++)
    SETFLOAT(x->output_list+i,convert(atom_getfloatarg(i,argc,argv)));
  outlet_list(x->float_outlet,0,argc,x->output_list);
}

static void ceil_set(t_ceil *x, t_float f)
{
  x->f = f;
}

static void ceil_bang(t_ceil *x)
{
  outlet_float(x->float_outlet,convert(x->f));
}

static void *ceil_new(void)
{
  t_ceil *x = (t_ceil *) pd_new(ceil_class);
  x->f = 0;
  x->float_outlet = outlet_new(&x->x_obj, 0);
  x->bytes = sizeof(t_atom);
  x->output_list = (t_atom *)getbytes(x->bytes);
  if(x->output_list==NULL) {
    pd_error(x,"ceil: memory allocation failure");
    return NULL;
  }
  return (x);
}

static void ceil_free(t_ceil *x)
{
  t_freebytes(x->output_list,x->bytes);
}

void ceil_setup(void)
{
  ceil_class = class_new(gensym("ceil"), (t_newmethod)ceil_new,
			  (t_method)ceil_free,sizeof(t_ceil), 0, 0);
  class_addfloat(ceil_class,(t_method)ceil_float);
  class_addlist(ceil_class,(t_method)ceil_list);
  class_addmethod(ceil_class,(t_method)ceil_set,gensym("set"),A_DEFFLOAT,0);
  class_addbang(ceil_class,(t_method)ceil_bang);
}
