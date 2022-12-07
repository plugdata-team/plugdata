/*
 Copyright (c) 2016 Marco Matteo Markidis
 mm.markidis@gmail.com

 For information on usage and redistribution, and for a DISCLAIMER OF ALL
 WARRANTIES, see the file, "LICENSE.txt," in this distribution.

 Made while listening:
 Disclosure -- Caracal
 */
#include "m_pd.h"
#include <common/api.h>
#include <math.h>

static t_class *atodb_class;

typedef struct _atodb
{
  t_object x_obj;
  t_outlet *float_outlet;
  t_int bytes;
  t_atom *output_list;
  t_float f;
} t_atodb;


static void *atodb_new(void);
static void atodb_free(t_atodb *x);
static void atodb_float(t_atodb *x, t_floatarg f);
static void atodb_bang(t_atodb *x);
static void atodb_set(t_atodb *x, t_floatarg f);

static t_float convert(t_float f);

static void atodb_float(t_atodb *x, t_floatarg f)
{
  x->f = f;
  outlet_float(x->float_outlet, convert(f));
}

static t_float convert(t_float f)
{
  if(f<0.f)
    f = 0.f;
  return 20*log10(f);
}

static void atodb_list(t_atodb *x, t_symbol *s, int argc, t_atom *argv)
{
  int old_bytes = x->bytes, i = 0;
  x->bytes = argc*sizeof(t_atom);
  x->output_list = (t_atom *)t_resizebytes(x->output_list,old_bytes,x->bytes);
  for(i=0;i<argc;i++)
    SETFLOAT(x->output_list+i,convert(atom_getfloatarg(i,argc,argv)));
  outlet_list(x->float_outlet,0,argc,x->output_list);
}

static void atodb_set(t_atodb *x, t_float f)
{
  x->f = f;
}

static void atodb_bang(t_atodb *x)
{
  outlet_float(x->float_outlet,convert(x->f));
}


static void *atodb_new(void)
{
  t_atodb *x = (t_atodb *) pd_new(atodb_class);

  x->float_outlet = outlet_new(&x->x_obj, 0);
  x->bytes = sizeof(t_atom);
  x->output_list = (t_atom *)getbytes(x->bytes);
  if(x->output_list==NULL) {
    pd_error(x,"atodb: memory allocation failure");
    return NULL;
  }
  return (x);
}

static void atodb_free(t_atodb *x)
{
  t_freebytes(x->output_list,x->bytes);
}

CYCLONE_OBJ_API void atodb_setup(void)
{
  atodb_class = class_new(gensym("atodb"), (t_newmethod)atodb_new,
			  (t_method)atodb_free,sizeof(t_atodb),0,0);

  class_addfloat(atodb_class,(t_method)atodb_float);
  class_addlist(atodb_class,(t_method)atodb_list);
  class_addmethod(atodb_class,(t_method)atodb_set,gensym("set"),A_DEFFLOAT,0);
  class_addbang(atodb_class,(t_method)atodb_bang);
}
