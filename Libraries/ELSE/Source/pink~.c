// matt barber and porres (2018)
// based on SuperCollider's pink UGen

#include "m_pd.h"
#include "random.h"

#define PINK_MAX_OCT 40

static t_class *pink_class;

typedef struct _pink{
    t_object       x_obj;
    float          x_signals[PINK_MAX_OCT];
    float          x_total;
    float          x_sr;
    int            x_octaves;
    int            x_octaves_set;
    t_random_state x_rstate;
    int             x_id;
}t_pink;

static void pink_init(t_pink *x){
    float *signals = x->x_signals;
    float total = 0;
    t_random_state *rstate = &x->x_rstate;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    for(int i = 0; i < x->x_octaves - 1; ++i){
        float noise = (random_frand(s1, s2, s3));
        total += noise;
        signals[i] = noise;
    }
    x->x_total = total;
}

static void pink_seed(t_pink *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
    pink_init(x);
}

static void pink_oct(t_pink *x, t_floatarg f){
    x->x_octaves = (int)f  < 1 ? 1 : (int)f > PINK_MAX_OCT ? PINK_MAX_OCT : (int)f;
    x->x_octaves_set = 0;
    pink_init(x);
}

static t_int *pink_perform(t_int *w){
    t_pink *x = (t_pink *)(w[1]);
    int n = (t_int)(w[2]);
    t_random_state *rstate = (t_random_state *)(w[3]);
    float *signals = (float*)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    float total = x->x_total;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    while(n--){
    	uint32_t rcounter = random_trand(s1, s2, s3);
    	float newrand = random_frand(s1, s2, s3);
    	int k = (CLZ(rcounter));
    	if(k < (x->x_octaves-1)){
    		float prevrand = signals[k];
    		signals[k] = newrand;
    		total += (newrand - prevrand);
    	}
    	newrand = (random_frand(s1, s2, s3));
    	*out++ = (t_float)(total+newrand)/x->x_octaves;
	}
	x->x_total = total;
    return(w+6);
}

static void pink_dsp(t_pink *x, t_signal **sp){
    if(x->x_octaves_set && x->x_sr != sp[0]->s_n){
        t_float sr = x->x_sr = sp[0]->s_sr;
        x->x_octaves = 1;
        while(sr >= 40){
            sr *= 0.5;
            x->x_octaves++;
        }
        pink_init(x);
    }
    dsp_add(pink_perform, 5, x, sp[0]->s_n, &x->x_rstate, &x->x_signals, sp[0]->s_vec);
}

static void *pink_new(t_symbol *s, int ac, t_atom *av){
    t_pink *x = (t_pink *)pd_new(pink_class);
    x->x_id = random_get_id();
    outlet_new(&x->x_obj, &s_signal);
    x->x_sr = 0;
    if(ac >= 2 && (atom_getsymbol(av) == gensym("-seed"))){
        t_atom at[1];
        SETFLOAT(at, atom_getfloat(av+1));
        ac-=2, av+=2;
        pink_seed(x, s, 1, at);
    }
    else
        pink_seed(x, s, 0, NULL);
    if(ac && av->a_type == A_FLOAT)
        pink_oct(x, atom_getfloat(av));
    else
    	x->x_octaves_set = 1;
    return(x);
}

void pink_tilde_setup(void){
    pink_class = class_new(gensym("pink~"), (t_newmethod)pink_new,
        0, sizeof(t_pink), 0, A_GIMME, 0);
    class_addfloat(pink_class, pink_oct);
    class_addmethod(pink_class, (t_method)pink_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(pink_class, (t_method)pink_seed, gensym("seed"), A_GIMME, 0);
}
