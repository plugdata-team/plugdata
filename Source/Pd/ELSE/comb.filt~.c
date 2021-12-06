// based on the one we did for cyclone

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "m_pd.h"

#define COMB_STACK  44100 //stack buf size, 1 sec at 48k for good measure
#define COMB_DELAY  1000 //maximum delay
#define COMB_MIND   1 //minumum delay
#define COMB_MAXD   4294967294 //max delay = 2**32 - 2

#define COMB_MINMS  0. //min delay in ms

#define COMB_DEFFF  0. //default ff gain
#define COMB_DEFFB  0. //default fb gain

static t_class *comb_class;

typedef struct _comb
{
    t_object  x_obj;
    t_inlet  *x_dellet;
    t_inlet  *x_alet;
    t_inlet  *x_blet;
    t_inlet  *x_clet;
    t_outlet  *x_outlet;
    int     x_sr;
    int     x_gain;
    //pointers to the delay bufs
    double  * x_ybuf;
    double x_ffstack[COMB_STACK];
    double * x_xbuf;
    double x_fbstack[COMB_STACK];
    int     x_alloc; //if we are using allocated bufs
    unsigned int     x_sz; //actual size of each delay buffer
    
    t_float     x_maxdel;  //maximum delay in ms
    unsigned int       x_wh;     //writehead
} t_comb;

static void comb_clear(t_comb *x){
    unsigned int i;
    for(i=0; i<x->x_sz; i++){
        x->x_xbuf[i] = 0.;
        x->x_ybuf[i] = 0.;
    };
    x->x_wh = 0;
}

static void comb_gain(t_comb *x, t_floatarg f1){
    x->x_gain = f1 != 0;
}

