/* ---------------- vu~ - envelope follower. ----------------- */
/* based on msp's env~ object: outputs both linear and dBFS vu */

#include "m_pd.h"
#include "math.h"

#define MAXOVERLAP 32
#define INITVSTAKEN 64
#define LOGTEN 2.302585092994

typedef struct sigvu{
    t_object    x_obj;
    void       *x_out_rms;             /* a "float" outlet */
    void       *x_out_peak;            /* a "float" outlet */
    void       *x_clock;               /* a "clock" object */
    t_sample   *x_buf;                 /* a Hanning window */
    int         x_phase;               /* number of points since last output */
    int         x_period;              /* requested period of output */
    int         x_realperiod;          /* period rounded up to vecsize multiple */
    int         x_npoints;             /* analysis window size in samples */
    t_float     x_result_rms;          /* result to output */
    t_float     x_result_peak;         /* result to output */
    t_sample    x_sumbuf[MAXOVERLAP];  /* summing buffer */
    int         x_allocforvs;          /* extra buffer for DSP vector size */
    int         x_block;               // block size
    t_float     x_value;
}t_sigvu;

t_class *vu_tilde_class;
static void vu_tilde_tick(t_sigvu *x);

static void vu_set(t_sigvu *x, t_floatarg f1, t_floatarg f2){
    t_sample *buf;
    int i;
    int size = f1;
    if (size < 1) size = 1024;
    else if (size < x->x_block) size = x->x_block;
    int hop = f2;
    if(hop < 1)
        hop = size/2;
    if(hop < size / MAXOVERLAP + 1)
        hop = size / MAXOVERLAP + 1;
    if(hop < x->x_block)
        hop = x->x_block;
    if(!(buf = getbytes(sizeof(t_sample) * (size + INITVSTAKEN))))
        pd_error(x, "[vu~]: couldn't allocate buffer");
    x->x_buf = buf;
    x->x_phase = 0;
    x->x_npoints = size;
    x->x_period = hop;
    // from dsp part 1
    if(x->x_period % x->x_block) x->x_realperiod =
        x->x_period + x->x_block - (x->x_period % x->x_block);
    else x->x_realperiod = x->x_period;
    // buffer
    for(i = 0; i < MAXOVERLAP; i++)
        x->x_sumbuf[i] = 0;
    for(i = 0; i < size; i++)
        buf[i] = (1. - cos((2 * 3.14159 * i) / size))/size;
    for(;i < size+INITVSTAKEN; i++)
        buf[i] = 0;
}

static void *vu_tilde_new(t_floatarg fnpoints, t_floatarg fperiod){
    int npoints = fnpoints;
    int period = fperiod;
    t_sigvu *x;
    t_sample *buf;
    int i;
    if(npoints < 1)
        npoints = 1024;
    if(period < 1)
        period = npoints/2;
    if(period < npoints / MAXOVERLAP + 1)
        period = npoints / MAXOVERLAP + 1;
    if(!(buf = getbytes(sizeof(t_sample) * (npoints + INITVSTAKEN)))){
        pd_error(x, "[vu~]: couldn't allocate buffer");
        return (0);
    }
    x = (t_sigvu *)pd_new(vu_tilde_class);
    x->x_buf = buf;
    x->x_npoints = npoints;
    x->x_phase = 0;
    x->x_period = period;
    for(i = 0; i < MAXOVERLAP; i++)
        x->x_sumbuf[i] = 0;
    for(i = 0; i < npoints; i++)
        buf[i] = (1. - cos((2 * 3.14159 * i) / npoints))/npoints; // HANNING / npoints
    for(; i < npoints+INITVSTAKEN; i++)
        buf[i] = 0;
    x->x_clock = clock_new(x, (t_method)vu_tilde_tick);
    x->x_out_rms = outlet_new(&x->x_obj, gensym("float"));
    x->x_out_peak = outlet_new(&x->x_obj, gensym("float"));
    x->x_allocforvs = INITVSTAKEN;
    x->x_value = 0.;
    return(x);
}

static t_int *vu_tilde_perform(t_int *w){
    t_sigvu *x = (t_sigvu *)(w[1]);
    t_sample *in = (t_sample *)(w[2]); // input
    int n = (int)(w[3]); // block
    int count;
    t_float p = x->x_value; // 'p' for 'peak'
    t_sample *sump; // defined sum variable
    in += n;
    for (count = x->x_phase, sump = x->x_sumbuf; // sum it up
         count < x->x_npoints; count += x->x_realperiod, sump++){
            t_sample *hp = x->x_buf + count;
            t_sample *fp = in;
            t_sample sum = *sump;
            int i;
            for (i = 0; i < n; i++){
                fp--;
                sum += *hp++ * (*fp * *fp); // sum = hp * inˆ2
                if (*fp > p)
                    p = *fp;
                else if (*fp < -p)
                    p = *fp * -1;
            }
        *sump = sum; // sum
    }
    sump[0] = 0;
    x->x_phase -= n;
    if (x->x_phase < 0){ // get result and reset
        x->x_result_rms = x->x_sumbuf[0];
        x->x_result_peak = p;
        for(count = x->x_realperiod, sump = x->x_sumbuf;
             count < x->x_npoints; count += x->x_realperiod, sump++)
            sump[0] = sump[1];
            sump[0] = 0;
            p = 0;
            x->x_phase = x->x_realperiod - n;
            clock_delay(x->x_clock, 0L); // output?
    }
    return(w+4);
}

static void vu_tilde_dsp(t_sigvu *x, t_signal **sp){
   x->x_block = sp[0]->s_n;
    if (x->x_period % sp[0]->s_n) x->x_realperiod =
        x->x_period + sp[0]->s_n - (x->x_period % sp[0]->s_n);
    else x->x_realperiod = x->x_period;
    if (sp[0]->s_n > x->x_allocforvs){
        void *xx = resizebytes(x->x_buf,
            (x->x_npoints + x->x_allocforvs) * sizeof(t_sample),
            (x->x_npoints + sp[0]->s_n) * sizeof(t_sample));
        if (!xx)
            {
            pd_error(x, "vu~: out of memory");
            return;
            }
        x->x_buf = (t_sample *)xx;
        x->x_allocforvs = sp[0]->s_n;
        }
    dsp_add(vu_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
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

static void vu_tilde_tick(t_sigvu *x){ // clock callback function
    outlet_float(x->x_out_peak, amp2db(x->x_result_peak));
    outlet_float(x->x_out_rms, pow2db(x->x_result_rms));
}
                              
static void vu_tilde_free(t_sigvu *x){  // cleanup
    clock_free(x->x_clock);
    freebytes(x->x_buf, (x->x_npoints + x->x_allocforvs) * sizeof(*x->x_buf));
}

void vu_tilde_setup(void ){
    vu_tilde_class = class_new(gensym("vu~"), (t_newmethod)vu_tilde_new,
        (t_method)vu_tilde_free, sizeof(t_sigvu), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(vu_tilde_class, nullfn, gensym("signal"), 0);
    class_addmethod(vu_tilde_class, (t_method)vu_tilde_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(vu_tilde_class, (t_method)vu_set, gensym("set"), A_DEFFLOAT, A_DEFFLOAT, 0);
}
