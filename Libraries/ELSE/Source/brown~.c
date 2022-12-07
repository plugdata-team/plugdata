// Matt Barber and Porres (2017-2022)
// Based on SuperCollider's BrownNoise UGen

#include "m_pd.h"
#include "g_canvas.h"
#include "magic.h"
#include "random.h"

static t_class *brown_class;

typedef struct _brown{
    t_object       x_obj;
    t_random_state x_rstate;
    t_glist       *x_glist;
    t_float        x_lastout;
    t_float        x_step;
    t_float        x_inmode;
    int            x_id;
}t_brown;

static void brown_seed(t_brown *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static void brown_step(t_brown *x, t_floatarg f){
    x->x_step = f < 0 ? 0 : f > 1 ? 1 : f;
}

static t_int *brown_perform(t_int *w){
    t_brown *x = (t_brown *)(w[1]);
    int nblock = (t_int)(w[2]);
    t_sample *in = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    t_float lastout = x->x_lastout;
    while(nblock--){
        if(x->x_inmode){
            t_float impulse = (*in++ != 0);
            if(impulse){
                t_float noise = random_frand(s1, s2, s3);
                lastout += (noise * x->x_step);
                if(lastout > 1)
                    lastout = 2 - lastout;
                if(lastout < -1)
                    lastout = -2 - lastout;
            }
            *out++ = lastout;
        }
        else{
            t_float noise = random_frand(s1, s2, s3);
            lastout += (noise * x->x_step);
            if(lastout > 1)
                lastout = 2 - lastout;
            if(lastout < -1)
                lastout = -2 - lastout;
            *out++ = lastout;
        }
    }
    x->x_lastout = lastout;
    return(w+5);
}

static void brown_dsp(t_brown *x, t_signal **sp){
    x->x_inmode = else_magic_inlet_connection((t_object *)x, x->x_glist, 0, &s_signal);
    dsp_add(brown_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void *brown_new(t_symbol *s, int ac, t_atom *av){
    t_brown *x = (t_brown *)pd_new(brown_class);
    x->x_id = random_get_id();
    x->x_glist = (t_glist *)canvas_getcurrent();
    x->x_step = 0.125;
    if(ac >= 2 && (atom_getsymbol(av) == gensym("-seed"))){
        t_atom at[1];
        SETFLOAT(at, atom_getfloat(av+1));
        ac-=2, av+=2;
        brown_seed(x, s, 1, at);
    }
    else
        brown_seed(x, s, 0, NULL);
    if(ac && av->a_type == A_FLOAT)
        brown_step(x, atom_getfloat(av));
    outlet_new(&x->x_obj, &s_signal);
    return(x);
}

void brown_tilde_setup(void){
    brown_class = class_new(gensym("brown~"), (t_newmethod)brown_new,
        0, sizeof(t_brown), 0, A_GIMME, 0);
    class_addfloat(brown_class, brown_step);
    class_addmethod(brown_class, nullfn, gensym("signal"), 0);
    class_addmethod(brown_class, (t_method)brown_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(brown_class, (t_method)brown_seed, gensym("seed"), A_GIMME, 0);
}
