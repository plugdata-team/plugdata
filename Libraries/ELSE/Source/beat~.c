// a pd wrapper for aubio tempo detection functions (porres 2023)

#include <m_pd.h>
#include <aubio/src/aubio.h>

static t_class *beat_class;

int DEFAULT_WINDOW_SIZE = 1024;
int DEFAULT_HOP_SIZE = 512;
int MIN_BUFFER_SIZE = 64;

typedef struct _beat{
    t_object       x_obj;
    t_float        x_f;
    uint_t         x_sr;
    t_int          x_pos; // frames % blocksize
    t_int          x_wsize;
    t_int          x_hopsize;
    t_int          x_mode;
    aubio_tempo_t *x_t;
    fvec_t        *x_vec;
    fvec_t        *x_out;
    t_outlet      *x_bpmout;
}t_beat;

static const char* modelLabels[9] ={
    "hfc",
    "energy",
    "complex",
    "phase",
    "wphase",
    "specdiff",
    "specflux",
    "kl",
    "mkl",
};

void beat_mode(t_beat *x, t_floatarg f){
    x->x_mode = f < 0 ? 0 : f > 9 ? 9 : (int)f;
    x->x_t = new_aubio_tempo(modelLabels[x->x_mode],
        x->x_wsize, x->x_hopsize, x->x_sr);
    post("[beat~] mode = %s", modelLabels[x->x_mode]);
}

void beat_window(t_beat *x, t_floatarg f){ // window size
    int size = (int)f;
    if(size < MIN_BUFFER_SIZE)
        size = MIN_BUFFER_SIZE;
    x->x_wsize = size;
    x->x_t = new_aubio_tempo(modelLabels[x->x_mode],
        x->x_wsize, x->x_hopsize, x->x_sr);
}

void beat_hop(t_beat *x, t_floatarg f){
    x->x_hopsize = f < MIN_BUFFER_SIZE ? MIN_BUFFER_SIZE : (int)f;
    x->x_vec = (fvec_t *)new_fvec(x->x_hopsize);
    x->x_t = new_aubio_tempo(modelLabels[x->x_mode],
        x->x_wsize, x->x_hopsize, x->x_sr);
}

void beat_thresh(t_beat *x, t_floatarg f){
    float thresh = (f < 0.01) ? 0.01 : (f > 1.) ? 1. : f;
    aubio_tempo_set_threshold(x->x_t, thresh);
}

void beat_silence(t_beat *x, t_floatarg f){
    aubio_tempo_set_silence(x->x_t, f);
}

static t_int *beat_perform(t_int *w){
    t_beat *x = (t_beat *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    for(int i = 0; i < n; i++){ // write input to datanew
        fvec_set_sample(x->x_vec, in[i], x->x_pos);
        if(x->x_pos < x->x_hopsize-1)
            x->x_pos++;
        else{  // time for fft
            aubio_tempo_do(x->x_t, x->x_vec, x->x_out);
            if(x->x_out->data[0])
                outlet_float(x->x_bpmout, aubio_tempo_get_bpm(x->x_t));
            x->x_pos = 0;
        }
    }
    return(w+4);
}

static void beat_dsp(t_beat *x, t_signal **sp){
    x->x_sr = (uint_t)sys_getsr();
    dsp_add(beat_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void beat_free(t_beat *x){
    del_aubio_tempo(x->x_t);
    del_fvec(x->x_out);
    del_fvec(x->x_vec);
}

static void *beat_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_beat *x = (t_beat *)pd_new(beat_class);
    float thresh = 0.3;
    x->x_wsize = DEFAULT_WINDOW_SIZE;
    x->x_hopsize = DEFAULT_HOP_SIZE;
    int mode = 5; // specdiff
    float silence = -70;
    int floatarg = 0;
    while(ac){
        if((av)->a_type == A_SYMBOL){
            if(floatarg)
                goto errstate;
            t_symbol *sym = atom_getsymbol(av);
            ac--, av++;
            if(sym == gensym("-mode")){
                if((av)->a_type == A_FLOAT){
                    mode = (int)atom_getfloat(av);
                    mode = mode < 0 ? 0 : mode > 9 ? 9 : (int)mode;
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else if(sym == gensym("-silence")){
                if((av)->a_type == A_FLOAT){
                    silence = atom_getfloat(av);
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else{
            floatarg = 1;
            float f = atom_getfloat(av);
            thresh = (f < 0.01) ? 0.01 : (f > 1.) ? 1. : f;
            ac--, av++;
            if(ac && (av)->a_type == A_FLOAT){ // wsize
                x->x_wsize = atom_getfloat(av);
                if(x->x_wsize < MIN_BUFFER_SIZE)
                    x->x_wsize = MIN_BUFFER_SIZE;
                ac--, av++;
                if(ac && (av)->a_type == A_FLOAT){ // hop
                    x->x_hopsize = atom_getfloat(av);
                    if(x->x_hopsize < MIN_BUFFER_SIZE)
                        x->x_hopsize = MIN_BUFFER_SIZE;
                    ac--, av++;
                }
            }
        }
    }
    x->x_t = new_aubio_tempo(modelLabels[mode], x->x_wsize,
        x->x_hopsize, (uint_t)sys_getsr());
    aubio_tempo_set_threshold(x->x_t, thresh);
    aubio_tempo_set_silence(x->x_t, silence);
    x->x_out = (fvec_t *)new_fvec(2);
    x->x_vec = (fvec_t *)new_fvec(x->x_hopsize);
    x->x_bpmout = outlet_new(&x->x_obj, &s_float);
    return(void *)x;
errstate:
    pd_error(x, "[beat~]: improper args");
    return(NULL);
}

void beat_tilde_setup(void){
    beat_class = class_new(gensym("beat~"), (t_newmethod)beat_new,
        (t_method)beat_free, sizeof(t_beat), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(beat_class, t_beat, x_f);
    class_addmethod(beat_class, (t_method)beat_dsp, gensym("dsp"), 0);
    class_addmethod(beat_class, (t_method)beat_mode, gensym("mode"), A_DEFFLOAT, 0);
    class_addmethod(beat_class, (t_method)beat_window, gensym("window"), A_DEFFLOAT, 0);
    class_addmethod(beat_class, (t_method)beat_hop, gensym("hop"), A_DEFFLOAT, 0);
    class_addmethod(beat_class, (t_method)beat_thresh, gensym("thresh"), A_DEFFLOAT, 0);
    class_addmethod(beat_class, (t_method)beat_silence, gensym("silence"), A_DEFFLOAT, 0);
}
