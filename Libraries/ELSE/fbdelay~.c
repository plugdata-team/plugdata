// Porres 2018

#include "m_pd.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FBD_STACK   48000       // stack buf size, 1 sec at 48k
#define FBD_MAXD    4294967294  // max delay = 2**32 - 2

static t_class *fbdelay_class;

typedef struct _fbdelay{
    t_object        x_obj;
    t_inlet        *x_dellet;
    t_inlet        *x_alet;
    t_outlet       *x_outlet;
    t_float         x_sr_khz;
    t_int           x_gain;
// pointers to delay buf
    t_int           x_alloc;  // if we are using allocated buf
    t_float         x_maxdel;   // maximum delay in ms
    double         *x_ybuf;
    double          x_fbstack[FBD_STACK];
    unsigned int    x_sz;       // actual size of each delay buffer
    unsigned int    x_wp;       // write head
    unsigned int    x_ms;       // ms flag
    unsigned int    x_freeze;
}t_fbdelay;

static void fbdelay_clear(t_fbdelay *x){
    for(unsigned int i = 0; i < x->x_sz; i++)
        x->x_ybuf[i] = 0.;
    x->x_wp = 0;
}

static void fbdelay_sz(t_fbdelay *x){ // deal with allocation issues
    unsigned int newsz = (unsigned int)ceil((double)x->x_maxdel*(double)x->x_sr_khz); // convert to samps
    newsz++; // add a sample, so if size is 2048 and you want a delay of 2048 samples (?)
    int alloc = x->x_alloc;
    unsigned int cursz = x->x_sz; //current size
    if(newsz < 1)
        newsz = 1;
    else if(newsz > FBD_MAXD)
        newsz = FBD_MAXD;
    if(!alloc && newsz > FBD_STACK){
        x->x_ybuf = (double *)malloc(sizeof(double)*newsz);
        x->x_alloc = 1;
        x->x_sz = newsz;
    }
    else if(alloc && newsz > cursz){
        x->x_ybuf = (double *)realloc(x->x_ybuf, sizeof(double)*newsz);
        x->x_sz = newsz;
    }
    else if(alloc && newsz < FBD_STACK){
        free(x->x_ybuf);
        x->x_sz = FBD_STACK;
        x->x_ybuf = x->x_fbstack;
        x->x_alloc = 0;
    };
    fbdelay_clear(x);
}

static void fbdelay_freeze(t_fbdelay *x, t_float f){
    x->x_freeze = (unsigned int)(f != 0);
}

static t_int *fbdelay_perform(t_int *w){
    t_fbdelay *x = (t_fbdelay *)(w[1]);
    t_int n = (int)(w[2]);
    t_float *xin = (t_float *)(w[3]);
    t_float *din = (t_float *)(w[4]);
    t_float *ain = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    for(t_int i = 0; i < n; i++){
        t_float del = din[i];
        double y_n;
        if(!x->x_ms)
            del /= x->x_sr_khz;
        if(del > x->x_maxdel)
            del = x->x_maxdel;
        t_float ms = del;
        del *= x->x_sr_khz;     // delay in samples
        if(del < 1)             // minimum of 1 sample delay
            del = 1;
        if((del - floor(del)) == 0){
            double rp = (double)x->x_wp + ((double)x->x_sz - del);  // find read point
            while(rp >= x->x_sz)       // wrap into length of delay buffer (???)
                rp -= (double)x->x_sz;
            unsigned int ndx = (unsigned int)rp;
            if(ndx >= x->x_sz - 1) // clip index
                ndx = x->x_sz - 1;
            y_n = x->x_ybuf[ndx];
        }
        else{ // lagrange interpolation
            double rp = (double)x->x_wp + ((double)x->x_sz - (del + 1));  // find read point
            while(rp >= x->x_sz)       // wrap into length of delay buffer (???)
                rp -= (double)x->x_sz;
            double frac = 1 - ((double)del - floor(del));
            unsigned int ndxm1 = (unsigned int)rp;
            unsigned int ndx = ndxm1 + 1, ndx1 = ndxm1 + 2, ndx2 = ndxm1 + 3;
            if(ndx2 > x->x_sz - 1)
                ndx2 = x->x_sz - 1;
            double a = x->x_ybuf[ndxm1];
            double b = x->x_ybuf[ndx];
            double c = x->x_ybuf[ndx1];
            double d = x->x_ybuf[ndx2];
            double cmb = c-b;
            y_n = b + frac*(cmb - (1.-frac)/6. * ((d-a - 3.0*cmb) * frac+d+ 2.0*a - 3.0*b));
        }
        if(!x->x_gain)
            ain[i] = ain[i] == 0 ? 0 : copysign(exp(log(0.001) * ms/fabs(ain[i])), ain[i]);
        double output = (double)xin[i] + y_n * (double)ain[i];
        out[i] = (t_float)output;
        if(!x->x_freeze)
            x->x_ybuf[x->x_wp] = output;       // write output to buffer
        x->x_wp = (x->x_wp + 1) % x->x_sz; // increment and wrap write head
    };
    return(w+7);
}

