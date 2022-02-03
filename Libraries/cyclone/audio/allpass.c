//almost complete rewrite since the previous version of Cyclone didn't implement it correctly (used only one delay buffer)
//- Derek Kwan 2016

#include <math.h>
#include <stdlib.h>
#include "m_pd.h"
#include <common/api.h>

#define ALLPASS_STACK 48000 //stack buf size, 1 sec at 48k for good measure
#define ALLPASS_DELAY  10.0 //maximum delay
#define ALLPASS_MIND 1 //minumum delay 
#define ALLPASS_MAXD 4294967294 //max delay = 2**32 - 2

#define ALLPASS_MINMS 0. //min delay in ms

#define ALLPASS_DEFGAIN 0. //default gain

static t_class *allpass_class;

typedef struct _allpass
{
    t_object  x_obj;
    t_inlet  *x_dellet;
    t_inlet  *x_alet;
    t_outlet  *x_outlet;
    int     x_sr;
    //pointers to the delay bufs
    double  * x_ybuf; 
    double x_ffstack[ALLPASS_STACK];
    double * x_xbuf;
    double x_fbstack[ALLPASS_STACK];
    int     x_alloc; //if we are using allocated bufs
    unsigned int     x_sz; //actual size of each delay buffer

    t_float     x_maxdel;  //maximum delay in ms
    unsigned int       x_wh;     //writehead
} t_allpass;

static void allpass_clear(t_allpass *x){
    unsigned int i;
    for(i=0; i<x->x_sz; i++){
        x->x_xbuf[i] = 0.;
        x->x_ybuf[i] = 0.;
    };
    x->x_wh = 0;
}

