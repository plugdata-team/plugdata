// Porres 2018

#include "m_pd.h"
#include <math.h>
#include <string.h>

static t_class *quantizer_class;

typedef struct _quantizer{
	t_object    x_obj;
	t_float     x_mode;
} t_quantizer;

static void quantizer_mode(t_quantizer *x, t_float f){
    x->x_mode = (int)f;
    if(x->x_mode < 0)
        x->x_mode = 0;
    if(x->x_mode > 3)
        x->x_mode = 3;
}

static t_int *quantizer_perform(t_int *w){
	t_quantizer *x = (t_quantizer *)(w[1]);
	t_float *in1 = (t_float *)(w[2]);
	t_float *in2 = (t_float *)(w[3]);
	t_float *out = (t_float *)(w[4]);
	int n = (int)(w[5]);
	while(n--){
		float output, div;
		float in = *(in1++);
		float step = *(in2++);
		if(step > 0.){ // quantize
			div = in/step; // get division
			if(x->x_mode == 0) // round
				output = step * round(div);
			else if(x->x_mode == 1) // truncate
                output = step * trunc(div);
            else if(x->x_mode == 2) // floor
                output = step * floor(div);
            else // ceil (mode == 3)
                output = step * ceil(div);
		}
		else // step is <= 0, do nothing
			output = in;
		*out++ = output;
	};
	return(w+6);
}

static void quantizer_dsp(t_quantizer *x, t_signal **sp){
	dsp_add(quantizer_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void *quantizer_new(t_symbol *s, int argc, t_atom *argv){
    t_quantizer *x = (t_quantizer *)pd_new(quantizer_class);
    float f = 0;
    int numargs = 0;
    x->x_mode = 0;
    while(argc > 0 ){
        if(argv->a_type == A_FLOAT){
            switch(numargs){
                case 0:
                    f = atom_getfloatarg(0, argc, argv);
                    numargs++;
                    argc--;
                    argv++;
                    break;
                case 1:
                    x->x_mode = (int)atom_getfloatarg(0, argc, argv);
                    numargs++;
                    argc--;
                    argv++;
                    break;
                default:
                    numargs++;
                    argc--;
                    argv++;
                    break;
            };
        }
        else if(argv -> a_type == A_SYMBOL){
            t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
            if(!strcmp(curarg->s_name, "-mode") && argc == 2){
                if((argv+1)->a_type == A_FLOAT){
                    x->x_mode = (int)atom_getfloatarg(1, argc, argv);
                    argc -= 2;
                    argv += 2;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    };
    if(x->x_mode < 0)
        x->x_mode = 0;
    if(x->x_mode > 3)
        x->x_mode = 3;
    pd_float( (t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal), f);
    outlet_new(&x->x_obj, gensym("signal"));
    return (x);
errstate:
    pd_error(x, "quantizer~: improper args");
    return NULL;
}

void quantizer_tilde_setup(void){
	quantizer_class = class_new(gensym("quantizer~"), (t_newmethod)quantizer_new, 0,
                                sizeof(t_quantizer), 0, A_GIMME, 0);
	class_addmethod(quantizer_class, nullfn, gensym("signal"), 0);
	class_addmethod(quantizer_class, (t_method)quantizer_dsp, gensym("dsp"), A_CANT, 0);
	class_addmethod(quantizer_class, (t_method)quantizer_mode,  gensym("mode"), A_FLOAT, 0);
}
