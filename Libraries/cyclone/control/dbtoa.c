/*
 Copyright (c) 2016 Marco Matteo Markidis
 mm.markidis@gmail.com

 Made while listening:
 Disclosure -- Caracal
 */
#include "m_pd.h"
#include <common/api.h>
#include <math.h>

static t_class *dbtoa_class;

typedef struct _dbtoa
{
  t_object x_obj;
  t_outlet *float_outlet;
  t_int bytes;
  t_atom *output_list;
  t_float f;
} t_dbtoa;


static void *dbtoa_new(void);
static void dbtoa_free(t_dbtoa *x);
static void dbtoa_float(t_dbtoa *x, t_floatarg f);
static void dbtoa_bang(t_dbtoa *x);
static void dbtoa_set(t_dbtoa *x, t_floatarg f);

static t_float convert(t_float f);

static void dbtoa_float(t_dbtoa *x, t_floatarg f)
{
  x->f = f;
  outlet_float(x->float_outlet, convert(f));
}

static t_float convert(t_float f)
{
  return pow(10, f / 20);
}

static void dbtoa_list(t_dbtoa *x, t_symbol *s, int argc, t_atom *argv)
{
  int old_bytes = x->bytes, i = 0;
  x->bytes = argc*sizeof(t_atom);
  x->output_list = (t_atom *)t_resizebytes(x->output_list,old_bytes,x->bytes);
  for(i=0;i<argc;i++)
    SETFLOAT(x->output_list+i,convert(atom_getfloatarg(i,argc,argv)));
  outlet_list(x->float_outlet,0,argc,x->output_list);
}

static void dbtoa_set(t_dbtoa *x, t_float f)
{
  x->f = f;
}

static void dbtoa_bang(t_dbtoa *x)
{
  outlet_float(x->float_outlet,convert(x->f));
}


static void *dbtoa_new(void)
{
  t_dbtoa *x = (t_dbtoa *) pd_new(dbtoa_class);

  x->float_outlet = outlet_new(&x->x_obj, 0);
  x->bytes = sizeof(t_atom);
  x->output_list = (t_atom *)getbytes(x->bytes);
  if(x->output_list==NULL) {
    pd_error(x,"dbtoa: memory allocation failure");
    return NULL;
  }
  return (x);
}

static void dbtoa_free(t_dbtoa *x)
{
  t_freebytes(x->output_list,x->bytes);
}

CYCLONE_OBJ_API void dbtoa_setup(void)
{
  dbtoa_class = class_new(gensym("dbtoa"), (t_newmethod)dbtoa_new,
			  (t_method)dbtoa_free,sizeof(t_dbtoa),0,0);

  class_addfloat(dbtoa_class,(t_method)dbtoa_float);
  class_addlist(dbtoa_class,(t_method)dbtoa_list);
  class_addmethod(dbtoa_class,(t_method)dbtoa_set,gensym("set"),A_DEFFLOAT,0);
  class_addbang(dbtoa_class,(t_method)dbtoa_bang);
}
