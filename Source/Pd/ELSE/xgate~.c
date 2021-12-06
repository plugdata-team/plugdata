// porres 2017-2020

#include "m_pd.h"
#include <stdlib.h>
#include <math.h>

static t_class *xgate_class;

#define OUTPUTLIMIT 512
#define HALF_PI (3.14159265358979323846 * 0.5)

typedef struct _xgate {
    t_object    x_obj;
    int         x_channel;
    int         x_lastchannel;
    int         x_outlets;
    double      x_fade_in_samps;
    float       x_sr_khz;
    int         x_active_channel[OUTPUTLIMIT];
    int         x_counter[OUTPUTLIMIT];
    double      x_fade[OUTPUTLIMIT];
    float       *x_outs[OUTPUTLIMIT];
    t_outlet    *x_out_status;
}t_xgate;

void xgate_float(t_xgate *x, t_floatarg ch){
  ch = (int)ch > x->x_outlets ? x->x_outlets : (int)ch;
  x->x_channel = ch < 0 ? 0 : ch;
  if(x->x_channel != x->x_lastchannel){
      if(x->x_channel)
          x->x_active_channel[x->x_channel - 1] = 1;
      if(x->x_lastchannel)
          x->x_active_channel[x->x_lastchannel - 1] = 0;
      x->x_lastchannel = x->x_channel;
  }
}

static t_int *xgate_perform(t_int *w){
    int i;
    t_xgate *x = (t_xgate *)(w[1]);
    int n = (int)(w[2]);
    t_float *in = (t_float *)(w[3]);
    for(i = 0; i < x->x_outlets; i++)
        x->x_outs[i] = (t_float *)(w[4 + i]); // all outputs
//    t_float *outputs = x->x_outs[i];
    while(n--){
        float input = *in++;
        for(i = 0; i < x->x_outlets; i++){
// fade in/out counter
            if(x->x_active_channel[i] && x->x_counter[i] < x->x_fade_in_samps)
                x->x_counter[i]++;
            else if(!x->x_active_channel[i] && x->x_counter[i] > 0){
                x->x_counter[i]--;
                if(x->x_counter[i] == 0){
                    t_atom at[2];
                    SETFLOAT(at, i + 1);
                    SETFLOAT(at+1, 0);
                    outlet_list(x->x_out_status, gensym("list"), 2, at);
                }
            }
// calculate fade value
            x->x_fade[i] = x->x_counter[i] / x->x_fade_in_samps;
            x->x_fade[i] = sin(x->x_fade[i] * HALF_PI); // Equal Power
// set fade to channel
            *x->x_outs[i]++ = input * x->x_fade[i];
        }
    }
    return(w + 4 + x->x_outlets);
}

static void xgate_dsp(t_xgate *x, t_signal **sp) {
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    int i, count = x->x_outlets + 3;
    t_int **sigvec;
    sigvec  = (t_int **) calloc(count, sizeof(t_int *));
    for(i = 0; i < count; i++)
        sigvec[i] = (t_int *) calloc(sizeof(t_int), 1); // init sigvec
    sigvec[0] = (t_int *)x; // 1st => object
    sigvec[1] = (t_int *)sp[0]->s_n; // 2nd => block (n)
    for(i = 2; i < count; i++) // in/outs
        sigvec[i] = (t_int *)sp[i-2]->s_vec;
    dsp_addv(xgate_perform, count, (t_int *) sigvec);
    free(sigvec);
}

static void xgate_time(t_xgate *x, t_floatarg ms) {
    int i;
    double last_fade_in_samps = x->x_fade_in_samps;
    ms = ms < 0 ? 0 : ms;
    x->x_fade_in_samps = x->x_sr_khz * ms + 1;
    for(i = 0; i < x->x_outlets; i++)
        if(x->x_counter[i]) // adjust counters
            x->x_counter[i] = x->x_counter[i] / last_fade_in_samps * x->x_fade_in_samps;
}

static void *xgate_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_xgate *x = (t_xgate *)pd_new(xgate_class);
    x->x_sr_khz = sys_getsr() * 0.001;
    t_float ch = 1, ms = 0, init_channel = 0;
    int i;
    int argnum = 0;
    while(argc){
        if(argv -> a_type == A_FLOAT) { //if current argument is a float
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
                    ch = argval;
                    break;
                case 1:
                    ms = argval;
                    break;
                case 2:
                    init_channel = argval;
                default:
                    break;
            };
        }
        argnum++;
        argc--;
        argv++;
    };
    x->x_outlets = ch < 1 ? 1 : ch;
    if(x->x_outlets > OUTPUTLIMIT)
        x->x_outlets = OUTPUTLIMIT;
    for(i = 0; i < x->x_outlets; i++)
        outlet_new(&x->x_obj, gensym("signal"));
    x->x_out_status = outlet_new(&x->x_obj, &s_list);
    ms = ms > 0 ? ms : 0;
    x->x_fade_in_samps = x->x_sr_khz * ms + 1;
    x->x_lastchannel = 0;
    for(i = 0; i < OUTPUTLIMIT; i++){
        x->x_active_channel[i] = 0;
        x->x_counter[i] = 0;
        x->x_fade[i] = 0;
    }
    xgate_float(x, init_channel);
    return(x);
}

void xgate_tilde_setup(void) {
    xgate_class = class_new(gensym("xgate~"), (t_newmethod)xgate_new, 0,
        sizeof(t_xgate), CLASS_DEFAULT, A_GIMME, 0);
    class_addfloat(xgate_class, (t_method)xgate_float);
    class_addmethod(xgate_class, nullfn, gensym("signal"), 0);
    class_addmethod(xgate_class, (t_method)xgate_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(xgate_class, (t_method)xgate_time, gensym("time"), A_FLOAT, 0);
}
