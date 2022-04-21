#define TWOPI (3.14159265358979323846 * 2)

#include "m_pd.h"
#include <math.h>
#include <string.h>

typedef struct xmod2{
    t_object    x_obj;
    t_float     x_f;
    t_int       x_type;
    t_float     x_x1, x_y1;
    t_float     x_x2, x_y2;     
    t_float     x_scale;
    t_inlet    *x_inlet_index1;
    t_inlet    *x_inlet_freq2;
    t_inlet    *x_inlet_index2;
}t_xmod2;

t_class *xmod2_class;

static t_int *xmod2_perform(t_int *w){
    t_xmod2 *x            = (t_xmod2 *)(w[1]);
    t_int n             = (t_int)(w[2]);
    t_float *in1        = (t_float *)(w[3]);
    t_float *index1     = (t_float *)(w[4]);
    t_float *in2        = (t_float *)(w[5]);
    t_float *index2     = (t_float *)(w[6]);
    t_float *out1       = (t_float *)(w[7]);
    t_float *out2       = (t_float *)(w[8]);
    t_float x1          = x->x_x1;
    t_float y1          = x->x_y1;
    t_float x2          = x->x_x2;
    t_float y2          = x->x_y2;
    t_float z1, dx1, dy1, z2, dx2, dy2;
    for(t_int i = 0; i < n; i++){
        t_float freq1 = tan(*in1++ * x->x_scale) / x->x_scale;
        t_float freq2 = tan(*in2++ * x->x_scale) / x->x_scale;
        // osc 1
        z1 = (freq1 + (*index2++ * x2)) * x->x_scale;
        dx1 = x1 - (z1 * y1);
        dy1 = y1 + (z1 * x1);
        x1 = dx1 > 1 ? 1 : dx1 < -1 ? -1 : dx1;
        y1 = dy1 > 1 ? 1 : dy1 < -1 ? -1 : dy1;
        // osc 2
        z2 = (freq2 + (*index1++ * x1)) * x->x_scale;
        dx2 = x2 - (z2 * y2);
        dy2 = y2 + (z2 * x2);
        x2 = dx2 > 1 ? 1 : dx2 < -1 ? -1 : dx2;
        y2 = dy2 > 1 ? 1 : dy2 < -1 ? -1 : dy2;
        *out1++ = x1;
        *out2++ = x2;
    }
    x->x_x1 = x1;
    x->x_y1 = y1;
    x->x_x2 = x2;
    x->x_y2 = y2;
    return(w + 9);
}

static void xmod2_dsp(t_xmod2 *x, t_signal **sp){
    x->x_scale = TWOPI / sp[0]->s_sr;
    dsp_add(xmod2_perform, 8, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec);
}

static void *xmod2_new(t_symbol *s, int ac, t_atom *av){
    t_xmod2 *x = (t_xmod2 *)pd_new(xmod2_class);
    t_symbol *dummy = s;
    dummy = NULL;
    x->x_x1 = x->x_x2 = 1;
    x->x_y1 = x->x_y2 = 0;
    t_int type = 0;
    x->x_f = 0;
    t_float freq2 = 0;
    t_float index1 = 0;
    t_float index2 = 0;
    t_int argnum = 0;
    x->x_scale = TWOPI / sys_getsr();
    while(ac > 0){
        if(av->a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0, ac, av);
            switch(argnum){
                case 0:
                    x->x_f = argval;
                    break;
                case 1:
                    index1 = argval;
                    break;
                case 2:
                    freq2 = argval;
                    break;
                case 3:
                    index2 = argval;
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
    x->x_inlet_index1 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_index1, index1);
    x->x_inlet_freq2 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_freq2, freq2);
    x->x_inlet_index2 = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_inlet_index2, index2);
    outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_type = type;
    return(void *)x;
    errstate:
        pd_error(x, "[xmod2~]: improper args");
        return NULL;
}

void xmod2_tilde_setup(void){
    xmod2_class = class_new(gensym("xmod2~"), (t_newmethod)xmod2_new, 0, sizeof(t_xmod2), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(xmod2_class, t_xmod2, x_f);
    class_addmethod(xmod2_class, (t_method)xmod2_dsp, gensym("dsp"), 0);
}
