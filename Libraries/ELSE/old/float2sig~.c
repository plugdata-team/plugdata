// porres 2017

#include "m_pd.h"
#include "math.h"

typedef struct _float2sig{
    t_object x_obj;
    double   x_coef;
    t_float  x_last;
    t_float  x_target;
    t_float  x_in;
    t_float  x_sr_khz;
    t_float  x_ms;
    double   x_incr;
    int      x_nleft;
}t_float2sig;


static t_class *float2sig_class;


static void float2sig_float(t_float2sig *x, t_float f){
    x->x_in = f;
}

static t_int *float2sig_perform(t_int *w){
    t_float2sig *x = (t_float2sig *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *out = (t_float *)(w[3]);
    t_float last = x->x_last;
    t_float target = x->x_target;
    double incr = x->x_incr;
    int nleft = x->x_nleft;
    while(nblock--){
        t_float f = x->x_in;
        t_float ms = x->x_ms;
        int n = roundf(ms * x->x_sr_khz);
        double coef = n <= 0 ? 0. : 1. / (double)n;
        
        if(coef != x->x_coef){ // if ms changed while in a ramp?
            x->x_coef = coef;
            if (f != last){
                if (n > 1){
                    incr = (f - last) * x->x_coef;
                    nleft = n;
                    *out++ = (last += incr);
                    continue;
                }
            }
            incr = 0.;
            nleft = 0;
            *out++ = last = f;
        }
        
        else if (f != target){ // if input != target, update target
            target = f;
            if (f != last){
                if (n > 1){
                    incr = (f - last) * x->x_coef;
                    nleft = n;
                    *out++ = (last += incr);
                    continue;
                }
            }
            incr = 0.;
            nleft = 0;
            *out++ = last = f;
        }
        
        else if(nleft > 0){ // if ramp
            *out++ = (last += incr);
            if (--nleft == 1){
                incr = 0.;
                last = target;
            }
        }
        else
            *out++ = target;
    };
    x->x_last = (PD_BIGORSMALL(last) ? 0. : last);
    x->x_target = (PD_BIGORSMALL(target) ? 0. : target);
    x->x_incr = incr;
    x->x_nleft = nleft;
    return (w + 4);
}

static void float2sig_dsp(t_float2sig *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(float2sig_perform, 3, x, sp[0]->s_n, sp[0]->s_vec);
}

static void *float2sig_new(t_floatarg ms, t_floatarg start){
    t_float2sig *x = (t_float2sig *)pd_new(float2sig_class);
    x->x_last = x->x_incr = x->x_nleft = 0.;
    x->x_in = start;
    x->x_target = start;
    int n = roundf((x->x_ms = ms) * (x->x_sr_khz = sys_getsr() * 0.001));
    x->x_coef = n <= 0 ? 0. : 1. / (double)n;
    floatinlet_new((t_object *)x, &x->x_ms);
    outlet_new((t_object *)x, &s_signal);
    return (x);
}

void float2sig_tilde_setup(void){
    float2sig_class = class_new(gensym("float2sig~"), (t_newmethod)float2sig_new, 0,
				 sizeof(t_float2sig), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(float2sig_class, float2sig_float);
    class_addmethod(float2sig_class, (t_method) float2sig_dsp, gensym("dsp"), A_CANT, 0);
}
