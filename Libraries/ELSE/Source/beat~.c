// a pd wrapper for aubio tempo detection functions (porres 2023)

#include <m_pd.h>
#include <aubio/src/aubio.h>

static t_class *beat_tilde_class;

typedef struct _beat_tilde{
  t_object       x_obj;
  t_float        threshold;
  t_float        silence;
  t_int          pos; // frames % blocksize
  t_int          bufsize;
  t_int          hopsize;
  aubio_tempo_t *t;
  fvec_t        *vec;
  fvec_t        *output;
  t_outlet      *beatbang;
}t_beat_tilde;

static t_int *beat_tilde_perform(t_int *w){
    t_beat_tilde *x = (t_beat_tilde *)(w[1]);
    t_sample    *in = (t_sample *)(w[2]);
    int           n = (int)(w[3]);
    for(int j = 0; j < n; j++){ // write input to datanew
        fvec_set_sample(x->vec, in[j], x->pos);
        if(x->pos == x->hopsize-1){  // time for fft
            aubio_tempo_do(x->t, x->vec, x->output);
            if(x->output->data[0])
                outlet_bang(x->beatbang);
            x->pos = -1; // so it will be zero next j loop
        }
        x->pos++;
    }
    return(w+4);
}

static void beat_tilde_dsp(t_beat_tilde *x, t_signal **sp){
    dsp_add(beat_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void *beat_tilde_new (t_floatarg f){
    t_beat_tilde *x = (t_beat_tilde *)pd_new(beat_tilde_class);
    x->threshold = (f < 0.01) ? 0.01 : (f > 1.) ? 1. : f;
    x->silence = -70.;
    x->bufsize = 1024;
    x->hopsize = x->bufsize / 2;
    x->t = new_aubio_tempo("specdiff", x->bufsize, x->hopsize, (uint_t)sys_getsr());
    aubio_tempo_set_silence(x->t, x->silence);
    aubio_tempo_set_threshold(x->t, x->threshold);
    x->output = (fvec_t *)new_fvec(2);
    x->vec = (fvec_t *)new_fvec(x->hopsize);
    floatinlet_new(&x->x_obj, &x->threshold);
    x->beatbang = outlet_new(&x->x_obj, &s_bang);
    return(void *)x;
}

static void beat_tilde_del(t_beat_tilde *x){
    del_aubio_tempo(x->t);
    del_fvec(x->output);
    del_fvec(x->vec);
}

void beat_tilde_setup (void){
    beat_tilde_class = class_new (gensym ("beat~"), (t_newmethod)beat_tilde_new,
        (t_method)beat_tilde_del, sizeof (t_beat_tilde), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(beat_tilde_class, (t_method)beat_tilde_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(beat_tilde_class, t_beat_tilde, threshold);
}
