// matt barber and porres (2018)
// based on SuperCollider's white UGen

#include "m_pd.h"
#include "random.h"

static t_class *white_class;

typedef struct _white{
    t_object       x_obj;
    int            x_clip;
    t_random_state x_rstate;
    int            x_id;
}t_white;

static void white_clip(t_white *x, t_floatarg f){
    x->x_clip = f != 0;
}

static void white_seed(t_white *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static t_int *white_perform(t_int *w){
    t_white *x = (t_white *)(w[1]);
    int n = (t_int)(w[2]);
    t_random_state *rstate = (t_random_state *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    uint32_t *s1 = &rstate->s1;
    uint32_t *s2 = &rstate->s2;
    uint32_t *s3 = &rstate->s3;
    while(n--){
        t_float noise = (t_float)(random_frand(s1, s2, s3));
        if(x->x_clip)
            noise = noise > 0 ? 1 : -1;
        *out++ = noise;
    }
    return(w+5);
}

static void white_dsp(t_white *x, t_signal **sp){
    dsp_add(white_perform, 4, x, sp[0]->s_n, &x->x_rstate, sp[0]->s_vec);
}

static void *white_new(t_symbol *s, int ac, t_atom *av){
    t_white *x = (t_white *)pd_new(white_class);
    x->x_id = random_get_id();
    outlet_new(&x->x_obj, &s_signal);
    x->x_clip = 0;
    white_seed(x, s, 0, NULL);
    while(ac){
        if(av->a_type == A_SYMBOL){
            if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
                t_atom at[1];
                SETFLOAT(at, atom_getfloat(av+1));
                ac-=2, av+=2;
                white_seed(x, s, 1, at);
            }
            else if(atom_getsymbol(av) == gensym("-clip")){
                x->x_clip = 1;
                ac--, av++;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    }
    return(x);
errstate:
    pd_error(x, "[white~]: improper args");
    return(NULL);
}

void white_tilde_setup(void){
    white_class = class_new(gensym("white~"), (t_newmethod)white_new, 0,
        sizeof(t_white), 0, A_GIMME, 0);
    class_addmethod(white_class, (t_method)white_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(white_class, (t_method)white_seed, gensym("seed"), A_GIMME, 0);
    class_addmethod(white_class, (t_method)white_clip, gensym("clip"), A_FLOAT, 0);
}
