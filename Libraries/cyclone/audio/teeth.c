//COPIED FROM comb.c BUT ADDING AN EXTRA INLET AND ARGT FOR SEPARATE DELAYS - DK

//- Derek Kwan 2016

#include <math.h>
#include <stdlib.h>
#include "m_pd.h"
#include <common/api.h>

#define TEETH_STACK 48000 //stack buf size, 1 sec at 48k for good measure
#define TEETH_DELAY  10.0 //maximum delay
#define TEETH_MIND 1 //minumum delay 
#define TEETH_MAXD 4294967294 //max delay = 2**32 - 2

#define TEETH_MINMS 0. //min delay in ms

#define TEETH_DEFGAIN 0. //default gain
#define TEETH_DEFFF 0. //default ff gain
#define TEETH_DEFFB 0. //default fb gain


static t_class *teeth_class;

typedef struct _teeth
{
    t_object  x_obj;
    t_inlet  *x_fflet; //ff delay inlet
    t_inlet  *x_fblet; //fb delay inlet
    t_inlet  *x_alet;
    t_inlet  *x_blet;
    t_inlet  *x_clet;
    t_outlet  *x_outlet;
    int     x_sr;
    //pointers to the delay bufs
    double  * x_ybuf; 
    double x_ffstack[TEETH_STACK];
    double * x_xbuf;
    double x_fbstack[TEETH_STACK];
    int     x_alloc; //if we are using allocated bufs
    unsigned int     x_sz; //actual size of each delay buffer

    t_float     x_maxdel;  //maximum delay in ms
    unsigned int       x_wh;     //writehead
} t_teeth;

static void teeth_clear(t_teeth *x){
    unsigned int i;
    for(i=0; i<x->x_sz; i++){
        x->x_xbuf[i] = 0.;
        x->x_ybuf[i] = 0.;
    };
    x->x_wh = 0;
}

