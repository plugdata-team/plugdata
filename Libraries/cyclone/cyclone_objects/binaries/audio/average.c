/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* CHECKME no reset after changing of a window size? */
/* CHECKME overlap */

/* Derek Kwan 2016
    OVERALL: old average~ was a chunked avg, this is now a moving avg
    ========================
   rewrite average_absolutesum, average_rmssum, average_bipolarsum for addition, subtraction
   rewrite perform, a lot of things in the struct to move from a chunked average to moving average
   to parsing of input signal outside of average_ methods and inside perform method
   getting rid of x_clock
   write average_zerobuf, average_reset
   changing float outlet to signal outlet
   changed new method to accept list rather that defsym and deffloat
    NOTE: I've written the bufrd so that it only uses [0, npoints) which makes overwriting the sample that drops
    off the moving average easier, but this won't work if you want to resize the buffer and not clear it
    also, averages are over npoints always, even if there are less than npoints accumulated so far */

#include <stdlib.h>
#include <math.h>
#include "m_pd.h"
#include <common/api.h>

#define AVERAGE_STACK    44100 //stack value
#define AVERAGE_MAXBUF  882000 //max buffer size
#define AVERAGE_DEFNPOINTS  100  /* CHECKME */
#define AVERAGE_DEFMODE     AVERAGE_BIPOLAR
enum { AVERAGE_BIPOLAR, AVERAGE_ABSOLUTE, AVERAGE_RMS };

typedef struct _average
{
    t_object    x_obj;
    t_inlet    *x_inlet1;
    int         x_mode;
    double      (*x_sumfn)(double, double, int);
    unsigned int         x_count; //number of samples seen so far, will no go beyond x_npoints + 1
    unsigned int         x_npoints; //number of samples for moving average
    double     x_accum; //sum
    double	   x_calib; //accumulator calibrator
    double      *x_buf; //buffer pointer
    double     x_stack[AVERAGE_STACK]; //buffer
    int         x_alloc; //if x_buf is allocated or stack
    unsigned int    x_sz; //allocated size for x_buf
    unsigned int    x_bufrd; //readhead for buffer
    unsigned int         x_max; //max size of buffer as specified by argt
} t_average;

static t_class *average_class;

static void average_zerobuf(t_average *x){

    unsigned int i;
    for(i=0; i < x->x_sz; i++){
        x->x_buf[i] = 0.;
    };
}

static void average_reset(t_average * x){
    //clear buffer and reset everything to 0
    x->x_count = 0;
    x->x_accum = 0;
    x->x_bufrd = 0;
    average_zerobuf(x);
};

static void average_sz(t_average *x, unsigned int newsz){
    //helper function to deal with allocation issues if needed
    int alloc = x->x_alloc;
    unsigned int cursz = x->x_sz; //current size
    //requested size
    if(newsz < 0){
        newsz = 0;
    }
    else if(newsz > AVERAGE_MAXBUF){
        newsz = AVERAGE_MAXBUF;
    };
    if(!alloc && newsz > AVERAGE_STACK){
        x->x_buf = (double *)malloc(sizeof(double)*newsz);
        x->x_alloc = 1;
        x->x_sz = newsz;
    }
    else if(alloc && newsz > cursz){
        x->x_buf = (double *)realloc(x->x_buf, sizeof(double)*newsz);
        x->x_sz = newsz;
    }
    else if(alloc && newsz < AVERAGE_STACK){
        free(x->x_buf);
        x->x_sz = AVERAGE_STACK;
        x->x_buf = x->x_stack;
        x->x_alloc = 0;
    };
    average_reset(x);
}


static double average_bipolarsum(double input, double accum, int add)
{
    if(add){
        accum += input;
    }
    else{
        accum -= input;
    };
    return (accum);
}

static double average_absolutesum(double input, double accum, int add)
{
    if(add){
        accum += (input >= 0 ? input : -input);
    }
    else{
        accum -= (input >= 0 ? input : -input);
    };
    return (accum);
}

static double average_rmssum(double input, double accum, int add)
{
    if(add){
        accum += (input * input);
    }
    else{
        accum -= (input * input);
    };
    return (accum);
}

static void average_setmode(t_average *x, int mode)
{
    if (mode == AVERAGE_BIPOLAR)
	x->x_sumfn = average_bipolarsum;
    else if (mode == AVERAGE_ABSOLUTE)
	x->x_sumfn = average_absolutesum;
    else if (mode == AVERAGE_RMS)
	x->x_sumfn = average_rmssum;
    x->x_mode = mode;
    average_reset(x);
}

static void average_float(t_average *x, t_float f)
{
    unsigned int i = (unsigned int)f;  /* CHECKME noninteger */
    if (i > 0)  /* CHECKME */
    {
        //clip at max
        if(i >= x->x_max){
            i = x->x_max;
        };
	x->x_npoints = i;
        average_reset(x);
    }
}

