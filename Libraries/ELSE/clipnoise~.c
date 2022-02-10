// matt barber and porres (2018)
// based on SuperCollider's clipnoise UGen

#include "m_pd.h"
#include "random.h"

static t_class *clipnoise_class;

typedef struct _clipnoise{
    t_object        x_obj;
    t_random_state  x_rstate;
    t_outlet       *x_outlet;
}t_clipnoise;

static void clipnoise_float(t_clipnoise *x, t_floatarg f){
    random_init(&x->x_rstate, f);
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

static float random_frand(uint32_t* s1, uint32_t* s2, uint32_t* s3)
{
    // return a float from -1.0 to +0.999...
    union { uint32_t i; float f; } u;        // union for floating point conversion of result
    u.i = 0x40000000 | (random_trand(s1, s2, s3) >> 9);
    return u.f - 3.f;
}

static t_int *clipnoise_perform(t_int *w){
    t_clipnoise *x = (t_clipnoise *)(w[1]);
    int n = (t_int)(w[2]);
    t_random_state *rstate = (t_random_state *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    while(n--)
        *out++ = (t_float)(random_frand(s1, s2, s3)) > 0 ? 1 : -1;
    return (w + 5);
}

static void clipnoise_dsp(t_clipnoise *x, t_signal **sp){
    dsp_add(clipnoise_perform, 4, x, sp[0]->s_n, &x->x_rstate, sp[0]->s_vec);
}

static void *clipnoise_new(t_symbol *s, int ac, t_atom *av){
    t_clipnoise *x = (t_clipnoise *)pd_new(clipnoise_class);
    x->x_outlet = outlet_new(&x->x_obj, &s_signal);
    static int seed = 1;
    if(ac && av->a_type == A_FLOAT)
        random_init(&x->x_rstate, atom_getfloatarg(0, ac, av));
    else
    	random_init(&x->x_rstate, seed++);
    return(x);
}

void clipnoise_tilde_setup(void){
    clipnoise_class = class_new(gensym("clipnoise~"), (t_newmethod)clipnoise_new,
            0, sizeof(t_clipnoise), 0, A_GIMME, 0);
    class_addmethod(clipnoise_class, (t_method)clipnoise_dsp, gensym("dsp"), A_CANT, 0);
    class_addfloat(clipnoise_class, clipnoise_float);
}
