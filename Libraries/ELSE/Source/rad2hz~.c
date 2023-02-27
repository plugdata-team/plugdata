// Porres 2016

#include "m_pd.h"
#include <math.h>

#define TWO_PI (2 * 3.14159265358979323846)

static t_class *rad2hz_class;

typedef struct _rad2hz {
  t_object x_obj;
  t_float   x_iradps;
  t_outlet *x_outlet;
} t_rad2hz;

void *rad2hz_new(void);
static t_int * rad2hz_perform(t_int *w);
static void rad2hz_dsp(t_rad2hz *x, t_signal **sp);

static t_int * rad2hz_perform(t_int *w)
{
  t_rad2hz *x = (t_rad2hz *)(w[1]);
  int n = (int)(w[2]);
  t_float *in = (t_float *)(w[3]);
  t_float *out = (t_float *)(w[4]);
  t_float iradps = x->x_iradps;
  while(n--)
*out++ = *in++ * iradps;
  return (w + 5);
}

static void rad2hz_dsp(t_rad2hz *x, t_signal **sp)
{
    x->x_iradps = sp[0]->s_sr / TWO_PI;
    dsp_add(rad2hz_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void *rad2hz_new(void)
{
  t_rad2hz *x = (t_rad2hz *)pd_new(rad2hz_class);
  x->x_iradps = sys_getsr() / TWO_PI;
  x->x_outlet = outlet_new(&x->x_obj, &s_signal);
  return (void *)x;
}

void rad2hz_tilde_setup(void) {
  rad2hz_class = class_new(gensym("rad2hz~"),
    (t_newmethod) rad2hz_new, 0, sizeof (t_rad2hz), 0, 0);
    class_addmethod(rad2hz_class, nullfn, gensym("signal"), 0);
  class_addmethod(rad2hz_class, (t_method) rad2hz_dsp, gensym("dsp"), A_CANT, 0);
}
