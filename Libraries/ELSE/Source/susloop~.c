// Porres 2017

#include "m_pd.h"
#include "math.h"

#define MAX_LIMIT 0x7fffffff

static t_class *susloop_class;

typedef struct _susloop{
    t_object    x_obj;
    double      x_phase;
    float       x_start;
    float       x_end;
    float       x_max;
    float       x_inc;
    int         x_loop;
    int         x_pause;
    int         x_up;
    int         x_continue;
    int         x_status;
    t_float     x_lastin;
    t_inlet     *x_inlet_inc;
    t_inlet     *x_inlet_start;
    t_inlet     *x_inlet_end;
    t_inlet     *x_inlet_max;
    t_outlet    *x_outlet;
    t_outlet    *x_outgate;
}t_susloop;

static void susloop_loop(t_susloop *x, t_floatarg f1, t_floatarg f2){
    if(f1 > 0 && f2 > 0){
        x->x_start = f1;
        x->x_end = f2;
    }
}

static void susloop_max(t_susloop *x, t_floatarg f){
    if(f > 0)
        x->x_max = f;
}

static void susloop_bang(t_susloop *x){
    if(x->x_up)
        x->x_phase =  0;
    else
        x->x_phase = x->x_max;
    if(!x->x_continue)
        x->x_continue = 1;
    if(!x->x_loop)
        x->x_loop = 1;
    if(x->x_pause)
        x->x_pause = 0;
    if(!x->x_status) // on
        outlet_float(x->x_outgate, x->x_status = 1);
}

static void susloop_stop(t_susloop *x){
    x->x_continue = x->x_pause = x->x_loop = 0;
    if(x->x_up)
        x->x_phase =  0;
    else
        x->x_phase =  x->x_max;
    if(x->x_status) // on
        outlet_float(x->x_outgate, x->x_status = 0);
}

static void susloop_float(t_susloop *x, t_floatarg f){
    if(f != 0)
        susloop_bang(x);
    else
        x->x_loop = 0;
}

static void susloop_release(t_susloop *x){
    x->x_loop = 0;
}

static void susloop_pause(t_susloop *x){
    if(!x->x_pause && x->x_continue){
        x->x_continue = 0;
        x->x_pause = 1;
    }
}

static void susloop_continue(t_susloop *x){
    if(x->x_pause){
        x->x_continue = 1;
        x->x_pause = 0;
    }
}

