// Porres 2016
 
#include "m_pd.h"
#include <math.h>

static t_class *floor_class;

typedef struct _floor
{
  t_object x_obj;
  t_outlet *float_outlet;
  t_int bytes;
  t_atom *output_list;
  t_float f;
} t_floor;

static t_float convert(t_float f);

static void floor_float(t_floor *x, t_floatarg f)
{
  x->f = f;
  outlet_float(x->float_outlet, convert(f));
}

static t_float convert(t_float f)
{
  return floor(f);
}

static void floor_list(t_floor *x, t_symbol *s, int argc, t_atom *argv)
{
  int old_bytes = x->bytes, i = 0;
  x->bytes = argc*sizeof(t_atom);
  x->output_list = (t_atom *)t_resizebytes(x->output_list,old_bytes,x->bytes);
  for(i=0;i<argc;i++)
    SETFLOAT(x->output_list+i,convert(atom_getfloatarg(i,argc,argv)));
  outlet_list(x->float_outlet,0,argc,x->output_list);
}

static void floor_set(t_floor *x, t_float f)
{
  x->f = f;
}

static void floor_bang(t_floor *x)
{
  outlet_float(x->float_outlet,convert(x->f));
}

static void *floor_new(void)
{
  t_floor *x = (t_floor *) pd_new(floor_class);
  x->f = 0;
  x->float_outlet = outlet_new(&x->x_obj, 0);
  x->bytes = sizeof(t_atom);
  x->output_list = (t_atom *)getbytes(x->bytes);
  if(x->output_list==NULL) {
    pd_error(x,"floor: memory allocation failure");
    return NULL;
  }
  return (x);
}

static void floor_free(t_floor *x)
{
  t_freebytes(x->output_list,x->bytes);
}

void floor_setup(void)
{
  floor_class = class_new(gensym("floor"), (t_newmethod)floor_new,
			  (t_method)floor_free,sizeof(t_floor), 0, 0);
  class_addfloat(floor_class,(t_method)floor_float);
  class_addlist(floor_class,(t_method)floor_list);
  class_addmethod(floor_class,(t_method)floor_set,gensym("set"),A_DEFFLOAT,0);
  class_addbang(floor_class,(t_method)floor_bang);
}
