/* ---------------- rms~ - rmselope follower. ----------------- */
/* based on msp's rms~-object: outputs both linear and dBFS rms */

#include "m_pd.h"
#include "math.h"

#define MAXOVERLAP 32
#define INITVSTAKEN 64
#define LOGTEN 2.302585092994

typedef struct sigrms{
    t_object x_obj;                 /* header */
    void *x_outlet;                 /* a "float" outlet */
    void *x_clock;                  /* a "clock" object */
    t_sample *x_buf;                   /* a Hanning window */
    int x_phase;                    /* number of points since last output */
    int x_period;                   /* requested period of output */
    int x_realperiod;               /* period rounded up to vecsize multiple */
    int x_npoints;                  /* analysis window size in samples */
    t_float x_result;                 /* result to output */
    t_sample x_sumbuf[MAXOVERLAP];     /* summing buffer */
    int x_allocforvs;               /* extra buffer for DSP vector size */
    int x_block; // block size
    int x_db;
}t_sigrms;

t_class *rms_tilde_class;
static void rms_tilde_tick(t_sigrms *x);

static void rms_db(t_sigrms *x){
    x->x_db = 1;
}

static void rms_linear(t_sigrms *x){
    x->x_db = 0;
}

static void rms_set(t_sigrms *x, t_floatarg f1, t_floatarg f2){
    t_sample *buf;
    int i;
    int size = f1;
    if (size < 1) size = 1024;
    else if (size < x->x_block) size = x->x_block;
    int hop = f2;
    if (hop < 1)
        hop = size/2;
    if (hop < size / MAXOVERLAP + 1)
        hop = size / MAXOVERLAP + 1;
    if (hop < x->x_block)
        hop = x->x_block;
    if (!(buf = getbytes(sizeof(t_sample) * (size + INITVSTAKEN))))
        pd_error(x, "rms: couldn't allocate buffer");
    x->x_buf = buf;
    x->x_phase = 0;
    x->x_npoints = size;
    x->x_period = hop;
// from dsp part 1
    if(x->x_period % x->x_block) x->x_realperiod =
        x->x_period + x->x_block - (x->x_period % x->x_block);
    else
        x->x_realperiod = x->x_period;
// buffer
    for(i = 0; i < MAXOVERLAP; i++)
        x->x_sumbuf[i] = 0;
    for(i = 0; i < size; i++)
        buf[i] = (1. - cos((2 * 3.14159 * i) / size))/size;
    for(; i < size+INITVSTAKEN; i++)
        buf[i] = 0;
}