static void fbdelay_dsp(t_fbdelay *x, t_signal **sp){
    t_float sr_khz = (t_float)sp[0]->s_sr * 0.001;
    if(sr_khz != x->x_sr_khz){
        x->x_sr_khz = sr_khz;
        fbdelay_sz(x);
    };
    dsp_add(fbdelay_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void fbdelay_size(t_fbdelay *x, t_floatarg f1){
    if(f1 < 0)
        f1 = 0;
    x->x_maxdel = f1;
    if(!x->x_ms)
        x->x_maxdel /= x->x_sr_khz;
    fbdelay_sz(x);
}

static void fbdelay_gain(t_fbdelay *x, t_floatarg f1){
    x->x_gain = f1 != 0;
}

static void *fbdelay_new(t_symbol *s, int argc, t_atom * argv){
    t_fbdelay *x = (t_fbdelay *)pd_new(fbdelay_class);
    t_symbol *cursym = s; // get rid of warning
    x->x_sr_khz = sys_getsr() * 0.001;
    x->x_alloc = 0;
    x->x_sz = FBD_STACK;
    x->x_ybuf = x->x_fbstack;   // set pointer to stack
    fbdelay_clear(x);           // clear out stack buf
    float del_time = 0;
    float delsize = 1000;
    float fb = 0;
    x->x_freeze = 0;
    x->x_gain = 0;
    x->x_ms = 1;
/////////////////////////////////////////////////////////////////////////////////
    int symarg = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_SYMBOL){
            if(!symarg)
                symarg = 1;
            cursym = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(cursym->s_name, "-size")){
                if(argc >= 2 && (argv+1)->a_type == A_FLOAT){
                    t_float curfloat = atom_getfloatarg(1, argc, argv);
                    delsize = curfloat < 0 ? 0 : curfloat;
                    argc -= 2;
                    argv += 2;
                    argnum++;
                }
                else
                    goto errstate;
            }
            else if(!strcmp(cursym->s_name, "-samps")){
                x->x_ms = 0;
                argc--;
                argv++;
                argnum++;
            }
            else if(!strcmp(cursym->s_name, "-gain")){
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
                {
                    del_time = (argval < 0 ? 0 : argval);
                    if(del_time > 0)
                        delsize = del_time;
                }
                    break;
                case 1:
                    fb = argval;
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
    x->x_maxdel = delsize;
    if(!x->x_ms)
        x->x_maxdel /= x->x_sr_khz;
    fbdelay_sz(x);
    x->x_dellet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_dellet, del_time);
    x->x_alet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_alet, fb);
    x->x_outlet = outlet_new((t_object *)x, &s_signal);
    return (x);
errstate:
    pd_error(x, "[fbdelay~]: improper args");
    return NULL;
}

static void * fbdelay_free(t_fbdelay *x){
    if(x->x_alloc)
        free(x->x_ybuf);
    inlet_free(x->x_dellet);
    inlet_free(x->x_alet);
    outlet_free(x->x_outlet);
    return(void *)x;
}

void fbdelay_tilde_setup(void){
    fbdelay_class = class_new(gensym("fbdelay~"), (t_newmethod)fbdelay_new,
                              (t_method)fbdelay_free, sizeof(t_fbdelay), 0, A_GIMME, 0);
    class_addmethod(fbdelay_class, nullfn, gensym("signal"), 0);
    class_addmethod(fbdelay_class, (t_method)fbdelay_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(fbdelay_class, (t_method)fbdelay_clear, gensym("clear"), 0);
    class_addmethod(fbdelay_class, (t_method)fbdelay_size, gensym("size"), A_DEFFLOAT, 0);
    class_addmethod(fbdelay_class, (t_method)fbdelay_gain, gensym("gain"), A_DEFFLOAT, 0);
    class_addmethod(fbdelay_class, (t_method)fbdelay_freeze, gensym("freeze"), A_DEFFLOAT, 0);
}
        