static void comb_sz(t_comb *x){
    //helper function to deal with allocation issues if needed
    //ie if wanted size x->x_maxdel is bigger than stack, allocate
    
    //convert ms to samps
    unsigned int newsz = (unsigned int)ceil((double)x->x_maxdel*0.001*(double)x->x_sr);
    newsz++; //add a sample for good measure since say bufsize is 2048 and
    //you want a delay of 2048 samples,.. problem!
    
    int alloc = x->x_alloc;
    unsigned int cursz = x->x_sz; //current size
    
    if(newsz < 0){
        newsz = 0;
    }
    else if(newsz > COMB_MAXD){
        newsz = COMB_MAXD;
    };
    if(!alloc && newsz > COMB_STACK){
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
    else if(alloc && newsz < COMB_STACK){
        free(x->x_xbuf);
        free(x->x_ybuf);
        x->x_sz = COMB_STACK;
        x->x_xbuf = x->x_ffstack;
        x->x_ybuf = x->x_fbstack;
        x->x_alloc = 0;
    };
    comb_clear(x);
}

static double comb_getlin(double tab[], unsigned int sz, double idx){
    //copying from my own library, linear interpolated reader - DK
    double output;
    unsigned int tabphase1 = (unsigned int)idx;
    unsigned int tabphase2 = tabphase1 + 1;
    double frac = idx - (double)tabphase1;
    if(tabphase1 >= sz - 1){
        tabphase1 = sz - 1; //checking to see if index falls within bounds
        output = tab[tabphase1];
    }
    else if(tabphase1 < 0){
        tabphase1 = 0;
        output = tab[tabphase1];
    }
    else{
        double yb = tab[tabphase2]; //linear interp
        double ya = tab[tabphase1];
        output = ya+((yb-ya)*frac);
        
    };
    return output;
}

static double comb_readmsdelay(t_comb *x, double arr[], t_float ms){
    //helper func, basically take desired ms delay, convert to samp, read from arr[]
    
    //eventual reading head
    double rh = (double)ms*((double)x->x_sr*0.001); //converting ms to samples
    //bounds checking for minimum delay in samples
    if(rh < COMB_MIND){
        rh = COMB_MIND;
    };
    rh = (double)x->x_wh+((double)x->x_sz-rh); //essentially subracting from writehead to find proper position in buffer
    //wrapping into length of delay buffer
    while(rh >= x->x_sz){
        rh -= (double)x->x_sz;
    };
    //now to read from the buffer!
    double output = comb_getlin(arr, x->x_sz, rh);
    return output;
    
}

static t_int *comb_perform(t_int *w)
{
    t_comb *x = (t_comb *)(w[1]);
    int n = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *din = (t_float *)(w[4]);
    t_float *coeff_in = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    
    int i;
    for(i=0; i<n;i++){
        int wh = x->x_wh;
        double input = (double)xin[i];
        //first off, write input to delay buf
        x->x_xbuf[wh] = input;
        //get delayed values of x and y
        if(din[i] > x->x_sr)
            din[i] = x->x_sr;
        t_float delms = din[i] <= 0 ? 0 : 1000 / din[i];
        //first bounds checking
        if(delms > x->x_maxdel){
            delms = x->x_maxdel;
        };
        if(delms == 0)
            out[i] = input;
        else{
            if(!x->x_gain && coeff_in[i] != 0)
                coeff_in[i] = copysign(exp(log(0.001) * delms/fabs(coeff_in[i])), coeff_in[i]);
            double delx = comb_readmsdelay(x, x->x_xbuf, delms);
            double dely = comb_readmsdelay(x, x->x_ybuf, delms);
            double output = input + (double)coeff_in[i]*delx + (double)coeff_in[i]*dely;
            //stick this guy in the ybuffer and output
            x->x_ybuf[wh] = output;
            out[i] = output;
        };
        
        //increment writehead
        x->x_wh = (wh + 1) % x->x_sz;
    };
    
    return (w + 7);
}

static void comb_dsp(t_comb *x, t_signal **sp)
{
    int sr = sp[0]->s_sr;
    if(sr != x->x_sr){
        //if new sample rate isn't old sample rate, need to realloc
        x->x_sr = sr;
        comb_sz(x);
    };
    dsp_add(comb_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *comb_new(t_symbol *s, int argc, t_atom * argv){
    t_comb *x = (t_comb *)pd_new(comb_class);
    t_symbol *cursym = s;
    //defaults
    t_float init_hz = 0;
    t_float coeff = COMB_DEFFF;
    x->x_sr = sys_getsr();
    
    x->x_alloc = 0;
    x->x_gain = 0;
    x->x_sz = COMB_STACK;
    //clear out stack bufs, set pointer to stack
    x->x_ybuf = x->x_fbstack;
    x->x_xbuf = x->x_ffstack;
    comb_clear(x);
    
    /////////////////////////////////////////////////////////////////////////////////
    int symarg = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_SYMBOL){
            if(!symarg)
                symarg = 1;
            cursym = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(cursym->s_name, "-gain")){
                x->x_gain = 1;
                argc--;
                argv++;
                argnum++;
            }
            else
                goto errstate;
        }
        else if(argv->a_type == A_FLOAT && !symarg){
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    init_hz = (argval < 0 ? 0 : argval);
                    break;
                case 1:
                    coeff = argval;
                    break;
                case 2:
                    x->x_gain = argval != 0;
                    break;
                default:
                    break;
            };
            argc--;
            argv++;
            argnum++;
        }
        else
            goto errstate;
    };
    /////////////////////////////////////////////////////////////////////////////////
    
    x->x_maxdel = COMB_DELAY;
    //ship off to the helper method to deal with allocation if necessary
    comb_sz(x);
    //inlets outlets
    x->x_dellet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_dellet, init_hz);
    x->x_blet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_blet, coeff);
    x->x_outlet = outlet_new((t_object *)x, &s_signal);
    return (x);
errstate:
    pd_error(x, "[comb.filt~]: improper args");
    return NULL;
}

static void * comb_free(t_comb *x){
    if(x->x_alloc){
        free(x->x_xbuf);
        free(x->x_ybuf);
    };
    inlet_free(x->x_dellet);
    inlet_free(x->x_blet);
    outlet_free(x->x_outlet);
    return (void *)x;
}


void setup_comb0x2efilt_tilde(void)
{
    comb_class = class_new(gensym("comb.filt~"), (t_newmethod)comb_new,
                               (t_method)comb_free, sizeof(t_comb), 0, A_GIMME, 0);
    class_addmethod(comb_class, nullfn, gensym("signal"), 0);
    class_addmethod(comb_class, (t_method)comb_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(comb_class, (t_method)comb_clear, gensym("clear"), 0);
    class_addmethod(comb_class, (t_method)comb_gain, gensym("gain"), A_DEFFLOAT, 0);
}