static void *rms_tilde_new(t_symbol *s, int argc, t_atom *argv){
    t_sigrms *x = (t_sigrms *)pd_new(rms_tilde_class);
    t_symbol *dummy = s;
    dummy = NULL;
    int npoints = 0;
    int period = 0;
    int dbstate = 0;
/////////////////////////////////////////////////////////////////////////////////////
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){ //if current argument is a float
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
        else if(argv->a_type == A_SYMBOL && !argnum){
            if(atom_getsymbolarg(0, argc, argv) == gensym("-db")){
                dbstate = 1;
                argc--, argv++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    };
/////////////////////////////////////////////////////////////////////////////////////
    t_sample *buf;
    int i;
    if(npoints < 1)
        npoints = 1024;
    if(period < 1) period = npoints/2;
    if(period < npoints / MAXOVERLAP + 1)
        period = npoints / MAXOVERLAP + 1;
    if(!(buf = getbytes(sizeof(t_sample) * (npoints + INITVSTAKEN)))){
        pd_error(x, "[rms]: couldn't allocate buffer");
        return(NULL);
    }
    x->x_buf = buf;
    x->x_npoints = npoints;
    x->x_phase = 0;
    x->x_db = dbstate;
    x->x_period = period;
    for(i = 0; i < MAXOVERLAP; i++) x->x_sumbuf[i] = 0;
    for(i = 0; i < npoints; i++)
        buf[i] = (1. - cos((2 * 3.14159 * i) / npoints))/npoints; // HANNING / npoints
    for(; i < npoints+INITVSTAKEN; i++) buf[i] = 0;
    x->x_clock = clock_new(x, (t_method)rms_tilde_tick);
    x->x_outlet = outlet_new(&x->x_obj, gensym("float"));
    x->x_allocforvs = INITVSTAKEN;
    return (x);
errstate:
    pd_error(x, "[rms~]: improper args");
    return(NULL);
}

static t_int *rms_tilde_perform(t_int *w){
    t_sigrms *x = (t_sigrms *)(w[1]);
    t_sample *in = (t_sample *)(w[2]); // input
    int n = (int)(w[3]); // block
    int count;
    t_sample *sump; // defined sum variable
    in += n;
    for(count = x->x_phase, sump = x->x_sumbuf; // sum it up
         count < x->x_npoints; count += x->x_realperiod, sump++)
    {
        t_sample *hp = x->x_buf + count;
        t_sample *fp = in;
        t_sample sum = *sump;
        int i;
        for(i = 0; i < n; i++){
            fp--;
            sum += *hp++ * (*fp * *fp); // sum = hp * inˆ2
        }
        *sump = sum; // sum
    }
    sump[0] = 0;
    x->x_phase -= n;
    if(x->x_phase < 0){ // get result and reset
        x->x_result = x->x_sumbuf[0];
        for(count = x->x_realperiod, sump = x->x_sumbuf; count < x->x_npoints;
        count += x->x_realperiod, sump++)
            sump[0] = sump[1];
        sump[0] = 0;
        x->x_phase = x->x_realperiod - n;
        clock_delay(x->x_clock, 0L); // output?
    }
    return(w+4);
}

static void rms_tilde_dsp(t_sigrms *x, t_signal **sp){
   x->x_block = sp[0]->s_n;
    if(x->x_period % sp[0]->s_n) x->x_realperiod =
        x->x_period + sp[0]->s_n - (x->x_period % sp[0]->s_n);
    else
        x->x_realperiod = x->x_period;
    if(sp[0]->s_n > x->x_allocforvs){
        void *xx = resizebytes(x->x_buf,
            (x->x_npoints + x->x_allocforvs) * sizeof(t_sample),
            (x->x_npoints + sp[0]->s_n) * sizeof(t_sample));
        if(!xx){
            pd_error(x, "[rms~]: out of memory");
            return;
        }
        x->x_buf = (t_sample *)xx;
        x->x_allocforvs = sp[0]->s_n;
    }
    dsp_add(rms_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
}

static float pow2db(t_float f){
    if(f <= 0)
        return(-999);
    else if(f == 1)
        return(0);
    else{
        float val = log(f) * 10./LOGTEN ;
        return(val < -999 ? -999 : val);
    }
}

static void rms_tilde_tick(t_sigrms *x){ // clock callback function
    if(x->x_db)
        outlet_float(x->x_outlet, pow2db(x->x_result));
    else
        outlet_float(x->x_outlet, sqrtf(x->x_result));
}
                              
static void rms_tilde_free(t_sigrms *x){  // cleanup
    clock_free(x->x_clock);
    freebytes(x->x_buf, (x->x_npoints + x->x_allocforvs) * sizeof(*x->x_buf));
}

void rms_tilde_setup(void ){
    rms_tilde_class = class_new(gensym("rms~"), (t_newmethod)rms_tilde_new,
        (t_method)rms_tilde_free, sizeof(t_sigrms), 0, A_GIMME, 0);
    class_addmethod(rms_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(rms_tilde_class, (t_method)rms_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(rms_tilde_class, (t_method)rms_set, gensym("set"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(rms_tilde_class, (t_method)rms_db, gensym("db"), 0);
    class_addmethod(rms_tilde_class, (t_method)rms_linear, gensym("linear"), 0);
}
