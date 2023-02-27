// a puredata wrapper for aubio pitch detection functions, GNU license


#include <m_pd.h>
#include <aubio/src/aubio.h>
#include <string.h>

static t_class *pitch_tilde_class;

typedef struct _pitch_tilde{
    t_object x_obj;
    t_float tolerance;
    t_int pos; // frames % dspblocksize
    t_int bufsize;
    t_int hopsize;
    char_t *method;
    aubio_pitch_t *o;
    fvec_t *vec;
    fvec_t *pitchvec;
    t_outlet *pitch;
}t_pitch_tilde;

static t_int *pitch_tilde_perform(t_int *w){
    t_pitch_tilde *x = (t_pitch_tilde *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    for(int j = 0; j < n; j++){
        fvec_set_sample(x->vec, in[j], x->pos); // write input to buffer
        if(x->pos == x->hopsize-1){ // time for fft
            // block loop
            aubio_pitch_do(x->o, x->vec, x->pitchvec);
            outlet_float(x->pitch, x->pitchvec->data[0]);
            // end of block loop
            x->pos = -1; // so it will be zero next j loop
        }
        x->pos++;
    }
    return(w+4);
}

static void pitch_tilde_dsp(t_pitch_tilde *x, t_signal **sp){
    dsp_add(pitch_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static void pitch_tilde_tolerance(t_pitch_tilde *x, t_floatarg a){
    if(a > 0){
        x->tolerance = a;
        aubio_pitch_set_tolerance(x->o, x->tolerance);
    }
    else
        post("[pitch~] tolerance set to %.2f", aubio_pitch_get_tolerance(x->o));
}

static void *pitch_tilde_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_pitch_tilde *x = (t_pitch_tilde *)pd_new(pitch_tilde_class);
    x->method = "default";
    x->bufsize = 2048;
    x->hopsize = x->bufsize / 2;
    if(ac >= 2){
        if(av[1].a_type == A_FLOAT)
            x->bufsize = (uint_t)(av[1].a_w.w_float);
        ac--;
    }
    x->hopsize = x->bufsize / 2;
    if(ac == 2){
        if(av[2].a_type == A_FLOAT)
            x->hopsize = (uint_t)(av[2].a_w.w_float);
        ac--;
    }
    if(ac == 1){
        if(av[0].a_type == A_SYMBOL)
            x->method = (char *)av[0].a_w.w_symbol->s_name;
        else{
            post("[pitch~]: first argument should be a symbol");
            return NULL;
        }
    }
    x->o = new_aubio_pitch(x->method, x->bufsize, x->hopsize, (uint_t)sys_getsr());
    if(x->o == NULL)
        return NULL;
    x->vec = (fvec_t *)new_fvec(x->hopsize);
    x->pitchvec = (fvec_t *)new_fvec(1);
    x->pitch = outlet_new (&x->x_obj, &s_float);
    return(void *)x;
}

void pitch_tilde_del(t_pitch_tilde *x){
    outlet_free(x->pitch);
    del_aubio_pitch(x->o);
    del_fvec(x->vec);
    del_fvec(x->pitchvec);
}

void pitch_tilde_setup (void){
    pitch_tilde_class = class_new (gensym ("pitch~"), (t_newmethod)pitch_tilde_new,
        (t_method)pitch_tilde_del, sizeof(t_pitch_tilde), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(pitch_tilde_class, (t_method)pitch_tilde_dsp, gensym("dsp"), 0);
    class_addmethod(pitch_tilde_class, (t_method)pitch_tilde_tolerance, gensym("tolerance"), A_DEFFLOAT, 0);
    class_addmethod(pitch_tilde_class, (t_method)pitch_tilde_tolerance, gensym("tol"), A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(pitch_tilde_class, t_pitch_tilde, tolerance);
}
