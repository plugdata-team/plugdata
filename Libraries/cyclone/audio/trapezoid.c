/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */
//updated 2016 by Derek Kwan

#include <string.h>
#include "m_pd.h"
#include <common/api.h>

#define TRAPEZOID_DEFUP  0.1  
#define TRAPEZOID_DEFDN  0.9  
#define TRAPEZOID_DEFLO  0.0
#define TRAPEZOID_DEFHI  1.0

typedef struct _trapezoid
{
	t_object   x_obj;
    float      x_low;
    float      x_range;
	float 	   x_high;
	t_float    x_f;
	t_inlet    *x_uplet;
	t_inlet    *x_downlet;
	t_outlet   *x_outlet;
} t_trapezoid;

static t_class *trapezoid_class;

static void trapezoid_lo(t_trapezoid *x, t_floatarg f)
{
    x->x_low = f;
    x->x_range = x->x_high - f;
}

static void trapezoid_hi(t_trapezoid *x, t_floatarg f)
{
	x->x_high = f;
    x->x_range = f - x->x_low;
}

/* LATER optimize */
static t_int *trapezoid_perform(t_int *w)
{
    t_trapezoid *x = (t_trapezoid *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *in1 = (t_float *)(w[3]);
    t_float *in2 = (t_float *)(w[4]);
    t_float *in3 = (t_float *)(w[5]);
    t_float *out = (t_float *)(w[6]);
    float low = x->x_low;
    float range = x->x_range;
    while (nblock--)
    {
	float ph = *in1++;
	float upph = *in2++;
	float dnph = *in3++;
	/* CHECKED ph wrapped */
	if (ph < 0.)
	    ph -= (int)ph - 1.;
	else if (ph > 1.)
	    ph -= (int)ph;
	/* CHECKED upph, dnph clipped  */
	if (upph < 0.)
	    upph = 0.;
	else if (upph > 1.)  /* CHECKME */
	    upph = 1.;
	if (dnph < upph)
	    dnph = upph;
	else if (dnph > 1.)
	    dnph = 1.;

	if (ph < upph)
	    ph /= upph;
	else if (ph < dnph)
	    ph = 1.;
	else if (dnph < 1.)
	    ph = (1. - ph) / (1. - dnph);
	else
	    ph = 0.;
	*out++ = low + ph * range;
    }
    return (w + 7);
}

static void trapezoid_dsp(t_trapezoid *x, t_signal **sp)
{
    dsp_add(trapezoid_perform, 6, x, sp[0]->s_n,
	    sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec);
}

static void *trapezoid_new(t_symbol *s, int argc, t_atom *argv)
{
    t_trapezoid *x = (t_trapezoid *)pd_new(trapezoid_class);
	t_float trapup, trapdown, traplo, traphi;

	trapup = TRAPEZOID_DEFUP;
	trapdown = TRAPEZOID_DEFDN;
	x->x_low = traplo = TRAPEZOID_DEFLO;
	x->x_high = traphi = TRAPEZOID_DEFHI;
	int argnum = 0;
	while(argc > 0){
		if(argv ->a_type == A_FLOAT){//if curarg is a number
			t_float argval = atom_getfloatarg(0, argc, argv);
			switch(argnum){
				case 0:
					trapup = argval;
					break;
				case 1:
					trapdown = argval;
					break;
				default:
					break;
				};
				argnum++;
				argc--;
				argv++;
		}
			else if(argv -> a_type == A_SYMBOL){
				t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
				if(strcmp(curarg->s_name, "@lo")==0){
					if(argc >= 2){
						traplo = atom_getfloatarg(1, argc, argv);
						argc-=2;
						argv+=2;
					}
					else{
						goto errstate;
					};
				}
				else if(strcmp(curarg->s_name, "@hi")==0){
					if(argc >= 2){
						traphi = atom_getfloatarg(1, argc, argv);
						argc-=2;
						argv+=2;
					}
					else{
						goto errstate;
					};
				}
				else{
					goto errstate;
				};
			}
		else{
			goto errstate;
		};
	};
	trapezoid_lo(x, traplo);
	trapezoid_hi(x, traphi);
	x->x_uplet=inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	pd_float((t_pd *)x->x_uplet, trapup);
	x->x_downlet=inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	pd_float((t_pd *)x->x_downlet, trapdown);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return (x);
	errstate:
		pd_error(x, "trapezoid~: improper args");
		return NULL;
}

void *trapezoid_free(t_trapezoid *x){
		inlet_free(x->x_uplet);
		inlet_free(x->x_downlet);
		outlet_free(x->x_outlet);
	return (void *)x;
}

CYCLONE_OBJ_API void trapezoid_tilde_setup(void)
{
    trapezoid_class = class_new(gensym("trapezoid~"),
				(t_newmethod)trapezoid_new, (t_method)trapezoid_free,
				sizeof(t_trapezoid), 0, A_GIMME, 0);
    class_addmethod(trapezoid_class, (t_method)trapezoid_dsp, gensym("dsp"), A_CANT, 0);
   CLASS_MAINSIGNALIN(trapezoid_class, t_trapezoid, x_f);
    class_addmethod(trapezoid_class, (t_method)trapezoid_lo,
		    gensym("lo"), A_DEFFLOAT, 0);  /* CHECKME */
    class_addmethod(trapezoid_class, (t_method)trapezoid_hi,
		    gensym("hi"), A_DEFFLOAT, 0);  /* CHECKME */
}
