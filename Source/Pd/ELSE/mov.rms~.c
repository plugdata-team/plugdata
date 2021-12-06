#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MRMS_MAXBUF         192000000   // max buffer size - undocumented
#define MRMS_DEF_BUFSIZE        1024    // default size
#define LOGTEN 2.302585092994

typedef struct _mrms{
    t_object        x_obj;
    t_inlet        *x_inlet_n;                  // inlet for n samples
    unsigned int    x_last_n;                   // last # of samples for moving average
    unsigned int    x_count;                    // for 1st round of accumulation
    double          x_accum;                    // accumulation
    double         *x_buf;                      // buffer pointer
    double          x_stack[MRMS_DEF_BUFSIZE];  // buffer
    int             x_alloc;                    // if x_buf is allocated or stack ?????
    unsigned int    x_size;                     // allocated size for x_buf
    unsigned int    x_bufrd;                    // buffer readhead
    int             x_db;
}t_mrms;

static t_class *mrms_class;

static void mrms_clear(t_mrms * x){ // clear buffer and reset things to 0
    x->x_count = x->x_accum = x->x_bufrd = 0;;
    for(unsigned int i = 0; i < x->x_size; i++)
        x->x_buf[i] = 0.;
};

static void mrms_size(t_mrms *x, t_float f){ // deals with allocation issues if needed
    unsigned int cursz = x->x_size;                     // current size
    unsigned int newsz = f < 1 ? 1 : (unsigned int)f;   // new requested size
    if(newsz > MRMS_MAXBUF)
        newsz = MRMS_MAXBUF;
    if(!x->x_alloc && newsz > MRMS_DEF_BUFSIZE){  // allocate
        x->x_buf = (double *)malloc(sizeof(double)*newsz);
        x->x_alloc = 1;
    }
    else if(x->x_alloc && newsz > cursz) // reallocate
        x->x_buf = (double *)realloc(x->x_buf, sizeof(double)*newsz);
    else if(x->x_alloc && newsz < MRMS_DEF_BUFSIZE){ // reset
        free(x->x_buf);
        x->x_size = MRMS_DEF_BUFSIZE;
        x->x_buf = x->x_stack;
        x->x_alloc = 0;
    };
    x->x_size = newsz;
    mrms_clear(x);
}

static void mrms_db(t_mrms *x){
    x->x_db = 1;
}

static void mrms_linear(t_mrms *x){
    x->x_db = 0;
}

static t_int *mrms_perform(t_int *w){
    t_mrms *x = (t_mrms *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    float last_n = x->x_last_n;
    for(int i = 0; i < nblock; i++){
        double result, input = (double)in1[i];
        double squared = input * input;
        float in_samples = in2[i];
        if(in_samples < 1)
            in_samples = 1;
        unsigned int n = in_samples;
        if(n > x->x_size)
            n = x->x_size;
        if(n != last_n)
            mrms_clear(x);
        x->x_accum += squared;   // accumulate
        if(x->x_count < n)       // update count
            x->x_count++;
        else // subtract first sample out of bounds from the moving sum
            x->x_accum -= x->x_buf[x->x_bufrd];
        x->x_buf[x->x_bufrd++] = squared; // store input, increment bufrd
        if(x->x_bufrd >= n) // loop bufrd
            x->x_bufrd = 0;
        result = x->x_accum/(double)n; // get average
        if(result <= 0)
            result = 0;
        else
            result = sqrt(result);  // get rms
        if(x->x_db){ // convert to db
            result = 20. * log(result)/LOGTEN;
            if(result < -999)
                result = -999;
        }
        out[i] = result;
        last_n = n;
    };
    x->x_last_n = last_n;
    return(w + 6);
}

static void mrms_dsp(t_mrms *x, t_signal **sp){
    dsp_add(mrms_perform, 5, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec);
}

static void mrms_free(t_mrms *x){
    if(x->x_alloc)
        free(x->x_buf);
}

static void *mrms_new(t_symbol *s, int argc, t_atom * argv){
    t_mrms *x = (t_mrms *)pd_new(mrms_class);
// default buf / size / n
    x->x_buf = x->x_stack;
    x->x_size = MRMS_DEF_BUFSIZE;
    float n_arg = 1;
    x->x_alloc = 0;
    x->x_db = 0;
/////////////////////////////////////////////////////////////////////////////////
    int symarg = 0;
    while(argc > 0){
        if(argv->a_type == A_SYMBOL){
            if(!symarg)
                symarg = 1;
            t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(cursym->s_name, "-size")){
                if(argc >= 2 && (argv+1)->a_type == A_FLOAT){
                    t_float curfloat = atom_getfloatarg(1, argc, argv);
                    x->x_size = (int)curfloat;
                    argc -= 2;
                    argv += 2;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-db")){
                x->x_db = 1;
                argc--;
                argv++;
            }
            else
                goto errstate;
        }
        else if(argv->a_type == A_FLOAT && !symarg){
            n_arg = (int)atom_getfloatarg(0, argc, argv);
            n_arg = (n_arg < 1 ? 1 : n_arg);
            x->x_size = (unsigned int)n_arg;
            argc--;
            argv++;
        }
        else
            goto errstate;
    };
/////////////////////////////////////////////////////////////////////////////////
    mrms_size(x, (float)x->x_size); // set size and allocate x_buf if necessary
    x->x_inlet_n = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_n, n_arg);
    outlet_new((t_object *)x, &s_signal);
    return (x);
errstate:
    pd_error(x, "[mov.rms~]: improper args");
    return NULL;
}

void setup_mov0x2erms_tilde(void){
    mrms_class = class_new(gensym("mov.rms~"), (t_newmethod)mrms_new,
            (t_method)mrms_free, sizeof(t_mrms), 0, A_GIMME, 0);
    class_addmethod(mrms_class, (t_method) mrms_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(mrms_class, nullfn, gensym("signal"), 0);
    class_addmethod(mrms_class, (t_method)mrms_clear, gensym("clear"), 0);
    class_addmethod(mrms_class, (t_method)mrms_size, gensym("size"), A_DEFFLOAT, 0);
    class_addmethod(mrms_class, (t_method)mrms_db, gensym("db"), 0);
    class_addmethod(mrms_class, (t_method)mrms_linear, gensym("linear"), 0);
}
