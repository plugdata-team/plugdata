//2016 by Derek Kwan

#include "m_pd.h"
#include <common/api.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#ifndef CYROUNDNEAR_DEF
#define CYROUNDNEAR_DEF 1
#endif

#ifndef CYROUNDNUM_DEF
#define CYROUNDNUM_DEF 0.
#endif

static t_class *round_tilde_class;

typedef struct _round_tilde
{
	t_object x_obj;
	float x_nearest; //nearest attribute (1 = rounding, 0 = truncation)
} t_round_tilde;

static void *round_tilde_new(t_symbol *s, int argc, t_atom *argv)
{ float f;
	t_round_tilde *x = (t_round_tilde *)pd_new(round_tilde_class);
		int numargs = 0;
		int pastargs = 0;//if we haven't declared any attrs yet
		x->x_nearest = CYROUNDNEAR_DEF;
		f = CYROUNDNUM_DEF;
		while(argc > 0 ){
			if(argv -> a_type == A_FLOAT){ //if nullpointer, should be float or int
				if(!pastargs){
					switch(numargs){//we haven't declared attrs yet
						case 0: 	f = atom_getfloatarg(0, argc, argv);
									numargs++;
									argc--;
									argv++;
									break;
						default:	argc--;
									argv++;
									break;
					};
				}
				else{
					argc--;
					argv++;
				};
			}
			else if(argv -> a_type == A_SYMBOL){
				t_symbol *curarg = atom_getsymbolarg(0, argc, argv); //returns nullpointer if not symbol
				pastargs = 1;
				int isnear = strcmp(curarg->s_name, "@nearest") == 0;
				if(isnear && argc >= 2){
					t_symbol *arg1 = atom_getsymbolarg(1, argc, argv);
					if(arg1 == &s_){// is a number
						x->x_nearest = atom_getfloatarg(1, argc, argv);
						argc -= 2;
						argv += 2;
						}
					else{
						goto errstate;
					};}
				else{
					goto errstate;
					};
			}
			else{
				goto errstate;
			};
	};

	pd_float( (t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal), f);
	 outlet_new(&x->x_obj, gensym("signal"));
	return (x);
	
	errstate:
		pd_error(x, "round~: improper args");
		return NULL;
}

static t_int *round_tilde_perform(t_int *w)
{
	t_round_tilde *x = (t_round_tilde *)(w[1]);
	t_float *in1 = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int)(w[5]);
	float nearfloat;
	int nearest;
	nearfloat = x->x_nearest;
	if(nearfloat <= 0.){
		nearest = 0;
	}
	else{
		nearest = 1;
	};
	while (n--){
		float rounded,div;
		float val = *(in1++);
		float roundto = *(in2++);
		if(roundto > 0.){
			div = val/roundto; //get the result of dividing the two
			if(nearest == 1){//rounding
				rounded = roundto*round(div); //round quotient to nearest int and multiply roundto by result
			}
			else{//truncation
				rounded = roundto*(float)((int)div); //else lop off the decimal and multiply roundto by result
			};
		}
		else{//round is 0, do nothing
			rounded = val;
		};
		*out++ = rounded;
	};
	return (w+6);
}

static void round_tilde_nearest(t_round_tilde *x, t_float f, t_float glob){
	if(f <= 0.){
		x->x_nearest = 0;
	}
	else{
		x->x_nearest = 1;
	};
}
static void round_tilde_dsp(t_round_tilde *x, t_signal **sp)
{
	dsp_add(round_tilde_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

CYCLONE_OBJ_API void round_tilde_setup(void)
{
	round_tilde_class = class_new(gensym("round~"), (t_newmethod)round_tilde_new, 0,
	sizeof(t_round_tilde), 0, A_GIMME, 0);
	class_addmethod(round_tilde_class, nullfn, gensym("signal"), 0);
	class_addmethod(round_tilde_class, (t_method)round_tilde_dsp, gensym("dsp"), A_CANT, 0);
	class_addmethod(round_tilde_class, (t_method)round_tilde_nearest,  gensym("nearest"), A_FLOAT, 0);
}
