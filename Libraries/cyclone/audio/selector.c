// Derek Kwan - 2016

#include "m_pd.h"
#include <common/api.h>
#include "common/magicbit.h"
//#include <math.h>

#define PDCYSELTORSIGPUT 1 //default number of sigputs (inlets without selector inlet)
#define MAXINTPUT 500 //maximum number of channel inlets
#define PDCYSELTORSTATE 0 //default state (0 = closed)

typedef struct _selector
{
    t_object   x_obj;
	t_float *x_mainsig; //the main signal being piped in
    int 	  x_sigputs; //inlets excluding selector idx (1 indexed)
	t_float   x_state; //0 = closed, nonzero = index of inlet to pass (1 indexed)
    t_float  **x_ivecs; // copying from matrix
    t_float  *x_ovec; // only should be single pointer since we're dealing with an array
						//rather than an array of arrays
	t_glist  *x_glist;
	t_float  **x_signalscalars;
	int      *x_hasfeeders;
} t_selector;

static t_class *selector_class;

//EXTERN t_float *obj_findsignalscalar(t_object *x, int m);

static t_int *selector_perform(t_int *w)
{
    t_selector *x = (t_selector *)(w[1]);
    int nblock = (int)(w[2]);
	
	t_float *state = x->x_mainsig;
    t_float **ivecs = x->x_ivecs;
    t_float *ovec = x->x_ovec;
    t_float **signalscalars = x->x_signalscalars;
    int     *hasfeeders = x->x_hasfeeders;

	int i,j;

	int sigputs = x->x_sigputs;

	for(i=0; i< nblock; i++){
        int curst = (int)state[i];
        if (curst > sigputs){
            curst = sigputs;
        }
		t_float output = 0;
		if(curst != 0){
			for(j=0; j<sigputs;j++){
				if (!magic_isnan(*(signalscalars[j])))
				{
					magic_setnan(signalscalars[j]);
					pd_error(x, "selector~: doesn't understand 'float'");
				}
				if(curst == (j+1)){
 					if (hasfeeders[j])
						output = ivecs[j][i];
 					else output = 0.0;
				};
			};
		};
		ovec[i] = output;
	};
    return (w + 3);
}


static void selector_dsp(t_selector *x, t_signal **sp)
{
	int i, nblock = sp[0]->s_n;
    t_signal **sigp = sp;
	x->x_mainsig = (*sigp++)->s_vec; //the first sig in is the selector idx
    for (i = 0; i < x->x_sigputs; i++){ //now for the sigputs
		*(x->x_ivecs+i) = (*sigp++)->s_vec;
		magic_setnan(x->x_signalscalars[i]);
		x->x_hasfeeders[i] = magic_inlet_connection((t_object *)x, x->x_glist, i+1, &s_signal);
	};
	x->x_ovec = (*sigp++)->s_vec; //now for the outlet
	dsp_add(selector_perform, 2, x, nblock);
}

static void *selector_new(t_symbol *s, int argc, t_atom *argv)
{
    t_selector *x = (t_selector *)pd_new(selector_class);
    t_float sigputs = (t_float) PDCYSELTORSIGPUT; //inlets not counting selector input
	t_float state = (t_float) PDCYSELTORSTATE; //start off closed initially
    int i;
    int argnum = 0;
    while(argc > 0){
        if(argv -> a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0, argc, argv);
            switch(argnum){
                case 0:
					sigputs = argval;
                    break;
				case 1:
					state = argval;
					break;
                default:
                    break;
            };
            argc--;
            argv++;
            argnum++;
        };
    };

	//bounds checking
	if(sigputs < (t_float)PDCYSELTORSIGPUT){
		sigputs = PDCYSELTORSIGPUT;
	}
    else if(sigputs > (t_float)MAXINTPUT){
    sigputs = MAXINTPUT;
    };
    if(state < 0){
		state = 0;
	}
	else if(state > sigputs){
		state = sigputs;
	};

	x->x_sigputs = (int)sigputs;
	x->x_state = state; 
	x->x_ivecs = getbytes(sigputs * sizeof(*x->x_ivecs));
	x->x_signalscalars = getbytes(sigputs * sizeof(*x->x_signalscalars));
	x->x_hasfeeders = getbytes(sigputs * sizeof(*x->x_hasfeeders));
    
	for (i = 0; i < sigputs; i++){
		t_float f;
		magic_setnan(&f);
        pd_float((t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal), f);
        x->x_signalscalars[i] = obj_findsignalscalar((t_object *)x, i+1);
    };
    outlet_new((t_object *)x, &s_signal);
    x->x_glist = canvas_getcurrent();
    
    return (x);
}

void * selector_free(t_selector *x){
    
	 freebytes(x->x_ivecs, x->x_sigputs * sizeof(*x->x_ivecs));
	 freebytes(x->x_signalscalars, x->x_sigputs * sizeof(*x->x_signalscalars));
	 freebytes(x->x_hasfeeders, x->x_sigputs * sizeof(x->x_hasfeeders));
         return (void *) x;
}

CYCLONE_OBJ_API void selector_tilde_setup(void)
{
    selector_class = class_new(gensym("selector~"), (t_newmethod)selector_new, (t_method)selector_free,
            sizeof(t_selector), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(selector_class, (t_method)selector_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(selector_class, t_selector, x_state);
}
