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


#ifndef CYROUNDNEAR_DEF
#define CYROUNDNEAR_DEF 1
#endif

#ifndef CYROUNDNUM_DEF
#define CYROUNDNUM_DEF 0.
#endif

static t_class *round_class;

typedef struct _round
{
	t_object x_obj;
	t_float x_round;
	float x_nearest; //nearest attribute (1 = rounding, 0 = truncation)
} t_round;


static void *round_new(t_symbol *s, int argc, t_atom *argv)
{ 
	t_round *x = (t_round *)pd_new(round_class);

		int numargs = 0;
		int pastargs = 0; //if we are past the args and entered an attribute
		x->x_nearest = CYROUNDNEAR_DEF;
		x->x_round = CYROUNDNUM_DEF;
		while(argc > 0 ){
			if(argv -> a_type == A_FLOAT){ //if nullpointer, should be float or int
				if(!pastargs){//if we haven't declared any attrs yet
					switch(numargs){
						case 0: 	x->x_round = atom_getfloatarg(0, argc, argv);
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


	
	floatinlet_new(&x->x_obj, &x->x_round);
 
	outlet_new(&x->x_obj, gensym("list"));
	return (x);
	errstate:
		pd_error(x, "round: improper args");
		return NULL;
	
}

static float rounding_algo(float val, float roundto, int mode){
//rounding util function, val = val to round, roundto = val to round to, mode = nearest
	float div, rounded;	
		if(roundto > 0.){
			div = val/roundto; //get the result of dividing the two
			if(mode == 1){//rounding
				rounded = roundto*round(div); //round quotient to nearest int and multiply roundto by result
			}
			else{//truncation
				rounded = roundto*(float)((int)div); //else lop off the decimal and multiply roundto by result
			};
		}
		else{//round is 0, do nothing
			rounded = val;

		};
		return rounded;

}

static void round_list(t_round *x, t_symbol *s,
    int argc, t_atom *argv){
	int i, nearest;
	float curterm, nearfloat, roundto;
	t_atom *outatom;
	nearfloat = x->x_nearest;
	roundto = x->x_round;
	if(nearfloat <= 0.){
		nearest = 0;
	}
	else{
		nearest = 1;
	};
	ATOMS_ALLOCA(outatom, argc); //allocate memory for outatom
	for(i = 0; i<argc; i++){ //get terms one by one and round/truncate them then stick them in outatom
		float rounded;
		curterm = atom_getfloatarg(i, argc, argv);
		rounded = rounding_algo(curterm, roundto, nearest);
		SETFLOAT((outatom+i), (t_float)rounded);
	};
	outlet_list(x->x_obj.ob_outlet, &s_list, argc, outatom);
	ATOMS_FREEA(outatom, argc); //free allocated memory for outatom
	
}

static void round_float(t_round *x, t_float f){
	//since outlet is of type list, make a 1 element list
	float rounded, nearfloat, roundto;
	int nearest;
	t_atom *outatom;
	nearfloat = x->x_nearest;
	roundto = x->x_round;
	if(nearfloat <= 0.){
		nearest = 0;
	}
	else{
		nearest = 1;
	};
	ATOMS_ALLOCA(outatom, 1); //allocate memory of outatom
	rounded = rounding_algo((float)f, roundto, nearest);
	SETFLOAT(outatom, (t_float)rounded);
	outlet_list(x->x_obj.ob_outlet, &s_list, 1, outatom);
	ATOMS_FREEA(outatom, 1); //deallocate memory of outatom
}


static void round_nearest(t_round *x, t_float f, t_float glob){
	if(f <= 0.){
		x->x_nearest = 0;
	}
	else{
		x->x_nearest = 1;
	};
}

CYCLONE_OBJ_API void round_setup(void)
{
	round_class = class_new(gensym("round"), (t_newmethod)round_new, 0,
	sizeof(t_round), 0, A_GIMME, 0);
	class_addfloat(round_class, (t_method)round_float);
	class_addlist(round_class, (t_method)round_list);	
	class_addmethod(round_class, (t_method)round_nearest,  gensym("nearest"), A_FLOAT, 0);
}