static void average_bipolar(t_average *x)
{
    average_setmode(x, AVERAGE_BIPOLAR);
}

static void average_absolute(t_average *x)
{
    average_setmode(x, AVERAGE_ABSOLUTE);
}

static void average_rms(t_average *x)
{
    average_setmode(x, AVERAGE_RMS);
}

static t_int *average_perform(t_int *w)
{
    t_average *x = (t_average *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    double (*sumfn)(double, double, int) = x->x_sumfn;
    int i;
    unsigned int npoints = x->x_npoints;
    for(i=0; i <nblock; i++){
        double result; //eventual result
        double input = (double)in[i];
        if(npoints > 1)
        {
            unsigned int bufrd = x->x_bufrd;
            //add input to accumulator
            x->x_accum = (*sumfn)(input, x->x_accum, 1);
            x->x_calib = (*sumfn)(input, x->x_calib, 1);
            unsigned int count = x->x_count;
            if(count < npoints){
                //update count
                count++;
                x->x_count = count;
            }
            else{
                //start subtracting from the accumulator the sample that drops off the moving sum

                /*say npoints = 2, then when bufrd = 1, 0 and 1 are the accum, then when bufrd = 2, 1 and 2 should be the accum
                  so you subtract 0 from accum and add 2 and 0 = bufrd - npoints
                  if we are only using npoints of the x_buf, then bufrd will be the value we want to subtract
                */
            
                //subtract current value in bufrd's location from accum
                x->x_accum = (*sumfn)(x->x_buf[bufrd], x->x_accum, 0);

            };

            //overwrite/store current input value into buf
            x->x_buf[bufrd] = input;
 

            //calculate result
            result = x->x_accum/(double)npoints;
            if (x->x_mode == AVERAGE_RMS)
                result = sqrt(result);           
            
            //incrementation step
            bufrd++;
            if(bufrd >= npoints){
                bufrd = 0;
                x->x_accum = x->x_calib;
                x->x_calib = 0.0;
            };
            x->x_bufrd = bufrd;

        }
        else{
            //npoints = 1, just pass through (or absolute)
            if(x->x_mode == AVERAGE_ABSOLUTE || x->x_mode == AVERAGE_RMS){
                result = fabs(input);
            }
            else{
                result = input;
            };
        };
        out[i] = result;
    };

    return (w + 5);
}

static void average_dsp(t_average *x, t_signal **sp)
{
    dsp_add(average_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void average_free(t_average *x)
{
    if(x->x_alloc){
        free(x->x_buf);
    };
}

static void *average_new(t_symbol *s, int argc, t_atom * argv)
{
    t_average *x = (t_average *)pd_new(average_class);
    int mode;
    t_symbol * modename = &s_;
    unsigned int maxbuf = AVERAGE_DEFNPOINTS; //default for max buf size
    //default to stack for now...
    x->x_buf = x->x_stack; 
    x->x_alloc = 0;
    x->x_sz = AVERAGE_STACK;

    while(argc){
        if(argv -> a_type == A_FLOAT){
            t_float curfloat = atom_getfloatarg(0, argc, argv);
            maxbuf = (unsigned int)curfloat;
        }
        else if(argv -> a_type == A_SYMBOL){
            modename = atom_getsymbolarg(0, argc, argv);
        };
        argc--;
        argv++;

    };
    /* CHECKED it looks like memory is allocated for the entire window,
       in tune with the refman's note about ``maximum averaging interval'' --
       needed for dynamic control over window size, or what? LATER rethink */
    x->x_npoints = (maxbuf > 0 ? maxbuf : 1);
    x->x_max = x->x_npoints; //designated max of average buffer
    if (modename == gensym("bipolar"))
	mode = AVERAGE_BIPOLAR;
    else if (modename == gensym("absolute"))
	mode = AVERAGE_ABSOLUTE;
    else if (modename == gensym("rms"))
	mode = AVERAGE_RMS;
    else
    {
	mode = AVERAGE_DEFMODE;
	/* CHECKME a warning if (s && s != &s_) */
    }
    average_setmode(x, mode);

    //now allocate x_buf if necessary
    average_sz(x, x->x_npoints);
    /* CHECKME if not x->x_phase = 0 */
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

CYCLONE_OBJ_API void average_tilde_setup(void)
{
    average_class = class_new(gensym("average~"), (t_newmethod)average_new,
            (t_method)average_free, sizeof(t_average), 0, A_GIMME, 0);
    class_addmethod(average_class, (t_method) average_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(average_class, nullfn, gensym("signal"), 0);
    class_addmethod(average_class, (t_method)average_bipolar, gensym("bipolar"), 0);
    class_addmethod(average_class, (t_method)average_absolute, gensym("absolute"), 0);
    class_addmethod(average_class, (t_method)average_rms, gensym("rms"), 0);
    class_addfloat(average_class, (t_method)average_float);
}
