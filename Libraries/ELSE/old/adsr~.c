// porres 2017

#include "m_pd.h"
#include <math.h>

#define LOG001 log(0.001)

typedef struct _adsr{
    t_object x_obj;
    t_inlet  *x_inlet_attack;
    t_inlet  *x_inlet_decay;
    t_inlet  *x_inlet_sustain;
    t_inlet  *x_inlet_release;
    t_float  x_f_gate;
    t_float  x_last_gate;
    t_int    x_status;
    t_float  x_last;
    t_float  x_target;
    t_float  x_sustain_target;
    t_float  x_sr_khz;
    t_outlet  *x_out2;
    double   x_incr;
    int      x_nleft;
    int      x_gate_status;
    int      x_retrigger;
    int      x_log;
}t_adsr;


static t_class *adsr_class;

static void adsr_log(t_adsr *x, t_floatarg f){
    x->x_log = (int)(f != 0);
}

static void adsr_bang(t_adsr *x){
    x->x_f_gate = x->x_last_gate;
    if(!x->x_status) // trigger it on
        outlet_float(x->x_out2, x->x_status = 1);
    else
        x->x_retrigger = 1;
}

static void adsr_float(t_adsr *x, t_floatarg f){
    x->x_f_gate = f;
    if(x->x_f_gate != 0){
        x->x_last_gate = x->x_f_gate;
        if(!x->x_status) // trigger it on
            outlet_float(x->x_out2, x->x_status = 1);
        else
            x->x_retrigger = 1;
    }
}

static t_int *adsr_perform(t_int *w){
    t_adsr *x = (t_adsr *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *in4 = (t_float *)(w[6]);
    t_float *in5 = (t_float *)(w[7]);
    t_float *out = (t_float *)(w[8]);
    t_float last = x->x_last;
    t_float target = x->x_target;
    t_float gate_status = x->x_gate_status;
    t_int status = x->x_status;
    double incr = x->x_incr;
    int nleft = x->x_nleft;
    while(nblock--){
        t_float input_gate = *in1++;
        t_float attack = *in2++;
        t_float decay = *in3++;
        t_float sustain_point = *in4++;
        t_float release = *in5++;
// get & clip 'n'; set a/d/r coefs
        t_float n_attack = roundf(attack * x->x_sr_khz);
        if(n_attack < 1)
            n_attack = 1;
        double coef_a = 1. / n_attack;
        t_float n_decay = roundf(decay * x->x_sr_khz);
        if(n_decay < 1)
            n_decay = 1;
        double coef_d = 1. / n_decay;
        t_float n_release = roundf(release * x->x_sr_khz);
        if(n_release < 1)
            n_release = 1;
        double coef_r = 1. / n_release;
// Gate status / get incr & nleft values!
        t_int audio_gate = (input_gate != 0);
        t_int control_gate = (x->x_f_gate != 0);
        if(x->x_retrigger){
            target = x->x_f_gate;
            incr = (target - last) * coef_a;
            nleft = n_attack + n_decay;
            x->x_retrigger = 0;
            gate_status = 1;
        }
        else if((audio_gate || control_gate) != gate_status){ // status changed
            gate_status = audio_gate || x->x_f_gate;
            target = x->x_f_gate != 0 ? x->x_f_gate : input_gate;
            if(gate_status){ // if gate opened
                if(!status)
                    outlet_float(x->x_out2, status = 1);
                incr = (target - last) * coef_a;
                nleft = n_attack + n_decay;
            }
            else{ // gate closed, set release incr
                incr =  -(last * coef_r);
                nleft = n_release;
            }
        }
// "attack + decay + sustain" phase
        if(gate_status){
            if(nleft > 0){ // "attack + decay" not over
                if(!x->x_log){ // linear
                    if(nleft <= n_decay) // attack is over, update incr
                        incr = ((target * sustain_point) - target) * coef_d;
                    *out++ = last += incr;
                }
                else{
                    if(nleft <= n_decay){ // decay
                        double a = exp(LOG001 / n_decay);
                        *out++ = last = (target * sustain_point) +
                            a*(last - (target * sustain_point));
                    }
                    else{
                        double a = exp(LOG001 / n_attack);
                        *out++ = last = target + a*(last - target);
                    }
                }
                nleft--;
            }
            else // "sustain" phase
                *out++ = last = target * sustain_point;
        }
// "release" phase
        else{
            if(!x->x_log){ // linear
                if(nleft > 0){ // "release" not over
                    *out++ = last += incr;
                    nleft--;
                }
                else{ // "release" over
                    if(status)
                        outlet_float(x->x_out2, status = 0);
                    *out++ = last = 0;
                }
            }
            else{
                double a = exp(LOG001 / n_release);
                *out++ = last = target + a*(last - target);
            }
        }
    };
    x->x_last = (PD_BIGORSMALL(last) ? 0. : last);
    x->x_target = (PD_BIGORSMALL(target) ? 0. : target);
    x->x_incr = incr;
    x->x_nleft = nleft;
    x->x_gate_status = gate_status;
    x->x_status = status;
    return (w + 9);
}

static void adsr_dsp(t_adsr *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(adsr_perform, 8, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
            sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec);
}

static void *adsr_new(t_symbol *sym, int ac, t_atom *av){
    t_adsr *x = (t_adsr *)pd_new(adsr_class);
    t_symbol *cursym = sym; // avoid warning
    x->x_sr_khz = sys_getsr() * 0.001;
    x->x_last = 0.;
    x->x_target = 0.;
    x->x_incr = 0.;
    x->x_nleft = 0;
    x->x_gate_status = 0;
    x->x_last_gate = 1;
    x->x_log = 0;
    
    float a = 0, d = 0, s = 0, r = 0;
    int symarg = 0;
    int argnum = 0;
    while(ac > 0){
        if(av->a_type == A_FLOAT && !symarg){
            float argval = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    a = argval;
                    break;
                case 1:
                    d = argval;
                    break;
                case 2:
                    s = argval;
                    break;
                case 3:
                    r = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            ac--;
            av++;
        }
        else if(av->a_type == A_SYMBOL){
            if(!symarg)
                symarg = 1;
            cursym = atom_getsymbolarg(0, ac, av);
            if(cursym == gensym("-log")){
                if(ac == 1){
                    ac--, av++;
                    x->x_log = 1;
                }
                else goto errstate;
            }
            else goto errstate;
        }
        else goto errstate;
    }
    
    
    x->x_inlet_attack = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_attack, a);
    x->x_inlet_decay = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_decay, d);
    x->x_inlet_sustain = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_sustain, s);
    x->x_inlet_release = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_release, r);
    outlet_new((t_object *)x, &s_signal);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    return(x);
errstate:
    pd_error(x, "[adsr~]: improper args");
    return NULL;
}

void adsr_tilde_setup(void){
    adsr_class = class_new(gensym("adsr~"), (t_newmethod)adsr_new, 0,
				 sizeof(t_adsr), 0, A_GIMME, 0);
    class_addmethod(adsr_class, nullfn, gensym("signal"), 0);
    class_addmethod(adsr_class, (t_method) adsr_dsp, gensym("dsp"), A_CANT, 0);
    class_addfloat(adsr_class, (t_method)adsr_float);
    class_addbang(adsr_class, (t_method)adsr_bang);
    class_addmethod(adsr_class, (t_method)adsr_log, gensym("log"), A_DEFFLOAT, 0);
}
