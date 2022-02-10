// porres 2017

#include "m_pd.h"
#include "math.h"

typedef struct _glide{
    t_object x_obj;
    t_inlet  *x_inlet_ms;
    t_float  x_in;
    int      x_n;
    int      x_nleft;
    int      x_reset;
    float    x_last_in;
    float    x_last_out;
    float    x_sr_khz;
    float    x_delta;
    float    x_start;
    float    x_exp;
}t_glide;

static t_class *glide_class;

static void glide_exp(t_glide *x, t_floatarg f){
    x->x_exp = f;
}

static void glide_reset(t_glide *x){
    x->x_reset = 1;
}

static float glide_get_step(t_glide *x){
    float step = (float)(x->x_n-x->x_nleft)/(float)x->x_n;
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

static t_int *glide_perform(t_int *w){
    t_glide *x = (t_glide *)(w[1]);
    int n = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    float last_in = x->x_last_in;
    float last_out = x->x_last_out;
    float start = x->x_start;
    while(n--){
        t_float in = *in1++;
        t_float ms = *in2++;
        if(ms <= 0)
            ms  = 0;
        x->x_n = (int)roundf(ms * x->x_sr_khz) + 1; // n samples
        if(x->x_n == 1)
            *out++ = last_out = last_in = in;
        else{
            if(x->x_reset){ // reset
                x->x_nleft = 0;
                x->x_reset = 0;
                *out++ = last_out = last_in = in;
            }
            else if(in != last_in){ // input change, update
                start = last_out;
                x->x_delta = (in - last_out);
                x->x_nleft = x->x_n - 1;
                float inc = glide_get_step(x) * x->x_delta;
                *out++ = last_out = (last_out + inc);
                last_in = in;
            }
            else{
                if(x->x_nleft > 0){
                    x->x_nleft--;
                    float inc = glide_get_step(x) * x->x_delta;
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
    return(w+6);
}

static void glide_dsp(t_glide *x, t_signal **sp){
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(glide_perform, 5, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec);
}

static void *glide_new(t_symbol *s, int ac, t_atom *av){
    t_glide *x = (t_glide *)pd_new(glide_class);
    t_symbol *cursym = s;
    float ms = 0;
    x->x_sr_khz = sys_getsr() * 0.001;
    x->x_last_in = 0.;
    x->x_reset = x->x_nleft = 0;
    x->x_exp = 1.;
    int symarg = 0;
    while(ac > 0){
        if(av->a_type == A_FLOAT && !symarg){
            ms = atom_getfloatarg(0, ac, av);
            ac--;
            av++;
        }
        else if(av->a_type == A_SYMBOL){
            if(!symarg)
                symarg = 1;
            cursym = atom_getsymbolarg(0, ac, av);
            if(cursym == gensym("-exp")){
                if(ac == 2){
                    ac--, av++;
                    x->x_exp = atom_getfloatarg(0, ac, av);
                    ac--, av++;
                }
                else goto errstate;
            }
            else goto errstate;
        }
        else goto errstate;
    }
    x->x_inlet_ms = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_ms, ms);
    outlet_new((t_object *)x, &s_signal);
    return(x);
errstate:
    pd_error(x, "[glide~]: improper args");
    return NULL;
}

void glide_tilde_setup(void){
    glide_class = class_new(gensym("glide~"), (t_newmethod)glide_new, 0,
                            sizeof(t_glide), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(glide_class, t_glide, x_in);
    class_addmethod(glide_class, (t_method) glide_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(glide_class, (t_method)glide_reset, gensym("reset"), 0);
    class_addmethod(glide_class, (t_method)glide_exp, gensym("exp"), A_FLOAT, 0);
}

