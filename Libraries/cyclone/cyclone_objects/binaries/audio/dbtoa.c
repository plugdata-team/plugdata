/*
  Copyright (c) 2016 Marco Matteo Markidis
  mm.markidis@gmail.com

  For information on usage and redistribution, and for a DISCLAIMER OF ALL
  WARRANTIES, see the file, "LICENSE.txt," in this distribution.

  Made while listening:
  Evan Parker Electro-Acoustic Ensemble -- Hasselt
*/
#include "m_pd.h"
#include <common/api.h>
#include <math.h>

static t_class *dbtoa_class;

typedef struct _dbtoa {
  t_object x_obj;
  t_inlet *x_inlet;
  t_outlet *x_outlet;
} t_dbtoa;

static t_int * dbtoa_perform(t_int *w)
{
  t_dbtoa *x = (t_dbtoa *)(w[1]);
  int n = (int)(w[2]);
  t_float *in = (t_float *)(w[3]);
  t_float *out = (t_float *)(w[4]);
    
  while(n--)
    *out++ = pow(10.,*in++/20);
  return (w + 5);
}


static void dbtoa_dsp(t_dbtoa *x, t_signal **sp)
{
  dsp_add(dbtoa_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *dbtoa_new(void)
{
  t_dbtoa *x = (t_dbtoa *)pd_new(dbtoa_class);
  x->x_outlet = outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

CYCLONE_OBJ_API void dbtoa_tilde_setup(void) {
  dbtoa_class = class_new(gensym("dbtoa~"),
			  (t_newmethod) dbtoa_new,
			  0,
			  sizeof (t_dbtoa),
			  CLASS_DEFAULT,
			  0);
  class_addmethod(dbtoa_class, nullfn, gensym("signal"), 0);
  class_addmethod(dbtoa_class, (t_method) dbtoa_dsp, gensym("dsp"), A_CANT, 0);
}

