// matt barber and porres (2017-2018)
// based on SuperCollider's GrayNoise UGen

#include "m_pd.h"
#include "random.h"

static t_class *gray_class;

typedef struct _gray{
    t_object       x_obj;
    int            x_base;
    t_random_state x_rstate;
    int            x_id;
}t_gray;

static void gray_seed(t_gray *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
    x->x_base = x->x_rstate.s1 ^ x->x_rstate.s2 ^ x->x_rstate.s3;
}

static t_int *gray_perform(t_int *w){
    t_gray *x = (t_gray *)(w[1]);
    int n = (t_int)(w[2]);
    t_random_state *rstate = (t_random_state *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    int base = x->x_base;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    while(n--){
    	base ^= 1L << (random_trand(s1, s2, s3) & 31);
    	*out++ = base * 4.65661287308e-10f; // That's 1/(2^31), so normalizes the int to 1.0
	}
    x->x_base = base;
    return(w+5);
}

static void gray_dsp(t_gray *x, t_signal **sp){
    dsp_add(gray_perform, 4, x, sp[0]->s_n, &x->x_rstate, sp[0]->s_vec);
}

static void *gray_new(t_symbol *s, int ac, t_atom *av){
    t_gray *x = (t_gray *)pd_new(gray_class);
    x->x_id = random_get_id();
    if(ac >= 2 && (atom_getsymbol(av) == gensym("-seed"))){
        t_atom at[1];
        SETFLOAT(at, atom_getfloat(av+1));
        ac-=2, av+=2;
        gray_seed(x, s, 1, at);
    }
    else
        gray_seed(x, s, 0, NULL);
    outlet_new(&x->x_obj, &s_signal);
    return(x);
}

void gray_tilde_setup(void){
    gray_class = class_new(gensym("gray~"), (t_newmethod)gray_new,
        0, sizeof(t_gray), 0, A_GIMME, 0);
    class_addmethod(gray_class, (t_method)gray_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(gray_class, (t_method)gray_seed, gensym("seed"), A_GIMME, 0);
}