static t_int *susloop_perform(t_int *w){
    t_susloop *x = (t_susloop *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // gate
    t_float *in2 = (t_float *)(w[4]); // rate
    t_float *out = (t_float *)(w[5]);
    double phase = x->x_phase;
    float loop_start = x->x_start;
    float loop_end = x->x_end;
    float max = x->x_max;
    t_float lastin = x->x_lastin;
    while(nblock--){
        float gate = *in1++;
        double phase_step = *in2++;
        if(phase_step > 0 && !x->x_up)
            x->x_up = 1;
        if(phase_step < 0 && x->x_up)
            x->x_up = 0;
        if(gate != 0 && lastin == 0){
            if(phase_step > 0)
                phase = 0;
            if(phase_step < 0)
                phase = max;
            if(!x->x_continue)
                x->x_continue = 1;
            if(!x->x_loop)
                x->x_loop = 1;
            if(x->x_pause)
                x->x_pause = 0;
            if(!x->x_status) // on
                outlet_float(x->x_outgate, x->x_status = 1);
        }
        else if(gate == 0 && lastin != 0)
            x->x_loop = 0;
        if(!x->x_loop){// don't loop
            if(phase < 0 && phase_step < 0){
                phase = max;
                x->x_continue = 0;
                if(x->x_status) // on
                    outlet_float(x->x_outgate, x->x_status = 0);
            }
            if(phase > max && phase_step > 0){
                phase = x->x_continue = 0;
                if(x->x_status) // on
                    outlet_float(x->x_outgate, x->x_status = 0);
            }
        }
        else{ // loop
            if(loop_start == loop_end)
                phase = loop_start;
            else{
                if(loop_start > loop_end){ // swap
                    float temp;
                    temp = loop_end;
                    loop_end = loop_start;
                    loop_start = temp;
                };
                if(loop_end > max)
                    loop_end = max;
                if(loop_start > max)
                    loop_start = max;
                if(loop_start == 0 && loop_end == max){
                    x->x_loop = 0;
                    if(phase < 0 && phase_step < 0){
                        phase = max;
                        x->x_continue = 0;
                    }
                    if(phase > max && phase_step > 0)
                        phase = x->x_continue = 0;
                }
                else{
                    if(phase < loop_start && phase_step < 0){ // loop
                        float range = loop_end - loop_start;
                        while(phase < loop_start)
                            phase += range;
                    }
                    else if(phase > loop_end && phase_step > 0){ // loop
                        float range = loop_end - loop_start;
                        phase = fmod(phase - loop_start, range)+ loop_start; // looped phase
                    }
                }
            }
        }
        *out++ = phase;
        lastin = gate;
        if(x->x_continue)
            phase += phase_step; // next phase
    }
    x->x_phase = phase;
    x->x_lastin = lastin; // last input
    return(w+6);
}

static void susloop_dsp(t_susloop *x, t_signal **sp){
    dsp_add(susloop_perform, 5, x, sp[0]->s_n,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *susloop_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_susloop *x = (t_susloop *)pd_new(susloop_class);
///////////////////////////
    x->x_lastin = 0;
    x->x_start = 0;
    x->x_end = x->x_max = MAX_LIMIT;
    x->x_inc = 1.;
    x->x_loop = 1.;
    x->x_continue = x->x_pause = x->x_status = 0;
    if(argc){
        int numargs = 0;
        while(argc > 0){
            if(argv -> a_type == A_FLOAT){
                switch(numargs){
                    case 0: x->x_inc = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 1: x->x_start = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 2: x->x_end = atom_getfloatarg(0, argc, argv);
                        numargs++;
                        argc--;
                        argv++;
                        break;
                    case 3: x->x_max = atom_getfloatarg(0, argc, argv);
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
            else
                goto errstate;
        };
    }
///////////////////////////
    if(x->x_start > x->x_end){ // swap
        float temp;
        temp = x->x_end;
        x->x_end = x->x_start;
        x->x_start = temp;
    };
    if(x->x_end > x->x_max)
        x->x_end = x->x_max;
    if(x->x_start > x->x_max)
        x->x_start = x->x_max;
    if(x->x_inc > 0)
        x->x_up = 1;
    else{
        x->x_up = 0;
        x->x_phase = x->x_max;
    }
    x->x_inlet_inc = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_inc, x->x_inc);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    x->x_outgate = outlet_new(&x->x_obj, &s_float);
    return(x);
    errstate:
        pd_error(x, "susloop~: improper args");
        return(NULL);
}

void susloop_tilde_setup(void){
    susloop_class = class_new(gensym("susloop~"),
        (t_newmethod)susloop_new, 0, sizeof(t_susloop), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(susloop_class, nullfn, gensym("signal"), 0);
    class_addmethod(susloop_class, (t_method)susloop_dsp, gensym("dsp"), A_CANT, 0);
    class_addbang(susloop_class, (t_method)susloop_bang);
    class_addfloat(susloop_class, (t_method)susloop_float);
    class_addmethod(susloop_class, (t_method)susloop_stop, gensym("stop"), 0);
    class_addmethod(susloop_class, (t_method)susloop_pause, gensym("pause"), 0);
    class_addmethod(susloop_class, (t_method)susloop_release, gensym("release"), 0);
    class_addmethod(susloop_class, (t_method)susloop_continue, gensym("continue"), 0);
    class_addmethod(susloop_class, (t_method)susloop_loop, gensym("loop"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(susloop_class, (t_method)susloop_max, gensym("size"), A_FLOAT, 0);
}
