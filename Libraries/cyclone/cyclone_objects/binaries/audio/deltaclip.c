/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */
// updated 2016 by Derek Kwan and 2021 by porres

#include "m_pd.h"
#include <common/api.h>

typedef struct _deltaclip{
    t_object x_obj;
    t_float  x_last;
	t_inlet  *x_lolet;
	t_inlet  *x_hilet;
}t_deltaclip;

static t_class *deltaclip_class;

static t_int *deltaclip_perform(t_int *w){
    t_deltaclip *x = (t_deltaclip *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    t_float last = x->x_last;
    while(nblock--){
    	float f = *in1++;
        float delta = f - last;
    	float lo = *in2++;
        if(lo > 0)
            lo = 0;
    	float hi = *in3++;
        if(hi < 0)
            hi = 0;
    	if(delta < lo)
            f = last + lo;
    	else if (delta > hi)
            f = last + hi;
        *out++ = last = f;
    }
    x->x_last = last;
    return(w+7);
}

static void deltaclip_dsp(t_deltaclip *x, t_signal **sp){
    dsp_add(deltaclip_perform, 6, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void deltaclip_reset(t_deltaclip *x){
    x->x_last = 0;
}

static void *deltaclip_free(t_deltaclip *x){
    inlet_free(x->x_lolet);
    inlet_free(x->x_hilet);
    return(void *)x;
}

static void *deltaclip_new(t_symbol *s, int argc, t_atom *argv){
    t_deltaclip *x = (t_deltaclip *)pd_new(deltaclip_class);
	t_float cliplo = 0, cliphi = 0;
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
    x->x_last = 0;
    return(x);
	errstate:
		pd_error(x, "deltaclip~: improper args");
		return(NULL);
}

CYCLONE_OBJ_API void deltaclip_tilde_setup(void){
    deltaclip_class = class_new(gensym("deltaclip~"), (t_newmethod)deltaclip_new,
        (t_method)deltaclip_free, sizeof(t_deltaclip), 0, A_GIMME, 0);
    class_addmethod(deltaclip_class, nullfn, gensym("signal"), 0);
    class_addmethod(deltaclip_class, (t_method)deltaclip_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(deltaclip_class, (t_method)deltaclip_reset, gensym("reset"), 0);
}
