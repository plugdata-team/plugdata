/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

#define CLIP_DEFLO  0.
#define CLIP_DEFHI  0.

typedef struct _clip {
    t_object x_obj;
    t_inlet  *x_lolet;
    t_inlet  *x_hilet;
} t_clip;

static t_class *clip_class;

static t_int *clip_perform(t_int *w)
{
    int nblock = (int)(w[1]);
    t_float *in1 = (t_float *)(w[2]);
    t_float *in2 = (t_float *)(w[3]);
    t_float *in3 = (t_float *)(w[4]);
    t_float *out = (t_float *)(w[5]);
    while (nblock--)
    {
    	float f = *in1++;
    	float lo = *in2++;
    	float hi = *in3++;
    	if (f < lo)
	    *out++ = lo;
    	else if (f > hi)
	    *out++ = hi;
	else
	    *out++ = f;
    }
    return (w + 6);
}

static void clip_dsp(t_clip *x, t_signal **sp)
{
    dsp_add(clip_perform, 5, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}


static void *clip_new(t_symbol *s, int argc, t_atom *argv)
{
    t_clip *x = (t_clip *)pd_new(clip_class);
    
    t_float cliplo, cliphi;
    cliplo = CLIP_DEFLO;
    cliphi = CLIP_DEFHI;
    
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0,argc,argv);
            switch(argnum){
                case 0:
                    cliplo = argval;
                    break;
                case 1:
                    cliphi = argval;
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
    pd_float((t_pd *)x->x_lolet, cliplo);
    x->x_hilet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    pd_float((t_pd *)x->x_hilet, cliphi);
    
    outlet_new((t_object *)x, &s_signal);
    return (x);
errstate:
    pd_error(x, "clip~: improper args");
    return NULL;
}

CYCLONE_OBJ_API void clip_tilde_setup(void)
{
    clip_class = class_new(gensym("cyclone/clip~"),
        (t_newmethod)clip_new, 0, sizeof(t_clip), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(clip_class, nullfn, gensym("signal"), 0);
    class_addmethod(clip_class, (t_method) clip_dsp, gensym("dsp"), A_CANT, 0);
    class_sethelpsymbol(clip_class, gensym("clip~"));
}

CYCLONE_OBJ_API void Clip_tilde_setup(void)
{
    clip_class = class_new(gensym("Clip~"),
        (t_newmethod)clip_new, 0, sizeof(t_clip), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(clip_class, nullfn, gensym("signal"), 0);
    class_addmethod(clip_class, (t_method) clip_dsp, gensym("dsp"), A_CANT, 0);
    class_sethelpsymbol(clip_class, gensym("clip~"));
    pd_error(clip_class, "Cyclone: please use [cyclone/clip~] instead of [Clip~] to suppress this error");
}
