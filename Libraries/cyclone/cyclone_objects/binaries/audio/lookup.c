//old code scrapped, brand new code by Derek Kwan - 2016

#include "m_pd.h"
#include <common/api.h>
#include <math.h>
#include "signal/cybuf.h"

#define CYLKUPENDPT 512

static t_class *lookup_class;

typedef struct _lookup
{
	t_object x_obj;
	t_cybuf *x_cybuf;

	t_inlet *x_startlet; //inlet for stpt
	t_inlet *x_endlet; //inlet for endpoint
	t_outlet *x_out; //outlet
} t_lookup;

static double lookup_getlin(t_word * arr, int sz, double idx){
        
        double output;
        int tabphase1 = (int)idx;
        int tabphase2 = tabphase1 + 1;
        double frac = idx - (double)tabphase1;
        if(tabphase1 >= (sz - 1)){
                tabphase1 = sz - 1; //checking to see if index falls within bounds
                output = arr[tabphase1].w_float;
        }
        else if(tabphase1 < 0){
                tabphase1 = 0;
                output = arr[tabphase1].w_float;
            }
        else{
	    //linear interp
            double yb = arr[tabphase2].w_float; //read the buffer!
            double ya = arr[tabphase1].w_float; //read the buffer!
            output = ya+((yb-ya)*frac);
        
        };
        return output;
}


static void lookup_set(t_lookup *x, t_symbol *s)
{
   cybuf_setarray(x->x_cybuf, s); 
}

static void *lookup_new(t_symbol *s, int argc, t_atom *argv){ 
	t_lookup *x = (t_lookup *)pd_new(lookup_class);

	t_symbol * name = NULL;
	int floatarg = 0;//argument counter for floatargs (don't include symbol arg)
	//setting defaults for start and end points
	t_float stpt = 0;
	t_float endpt = CYLKUPENDPT;

	while(argc){
		if(argv -> a_type == A_SYMBOL){
			if(floatarg == 0){
				//we haven't hit any floatargs, go ahead and set name
				name = atom_getsymbolarg(0, argc, argv);
			};
		}
		else{
			//else we're dealing with a float
			switch(floatarg){
				case 0:
					stpt = atom_getfloatarg(0, argc, argv);
					break;
				case 1:
					endpt = atom_getfloatarg(0, argc, argv);
					break;
				default:
					break;
			};
			floatarg++; //increment the floatarg we're looking at
		};
		argc--;
		argv++;
	};

        x->x_cybuf = cybuf_init((t_class *) x, name, 1, 0);
	//boundschecking
	if(stpt < 0){
		stpt = 0;
	}
	else{
		stpt = (t_float)floor(stpt); //basically typecasting to int without actual typecasting
	};
	if(endpt < 0){
		endpt = 0;
	}
	else{
		endpt = (t_float)floor(endpt);
	};

	x->x_startlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	x->x_endlet = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	pd_float((t_pd *)x->x_startlet, stpt);
	pd_float( (t_pd *)x->x_endlet, endpt);
	x->x_out = outlet_new(&x->x_obj, gensym("signal"));
	
	return (x);
}

static t_int *lookup_perform(t_int *w)
{
	t_lookup *x = (t_lookup *)(w[1]);
        t_cybuf *c = x->x_cybuf;
	t_float *phase = (t_float *)(w[2]);
	t_float *stpt = (t_float *)(w[3]);
	t_float *endpt = (t_float *)(w[4]);
	t_float *out = (t_float *)(w[5]);
	int n = (int)(w[6]);
	int npts = c->c_npts;
	int maxidx = npts -1;
	t_word *buf = (t_word *)c->c_vectors[0];

	int i;
	for(i=0;i<n;i++){
		double curout = 0.f;
		double curphs = phase[i]; //current phase
		int curendpt = (int)endpt[i]; //current endpoint
		int curstpt = (int)stpt[i]; //current start point
		int cursz; //current size
		int curdir = 1.f; //current direction

		
		//if curstpt is bigger than curendpt, swap them and change the direction
		if(curstpt > curendpt){
			int temp;
			temp = curstpt;
			curstpt = curendpt;
			curendpt = temp;
			curdir *= -1.f;
		};
		

		//bounds checking
		if(curendpt > npts){
			curendpt = npts;
		};
		if(curstpt < 0){
			curstpt = 0;
		}
		else if(curstpt > maxidx){
			curstpt = maxidx;
		};
		//current size calculation, remember that maxidxes are alays size -1
		cursz = curendpt - curstpt;
		if(cursz < 1){
			cursz = 1;
		};

		if(curphs >= -1 && curphs <= 1 && buf){
			//if phase if b/w -1 and 1, map and read, else 0
			//mapping curphs to the realidx where -1 maps to stpt and 1 maps to stpt + cursz - 1
			//resulting in mappings to endpt indices (if stpt = 0, and endpt = 9, 8 is the maxidx to map to)
			//if curdir is -1, need to flip relation  
			
                        //calc the  real mapped idx in the buffer
			double realidx = (double)curstpt + (((curphs*curdir)+1.) * 0.5 * ((double)cursz-1.));

			//bounds checking
			if(realidx > maxidx){
				realidx = (double)maxidx;
			};
                        if(c->c_playable && npts){
			    curout = lookup_getlin(buf, npts, realidx); //read the buffer!
                        }
                        else{
                            curout = 0.;
                        };
		};

		out[i] = curout;
	};
	
	return (w+7);
}

static void lookup_dsp(t_lookup *x, t_signal **sp)
{
    cybuf_checkdsp(x->x_cybuf);	
    dsp_add(lookup_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
}

static void *lookup_free(t_lookup *x)
{
	inlet_free(x->x_startlet);
	inlet_free(x->x_endlet);
	outlet_free(x->x_out);
	return (void *)x;
}

CYCLONE_OBJ_API void lookup_tilde_setup(void)
{
   lookup_class = class_new(gensym("lookup~"),
			     (t_newmethod)lookup_new,
			     (t_method)lookup_free,
			     sizeof(t_lookup), CLASS_DEFAULT,
			     A_GIMME, 0);
    class_addmethod(lookup_class, nullfn, gensym("signal"), 0);
    class_addmethod(lookup_class, (t_method)lookup_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(lookup_class, (t_method)lookup_set,
		    gensym("set"), A_SYMBOL, 0);
}
