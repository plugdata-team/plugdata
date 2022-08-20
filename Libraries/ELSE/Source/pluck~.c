// Porres 2017-2022

#include "m_pd.h"
#include "random.h"
#include <math.h>
#include <stdlib.h>

#define TWO_PI      (3.14159265358979323846 * 2)
#define DEF_RADIANS (0.31830989569 * 0.5)
#define PLUCK_STACK 48000 // stack buf size
#define PLUCK_MAXD  4294967294 // max delay = 2**32 - 2

static t_class *pluck_class;

typedef struct _pluck{
    t_object        x_obj;
    t_random_state  x_rstate;
    t_inlet        *x_freq_inlet;
    t_inlet        *x_decay_inlet;
    t_inlet        *x_cutoff_inlet;
    float           x_sr;
    float           x_freq;
    float           x_float_trig;
    int             x_control_trig;
    int             x_noise_input;
    // pointers to the delay buf
    double         *x_ybuf;
    double          x_fbstack[PLUCK_STACK];
    int             x_alloc;                    // if we are using allocated buf
    unsigned int    x_sz;                       // actual size of each delay buffer
    t_float         x_maxdel;                   // maximum delay in ms
    unsigned int    x_wh;                       // writehead
    float           x_sum;
    float           x_amp;
    float           x_last_trig;
    double          x_xnm1;
    double          x_ynm1;
    double          x_f;
    float           x_ain;
    double          x_delms;
    int             x_samps;
    double          x_fb;
    double          x_a0;
    double          x_a1;
    double          x_b1;
}t_pluck;

static void pluck_bang(t_pluck *x){
    x->x_control_trig = 1;
}

static void pluck_float(t_pluck *x, t_float f){
    if(f != 0){
        x->x_float_trig = f;
        pluck_bang(x);
    }
}

static void pluck_clear(t_pluck *x){
    for(unsigned int i = 0; i < x->x_sz; i++)
        x->x_ybuf[i] = 0.;
    x->x_wh = 0;
    x->x_xnm1 = x->x_ynm1 = 0.;
}

static void pluck_sz(t_pluck *x){
    // helper function to deal with allocation issues if needed
    // ie if wanted size x->x_maxdel is bigger than stack, allocate

    // convert ms to samps
    unsigned int newsz = (unsigned int)ceil((double)x->x_maxdel*0.001*(double)x->x_sr);
    newsz++; // add a sample for good measure since say bufsize is 2048 and
    // you want a delay of 2048 samples,.. problem!
    int alloc = x->x_alloc;
    unsigned int cursz = x->x_sz; //current size
    if(newsz > PLUCK_MAXD)
        newsz = PLUCK_MAXD;
    if(!alloc && newsz > PLUCK_STACK){
        x->x_ybuf = (double *)malloc(sizeof(double)*newsz);
        x->x_alloc = 1;
        x->x_sz = newsz;
    }
    else if(alloc && newsz > cursz){
        x->x_ybuf = (double *)realloc(x->x_ybuf, sizeof(double)*newsz);
        x->x_sz = newsz;
    }
    else if(alloc && newsz < PLUCK_STACK){
        free(x->x_ybuf);
        x->x_sz = PLUCK_STACK;
        x->x_ybuf = x->x_fbstack;
        x->x_alloc = 0;
    };
    pluck_clear(x);
}

static double pluck_getlin(double tab[], unsigned int sz, double idx){
    // linear interpolated reader, copied from Derek Kwan's library
    double output;
    unsigned int tabphase1 = (unsigned int)idx;
    unsigned int tabphase2 = tabphase1 + 1;
    double frac = idx - (double)tabphase1;
    if(tabphase1 >= sz - 1){
        tabphase1 = sz - 1; // checking to see if index falls within bounds
        output = tab[tabphase1];
    }
    else{
        double yb = tab[tabphase2]; // linear interp
        double ya = tab[tabphase1];
        output = ya+((yb-ya)*frac);
    };
    return(output);
}

static double pluck_read_delay(t_pluck *x, double arr[], int in_samps){
    double rh = in_samps < 1 ? 1 : (double)in_samps; // read head size in samples
    rh = (double)x->x_wh + ((double)x->x_sz-rh); // subtract from writehead to find buffer position
    while(rh >= x->x_sz) // wrap to delay buffer length
        rh -= (double)x->x_sz;
    return(pluck_getlin(arr, x->x_sz, rh)); // read from buffer
}

static void update_coeffs(t_pluck *x, double f){
    x->x_f = f;
    double omega = f * TWO_PI/x->x_sr;
    if(omega < 0)
        omega = 0;
    if(omega > 2){
        x->x_a0 = 1;
        x->x_a1 = x->x_b1 = 0;
    }
    else{
        x->x_a0 = x->x_a1 = omega * 0.5;
        x->x_b1 = 1 - omega;
    }
}

