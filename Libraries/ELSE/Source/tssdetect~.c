/**
 *
 * a puredata wrapper for aubio tss detection functions
 *
 * Thanks to Johannes M Zmolnig for writing the excellent HOWTO:
 *       http://iem.kug.ac.at/pd/externals-HOWTO/
 *
 * */

#include <m_pd.h>
#define AUBIO_UNSTABLE 1
#include <aubio/src/aubio.h>

static t_class *tss_tilde_class;

void tss_tilde_setup (void);

typedef struct _tss_tilde
{
  t_object x_obj;
  t_float thres;
  t_int pos; /*frames%dspblocksize*/
  t_int bufsize;
  t_int hopsize;
  t_inlet *inlet;
  t_outlet *outlet_trans;
  t_outlet *outlet_stead;
  aubio_pvoc_t * pv;
  aubio_pvoc_t * pvt;
  aubio_pvoc_t * pvs;
  aubio_tss_t * tss;
  fvec_t *vec;
  cvec_t *fftgrain;
  cvec_t *cstead;
  cvec_t *ctrans;
  fvec_t *trans;
  fvec_t *stead;
} t_tss_tilde;

static t_int *tss_tilde_perform(t_int *w)
{
  t_tss_tilde *x = (t_tss_tilde *)(w[1]);
  t_sample *in          = (t_sample *)(w[2]);
  t_sample *outtrans    = (t_sample *)(w[3]);
  t_sample *outstead    = (t_sample *)(w[4]);
  int n                 = (int)(w[5]);
  int j;
  for (j=0;j<n;j++) {
    /* write input to datanew */
    fvec_set_sample(x->vec, in[j], x->pos);
    /*time for fft*/
    if (x->pos == x->hopsize-1) {
      /* block loop */
      /* test for silence */
      //if (!aubio_silence_detection(x->vec, x->threshold2))
      aubio_pvoc_do  (x->pv,  x->vec, x->fftgrain);
      aubio_tss_set_threshold ( x->tss, x->thres);
      aubio_tss_do   (x->tss, x->fftgrain, x->ctrans, x->cstead);
      aubio_pvoc_rdo (x->pvt, x->ctrans, x->trans);
      aubio_pvoc_rdo (x->pvs, x->cstead, x->stead);
      //}
      /* end of block loop */
      x->pos = -1; /* so it will be zero next j loop */
    }
    x->pos++;
    *outtrans++ = x->trans->data[x->pos];
    *outstead++ = x->stead->data[x->pos];
  }
  return (w+6);
}

static void tss_tilde_dsp(t_tss_tilde *x, t_signal **sp)
{
  dsp_add(tss_tilde_perform, 5, x,
      sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void *tss_tilde_new (t_floatarg f)
  //, t_floatarg bufsize)
{
  t_tss_tilde *x =
    (t_tss_tilde *)pd_new(tss_tilde_class);

  x->thres    = (f < 1e-5) ? 0.01 : (f > 1.) ? 1. : f;
  x->bufsize  = 1024; //(bufsize < 64) ? 1024: (bufsize > 16385) ? 16385: bufsize;
  x->hopsize  = x->bufsize / 4;

  x->vec = (fvec_t *)new_fvec(x->hopsize);

  x->fftgrain  = (cvec_t *)new_cvec(x->bufsize);
  x->ctrans = (cvec_t *)new_cvec(x->bufsize);
  x->cstead = (cvec_t *)new_cvec(x->bufsize);

  x->trans = (fvec_t *)new_fvec(x->hopsize);
  x->stead = (fvec_t *)new_fvec(x->hopsize);

  x->pv  = (aubio_pvoc_t *)new_aubio_pvoc(x->bufsize, x->hopsize);
  x->pvt = (aubio_pvoc_t *)new_aubio_pvoc(x->bufsize, x->hopsize);
  x->pvs = (aubio_pvoc_t *)new_aubio_pvoc(x->bufsize, x->hopsize);

  x->tss = (aubio_tss_t *)new_aubio_tss(x->bufsize, x->hopsize);

  x->inlet = floatinlet_new (&x->x_obj, &x->thres);
  x->outlet_trans = outlet_new(&x->x_obj, gensym("signal"));
  x->outlet_stead = outlet_new(&x->x_obj, gensym("signal"));
  return (void *)x;
}

void tss_tilde_del (t_tss_tilde *x)
{
  inlet_free(x->inlet);
  outlet_free(x->outlet_trans);
  outlet_free(x->outlet_stead);
  del_aubio_pvoc(x->pv);
  del_aubio_pvoc(x->pvt);
  del_aubio_pvoc(x->pvs);
  del_aubio_tss(x->tss);
}

void tssdetect_tilde_setup (void)
{
  tss_tilde_class = class_new (gensym ("tssdetect~"),
      (t_newmethod)tss_tilde_new,
      (t_method)tss_tilde_del,
      sizeof (t_tss_tilde),
      CLASS_DEFAULT, A_DEFFLOAT, 0);
  class_addmethod(tss_tilde_class,
      (t_method)tss_tilde_dsp,
      gensym("dsp"), 0);
  CLASS_MAINSIGNALIN(tss_tilde_class,
      t_tss_tilde, thres);
}
