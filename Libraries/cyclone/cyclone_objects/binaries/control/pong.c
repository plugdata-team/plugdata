//2016 by Derek Kwan

#include "m_pd.h"
#include <common/api.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef HAVE_ALLOCA     /* can work without alloca() but we never need it */
#define HAVE_ALLOCA 1
#endif
#define TEXT_NGETBYTE 100 /* bigger that this we use alloc, not alloca */
#if HAVE_ALLOCA
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)((n) < TEXT_NGETBYTE ?  \
			        alloca((n) * sizeof(t_atom)) : getbytes((n) * sizeof(t_atom))))
#define ATOMS_FREEA(x, n) ( \
		    ((n) < TEXT_NGETBYTE || (freebytes((x), (n) * sizeof(t_atom)), 0)))
#else
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)getbytes((n) * sizeof(t_atom)))
#define ATOMS_FREEA(x, n) (freebytes((x), (n) * sizeof(t_atom)))
#endif

#ifndef CYPONGMODE_DEF
#define CYPONGMODE_DEF 3
#endif

#ifndef CYPONGLO_DEF
#define CYPONGLO_DEF 0.f
#endif

#ifndef CYPONGHI_DEF
#define CYPONGHI_DEF 0.f
#endif

static t_class *pong_class;

typedef struct _pong {//pong (control rate) 
	t_object x_obj;
	int mode; //0=fold, 1 = wrap, 2 = clip, 3 = none
	t_float minval;
	t_float maxval;
} t_pong;




static int pong_setmode_help(char const * mode){
//helper function for setting mode
	int retmode; //int val for mode (see struct)
		if(strcmp(mode, "clip") == 0){
			retmode = 2;
		}
		else if(strcmp(mode, "wrap") == 0){
			retmode = 1;
		}
		else if(strcmp(mode, "fold") == 0){
			retmode = 0;
		}
		else{//default to none o/wise
			retmode = 3;
		};
	
	return retmode;
	
};



