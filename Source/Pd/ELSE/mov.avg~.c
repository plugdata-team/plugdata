#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAVG_MAXBUF         192000000   // max buffer size - undocumented
#define MAVG_DEF_BUFSIZE        100         // default size

typedef struct _mavg{
    t_object        x_obj;
    t_inlet        *x_inlet_n;                  // inlet for n samples
    unsigned int    x_last_n;                   // last # of samples for moving average
    unsigned int    x_count;                    // for 1st round of accumulation
    double          x_accum;                    // accumulation
    double         *x_buf;                      // buffer pointer
    double          x_stack[MAVG_DEF_BUFSIZE];  // buffer
    int             x_alloc;                    // if x_buf is allocated or stack ?????
    int             x_abs;
    unsigned int    x_size;                     // allocated size for x_buf
    unsigned int    x_bufrd;                    // buffer readhead
}t_mavg;

static t_class *mavg_class;

static void mavg_clear(t_mavg * x){ // clear buffer and reset things to 0
    x->x_count = x->x_accum = x->x_bufrd = 0;;
    for(unsigned int i = 0; i < x->x_size; i++)
        x->x_buf[i] = 0.;
};

static void mavg_abs(t_mavg *x, t_float f){
    x->x_abs = f != 0;
}

static void mavg_size(t_mavg *x, t_float f){ // deals with allocation issues if needed
    unsigned int cursz = x->x_size;                     // current size
    unsigned int newsz = f < 1 ? 1 : (unsigned int)f;   // new requested size
    if(newsz > MAVG_MAXBUF)
        newsz = MAVG_MAXBUF;
    if(!x->x_alloc && newsz > MAVG_DEF_BUFSIZE){  // allocate
        x->x_buf = (double *)malloc(sizeof(double)*newsz);
        x->x_alloc = 1;
    }
    else if(x->x_alloc && newsz > cursz) // reallocate
        x->x_buf = (double *)realloc(x->x_buf, sizeof(double)*newsz);
    else if(x->x_alloc && newsz < MAVG_DEF_BUFSIZE){ // reset
        free(x->x_buf);
        x->x_size = MAVG_DEF_BUFSIZE;
        x->x_buf = x->x_stack;
        x->x_alloc = 0;
    };
    x->x_size = newsz;
    mavg_clear(x);
}

static t_int *mavg_perform(t_int *w){
    t_mavg *x = (t_mavg *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    float last_n = x->x_last_n;
    for(int i = 0; i < nblock; i++){
        double result, input = (double)in1[i];
        float in_samples = in2[i];
        if(x->x_abs)
            input = fabs(input);
        if(in_samples < 1)
            in_samples = 1;
        unsigned int n = in_samples;
        if(n > x->x_size)
            n = x->x_size;
        if(n != last_n)
            mavg_clear(x);
        if(n > 1){ // do average
            x->x_accum += input;                // accumulate
            if(x->x_count < n)       // update count
                x->x_count++;
            else // subtract first sample out of bounds from the moving sum
                x->x_accum -= x->x_buf[x->x_bufrd];
            x->x_buf[x->x_bufrd++] = input; // store input, increment bufrd
            if(x->x_bufrd >= n) // loop bufrd
                x->x_bufrd = 0;
            result = x->x_accum/(double)n; // get average
        }
        else // npoints = 1, just pass through
            result = input;
        out[i] = result;
        last_n = n;
    };
    x->x_last_n = last_n;
    return(w + 6);
}

static void mavg_dsp(t_mavg *x, t_signal **sp){
    dsp_add(mavg_perform, 5, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec);
}

static void mavg_free(t_mavg *x){
    if(x->x_alloc)
        free(x->x_buf);
}

static void *mavg_new(t_symbol *s, int argc, t_atom * argv){
    t_mavg *x = (t_mavg *)pd_new(mavg_class);
    t_symbol *dummy = s;
    dummy = NULL;
// default buf / size / n
    x->x_buf = x->x_stack;
    x->x_size = MAVG_DEF_BUFSIZE;
    float n_arg = 1;
    x->x_alloc = 0.;
    x->x_abs = 0.;
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
            else if(!strcmp(cursym->s_name, "-abs")){
                x->x_abs = 1;
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
    mavg_size(x, (float)x->x_size); // set size and allocate x_buf if necessary
    x->x_inlet_n = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_n, n_arg);
    outlet_new((t_object *)x, &s_signal);
    return (x);
errstate:
    pd_error(x, "[mov.avg~]: improper args");
    return NULL;
}

void setup_mov0x2eavg_tilde(void){
    mavg_class = class_new(gensym("mov.avg~"), (t_newmethod)mavg_new,
            (t_method)mavg_free, sizeof(t_mavg), 0, A_GIMME, 0);
    class_addmethod(mavg_class, (t_method) mavg_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(mavg_class, nullfn, gensym("signal"), 0);
    class_addmethod(mavg_class, (t_method)mavg_clear, gensym("clear"), 0);
    class_addmethod(mavg_class, (t_method)mavg_size, gensym("size"), A_DEFFLOAT, 0);
    class_addmethod(mavg_class, (t_method)mavg_abs, gensym("abs"), A_DEFFLOAT, 0);
}
