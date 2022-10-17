// porres 2020

#include "m_pd.h"
#include <stdlib.h>
#include "random.h"

typedef struct _chance{
	t_object       x_obj;
    t_float        x_lastin;
    t_int          x_n_outlets;
	t_float       *x_probabilities; // numbers to chance against
    t_random_state x_rstate;
    t_float        x_range;
    t_float      **ins;
    t_float      **outs;
    int            x_id;
}t_chance;

static t_class *chance_class;

static void chance_seed(t_chance *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
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
    int n = (int)w[outlets + 3]; // last is block
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
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
            float random = (t_float)(random_frand(s1, s2, s3));
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
    x->x_lastin = inlet[n-1];
    return(w+outlets+4);
}

void chance_dsp(t_chance *x, t_signal **sp){
    int n_sig = x->x_n_outlets + 3; // outs + 3 (ob / in / block size)
    t_int* sigvec = (t_int*)calloc(n_sig, sizeof(t_int));
	sigvec[0] = (t_int)x; // object
    for(int i = 1; i < n_sig - 1; i++) // I/O
        sigvec[i] = (t_int)sp[i-1]->s_vec;
    sigvec[n_sig - 1] = (t_int)sp[0]->s_n; // block size (n)
    dsp_addv(chance_perform, n_sig, (t_int *)sigvec);
    free(sigvec);
}

void chance_free(t_chance *x){
    free(x->x_probabilities);
    free(x->outs);
    free(x->ins[0]);
    free(x->ins);
}

void *chance_new(t_symbol *s, short ac, t_atom *av){
    s = NULL;
    t_chance *x = (t_chance *)pd_new(chance_class);
    x->x_id = random_get_id();
    chance_seed(x, s, 0, NULL);
    x->x_lastin = 0;
    x->x_range = 0;
    if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
        t_atom at[1];
        SETFLOAT(at, atom_getfloat(av+1));
        ac-=2, av+=2;
        chance_seed(x, s, 1, at);
    }
    if(!ac){
        x->x_n_outlets = 2;
        outlet_new(&x->x_obj, gensym("signal"));
        outlet_new(&x->x_obj, gensym("signal"));
        x->x_probabilities = (float *) malloc(2 * sizeof(float));
        x->x_probabilities[0] = 50;
        x->x_probabilities[1] = 100;
        x->x_range = 100;
    }
    else if(ac == 1){
        x->x_n_outlets = 2;
        outlet_new(&x->x_obj, gensym("signal"));
        outlet_new(&x->x_obj, gensym("signal"));
        x->x_probabilities = (float *) malloc(2 * sizeof(float));
        x->x_probabilities[0] = atom_getfloatarg(0, ac, av);
        x->x_probabilities[1] = 100;
        x->x_range = 100;
    }
    else{
        int i;
        x->x_n_outlets = (t_int)ac;
        for(i = 0; i < x->x_n_outlets ; i++)
            outlet_new(&x->x_obj, gensym("signal"));
        x->x_probabilities = (float *) malloc((x->x_n_outlets) * sizeof(float));
        for(i = 0; i < ac; i++)
            x->x_probabilities[i] = (x->x_range += atom_getfloatarg(i, ac, av));
    }
    x->ins = (t_float **) malloc(1 * sizeof(t_float *));
    x->outs = (t_float **) malloc(x->x_n_outlets * sizeof(t_float *));
    x->ins[0] = (t_float *) malloc(8192 * sizeof(t_float));
    return(x);
}

void chance_tilde_setup(void){
    chance_class = class_new(gensym("chance~"), (t_newmethod)chance_new,
        (t_method)chance_free, sizeof(t_chance), 0, A_GIMME, 0);
    class_addmethod(chance_class, nullfn, gensym("signal"), 0);
    class_addmethod(chance_class, (t_method)chance_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(chance_class, (t_method)chance_seed, gensym("seed"), A_GIMME, 0);
}