static void *pong_new(t_symbol *s, int argc, t_atom *argv){
	//two optional args (lo, hi), then attributes for mode (str) and range (2 fl)
	t_pong *x = (t_pong *)pd_new(pong_class);
	int numargs = 0;//number of args read
	int pastargs = 0; //if any attrs have been declared yet
	x->minval = CYPONGLO_DEF;
	x->maxval = CYPONGHI_DEF;
	x->mode = CYPONGMODE_DEF;
	while(argc > 0 ){
			if(argv -> a_type == A_FLOAT){ //if nullpointer, should be float or int
				if(!pastargs){//if we aren't past the args yet
					switch(numargs){
						case 0: 	x->minval = atom_getfloatarg(0, argc, argv);
									numargs++;
									argc--;
									argv++;
									break;
					
						case 1: 	x->maxval = atom_getfloatarg(0, argc, argv);
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
				int isrange = strcmp(curarg->s_name, "@range") == 0;
				int ismode = strcmp(curarg->s_name, "@mode") == 0;
				if(isrange && argc >= 3){
						t_symbol *arg1 = atom_getsymbolarg(1, argc, argv);
						t_symbol *arg2 = atom_getsymbolarg(2, argc, argv);
						if(arg1 == &s_ && arg2 == &s_){
							x->minval = atom_getfloatarg(1, argc, argv);
							x->maxval = atom_getfloatarg(2, argc, argv);
							argc -= 3;
							argv += 3;
						}
						else{
							goto errstate;
						};}
		
				else if(ismode && argc >= 2){
						t_symbol *arg3 = atom_getsymbolarg(1, argc, argv);
						if(arg3 != &s_){
							x->mode = pong_setmode_help(arg3->s_name);
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

	floatinlet_new(&x->x_obj, &x->minval);
	floatinlet_new(&x->x_obj, &x->maxval);
	outlet_new(&x->x_obj, gensym("list"));
	return (x);
	errstate:
		pd_error(x, "pong: improper args");
		return NULL;
}


static float pong_ponger(float input, float minval, float maxval, int mode){
	//pong helper function
	float returnval;
	float range = maxval - minval;
	if(input < maxval && input >= minval){//if input in range, return input
		returnval = input;
		}
	else if(minval == maxval && mode != 3){
		returnval = minval;
	}
	else if(mode == 0){//folding
		if(input < minval){
			float diff = minval - input; //diff between input and minimum (positive)
			int mag = (int)(diff/range); //case where input is more than a range away from minval
			if(mag % 2 == 0){// even number of ranges away = counting up from min
				diff = diff - ((float)mag)*range;
				returnval = diff + minval;
				}
			else{// odd number of ranges away = counting down from max
				diff = diff - ((float)mag)*range;
				returnval = maxval - diff;
				};
			}
		else{ //input > maxval
			float diff = input - maxval; //diff between input and max (positive)
			int mag  = (int)(diff/range); //case where input is more than a range away from maxval
			if(mag % 2 == 0){//even number of ranges away = counting down from max
				diff = diff - (float)mag*range;
				returnval = maxval - diff;
				}
			else{//odd number of ranges away = counting up from min
				diff = diff - (float)mag*range;
				returnval = diff + minval;
				};
			};
		}
	else if (mode == 1){// wrapping
		if(input < minval){
			returnval = input;
			while(returnval < minval){
					returnval += range;
			};
		}
		else{
			returnval = fmod(input-minval,maxval-minval) + minval;
		};
	}
	else if(mode == 2){//clipping
		if(input < minval){
			returnval = minval;
		}
		else{//input > maxval
			returnval = maxval;
		};
	}
	else{//mode = 3, no effect
		returnval = input;
	};

	return returnval;

}

static void pong_float(t_pong *x, t_float f){
	float returnval, minv, maxv, ipt;
	int mode;
	t_atom *outatom; //since outlet is of type list, need to use list of len 1 instead of float for output
	mode = x->mode;
	minv = x->minval;
	maxv = x->maxval;
	ipt = f;
	
	ATOMS_ALLOCA(outatom, 1); //allocate memory for outatom of len 1

	if(minv > maxv){//checking ranges
		float temp;
		temp = maxv;
		maxv = minv;
		minv = temp;
		};

	returnval = pong_ponger(ipt, minv, maxv, mode);
	SETFLOAT(outatom, (t_float)returnval);
	outlet_list(x->x_obj.ob_outlet, &s_list, 1, outatom);
	ATOMS_FREEA(outatom, 1); //free allocated memory for outatom
	
}


static void pong_list(t_pong *x, t_symbol *s, int argc, t_atom *argv){

	float minv, maxv;
	int mode, i;
	t_atom *outatom;
	mode = x->mode;
	minv = x->minval;
	maxv = x->maxval;
	
	ATOMS_ALLOCA(outatom, argc); //allocate memory for outatom 

	if(minv > maxv){//checking ranges
		float temp;
		temp = maxv;
		maxv = minv;
		minv = temp;
		};
	for(i=0; i<argc; i++){//use helper function to set outatom one by one
		float returnval, curterm;
		curterm = atom_getfloatarg(i, argc, argv);
		returnval = pong_ponger(curterm, minv, maxv, mode);
		SETFLOAT((outatom+i), (t_float)returnval);
	};
	outlet_list(x->x_obj.ob_outlet, &s_list, argc, outatom);
	ATOMS_FREEA(outatom, argc); //free allocated memory for outatom
	
}

static void pong_setrange(t_pong *x, t_float lo, t_float hi){

	x->minval = lo;
	x->maxval = hi;

}

static void pong_setmode(t_pong *x, t_symbol *s)
    {
		int setmode;
			setmode = pong_setmode_help(s->s_name);
				x->mode = setmode;
    }

	CYCLONE_OBJ_API void pong_setup(void){
	pong_class = class_new(gensym("pong"), (t_newmethod)pong_new, 0,
			sizeof(t_pong), 0, A_GIMME, 0);
	class_addfloat(pong_class, pong_float);
	class_addlist(pong_class, pong_list);
	class_addmethod(pong_class, (t_method)pong_setrange, gensym("range"), A_FLOAT, A_FLOAT, 0);
	class_addmethod(pong_class, (t_method)pong_setmode, gensym("mode"), A_SYMBOL, 0);
}