static void teeth_sz(t_teeth *x){
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
    else if(newsz > TEETH_MAXD){
        newsz = TEETH_MAXD;
    };
    if(!alloc && newsz > TEETH_STACK){
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
    else if(alloc && newsz < TEETH_STACK){
        free(x->x_xbuf);
        free(x->x_ybuf);
        x->x_sz = TEETH_STACK;
        x->x_xbuf = x->x_ffstack;
        x->x_ybuf = x->x_fbstack;
        x->x_alloc = 0;
    };
    teeth_clear(x);
}




static double teeth_getlin(double tab[], unsigned int sz, double idx){
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

static double teeth_readmsdelay(t_teeth *x, double arr[], t_float ms){
    //helper func, basically take desired ms delay, convert to samp, read from arr[]

    //eventual reading head
    double rh = (double)ms*((double)x->x_sr*0.001); //converting ms to samples
    //bounds checking for minimum delay in samples
        if(rh < TEETH_MIND){
            rh = TEETH_MIND;
        };
        rh = (double)x->x_wh+((double)x->x_sz-rh); //essentially subracting from writehead to find proper position in buffer
        //wrapping into length of delay buffer
        while(rh >= x->x_sz){
            rh -= (double)x->x_sz;
        };
        //now to read from the buffer!
        double output = teeth_getlin(arr, x->x_sz, rh);
        return output;
        
}



static t_int *teeth_perform(t_int *w)
{
    t_teeth *x = (t_teeth *)(w[1]);
    int n = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *ffin = (t_float *)(w[4]);
    t_float *fbin = (t_float *)(w[5]);
    t_float *ain = (t_float *)(w[6]);
    t_float *bin = (t_float *)(w[7]);
    t_float *cin = (t_float *)(w[8]);
    t_float *out = (t_float *)(w[9]);
    
    int i;
    for(i=0; i<n;i++){
        int wh = x->x_wh;
        double input = (double)xin[i];
        //first off, write input to delay buf
        x->x_xbuf[wh] = input;
        //get delayed values of x and y
        t_float ffms = ffin[i];
        t_float fbms = fbin[i];
        //first bounds checking
        if(ffms < 0){
            ffms = 0;
        }
        else if(ffms > x->x_maxdel){
            ffms = x->x_maxdel;
        };
      
          if(fbms < 0){
            fbms = 0;
        }
        else if(fbms > x->x_maxdel){
            fbms = x->x_maxdel;
        };

        //now get those delayed vals
        double delx = teeth_readmsdelay(x, x->x_xbuf, ffms);
        double dely = teeth_readmsdelay(x, x->x_ybuf, fbms);
        //figure out your current y term, y[n] = a*x[n]+b*x[n-d1] + c*y[n-d2]
        double output = (double)ain[i]*input + (double)bin[i]*delx + (double)cin[i]*dely;
        //stick this guy in the ybuffer and output
        x->x_ybuf[wh] = output;
        out[i] = output;

        //increment writehead
        x->x_wh = (wh + 1) % x->x_sz;
    };
    
    return (w + 10);
}

static void teeth_dsp(t_teeth *x, t_signal **sp)
{
    int sr = sp[0]->s_sr;
    if(sr != x->x_sr){
        //if new sample rate isn't old sample rate, need to realloc
        x->x_sr = sr;
        teeth_sz(x);
    };
    dsp_add(teeth_perform, 9, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
	    sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec, sp[6]->s_vec);
}

static void teeth_list(t_teeth *x, t_symbol *s, int argc, t_atom * argv){
  
   
    int argnum = 0; //current argument
    while(argc){
        if(argv -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    //maxdel
                    x->x_maxdel = curf > 0 ? curf : TEETH_DELAY; 
                    teeth_sz(x);
                    break;
                case 1:
                    //ffdel
                        if(curf < TEETH_MINMS){
                            curf = TEETH_MINMS;
                          }
                        else if(curf > x->x_maxdel){
                             curf = x->x_maxdel;
                         };
                    pd_float((t_pd *)x->x_fflet, curf);
                    break;
                case 2:
                    //fbdel
                    if(curf < TEETH_MINMS){
                        curf = TEETH_MINMS;
                      }
                    else if(curf > x->x_maxdel){
                         curf = x->x_maxdel;
                     };
                    pd_float((t_pd *)x->x_fblet, curf);
                    break;
                case 3:
                    //gain
                    pd_float((t_pd *)x->x_alet, curf);
                    break;
                case 4:
                    //ffcoeff
                    pd_float((t_pd *)x->x_blet, curf);
                    break;
                case 5:
                    //fbcoeff
                    pd_float((t_pd *)x->x_clet, curf);
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


static void *teeth_new(t_symbol *s, int argc, t_atom * argv){
    t_teeth *x = (t_teeth *)pd_new(teeth_class);
   
    //defaults
    t_float maxdel = TEETH_DELAY;
    t_float ffdel = TEETH_MINMS;
    t_float fbdel = TEETH_MINMS;
    t_float gain = TEETH_DEFGAIN;
    t_float ffcoeff = TEETH_DEFFF;
    t_float fbcoeff = TEETH_DEFFB;
    x->x_sr = sys_getsr();
    
    x->x_alloc = 0;
    x->x_sz = TEETH_STACK;
    //clear out stack bufs, set pointer to stack
    x->x_ybuf = x->x_fbstack;
    x->x_xbuf = x->x_ffstack;
    teeth_clear(x);

    int argnum = 0; //current argument
    while(argc){
        if(argv -> a_type == A_FLOAT){
            t_float curf = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    maxdel = curf;
                    break;
                case 1:
                    ffdel = curf;
                    break;
                case 2:
                    fbdel = curf;
                    break;
                case 3:
                    gain = curf;
                    break;
                case 4:
                    ffcoeff = curf;
                    break;
                case 5:
                    fbcoeff = curf;
                    break;
                default:
                    break;
            };
            argnum++;
        };
        argc--;
        argv++;
    };
    

    x->x_maxdel = maxdel > 0 ? maxdel : TEETH_DELAY; 
      //ship off to the helper method to deal with allocation if necessary
    teeth_sz(x);
    //boundschecking
    //this is 1/44.1 (1/(sr*0.001) rounded up, good enough?
    if(ffdel < TEETH_MINMS){
        ffdel = TEETH_MINMS;
    }
    else if(ffdel > x->x_maxdel){
        ffdel = x->x_maxdel;
    };

    if(fbdel < TEETH_MINMS){
        fbdel = TEETH_MINMS;
    }
    else if(fbdel > x->x_maxdel){
        fbdel = x->x_maxdel;
    };


    //inlets outlets
    x->x_fflet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_fflet, ffdel);
    x->x_fblet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_fblet, fbdel);
    x->x_alet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_alet, gain);
    x->x_blet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_blet, ffcoeff);
    x->x_clet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_clet, fbcoeff);
    x->x_outlet = outlet_new((t_object *)x, &s_signal);
    return (x);
}

static void * teeth_free(t_teeth *x){
    if(x->x_alloc){
        free(x->x_xbuf);
        free(x->x_ybuf);
    };
    inlet_free(x->x_fflet);
    inlet_free(x->x_fblet);
    inlet_free(x->x_alet);
    inlet_free(x->x_blet);
    inlet_free(x->x_clet);
    outlet_free(x->x_outlet);
    return (void *)x;
}

CYCLONE_OBJ_API void teeth_tilde_setup(void)
{
    teeth_class = class_new(gensym("teeth~"),
			   (t_newmethod)teeth_new,
			   (t_method)teeth_free,
			   sizeof(t_teeth), 0,
			   A_GIMME, 0);
    class_addmethod(teeth_class, nullfn, gensym("signal"), 0);
    class_addmethod(teeth_class, (t_method)teeth_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(teeth_class, (t_method)teeth_clear, gensym("clear"), 0);
    class_addlist(teeth_class, (t_method)teeth_list);
}
