/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include <common/random.h>

typedef struct _decide{
    t_object      x_ob;
    t_cyrandom_state  x_rstate;
    int             x_id;
}t_decide;

static t_class *decide_class;

static void decide_bang(t_decide *x){
    t_cyrandom_state *rstate = &x->x_rstate;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    t_float random = (t_float)(cyrandom_frand(s1, s2, s3));
    outlet_float(((t_object *)x)->ob_outlet, random > 0);
}

static void decide_init(t_decide *x){
    t_cyrandom_state *rstate = &x->x_rstate;
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    cyrandom_frand(s1, s2, s3);
}

static void decide_seed(t_decide *x, t_symbol *s, int ac, t_atom *av){
    cyrandom_init(&x->x_rstate, cyget_seed(s, ac, av, x->x_id));
    decide_init(x);
}
static void decide_ft1(t_decide *x, t_floatarg f){
      if((unsigned int)f > 0){
        t_atom at[1];
        SETFLOAT(at, f);
        decide_seed(x, NULL, 1, at);
    }
    else
        decide_seed(x, NULL, 0, NULL);
    decide_init(x);
}

static void *decide_new(t_floatarg f){
    t_decide *x = (t_decide *)pd_new(decide_class);
    x->x_id = cyrandom_get_id();
    if((unsigned int)f > 0){
        t_atom at[1];
        SETFLOAT(at, f);
        decide_seed(x, NULL, 1, at);
    }
    else
        decide_seed(x, NULL, 0, NULL);
    decide_init(x);
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    return(x);
}

CYCLONE_OBJ_API void decide_setup(void){
    decide_class = class_new(gensym("decide"), (t_newmethod)decide_new, 0,
        sizeof(t_decide), 0, A_DEFFLOAT, 0);
    class_addbang(decide_class, decide_bang);
    class_addfloat(decide_class, decide_bang);
    class_addmethod(decide_class, (t_method)decide_ft1, gensym("ft1"), A_FLOAT, 0);
}
