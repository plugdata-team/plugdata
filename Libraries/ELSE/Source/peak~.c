// similar to rms~ but outputs peak amplitude

#include "m_pd.h"
#include <math.h>

#define MAXOVERLAP 32
#define INITVSTAKEN 64
#define LOGTEN 2.302585092994

typedef struct sigpeak{
    t_object x_obj;                 /* header */
    void *x_outlet;                 /* a "float" outlet */
    void *x_clock;                  /* a "clock" object */
    int x_phase;                    /* number of points since last output */
    int x_period;                   /* requested period of output */
    int x_realperiod;               /* period rounded up to vecsize multiple */
    int x_npoints;                  /* analysis window size in samples */
    t_float x_result;                 /* result to output */
    int x_allocforvs;               /* extra buffer for DSP vector size */
    int x_block; // block size
    t_float   x_value;
    int x_db;
}t_sigpeak;

t_class *peak_tilde_class;
static void peak_tilde_tick(t_sigpeak *x);

static void peak_db(t_sigpeak *x){
    x->x_db = 1;
}

static void peak_linear(t_sigpeak *x){
    x->x_db = 0;
}

static void peak_set(t_sigpeak *x, t_floatarg f1, t_floatarg f2){
    int size = f1;
    if(size < 1)
        size = 1024;
    else if (size < x->x_block)
        size = x->x_block;
    int hop = f2;
    if(hop < 1)
        hop = size/2;
    if(hop < size / MAXOVERLAP + 1)
        hop = size / MAXOVERLAP + 1;
    if(hop < x->x_block)
        hop = x->x_block;
    x->x_phase = 0;
    x->x_npoints = size;
    x->x_period = hop;
    // from dsp
    if(x->x_period % x->x_block)
        x->x_realperiod = x->x_period + x->x_block - (x->x_period % x->x_block);
    else
        x->x_realperiod = x->x_period;
}


static void *peak_tilde_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_sigpeak *x;
    int npoints = 0;
    int period = 0;
    int dbstate = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    npoints = argval;
                    break;
                case 1:
                    period = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--, argv++;
        }
        else if(argv->a_type == A_SYMBOL){
            if(atom_getsymbolarg(0, argc, argv) == gensym("-db") && !argnum){
                dbstate = 1;
                argc--, argv++;
            }
            else
                goto errstate;
        }
    };
/////////////////////////////////////////////////////////////////////////////////////
    if(npoints < 1)
        npoints = 1024;
    if(period < 1)
        period = npoints/2;
    if(period < npoints / MAXOVERLAP + 1)
        period = npoints / MAXOVERLAP + 1;
    x = (t_sigpeak *)pd_new(peak_tilde_class);
    x->x_npoints = npoints;
    x->x_phase = 0;
    x->x_value = 0.;
    x->x_period = period;
    x->x_clock = clock_new(x, (t_method)peak_tilde_tick);
    x->x_outlet = outlet_new(&x->x_obj, gensym("float"));
    x->x_allocforvs = INITVSTAKEN;
    x->x_db = dbstate;
    return(x);
errstate:
    pd_error(x, "[peak~]: improper args");
    return(NULL);
}


static t_int *peak_tilde_perform(t_int *w){
    t_sigpeak *x = (t_sigpeak *)(w[1]);
    t_sample *in = (t_sample *)(w[2]); // input
    int n = (int)(w[3]); // block
    t_float p = x->x_value; // 'p' for 'peak'
    in += n;
    t_sample *f = in;
    int i;
    for(i = 0; i < n; i++){
        f--;
        if(*f > p)
            p = *f;
        else if(*f < -p)
            p = -*f;
    }
    x->x_phase -= n;
    if(x->x_phase < 0){ // get result and reset
        x->x_result = p;
        p = 0;
        x->x_phase = x->x_realperiod - n;
        clock_delay(x->x_clock, 0L); // output?
    }
    x->x_value = p;
    return(w+4);
}

static void peak_tilde_dsp(t_sigpeak *x, t_signal **sp){
   x->x_block = sp[0]->s_n;
    if(x->x_period % sp[0]->s_n) x->x_realperiod =
        x->x_period + sp[0]->s_n - (x->x_period % sp[0]->s_n);
    else
        x->x_realperiod = x->x_period;
    if(sp[0]->s_n > x->x_allocforvs)
        x->x_allocforvs = sp[0]->s_n;
    dsp_add(peak_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static float amp2db(t_float f){
    if(f <= 0)
        return(-999);
    else if(f == 1)
        return(0);
    else{
        float val = log(f) * 20./LOGTEN ;
        return(val < -999 ? -999 : val);
    }
}

static void peak_tilde_tick(t_sigpeak *x){ // clock callback function
    if(x->x_db)
        outlet_float(x->x_outlet, amp2db(x->x_result));
    else
        outlet_float(x->x_outlet, x->x_result);
}

static void peak_tilde_free(t_sigpeak *x){  // cleanup
    clock_free(x->x_clock);
}

void peak_tilde_setup(void){
    peak_tilde_class = class_new(gensym("peak~"), (t_newmethod)peak_tilde_new,
        (t_method)peak_tilde_free, sizeof(t_sigpeak), 0, A_GIMME, 0);
    class_addmethod(peak_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(peak_tilde_class, (t_method)peak_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(peak_tilde_class, (t_method)peak_set, gensym("set"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(peak_tilde_class, (t_method)peak_db, gensym("db"), 0);
    class_addmethod(peak_tilde_class, (t_method)peak_linear, gensym("linear"), 0);
}
