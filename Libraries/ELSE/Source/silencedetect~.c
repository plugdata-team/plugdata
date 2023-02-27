/**
 *
 * a puredata wrapper for silence
 *
 * Thanks to Johannes M Zmolnig for writing the excellent HOWTO:
 *       http://iem.kug.ac.at/pd/externals-HOWTO/
 *
 * */

#include <m_pd.h>
#include <aubio/src/aubio.h>

static t_class *silence_tilde_class;

void silencedetect_tilde_setup (void);

typedef struct _silence_tilde
{
  t_object x_obj;
  t_float silence;
  t_int pos; /*frames%dspblocksize*/
  t_int bufsize;
  t_int hopsize;
  t_int wassilence;
  t_int issilence;
  fvec_t *vec;
  t_outlet *silencebang;
  t_outlet *noisybang;
} t_silence_tilde;

static t_int *silence_tilde_perform(t_int *w)
{
  t_silence_tilde *x = (t_silence_tilde *)(w[1]);
  t_sample *in          = (t_sample *)(w[2]);
  int n                 = (int)(w[3]);
  int j;
  for (j=0;j<n;j++) {
    /* write input to datanew */
    fvec_set_sample(x->vec, in[j], x->pos);
    /*time for fft*/
    if (x->pos == x->hopsize-1) {
      /* block loop */
      if (aubio_silence_detection(x->vec, x->silence)==1) {
        if (x->wassilence==1) {
          x->issilence = 1;
        } else {
          x->issilence = 2;
          outlet_bang(x->silencebang);
        }
        x->wassilence=1;
      } else {
        if (x->wassilence<=0) {
          x->issilence = 0;
        } else {
          x->issilence = -1;
          outlet_bang(x->noisybang);
        }
        x->wassilence=0;
      }
      /* end of block loop */
      x->pos = -1; /* so it will be zero next j loop */
    }
    x->pos++;
  }
  return (w+4);
}

static void silence_tilde_dsp(t_silence_tilde *x, t_signal **sp)
{
  dsp_add(silence_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void *silence_tilde_new (t_floatarg f)
{
  t_silence_tilde *x = (t_silence_tilde *)pd_new(silence_tilde_class);

  x->silence = (f < -1000.) ? -70 : (f >= 0.) ? -70. : f;
  x->bufsize   = 1024;
  x->hopsize   = x->bufsize / 2;

  x->vec = (fvec_t *)new_fvec(x->hopsize);
  x->wassilence = 1;

  floatinlet_new (&x->x_obj, &x->silence);
  x->silencebang = outlet_new (&x->x_obj, &s_bang);
  x->noisybang = outlet_new (&x->x_obj, &s_bang);
  return (void *)x;
}

void silencedetect_tilde_setup (void)
{
  silence_tilde_class = class_new (gensym ("silencedetect~"),
      (t_newmethod)silence_tilde_new,
      0, sizeof (t_silence_tilde),
      CLASS_DEFAULT, A_DEFFLOAT, 0);
  class_addmethod(silence_tilde_class,
      (t_method)silence_tilde_dsp,
      gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(silence_tilde_class,
      t_silence_tilde, silence);
}