static void allpass_sz(t_allpass *x){
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
    else if(newsz > ALLPASS_MAXD){
        newsz = ALLPASS_MAXD;
    };
    if(!alloc && newsz > ALLPASS_STACK){
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
    else if(alloc && newsz < ALLPASS_STACK){
        free(x->x_xbuf);
        free(x->x_ybuf);
        x->x_sz = ALLPASS_STACK;
        x->x_xbuf = x->x_ffstack;
        x->x_ybuf = x->x_fbstack;
        x->x_alloc = 0;
    };
    allpass_clear(x);
}




static double allpass_getlin(double tab[], unsigned int sz, double idx){
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

static double allpass_readmsdelay(t_allpass *x, double arr[], t_float ms){
    //helper func, basically take desired ms delay, convert to samp, read from arr[]

    //eventual reading head
    double rh = (double)ms*((double)x->x_sr*0.001); //converting ms to samples
    //bounds checking for minimum delay in samples
        if(rh < ALLPASS_MIND){
            rh = ALLPASS_MIND;
        };
        rh = (double)x->x_wh+((double)x->x_sz-rh); //essentially subracting from writehead to find proper position in buffer
        //wrapping into length of delay buffer
        while(rh >= x->x_sz){
            rh -= (double)x->x_sz;
        };
        //now to read from the buffer!
        double output = allpass_getlin(arr, x->x_sz, rh);
        return output;
        
}



static t_int *allpass_perform(t_int *w)
{
    t_allpass *x = (t_allpass *)(w[1]);
    int n = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *din = (t_float *)(w[4]);
    t_float *ain = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    
    int i;
    for(i=0; i<n;i++){
        int wh = x->x_wh;
        double input = (double)xin[i];
        //first off, write input to delay buf
        x->x_xbuf[wh] = input;
        //get delayed values of x and y
        t_float delms = din[i];
        //first bounds checking
        if(delms < 0){
            delms = 0;
        }
        else if(delms > x->x_maxdel){
            delms = x->x_maxdel;
        };
        //now get those delayed vals
        double delx = allpass_readmsdelay(x, x->x_xbuf, delms);
        double dely = allpass_readmsdelay(x, x->x_ybuf, delms);
        //figure out your current y term: y[n] = -a*x[n] + x[n-d] + a*y[n-d]
        double output = (double)ain[i]*-1.*input + delx + (double)ain[i]*dely;
        //stick this guy in the ybuffer and output
        x->x_ybuf[wh] = output;
        out[i] = output;

        //increment writehead
        x->x_wh = (wh + 1) % x->x_sz;
    };
    
    return (w + 7);
}

static void allpass_dsp(t_allpass *x, t_signal **sp)
{
    int sr = sp[0]->s_sr;
    if(sr != x->x_sr){
        //if new sample rate isn't old sample rate, need to realloc
        x->x_sr = sr;
        allpass_sz(x);
    };
    dsp_add(allpass_perform, 6, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
	    sp[3]->s_vec);
}

static void allpass_list(t_allpass *x, t_symbol *s, int argc, t_atom * argv){
  
   
    int argnum = 0; //current argument
    while(argc){
        if(argv -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    //maxdel
                    x->x_maxdel = curf > 0 ? curf : ALLPASS_DELAY; 
                    allpass_sz(x);
                    break;
                case 1:
                    //initdel
                        if(curf < ALLPASS_MINMS){
                            curf = ALLPASS_MINMS;
                          }
                        else if(curf > x->x_maxdel){
                             curf = x->x_maxdel;
                         };
                    pd_float((t_pd *)x->x_dellet, curf);
                    break;
                case 2:
                    //gain
                    pd_float((t_pd *)x->x_alet, curf);
                    break;
                default:
                    break;
            };
            argnum++;
        };
        argc--;
        argv++;
    };



}


static void *allpass_new(t_symbol *s, int argc, t_atom * argv){
    t_allpass *x = (t_allpass *)pd_new(allpass_class);
   
    //defaults
    t_float maxdel = ALLPASS_DELAY;
    t_float initdel = ALLPASS_MINMS;
    t_float gain = ALLPASS_DEFGAIN;
    x->x_sr = sys_getsr();
    
    x->x_alloc = 0;
    x->x_sz = ALLPASS_STACK;
    //clear out stack bufs, set pointer to stack
    x->x_ybuf = x->x_fbstack;
    x->x_xbuf = x->x_ffstack;
    allpass_clear(x);

    int argnum = 0; //current argument
    while(argc){
        if(argv -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    maxdel = curf;
                    break;
                case 1:
                    initdel = curf;
                    break;
                case 2:
                    gain = curf;
                    break;
                default:
                    break;
            };
            argnum++;
        };
        argc--;
        argv++;
    };
    

    x->x_maxdel = maxdel > 0 ? maxdel : ALLPASS_DELAY; 
      //ship off to the helper method to deal with allocation if necessary
    allpass_sz(x);
    //boundschecking
    //this is 1/44.1 (1/(sr*0.001) rounded up, good enough?

    if(initdel < ALLPASS_MINMS){
        initdel = ALLPASS_MINMS;
    }
    else if(initdel > x->x_maxdel){
        initdel = x->x_maxdel;
    };


    //inlets outlets
    x->x_dellet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_dellet, initdel);
    x->x_alet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_alet, gain);
    x->x_outlet = outlet_new((t_object *)x, &s_signal);
    return (x);
}





static void * allpass_free(t_allpass *x){
    if(x->x_alloc){
        free(x->x_xbuf);
        free(x->x_ybuf);
    };
    inlet_free(x->x_dellet);
    inlet_free(x->x_alet);
    outlet_free(x->x_outlet);
    return (void *)x;
}

CYCLONE_OBJ_API void allpass_tilde_setup(void)
{
    allpass_class = class_new(gensym("allpass~"),
			   (t_newmethod)allpass_new,
			   (t_method)allpass_free,
			   sizeof(t_allpass), 0,
			   A_GIMME, 0);
    class_addmethod(allpass_class, nullfn, gensym("signal"), 0);
    class_addmethod(allpass_class, (t_method)allpass_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(allpass_class, (t_method)allpass_clear, gensym("clear"), 0);
    class_addlist(allpass_class, (t_method)allpass_list);
}
