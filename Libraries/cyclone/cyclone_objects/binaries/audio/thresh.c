/* Copyright (c) 2016 Porres
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

#define THRESH_DEFLO  0.
#define THRESH_DEFHI  0.

typedef struct _thresh
{
    t_object x_obj;
    t_float  x_last;
    t_inlet  *x_lolet;
    t_inlet  *x_hilet;
} t_thresh;

static t_class *thresh_class;

static t_int *thresh_perform(t_int *w)
{
    t_thresh *x = (t_thresh *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float last = x->x_last;
    while (nblock--)
    {
        float in = *in1++;
        float lo = *in2++;
        float hi = *in3++;
        *out++ = last = (in > lo && (in >= hi || last));
    }
    x->x_last = last;
    return (w + 7);
}

static void thresh_dsp(t_thresh *x, t_signal **sp)
{
    dsp_add(thresh_perform, 6, x, sp[0]->s_n,
            sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *thresh_free(t_thresh *x)
{
    inlet_free(x->x_lolet);
    inlet_free(x->x_hilet);
    return (void *)x;
}

static void *thresh_new(t_symbol *s, int argc, t_atom *argv)
{
    t_thresh *x = (t_thresh *) pd_new(thresh_class);
    
    t_float thlo, thhi;
    thlo = THRESH_DEFLO;
    thhi = THRESH_DEFHI;
    
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0,argc,argv);
            switch(argnum){
                case 0:
                    thlo = argval;
                    break;
                case 1:
                    thhi = argval;
                    break;
                default:
                    break;
            };
            
            argc--;
            argv++;
            argnum++;
        }
        else{
            goto errstate;
        };
    };
    
    x->x_lolet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_lolet, thlo);
    x->x_hilet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_hilet, thhi);
    
    outlet_new((t_object *)x, &s_signal);
    x->x_last = 0;
    return (x);
    errstate:
    pd_error(x, "thresh~: improper args");
    return NULL;
    
}

CYCLONE_OBJ_API void thresh_tilde_setup(void)
{
    thresh_class = class_new(gensym("thresh~"),
        (t_newmethod)thresh_new,
        (t_method)thresh_free,
        sizeof(t_thresh),
        0,
        A_GIMME, 0);
    class_addmethod(thresh_class, nullfn, gensym("signal"), 0);
    class_addmethod(thresh_class, (t_method)thresh_dsp, gensym("dsp"), A_CANT, 0);
}