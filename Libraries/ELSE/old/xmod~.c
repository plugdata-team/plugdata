// Porres 2017

#include "m_pd.h"
#include "math.h"
#include "string.h"

#define TWOPI (3.14159265358979323846 * 2)

static t_class *xmod_class;

typedef struct _xmod{
    t_object    x_obj;
    double      x_phase_1;
    double      x_phase_2;
    t_float     x_pm;
    t_float     x_yn;
    t_float     x_freq;
    t_inlet    *x_inlet_fb;
    t_inlet    *x_inlet_freq2;
    t_inlet    *x_inlet_fb2;
    t_outlet   *x_outlet1;
    t_outlet   *x_outlet2;
    t_float     x_sr;
}t_xmod;

void xmod_pm(t_xmod *x){
    x->x_pm = 1;
}

void xmod_fm(t_xmod *x){
    x->x_pm = 0;
}

static t_int *xmod_perform(t_int *w){
    t_xmod *x = (t_xmod *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_float *in1 = (t_float *)(w[3]); // freq1
    t_float *in2 = (t_float *)(w[4]); // index1
    t_float *in3 = (t_float *)(w[5]); // freq2
    t_float *in4 = (t_float *)(w[6]); // index2
    t_float *out1 = (t_float *)(w[7]);
    t_float *out2 = (t_float *)(w[8]);
    float yn = x->x_yn;
    double phase1 = x->x_phase_1;
    double phase2 = x->x_phase_2;
    float sr = x->x_sr;
    while (nblock--){
        if(x->x_pm){ // phase modulation
            float freq1 = *in1++;
            float index1 = *in2++;
            float freq2 = *in3++;
            float index2 = *in4++;
            float radians1 = (phase1 + (yn * index2)) * TWOPI;
            float output1 = sin(radians1);
            float radians2 = (phase2 + (output1 * index1)) * TWOPI;
            *out1++ = output1;
            *out2++  = yn = sin(radians2);
            phase1 += (double)(freq1 / sr);
            phase2 += (double)(freq2 / sr);
        }
        else{ // frequency modulation
            float freq1 = *in1++;
            float index1 = *in2++;
            float freq2 = *in3++;
            float index2 = *in4++;
            
            float hz1 = freq1 + yn * index2;
            float output1 = sin(phase1 * TWOPI);
            *out1++ = output1;
            float hz2 = freq2 + output1 * index1;
            *out2++ = yn = sin(phase2 * TWOPI);
            
            double phase_step1 = (double)(hz1 / sr);
            phase1 += phase_step1;
            double phase_step2 = (double)(hz2 / sr);
            phase2 += phase_step2;
        }
    }
    x->x_yn = yn; // 1 sample feedback
    x->x_phase_1 = fmod(phase1, 1);
    x->x_phase_2 = fmod(phase2, 1);
    return(w + 9);
}

static void xmod_dsp(t_xmod *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    dsp_add(xmod_perform, 8, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec);
}

static void *xmod_free(t_xmod *x){
    inlet_free(x->x_inlet_fb);
    inlet_free(x->x_inlet_freq2);
    inlet_free(x->x_inlet_fb2);
    outlet_free(x->x_outlet1);
    outlet_free(x->x_outlet2);
    return(void *)x;
}

static void *xmod_new(t_symbol *s, int ac, t_atom *av){
    t_xmod *x = (t_xmod *)pd_new(xmod_class);
    t_symbol *dummy = s;
    dummy = NULL;
    t_float init_freq1 = 0;
    t_float init_fb1 = 0;
    t_float init_freq2 = 0;
    t_float init_fb2 = 0;
    t_int pm = 0;
    t_int argnum = 0;
    while(ac > 0){
        if(av->a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbolarg(0, ac, av);
            if(!strcmp(curarg->s_name, "-pm")){
                if(ac == 1){
                    pm = 1;
                    ac--;
                    av++;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else if(av->a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    init_freq1 = argval;
                    break;
                case 1:
                    init_fb1 = argval;
                    break;
                case 2:
                    init_freq2 = argval;
                    break;
                case 3:
                    init_fb2 = argval;
                    break;
                default:
                    break;
            };
            argnum++;
            ac--;
            av++;
        }
        else
            goto errstate;
    };
    x->x_pm = pm;
    x->x_freq = init_freq1;
    x->x_inlet_fb = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_fb, init_fb1);
    x->x_inlet_freq2 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_freq2, init_freq2);
    x->x_inlet_fb2 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
        pd_float((t_pd *)x->x_inlet_fb2, init_fb2);
    x->x_outlet1 = outlet_new(&x->x_obj, &s_signal);
    x->x_outlet2 = outlet_new(&x->x_obj, &s_signal);
    return (x);
errstate:
    pd_error(x, "[xmod~]: improper args");
    return NULL;
}

void xmod_tilde_setup(void){
    xmod_class = class_new(gensym("xmod~"),
        (t_newmethod)xmod_new, (t_method)xmod_free,
        sizeof(t_xmod), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(xmod_class, t_xmod, x_freq);
    class_addmethod(xmod_class, (t_method)xmod_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(xmod_class, (t_method)xmod_pm, gensym("pm"), 0);
    class_addmethod(xmod_class, (t_method)xmod_fm, gensym("fm"), 0);
}
