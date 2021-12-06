// porres 2020

#include "m_pd.h"
#include <stdlib.h>

typedef struct _chance{
	t_object    x_obj;
    t_float     x_lastin;
    t_int       x_n_outlets;
    int         x_val;
	t_float    *x_probabilities; // numbers to chance against
    t_float     x_range;
    t_float   **ins;
    t_float   **outs;
}t_chance;

static t_class *chance_class;

static void chance_seed(t_chance *x, t_floatarg f){
    x->x_val = (int)f * 1319;
}

t_int *chance_perform(t_int *w){
    int i, j;
    t_chance *x = (t_chance *) w[1]; // first is object
    t_float **ins = x->ins;
    t_float **outs = x->outs;
    t_float *main_input;
	t_float *inlet;
	t_float *chance_outlet;
    t_int outlets = x->x_n_outlets;
    t_float last = x->x_lastin;
    int val = x->x_val;
    int n = (int)w[outlets + 3]; // last is block
// copy main input vector
    main_input = (t_float *) w[2]; // main input
    for(i = 0; i < n; i++)
        ins[0][i] = main_input[i];
    inlet = ins[0];
// assign output vectors
    for(i = 0; i < outlets; i++)
        outs[i] = (t_float *) w[3 + i];
// clean outlets
    for(i = 0; i < outlets; i++){
		chance_outlet = outs[i];
		for(j = 0; j < n; j++)
			chance_outlet[j] = 0.0;
	}
// chance
	for(i = 0; i < n; i++){
        if(inlet[i] != 0 && last == 0){ // impulse
            float random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
            val = val * 435898247 + 382842987;
            random = x->x_range * (random + 1) / 2;
			for(j = 0; j < outlets; j++){
                if(random < x->x_probabilities[j]){ 
                    chance_outlet = outs[j];
                    chance_outlet[i] = inlet[i];
                    break;
                }
			}
        }
        last = inlet[i];
	}
    x->x_val = val;
    x->x_lastin = inlet[n-1];
    return(w+outlets+4);
}

void chance_dsp(t_chance *x, t_signal **sp){
	int i;
    t_int **sigvec;
    int n_sig = x->x_n_outlets + 3; // outs + 3 (ob / in / block size)
    sigvec  = (t_int **) calloc(n_sig, sizeof(t_int *));
	for(i = 0; i < n_sig; i++)
		sigvec[i] = (t_int *) calloc(sizeof(t_int), 1);
	sigvec[0] = (t_int *)x; // object
	sigvec[n_sig - 1] = (t_int *)sp[0]->s_n; // block size (n)
	for(i = 1; i < n_sig - 1; i++) // I/O
		sigvec[i] = (t_int *)sp[i-1]->s_vec;
    dsp_addv(chance_perform, n_sig, (t_int *)sigvec);
    free(sigvec);
}

void chance_free(t_chance *x){
    free(x->x_probabilities);
    free(x->outs);
    free(x->ins[0]);
    free(x->ins);
}

void *chance_new(t_symbol *s, short argc, t_atom *argv){
    t_chance *x = (t_chance *)pd_new(chance_class);
    s = NULL;
    static int init_seed = 234599;
    init_seed *= 1319;
    t_int seed = init_seed;
    x->x_lastin = 0;
    x->x_range = 0;
    if(!argc){
        x->x_n_outlets = 2;
        outlet_new(&x->x_obj, gensym("signal"));
        outlet_new(&x->x_obj, gensym("signal"));
        x->x_probabilities = (float *) malloc(2 * sizeof(float));
        x->x_probabilities[0] = 50;
        x->x_probabilities[1] = 100;
        x->x_range = 100;
    }
    else if(argc == 1){
        x->x_n_outlets = 2;
        outlet_new(&x->x_obj, gensym("signal"));
        outlet_new(&x->x_obj, gensym("signal"));
        x->x_probabilities = (float *) malloc(2 * sizeof(float));
        x->x_probabilities[0] = atom_getfloatarg(0, argc, argv);
        x->x_probabilities[1] = 100;
        x->x_range = 100;
    }
    else{
        int i;
        x->x_n_outlets = (t_int)argc;
        for(i = 0; i < x->x_n_outlets ; i++)
            outlet_new(&x->x_obj, gensym("signal"));
        x->x_probabilities = (float *) malloc((x->x_n_outlets) * sizeof(float));
        for(i = 0; i < argc; i++)
            x->x_probabilities[i] = (x->x_range += atom_getfloatarg(i, argc, argv));
    }
    x->ins = (t_float **) malloc(1 * sizeof(t_float *));
    x->outs = (t_float **) malloc(x->x_n_outlets * sizeof(t_float *));
    x->ins[0] = (t_float *) malloc(8192 * sizeof(t_float));
    x->x_val = seed; // load seed value
    return(x);
}

void chance_tilde_setup(void){
    chance_class = class_new(gensym("chance~"), (t_newmethod)chance_new,
        (t_method)chance_free, sizeof(t_chance), 0, A_GIMME, 0);
    class_addmethod(chance_class, nullfn, gensym("signal"), 0);
    class_addmethod(chance_class, (t_method)chance_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(chance_class, (t_method)chance_seed, gensym("seed"), A_DEFFLOAT, 0);
}
