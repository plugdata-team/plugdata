// matt barber and porres (2017-2018)
// based on SuperCollider's GrayNoise UGen

#include "m_pd.h"
#include "random.h"

static t_class *gray_class;

typedef struct _gray{
    t_object  x_obj;
    int x_base;
    t_random_state x_rstate;
    t_outlet *x_outlet;

} t_gray;

static void gray_float(t_gray *x, t_floatarg f){
    random_init(&x->x_rstate, f);
    x->x_base = x->x_rstate.s1 ^ x->x_rstate.s2 ^ x->x_rstate.s3;
}

static uint32_t random_trand(uint32_t* s1, uint32_t* s2, uint32_t* s3 ){
    // This function is provided for speed in inner loops where the
    // state variables are loaded into registers.
    // Thus updating the instance variables can
    // be postponed until the end of the loop.
    *s1 = ((*s1 &  (uint32_t)- 2) << 12) ^ (((*s1 << 13) ^  *s1) >> 19);
    *s2 = ((*s2 &  (uint32_t)- 8) <<  4) ^ (((*s2 <<  2) ^  *s2) >> 25);
    *s3 = ((*s3 &  (uint32_t)-16) << 17) ^ (((*s3 <<  3) ^  *s3) >> 11);
    return *s1 ^ *s2 ^ *s3;
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
    /*while(n--){
        t_float noise = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(16.0 / 0x40000000);
        val = val * 435898247 + 382842987;
        int shift = (int)(noise + 16.0);
        shift = (shift == 32 ? 31 : shift); // random numbers from 0 - 31
        base ^= 1L << shift;
        *out++ = base * 4.65661287308e-10f; // That's 1/(2^31), so normalizes the int to 1.0
    }    */
    while(n--){
    	base ^= 1L << (random_trand(s1, s2, s3) & 31);
    	*out++ = base * 4.65661287308e-10f; // That's 1/(2^31), so normalizes the int to 1.0
	}
    x->x_base = base;
    return (w + 5);
}

static void gray_dsp(t_gray *x, t_signal **sp){
    dsp_add(gray_perform, 4, x, sp[0]->s_n, &x->x_rstate, sp[0]->s_vec);
}

static void *gray_new(t_symbol *s, int ac, t_atom *av){
    t_gray *x = (t_gray *)pd_new(gray_class);
    static int seed = 1;
    if ((ac) && (av->a_type == A_FLOAT))
        random_init(&x->x_rstate, atom_getfloatarg(0, ac, av));
    else
    	random_init(&x->x_rstate, seed++);
    x->x_base = x->x_rstate.s1 ^ x->x_rstate.s2 ^ x->x_rstate.s3;
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    return(x);
}

void gray_tilde_setup(void){
    gray_class = class_new(gensym("gray~"), (t_newmethod)gray_new,
                            0, sizeof(t_gray), 0, A_GIMME, 0);
    class_addmethod(gray_class, (t_method)gray_dsp, gensym("dsp"), A_CANT, 0);
    class_addlist(gray_class, gray_float);
}
