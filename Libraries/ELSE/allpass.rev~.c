// Porres 2017
// Based on cyclone's allpass

#include <math.h>
#include <stdlib.h>
#include "m_pd.h"

#define allpass_rev_STACK 48000 // stack buf size, 1 sec at 48k
#define allpass_rev_MAXD 4294967294 // max delay = 2**32 - 2

static t_class *allpass_rev_class;

typedef struct _allpass_rev
{
    t_object        x_obj;
    t_inlet         *x_dellet;
    t_inlet         *x_alet;
    t_outlet        *x_outlet;
    int             x_sr;
    int             x_gain;
// pointers to the delay bufs
    double          * x_ybuf;
    double          x_ffstack[allpass_rev_STACK];
    double          * x_xbuf;
    double          x_fbstack[allpass_rev_STACK];
    int             x_alloc; // if we are using allocated bufs
    unsigned int    x_sz; // actual size of each delay buffer
    t_float         x_maxdel;  // maximum delay in ms
    unsigned int    x_wh;     // writehead
} t_allpass_rev;

static void allpass_rev_clear(t_allpass_rev *x){
    unsigned int i;
    for(i=0; i<x->x_sz; i++){
        x->x_xbuf[i] = 0.;
        x->x_ybuf[i] = 0.;
    };
    x->x_wh = 0;
}

static void allpass_rev_sz(t_allpass_rev *x){
    // helper function to deal with allocation issues if needed
    // ie if wanted size x->x_maxdel is bigger than stack, allocate
    
    // convert ms to samps
    unsigned int newsz = (unsigned int)ceil((double)x->x_maxdel*0.001*(double)x->x_sr);
    newsz++; // add a sample for good measure since say bufsize is 2048 and
    // you want a delay of 2048 samples,.. problem!
    
    int alloc = x->x_alloc;
    unsigned int cursz = x->x_sz; //current size
    
    if(newsz < 0)
        newsz = 0;
    else if(newsz > allpass_rev_MAXD)
        newsz = allpass_rev_MAXD;
    if(!alloc && newsz > allpass_rev_STACK){
        x->x_xbuf = (double *)malloc(sizeof(double)*newsz);
        x->x_ybuf = (double *)malloc(sizeof(double)*newsz);
        x->x_alloc = 1;
        x->x_sz = newsz;
    }
    else if(alloc && newsz > cursz){
        x->x_xbuf = (double *)realloc(x->x_xbuf, sizeof(double)*newsz);
        x->x_ybuf = (double *)realloc(x->x_ybuf, sizeof(double)*newsz);
        x->x_sz = newsz;
    }
    else if(alloc && newsz < allpass_rev_STACK){
        free(x->x_xbuf);
        free(x->x_ybuf);
        x->x_sz = allpass_rev_STACK;
        x->x_xbuf = x->x_ffstack;
        x->x_ybuf = x->x_fbstack;
        x->x_alloc = 0;
    };
    allpass_rev_clear(x);
}

static double allpass_rev_getlin(double tab[], unsigned int sz, double idx){
    // linear interpolated reader, copied from Derek Kwan's library
    double output;
    unsigned int tabphase1 = (unsigned int)idx;
    unsigned int tabphase2 = tabphase1 + 1;
    double frac = idx - (double)tabphase1;
    if(tabphase1 >= sz - 1){
        tabphase1 = sz - 1; // checking to see if index falls within bounds
        output = tab[tabphase1];
    }
    else if(tabphase1 < 0){
        tabphase1 = 0;
        output = tab[tabphase1];
    }
    else{
        double yb = tab[tabphase2]; // linear interp
        double ya = tab[tabphase1];
        output = ya+((yb-ya)*frac);
    };
    return output;
}

static double allpass_rev_readmsdelay(t_allpass_rev *x, double arr[], t_float ms){
    //helper func, basically take desired ms delay, convert to samp, read from arr[]
    
    //eventual reading head
    double rh = (double)ms*((double)x->x_sr*0.001); //converting ms to samples
    //bounds checking for minimum delay in samples
    if(rh < 0)
        rh = 0;
    rh = (double)x->x_wh+((double)x->x_sz-rh); //essentially subracting from writehead to find proper position in buffer
    //wrapping into length of delay buffer
    while(rh >= x->x_sz){
        rh -= (double)x->x_sz;
    };
    //now to read from the buffer!
    double output = allpass_rev_getlin(arr, x->x_sz, rh);
    return output;
}

