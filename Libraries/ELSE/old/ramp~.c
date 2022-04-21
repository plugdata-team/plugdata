// Porres 2017

#include "m_pd.h"
#include "math.h"
#include <string.h>

#define MAX_LIMIT 0x7fffffff

static t_class *ramp_class;

typedef struct _ramp{
    t_object    x_obj;
    double      x_phase;
    float       x_min;
    float       x_max;
    float       x_inc;
    float       x_reset;
    int         x_continue;
    int         x_mode;
    int         x_clip;
    t_float     x_lastin;
    t_inlet     *x_inlet_inc;
    t_inlet     *x_inlet_min;
    t_inlet     *x_inlet_max;
    t_outlet    *x_outlet;
    t_outlet    *x_bangout;
    t_clock     *x_clock;
}t_ramp;

static void ramp_tick(t_ramp *x){
    outlet_bang(x->x_bangout);
}

static void ramp_float(t_ramp *x, t_floatarg f){
    x->x_phase = f;
}

static void ramp_mode(t_ramp *x, t_floatarg f){
    int i;
    i = (int)f;
    if (i < 0 )
        i = 0;
    if (i > 2)
        i = 2;
    x->x_mode = i;
}

static void ramp_bang(t_ramp *x){
    x->x_phase = x->x_reset;
    if(!x->x_continue)
        x->x_continue = 1;
}

static void ramp_reset(t_ramp *x){
    x->x_phase = x->x_reset;
    x->x_continue = 0;
}

static void ramp_stop(t_ramp *x){
    x->x_continue = 0;
}

static void ramp_start(t_ramp *x){
    x->x_continue = 1;
}

static void ramp_set(t_ramp *x, t_floatarg f){
    x->x_reset = f;
}

