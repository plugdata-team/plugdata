// Porres 2016
 
#include "m_pd.h"
#include <math.h>

#define TWO_PI (2 * 3.14159265358979323846)

static t_class *rad2hz_class;

typedef struct _rad2hz
{
  t_object x_obj;
  t_outlet *float_outlet;
  t_int bytes;
  t_atom *output_list;
  t_float f;
} t_rad2hz;

static void *rad2hz_new(t_floatarg f1);
static void rad2hz_free(t_rad2hz *x);
static void rad2hz_float(t_rad2hz *x, t_floatarg f);
static void rad2hz_bang(t_rad2hz *x);
static void rad2hz_set(t_rad2hz *x, t_floatarg f);

static t_float convert(t_rad2hz *x, t_float f);

static void rad2hz_float(t_rad2hz *x, t_floatarg f)
{
  x->f = f;
  outlet_float(x->float_outlet, convert(x,f));
}

static t_float convert(t_rad2hz *x, t_float f)
{
    t_float iradps = sys_getsr() / TWO_PI;
    return f * iradps;
}

static void rad2hz_list(t_rad2hz *x, t_symbol *s, int argc, t_atom *argv)
{
  int old_bytes = x->bytes, i = 0;
  x->bytes = argc*sizeof(t_atom);
  x->output_list = (t_atom *)t_resizebytes(x->output_list,old_bytes,x->bytes);
  for(i=0;i<argc;i++)
    SETFLOAT(x->output_list+i,convert(x,atom_getfloatarg(i,argc,argv)));
  outlet_list(x->float_outlet,0,argc,x->output_list);
}

static void rad2hz_set(t_rad2hz *x, t_float f)
{
  x->f = f;
}

static void rad2hz_bang(t_rad2hz *x)
{
  outlet_float(x->float_outlet,convert(x, x->f));
}

static void *rad2hz_new(t_floatarg f1)
{
  t_rad2hz *x = (t_rad2hz *) pd_new(rad2hz_class);
  x->f = f1;
  x->float_outlet = outlet_new(&x->x_obj, 0);
  x->bytes = sizeof(t_atom);
  x->output_list = (t_atom *)getbytes(x->bytes);
  return (x);
}

static void rad2hz_free(t_rad2hz *x)
{
  t_freebytes(x->output_list,x->bytes);
}

void rad2hz_setup(void)
{
  rad2hz_class = class_new(gensym("rad2hz"), (t_newmethod)rad2hz_new,
			  (t_method)rad2hz_free,sizeof(t_rad2hz),0, A_DEFFLOAT, 0);
  class_addfloat(rad2hz_class,(t_method)rad2hz_float);
  class_addlist(rad2hz_class,(t_method)rad2hz_list);
  class_addmethod(rad2hz_class,(t_method)rad2hz_set,gensym("set"),A_DEFFLOAT,0);
  class_addbang(rad2hz_class,(t_method)rad2hz_bang);
}
