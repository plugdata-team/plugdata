// porres 2017

#include "m_pd.h"
#include "math.h"

typedef struct _glide2{
    t_object x_obj;
    t_inlet  *x_inlet_ms_up;
    t_inlet  *x_inlet_ms_down;
    t_float  x_in;
    int      x_n_up;
    int      x_nleft_up;
    int      x_n_down;
    int      x_nleft_down;
    int      x_reset;
    float    x_last_in;
    float    x_last_out;
    float    x_sr_khz;
    float    x_delta;
    float    x_start;
    float    x_exp;
}t_glide2;

static t_class *glide2_class;

static void glide2_exp(t_glide2 *x, t_floatarg f){
    x->x_exp = f;
}

static void glide2_reset(t_glide2 *x){
    x->x_reset = 1;
}

static float glide2_get_step(t_glide2 *x, int n, int nleft){
    float step = (float)(n-nleft)/(float)n;
    if(fabs(x->x_exp) != 1){ // EXPONENTIAL
        if(x->x_exp >= 0){ // positive exponential
            if(x->x_delta > 0)
                step = pow(step, x->x_exp);
            else
                step = 1-pow(1-step, x->x_exp);
        }
        else{ // negative exponential
            if(x->x_delta > 0)
                step = 1-pow(1-step, -x->x_exp);
            else
                step = pow(step, -x->x_exp);
        }
    }
    return(step);
}

static t_int *glide2_perform(t_int *w){
    t_glide2 *x = (t_glide2 *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    float last_in = x->x_last_in;
    float last_out = x->x_last_out;
    float start = x->x_start;
    while(n--){
        t_float in = *in1++;
        t_float ms_up = *in2++;
        t_float ms_down = *in3++;
        if(ms_up <= 0)
            ms_up  = 0;
        if(ms_down <= 0)
            ms_down  = 0;
        x->x_n_up = (int)roundf(ms_up * x->x_sr_khz) + 1; // n samples
        x->x_n_down = (int)roundf(ms_down * x->x_sr_khz) + 1; // n samples
        if(x->x_reset){ // reset
            x->x_nleft_up = x->x_nleft_down = 0;
            x->x_reset = 0;
            *out++ = last_out = last_in = in;
        }
        else if(in != last_in){ // input change, update
            start = last_out;
            x->x_delta = (in - last_out);
            if(x->x_delta > 0){
                x->x_nleft_up = x->x_n_up - 1;
                float step = glide2_get_step(x, x->x_n_up, x->x_nleft_up);
                float inc = step * x->x_delta;
                *out++ = last_out = (last_out + inc);
                last_in = in;
            }
            else{
                x->x_nleft_down = x->x_n_down - 1;
                float step = glide2_get_step(x, x->x_n_down, x->x_nleft_down);
                float inc = step * x->x_delta;
                *out++ = last_out = (last_out + inc);
                last_in = in;
            }
        }
        else{
            if(x->x_delta > 0){
                if(x->x_nleft_up > 0){
                    x->x_nleft_up--;
                    float step = glide2_get_step(x, x->x_n_up, x->x_nleft_up);
                    float inc = step * x->x_delta;
                    *out++ = last_out = (start + inc);
                }
                else
                    *out++ = last_out = last_in = in;
            }
            else{
                if(x->x_nleft_down > 0){
                    x->x_nleft_down--;
                    float step = glide2_get_step(x, x->x_n_down, x->x_nleft_down);
                    float inc = step * x->x_delta;
                    *out++ = last_out = (start + inc);
                }
                else
                    *out++ = last_out = last_in = in;
            }
        }
        
    };
    x->x_start = start;
    x->x_last_in = last_in;
    x->x_last_out = last_out;
    return(w+7);
}

static void glide2_dsp(t_glide2 *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(glide2_perform, 6, x, sp[0]->s_n, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *glide2_new(t_symbol *s, int ac, t_atom *av){
    t_glide2 *x = (t_glide2 *)pd_new(glide2_class);
    s = NULL;
    float ms_up = 0;
    float ms_down = 0;
    x->x_sr_khz = sys_getsr() * 0.001;
    x->x_last_in = 0.;
    x->x_reset = x->x_nleft_up = 0;
    x->x_exp = 1.;
    int argnum = 0;
    while(ac > 0){
        if(av->a_type == A_FLOAT){
            float argval = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    ms_up = argval;
                    break;
                case 1:
                    ms_down = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            ac--, av++;
        }
        else if(av->a_type == A_SYMBOL && !argnum){
            if(atom_getsymbolarg(0, ac, av) == gensym("-exp")){
                if(ac >= 2){
                    ac--, av++;
                    x->x_exp = atom_getfloatarg(0, ac, av);
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    }
    x->x_inlet_ms_up = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_ms_up, ms_up);
    x->x_inlet_ms_down = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_ms_down, ms_down);
    outlet_new((t_object *)x, &s_signal);
    return(x);
errstate:
    pd_error(x, "[glide2~]: improper args");
    return NULL;
}

void glide2_tilde_setup(void){
    glide2_class = class_new(gensym("glide2~"), (t_newmethod)glide2_new, 0,
        sizeof(t_glide2), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(glide2_class, t_glide2, x_in);
    class_addmethod(glide2_class, (t_method) glide2_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(glide2_class, (t_method)glide2_reset, gensym("reset"), 0);
    class_addmethod(glide2_class, (t_method)glide2_exp, gensym("exp"), A_FLOAT, 0);
}