static t_int *allpass_rev_perform(t_int *w){
    t_allpass_rev *x = (t_allpass_rev *)(w[1]);
    int n = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *din = (t_float *)(w[4]);
    t_float *ain = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    int i;
    for(i=0; i<n;i++){
        int wh = x->x_wh;
        double input = (double)xin[i];
        double output;
// first off, write input to delay buf
        x->x_xbuf[wh] = input;
// get delayed values of x and y
        t_float delms = din[i];
// bounds checking
        if(delms < 0)
            delms = 0;
        else if(delms > x->x_maxdel)
            delms = x->x_maxdel;
// now get those delayed vals
        double delx = allpass_rev_readmsdelay(x, x->x_xbuf, delms);
        double dely = allpass_rev_readmsdelay(x, x->x_ybuf, delms);
        if(!x->x_gain)
            {
            if (ain[i] == 0)
                ain[i] = 0;
            else
                ain[i] = copysign(exp(log(0.001) * delms/fabs(ain[i])), ain[i]);
            }
// y[n] = -a * x[n] + x[n-d] + a * y[n-d]
        if (delms == 0)
            output = input;
        else
            output = (double)ain[i]*-1.*input + delx + (double)ain[i]*dely;
        x->x_ybuf[wh] = output;
        out[i] = output;
        x->x_wh = (wh + 1) % x->x_sz; // increment writehead
    };
    return (w + 7);
}

static void allpass_rev_dsp(t_allpass_rev *x, t_signal **sp)
{
    int sr = sp[0]->s_sr;
    if(sr != x->x_sr){
// if new sample rate isn't old sample rate, need to realloc
        x->x_sr = sr;
        allpass_rev_sz(x);
    };
    dsp_add(allpass_rev_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
        sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void allpass_rev_size(t_allpass_rev *x, t_floatarg f1){
    if(f1 < 0)
        f1 = 0;
    x->x_maxdel = f1;
    allpass_rev_sz(x);
}

static void allpass_rev_gain(t_allpass_rev *x, t_floatarg f1){
    x->x_gain = f1 != 0;
}

static void *allpass_rev_new(t_symbol *s, int argc, t_atom *argv){
    t_allpass_rev *x = (t_allpass_rev *)pd_new(allpass_rev_class);
    x->x_sr = sys_getsr();
    x->x_alloc = 0;
    x->x_gain = 0;
    x->x_sz = allpass_rev_STACK;
// clear out stack bufs, set pointer to stack
    x->x_ybuf = x->x_fbstack;
    x->x_xbuf = x->x_ffstack;
    allpass_rev_clear(x);
/////////////////////////////////////////////////////////////////////////////////////
    float init_maxdelay;
    float init_coeff;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    init_maxdelay = argval;
                    break;
                case 1:
                    init_coeff = argval;
                    break;
                case 2:
                    x->x_gain = argval != 0;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--;
            argv++;
        }
        else
            goto errstate;
    }
/////////////////////////////////////////////////////////////////////////////////////
    if (init_maxdelay < 0)
        init_maxdelay = 0;
    x->x_maxdel = init_maxdelay;
// ship off to the helper method to deal with allocation if necessary
    allpass_rev_sz(x);
// boundschecking
// this is 1/44.1 (1/(sr*0.001) rounded up, good enough?
    
// inlets / outlet
    x->x_dellet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_dellet, init_maxdelay);
    x->x_alet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_alet, init_coeff);
    x->x_outlet = outlet_new((t_object *)x, &s_signal);
    return (x);
errstate:
    pd_error(x, "allpass.rev~: improper args");
    return NULL;
}

static void * allpass_rev_free(t_allpass_rev *x){
    if(x->x_alloc){
        free(x->x_xbuf);
        free(x->x_ybuf);
    };
    inlet_free(x->x_dellet);
    inlet_free(x->x_alet);
    outlet_free(x->x_outlet);
    return (void *)x;
}

void setup_allpass0x2erev_tilde(void)
{
    allpass_rev_class = class_new(gensym("allpass.rev~"), (t_newmethod)allpass_rev_new,
        (t_method)allpass_rev_free, sizeof(t_allpass_rev), 0, A_GIMME, 0);
    class_addmethod(allpass_rev_class, nullfn, gensym("signal"), 0);
    class_addmethod(allpass_rev_class, (t_method)allpass_rev_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(allpass_rev_class, (t_method)allpass_rev_clear, gensym("clear"), 0);
    class_addmethod(allpass_rev_class, (t_method)allpass_rev_size, gensym("size"), A_DEFFLOAT, 0);
    class_addmethod(allpass_rev_class, (t_method)allpass_rev_gain, gensym("gain"), A_DEFFLOAT, 0);
}
