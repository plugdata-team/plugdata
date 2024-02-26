/* Copyright (c) 1997-2003 Miller Puckette, krzYszcz, and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include "common/shared.h"
#include <common/random.h>

typedef struct _rand{
    t_object        x_obj;
    t_float         x_input;
    double          x_lastphase;
    double          x_nextphase;
    float           x_rcpsr;
    float           x_sr;
    float           x_target;
    float           x_scaling;  // LATER use phase increment
    t_cyrandom_state  x_rstate;
    int             x_id;
}t_rand;

static t_class *rand_class;

static t_int *rand_perform(t_int *w){
    t_rand *x = (t_rand *)(w[1]);
    int nblock = (int)(w[2]);
    t_float *rin = (t_float *)(w[3]);
    t_float *out = (t_float *)(w[4]);
    double lastph = x->x_lastphase;
    double ph = x->x_nextphase;
    double tfph = ph + SHARED_UNITBIT32;
    t_shared_wrappy wrappy;
    int32_t normhipart;
    float rcpsr = x->x_rcpsr;
    float sr = x->x_sr;
    float target = x->x_target;
    float scaling = x->x_scaling;
    wrappy.w_d = SHARED_UNITBIT32;
    normhipart = wrappy.w_i[SHARED_HIOFFSET];
    t_cyrandom_state *rstate = &x->x_rstate;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    while (nblock--){
        float rate = *rin++;
        if(rate < 0)
            rate = 0;
        if(ph > lastph){
            t_float newtarget = (t_float)(cyrandom_frand(s1, s2, s3));
            x->x_scaling = scaling = target - newtarget;
            x->x_target = target = newtarget;
        }
        *out++ = ph * scaling + target;
        lastph = ph;
        if(rate >= sr)
            rate = sr - 1;
        if(rate > 0)
            rate = -rate;
    	tfph += rate * rcpsr;
        wrappy.w_d = tfph;
    	wrappy.w_i[SHARED_HIOFFSET] = normhipart;
        ph = wrappy.w_d - SHARED_UNITBIT32;
    }
    x->x_lastphase = lastph;
    x->x_nextphase = ph;
    return(w+5);
}

static void rand_dsp(t_rand *x, t_signal **sp){
    x->x_sr = sp[0]->s_sr;
    x->x_rcpsr = 1. / sp[0]->s_sr;
    dsp_add(rand_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void rand_seed(t_rand *x, t_symbol *s, int ac, t_atom *av){
    cyrandom_init(&x->x_rstate, cyget_seed(s, ac, av, x->x_id));
}

static void *rand_new(t_floatarg f){
    t_rand *x = (t_rand *)pd_new(rand_class);
    x->x_id = cyrandom_get_id();
    t_symbol *s = NULL;
    rand_seed(x, s, 0, NULL);
    x->x_lastphase = 0.;
    x->x_nextphase = 1.;  /* start from 0, force retargetting */
    x->x_target = x->x_scaling = 0;
    x->x_input = (f > 0. ? f : 0.);
    outlet_new((t_object *)x, &s_signal);
    return(x);
}

CYCLONE_OBJ_API void rand_tilde_setup(void){
    rand_class = class_new(gensym("rand~"), (t_newmethod)rand_new, 0,
        sizeof(t_rand), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addmethod(rand_class, (t_method)rand_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(rand_class, t_rand, x_input);
}
