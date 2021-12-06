/* Based on Chamberlin's prototype from "Musical Applications of
   Microprocessors" (csound's svfilter).  Slightly distorted,
   no upsampling. */

/* CHECKED scalar case: input preserved (not coefs) after changing mode */
/* CHECKME if creation args (or defaults) restored after signal disconnection */

#include <math.h>
#include "m_pd.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define SVFILTER_DRIVE     .0001
#define SVFILTER_QSTRETCH  1.2   /* CHECKED */
#define SVFILTER_MINR      0.    /* CHECKME */
#define SVFILTER_MAXR      1.2   /* CHECKME */
#define SVFILTER_MINOMEGA  0.    /* CHECKME */
#define SVFILTER_MAXOMEGA  (M_PI * .5)  /* CHECKME */
#define TWO_PI             (M_PI * 2.)
#define SVFILTER_DEFFREQ   0.
#define SVFILTER_DEFQ      .01  /* CHECKME */

typedef struct _svfilter{
    t_object x_obj;
    t_inlet *svfilter;
    t_inlet  *x_freq_inlet;
    t_inlet  *x_q_inlet;
    int    x_mode;
    float  x_srcoef;
    float  x_band;
    float  x_low;
}t_svfilter;

static t_class *svfilter_class;

static void svfilter_clear(t_svfilter *x){
    x->x_band = x->x_low = 0.;
}

static t_int *svfilter_perform(t_int *w){
    t_svfilter *x = (t_svfilter *)(w[1]);
    int n = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float fin0 = *(t_float *)(w[4]);
    t_float rin0 = *(t_float *)(w[5]);
    t_float *lout = (t_float *)(w[6]);
    t_float *hout = (t_float *)(w[7]);
    t_float *bout = (t_float *)(w[8]);
    t_float *nout = (t_float *)(w[9]);
    float band = x->x_band;
    float low = x->x_low;
    while(n--){
        float c1, c2;
        float r = (1. - rin0) * SVFILTER_QSTRETCH;  /* CHECKED */
        if (r < SVFILTER_MINR)
            r = SVFILTER_MINR;
        else if (r > SVFILTER_MAXR)
            r = SVFILTER_MAXR;
        c2 = r * r;
        
        float omega = fin0 * x->x_srcoef;
        if (omega < SVFILTER_MINOMEGA)
            omega = SVFILTER_MINOMEGA;
        else if (omega > SVFILTER_MAXOMEGA)
            omega = SVFILTER_MAXOMEGA;
        c1 = sinf(omega);
        
        float high, xn = *xin++;
        *lout++ = low = low + c1 * band;
        *hout++ = high = xn - low - c2 * band;
        *bout++ = band = c1 * high + band;
        *nout++ = low + high;
        band -= band * band * band * SVFILTER_DRIVE;
    }
    x->x_band = (PD_BIGORSMALL(band) ? 0. : band);
    x->x_low = (PD_BIGORSMALL(low) ? 0. : low);
    return(w + 10);
}

static void svfilter_dsp(t_svfilter *x, t_signal **sp){
    x->x_srcoef = TWO_PI / sp[0]->s_sr;
    svfilter_clear(x);
    dsp_add(svfilter_perform, 9, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec,
	    sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec);
}

static void *svfilter_new(t_symbol *s, int ac, t_atom *av){
    t_svfilter *x = (t_svfilter *)pd_new(svfilter_class);
    t_float freq = SVFILTER_DEFFREQ, qcoef = SVFILTER_DEFQ;
    if(ac && av->a_type == A_FLOAT){
        freq = av->a_w.w_float;
        ac--; av++;
        if(ac && av->a_type == A_FLOAT)
            qcoef = av->a_w.w_float;
    }
    x->x_srcoef = M_PI / sys_getsr();
    x->x_freq_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_freq_inlet, freq);
    x->x_q_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_q_inlet, qcoef);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    svfilter_clear(x);
    return (x);
}

void svfilter_tilde_setup(void){
    svfilter_class = class_new(gensym("svfilter~"),
        (t_newmethod)svfilter_new, 0, sizeof(t_svfilter), 0, A_GIMME, 0);
    class_addmethod(svfilter_class, nullfn, gensym("signal"), 0);
    class_addmethod(svfilter_class, (t_method)svfilter_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(svfilter_class, (t_method)svfilter_clear, gensym("clear"), 0);
}