static void update_fb(t_pluck *x, double fb, double delms){
    x->x_ain = (float)fb;
    x->x_fb = fb == 0 ? 0 : copysign(exp(log(0.001) * delms/fabs(fb)), fb);
}

static void update_time(t_pluck *x, float hz){
    x->x_freq = hz;
    double period = 1./(double)hz;
    x->x_delms = period * 1000;
    x->x_samps = (int)roundf(period * x->x_sr);
}

////////////////////////////////////////////////////////////////////////////////////

static t_int *pluck_perform_noise_input(t_int *w){
    t_pluck *x = (t_pluck *)(w[1]);
    int n = (int)(w[2]);
    t_float *t_in = (t_float *)(w[3]);
    t_float *hz_in = (t_float *)(w[4]);
    t_float *ain = (t_float *)(w[5]);
    t_float *cut_in = (t_float *)(w[6]);
    t_float *noise_in = (t_float *)(w[7]);
    t_float *out = (t_float *)(w[8]);
    t_float sr = x->x_sr;
    t_float nyq = sr * 0.5;
    t_float last_trig = x->x_last_trig;
    t_float sum = x->x_sum;
    t_float amp = x->x_amp;
    double xnm1 = x->x_xnm1;
    double ynm1 = x->x_ynm1;
    for(int i = 0; i < n; i++){
        t_float hz = hz_in[i];
        t_float trig = t_in[i];
        float a_in = ain[i];
        if(hz < 1){
            out[i] = sum = 0;
            xnm1 = ynm1 = 0;
        }
        else{
            if(hz != x->x_freq){
                update_time(x, hz);
                goto update_fb;
            }
            if(x->x_ain != a_in){
                update_fb:
                update_fb(x, a_in, x->x_delms);
            }
            double fb_del = pluck_read_delay(x, x->x_ybuf, x->x_samps); // get delayed vals
            if((trig != 0 && last_trig == 0) || x->x_control_trig){ // trigger
                amp = x->x_control_trig ? x->x_float_trig : trig;
                sum = 0;
                x->x_control_trig = 0;
            }
// Filter stuff
            double cuttoff = (double)cut_in[i];
            if(cuttoff < 0.000001)
                cuttoff = 0.000001;
            if(cuttoff > nyq - 0.000001)
                cuttoff = nyq - 0.000001;
            if(x->x_f != cuttoff)
                update_coeffs(x, cuttoff);
            // gate
            t_float gate = (sum++ <= x->x_samps) * amp;
            // noise
            t_float noise = gate ? noise_in[i] * gate : 0;
            // output
            double output = (double)noise + x->x_fb * fb_del;
            out[i] = output;
            
            double yn = x->x_a0 * output + x->x_a1 * xnm1 + x->x_b1 * ynm1;

            // put into delay buffer
            int wh = x->x_wh;
            x->x_ybuf[wh] = yn;
            x->x_wh = (wh + 1) % x->x_sz; // increment writehead
            
            last_trig = trig;
            xnm1 = output;
            ynm1 = yn;
        }
    };
    x->x_sum = sum; // next
    x->x_last_trig = amp;
    x->x_last_trig = last_trig;
    x->x_xnm1 = xnm1;
    x->x_ynm1 = ynm1;
    return(w+9);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

static t_int *pluck_perform(t_int *w){
    t_pluck *x = (t_pluck *)(w[1]);
    int n = (int)(w[2]);
    t_random_state *rstate = (t_random_state *)(w[3]);
    t_float *t_in = (t_float *)(w[4]);
    t_float *hz_in = (t_float *)(w[5]);
    t_float *ain = (t_float *)(w[6]);
    t_float *cut_in = (t_float *)(w[7]);
    t_float *out = (t_float *)(w[8]);
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    t_float nyq = x->x_sr * 0.5;
    t_float last_trig = x->x_last_trig;
    t_float sum = x->x_sum;
    t_float amp = x->x_amp;
    double xnm1 = x->x_xnm1;
    double ynm1 = x->x_ynm1;
    for(int i = 0; i < n; i++){
        t_float hz = hz_in[i];
        t_float trig = t_in[i];
        float a_in = ain[i];
        if(hz < 1){
            out[i] = sum = 0;
            xnm1 = ynm1 = 0;
        }
        else{
            if(hz != x->x_freq){
                update_time(x, hz);
                goto update_fb;
            }
            if(x->x_ain != a_in){
                update_fb:
                update_fb(x, a_in, x->x_delms);
            }
            double fb_del = pluck_read_delay(x, x->x_ybuf, x->x_samps); // get delayed vals
            if((trig != 0 && last_trig == 0) || x->x_control_trig){ // trigger
                amp = x->x_control_trig ? x->x_float_trig : trig;
                sum = 0;
                x->x_control_trig = 0;
            }
            // Filter stuff
            double cuttoff = (double)cut_in[i];
            if(cuttoff < 0.000001)
                cuttoff = 0.000001;
            if(cuttoff > nyq - 0.000001)
                cuttoff = nyq - 0.000001;
            if(x->x_f != cuttoff)
                update_coeffs(x, cuttoff);
            // gate
            t_float gate = (sum++ <= x->x_samps) * amp;
            // noise
            t_float noise = (gate != 0) ? (t_float)(random_frand(s1, s2, s3)) * gate : 0;
            // output
            double output = (double)noise + x->x_fb * fb_del;
            out[i] = output;
        
            double yn = x->x_a0 * output + x->x_a1 * xnm1 + x->x_b1 * ynm1;
            
            // put into delay buffer
            int wh = x->x_wh;
            x->x_ybuf[wh] = yn;
            x->x_wh = (wh + 1) % x->x_sz; // increment writehead
            last_trig = trig;
            
            xnm1 = output;
            ynm1 = yn;
        }
    };
    x->x_sum = sum; // next
    x->x_last_trig = amp;
    x->x_last_trig = last_trig;
    x->x_xnm1 = xnm1;
    x->x_ynm1 = ynm1;
    return(w+9);
}

static void pluck_dsp(t_pluck *x, t_signal **sp){
    int sr = sp[0]->s_sr;
    if(sr != x->x_sr){ // if new sample rate isn't old sample rate, need to realloc
        x->x_sr = sr;
        pluck_sz(x);
        update_coeffs(x, x->x_f);
    };
    if(x->x_noise_input)
        dsp_add(pluck_perform_noise_input, 8, x, sp[0]->s_n, sp[0]->s_vec,
                sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec);
    else
        dsp_add(pluck_perform, 8, x, sp[0]->s_n, &x->x_rstate, sp[0]->s_vec,
                sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void *pluck_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_pluck *x = (t_pluck *)pd_new(pluck_class);
    x->x_sr = sys_getsr();
    static int seed = 1;
    random_init(&x->x_rstate, seed++);
    float freq = 0;
    float decay = 0;
    float cut_freq = DEF_RADIANS * x->x_sr;
    x->x_float_trig = 1;
    x->x_control_trig = 0;
    x->x_noise_input = 0;
    int argnum = 0;
    while(argc > 0){
        if(argv->a_type == A_FLOAT){ //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    freq = argval;
                    break;
                case 1:
                    decay = argval;
                    break;
                case 2:
                    cut_freq = argval < 0 ? 0 : argval;
                    break;
                default:
                    break;
            };
            argnum++;
            argc--, argv++;
        }
        else if(argv->a_type == A_SYMBOL && !argnum){
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(curarg == gensym("-in")){
                x->x_noise_input = 1;
                argc--, argv++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    };
    x->x_alloc = x->x_last_trig = 0;
    x->x_xnm1 = x->x_ynm1 = 0.;
    x->x_sum = PLUCK_MAXD;
    x->x_sz = PLUCK_STACK;
// clear out stack buf, set pointer to stack
    x->x_ybuf = x->x_fbstack;
    pluck_clear(x);
    x->x_freq = (double)freq;
    x->x_ain = decay;
    x->x_f = (double)cut_freq;
    x->x_maxdel = 1000;
// ship off to the helper method to deal with allocation if necessary
    pluck_sz(x);
    
    if(x->x_freq > 1){
        update_time(x, x->x_freq);
        update_fb(x, x->x_ain, x->x_delms);
    }
    if(x->x_f >= 0)
        update_coeffs(x, x->x_f);
// inlets / outlet
    x->x_freq_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_freq_inlet, freq);
    x->x_decay_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal); // decay time
        pd_float((t_pd *)x->x_decay_inlet, decay);
    x->x_cutoff_inlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_cutoff_inlet, cut_freq);
    if(x->x_noise_input)
        inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    outlet_new((t_object *)x, &s_signal);
    return(x);
errstate:
    pd_error(x, "[pluck~]: improper args");
    return(NULL);
}

static void * pluck_free(t_pluck *x){
    if(x->x_alloc)
        free(x->x_ybuf);
    inlet_free(x->x_freq_inlet);
    inlet_free(x->x_decay_inlet);
    inlet_free(x->x_cutoff_inlet);
    return(void *)x;
}

void pluck_tilde_setup(void){
    pluck_class = class_new(gensym("pluck~"), (t_newmethod)pluck_new,
        (t_method)pluck_free, sizeof(t_pluck), 0, A_GIMME, 0);
    class_addmethod(pluck_class, nullfn, gensym("signal"), 0);
    class_addfloat(pluck_class, pluck_float);
    class_addbang(pluck_class, pluck_bang);
    class_addmethod(pluck_class, (t_method)pluck_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(pluck_class, (t_method)pluck_clear, gensym("clear"), 0);
}
