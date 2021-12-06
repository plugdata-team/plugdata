// porres 2017-2020

#include "m_pd.h"
#include <stdlib.h>
#include <math.h>

static t_class *xselect_class;

#define INPUTLIMIT 512
#define HALF_PI (3.14159265358979323846 * 0.5)

typedef struct _xselect{
    t_object    x_obj;
    int         x_channel;
    int         x_lastchannel;
    int         x_ninlets;
    double      x_fade_in_samps;
    float       x_sr_khz;
    int         x_active_channel[INPUTLIMIT];
    int         x_counter[INPUTLIMIT];
    double      x_fade[INPUTLIMIT];
    float       *x_in[INPUTLIMIT];
    t_outlet    *x_out_status;
}t_xselect;

void xselect_float(t_xselect *x, t_floatarg ch){
  ch = (int)ch > x->x_ninlets ? x->x_ninlets : (int)ch;
  x->x_channel = ch < 0 ? 0 : (int)ch;
  if(x->x_channel != x->x_lastchannel){
      if(x->x_channel)
          x->x_active_channel[x->x_channel - 1] = 1;
      if(x->x_lastchannel)
          x->x_active_channel[x->x_lastchannel - 1] = 0;
      x->x_lastchannel = x->x_channel;
  }
}

static t_int *xselect_perform(t_int *w){
    int i;
    t_xselect *x = (t_xselect *)(w[1]);
    int n = (int)(w[2]);
    for(i = 0; i < x->x_ninlets; i++)
        x->x_in[i] = (t_float *)(w[3 + i]); // all inputs
    float *out = (t_float *)(w[3 + x->x_ninlets]);
    while(n--){
        float sum = 0;
        for(i = 0; i < x->x_ninlets; i++){
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
            x->x_fade[i] = x->x_counter[i] / x->x_fade_in_samps;
            x->x_fade[i] = sin(x->x_fade[i] * HALF_PI); // equal power
            sum += *x->x_in[i]++ * x->x_fade[i];
        }
        *out++ = sum;
    }
    return(w + 4 + x->x_ninlets);
}

static void xselect_dsp(t_xselect *x, t_signal **sp) {
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    int i, count = x->x_ninlets + 3;
    t_int **sigvec;
    sigvec = (t_int **) calloc(count, sizeof(t_int *));
    for(i = 0; i < count; i++)
        sigvec[i] = (t_int *) calloc(sizeof(t_int), 1); // init sigvec
    sigvec[0] = (t_int *)x; // 1st => object
    sigvec[1] = (t_int *)sp[0]->s_n; // 2nd => block (n)
    for(i = 2; i < count; i++) // ins/out
        sigvec[i] = (t_int *)sp[i-2]->s_vec;
    dsp_addv(xselect_perform, count, (t_int *) sigvec);
    free(sigvec);
}

static void xselect_time(t_xselect *x, t_floatarg ms){
    int i;
    double last_fade_in_samps = x->x_fade_in_samps;
    ms = ms < 0 ? 0 : ms;
    x->x_fade_in_samps = x->x_sr_khz * ms;
    for(i = 0; i < x->x_ninlets; i++)
        if(x->x_counter[i]) // adjust counters
            x->x_counter[i] = x->x_counter[i] / last_fade_in_samps * x->x_fade_in_samps;
}

static void *xselect_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_xselect *x = (t_xselect *)pd_new(xselect_class);
    x->x_sr_khz = sys_getsr() * 0.001;
    t_float ch = 1, ms = 0, init_channel = 0;
    int i;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){ //if current argument is a float
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
        };
        argnum++;
        argc--;
        argv++;
    };
    x->x_ninlets = ch < 1 ? 1 : ch;
    if(x->x_ninlets > INPUTLIMIT)
        x->x_ninlets = INPUTLIMIT;
    for(i = 0; i < x->x_ninlets - 1; i++)
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_out_status = outlet_new(&x->x_obj, &s_list);
    ms = ms > 0 ? ms : 0;
    x->x_fade_in_samps = x->x_sr_khz * ms + 1;
    x->x_lastchannel = 0;
    for(i = 0; i < INPUTLIMIT; i++){
        x->x_active_channel[i] = 0;
        x->x_counter[i] = 0;
        x->x_fade[i] = 0;
    }
    xselect_float(x, init_channel);
    return(x);
}

void xselect_tilde_setup(void){
    xselect_class = class_new(gensym("xselect~"), (t_newmethod)xselect_new, 0,
        sizeof(t_xselect), CLASS_DEFAULT, A_GIMME, 0);
    class_addfloat(xselect_class, (t_method)xselect_float);
    class_addmethod(xselect_class, nullfn, gensym("signal"), 0);
    class_addmethod(xselect_class, (t_method)xselect_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(xselect_class, (t_method)xselect_time, gensym("time"), A_FLOAT, 0);
}