static t_int *ramp_perform(t_int *w){
    t_ramp *x = (t_ramp *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // trigger
    t_float *in2 = (t_float *)(w[4]); // increment
    t_float *in3 = (t_float *)(w[5]); // min
    t_float *in4 = (t_float *)(w[6]); // max
    t_float *out = (t_float *)(w[7]);
    double phase = x->x_phase;
    float reset = x->x_reset; // reset point
    t_float lastin = x->x_lastin;
    double output;
    while(nblock--){
        float trig = *in1++;        // trigger
        double phase_step = *in2++; // phase_step
        float min = *in3++;         // min
        float max = *in4++;         // max
        float range = max - min;
        if(range == 0)
            output = min;
        else{
            if(trig > 0 && lastin <= 0){
                phase = reset;
                if(!x->x_continue)
                    x->x_continue = 1;
            }
            else{
                if(min > max){ // swap values
                    float temp;
                    temp = max;
                    max = min;
                    min = temp;
                };
                if(x->x_mode == 0){ // loop
                    x->x_clip = 0;
                    if(phase <= min || phase >= max){
                        if(phase <= min && phase_step < 0){
                            while(phase <= min)
                                phase += range;
                            clock_delay(x->x_clock, 0); // outlet bang
                        }
                        else if(phase >= max && phase_step > 0){
                            while(phase >= max)
                                phase -= range;
                            clock_delay(x->x_clock, 0); // outlet bang
                        }
                    }
                }
                else if(x->x_mode == 1){ // clip
                    if(phase < min && phase_step < 0){
                        phase = min;
                        if(!x->x_clip){
                            clock_delay(x->x_clock, 0); // outlet bang
                            x->x_clip = 1;
                        }
                    }
                    if(phase > max && phase_step > 0){
                        phase = max;
                        if(!x->x_clip){
                            clock_delay(x->x_clock, 0); // outlet bang
                            x->x_clip = 1;
                        }
                    }
                    else
                        x->x_clip = 0;
                }
                else if (x->x_mode == 2){// reset
                    x->x_clip = 0;
                    if(phase > max && phase_step > 0){
                        phase = reset;
                        x->x_continue = 0;
                        clock_delay(x->x_clock, 0); // outlet bang
                    }
                    if(phase < min && phase_step < 0){
                        phase = reset;
                        x->x_continue = 0;
                        clock_delay(x->x_clock, 0); // outlet bang
                    }
                }
            }
            output = phase;
        }
        *out++ = output;
        lastin = trig;
        if(x->x_continue)
            phase += phase_step; // next phase
    }
    x->x_phase = phase;
    x->x_lastin = lastin; // last input
    return (w + 8);
}

static void ramp_dsp(t_ramp *x, t_signal **sp){
    dsp_add(ramp_perform, 7, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec);
}

static void *ramp_free(t_ramp *x){
    if(x->x_clock)
        clock_free(x->x_clock);
    inlet_free(x->x_inlet_inc);
    inlet_free(x->x_inlet_min);
    inlet_free(x->x_inlet_max);
    outlet_free(x->x_outlet);
    return (void *)x;
}

static void *ramp_new(t_symbol *s, int argc, t_atom *argv){
    t_ramp *x = (t_ramp *)pd_new(ramp_class);
///////////////////////////
    t_symbol *curarg = s; // get rid of warning
    x->x_lastin = 0;
    x->x_reset = 0;
    x->x_min = 0.;
    x->x_max = MAX_LIMIT;
    x->x_inc = 1.;
    x->x_continue = 1.;
    float mode = 0.;
    if(argc){
        int numargs = 0;
        while(argc > 0 ){
            if(argv -> a_type == A_FLOAT){
                switch(numargs){
                    case 0: x->x_inc = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 1: x->x_min = x->x_reset = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 2: x->x_max = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 3: x->x_reset = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    default:
                        argc--;
                        argv++;
                        break;
                };
            }
            else if (argv -> a_type == A_SYMBOL){
                curarg = atom_getsymbolarg(0, argc, argv);
                int isoff = strcmp(curarg->s_name, "-off") == 0;
                int ismode = strcmp(curarg->s_name, "-mode") == 0;
                if(ismode && argc >= 2){
                    t_symbol *arg1 = atom_getsymbolarg(1, argc, argv);
                    if(arg1 == &s_){
                        mode = atom_getfloatarg(1, argc, argv);
                        argc -= 2;
                        argv += 2;
                    }
                    else{
                        goto errstate;
                    };
                }
                else if(isoff){
                    x->x_continue = 0;
                    argc --;
                    argv ++;
                    }
                else
                    goto errstate;
            }
            else
                goto errstate;
        };
    }
///////////////////////////
    mode = (int)mode;
    if (mode <= 0 )
        mode = 0;
    if (mode >= 2)
        mode = 2;
    x->x_mode = mode;
    x->x_phase = (double)x->x_reset ;
    x->x_inlet_inc = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_inc, x->x_inc);
    x->x_inlet_min = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_min, x->x_min);
    x->x_inlet_max = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_max, x->x_max);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    x->x_bangout = outlet_new((t_object *)x, &s_bang);
    x->x_clock = clock_new(x, (t_method)ramp_tick);
    return (x);
    errstate:
        pd_error(x, "ramp~: improper args");
        return NULL;
}

void ramp_tilde_setup(void)
{
    ramp_class = class_new(gensym("ramp~"),
        (t_newmethod)ramp_new, (t_method)ramp_free,
        sizeof(t_ramp), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(ramp_class, nullfn, gensym("signal"), 0);
    class_addmethod(ramp_class, (t_method)ramp_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(ramp_class, (t_method)ramp_bang);
    class_addfloat(ramp_class, (t_method)ramp_float);
    class_addmethod(ramp_class, (t_method)ramp_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(ramp_class, (t_method)ramp_mode, gensym("mode"), A_FLOAT, 0);
    class_addmethod(ramp_class, (t_method)ramp_reset, gensym("reset"), 0);
    class_addmethod(ramp_class, (t_method)ramp_stop, gensym("stop"), 0);
    class_addmethod(ramp_class, (t_method)ramp_start, gensym("start"), 0);
}
