// Porres 2016
 
#include "m_pd.h"
#include <math.h>

static t_class *ratio2cents_class;

typedef struct _ratio2cents
{
  t_object x_obj;
  t_outlet *float_outlet;
  t_int bytes;
  t_atom *output_list;
  t_float f;
} t_ratio2cents;

static void *ratio2cents_new(t_floatarg f1);
static void ratio2cents_free(t_ratio2cents *x);
static void ratio2cents_float(t_ratio2cents *x, t_floatarg f);
static void ratio2cents_bang(t_ratio2cents *x);
static void ratio2cents_set(t_ratio2cents *x, t_floatarg f);

static t_float convert(t_float f);

static void ratio2cents_float(t_ratio2cents *x, t_floatarg f)
{
  x->f = f;
  outlet_float(x->float_outlet, convert(f));
}

static t_float convert(t_float f)
{
    if(f<0.f)
    f = 0.f;
    return log2(f) * 1200;
}

static void ratio2cents_list(t_ratio2cents *x, t_symbol *s, int argc, t_atom *argv)
{
  int old_bytes = x->bytes, i = 0;
  x->bytes = argc*sizeof(t_atom);
  x->output_list = (t_atom *)t_resizebytes(x->output_list,old_bytes,x->bytes);
  for(i=0;i<argc;i++)
    SETFLOAT(x->output_list+i,convert(atom_getfloatarg(i,argc,argv)));
  outlet_list(x->float_outlet,0,argc,x->output_list);
}

static void ratio2cents_set(t_ratio2cents *x, t_float f)
{
  x->f = f;
}

static void ratio2cents_bang(t_ratio2cents *x)
{
  outlet_float(x->float_outlet,convert(x->f));
}

static void *ratio2cents_new(t_floatarg f1)
{
  t_ratio2cents *x = (t_ratio2cents *) pd_new(ratio2cents_class);
  x->f = f1 == 0 ? 1: f1;
  x->float_outlet = outlet_new(&x->x_obj, 0);
  x->bytes = sizeof(t_atom);
  x->output_list = (t_atom *)getbytes(x->bytes);
  return (x);
}

static void ratio2cents_free(t_ratio2cents *x)
{
  t_freebytes(x->output_list,x->bytes);
}

void ratio2cents_setup(void)
{
  ratio2cents_class = class_new(gensym("ratio2cents"), (t_newmethod)ratio2cents_new,
			  (t_method)ratio2cents_free,sizeof(t_ratio2cents),0, A_DEFFLOAT, 0);
  class_addfloat(ratio2cents_class,(t_method)ratio2cents_float);
  class_addlist(ratio2cents_class,(t_method)ratio2cents_list);
  class_addmethod(ratio2cents_class,(t_method)ratio2cents_set,gensym("set"),A_DEFFLOAT,0);
  class_addbang(ratio2cents_class,(t_method)ratio2cents_bang);
}
