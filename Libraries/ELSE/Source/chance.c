// porres 2020

#include "m_pd.h"
#include "random.h"

static t_class *chance_class;

typedef struct _chance{
    t_object    x_obj;
    t_atom     *x_av;
    t_random_state x_rstate;
    int         x_ac;
    int         x_bytes;
    int         x_coin;
    int         x_id;
    float       x_chance;
    t_float     x_range;
    t_outlet  **x_outs;
    t_outlet   *x_out_index;
}t_chance;

static void chance_output(t_chance *x, t_floatarg f){
    int n;
    if(x->x_coin)
        outlet_bang(x->x_outs[f > x->x_chance]);
    else{
        for(n = 0; n < x->x_ac; n++){
            if(f < x->x_av[n].a_w.w_float){
                outlet_bang(x->x_outs[n]);
                return;
            }
        }
    }
}

static void chance_bang(t_chance *x){
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    t_float random = (t_float)(random_frand(s1, s2, s3));
    chance_output(x, x->x_range * (random + 1) / 2);
}

static void chance_list(t_chance *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(!argc)
        chance_bang(x);
    else if(!x->x_coin){
        x->x_range = 0;
        for(int i = 0; i < x->x_ac; i++){
            if(argv->a_type == A_FLOAT){
                t_float argval = atom_getfloatarg(0, argc, argv);
                SETFLOAT(x->x_av+i, x->x_range += argval);
                argv++;
            }
        }
    }
}

static void chance_seed(t_chance *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static void *chance_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_chance *x = (t_chance *)pd_new(chance_class);
    x->x_id = random_get_id();
    x->x_range = 0;
    x->x_coin = 0;
    chance_seed(x, s, 0, NULL);
    t_outlet **outs;
    if(!argc){
        x->x_bytes = x->x_ac*sizeof(t_atom);
        x->x_av = (t_atom *)getbytes(x->x_bytes);
        x->x_ac = 2;
        x->x_outs = (t_outlet **)getbytes(x->x_ac * sizeof(*outs));
        x->x_outs[0] = outlet_new(&x->x_obj, &s_bang);
        x->x_outs[1] = outlet_new(&x->x_obj, &s_bang);
        x->x_chance = 50;
        x->x_range = 100;
        x->x_coin = 1;
    }
    else if(argc == 1){
        if(argv->a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0, argc, argv);
            x->x_ac = 2;
            x->x_bytes = x->x_ac*sizeof(t_atom);
            x->x_av = (t_atom *)getbytes(x->x_bytes);
            x->x_chance = argval < 0 ? 0 : argval > 100 ? 100 : argval;
            x->x_outs = (t_outlet **)getbytes(x->x_ac * sizeof(*outs));
            x->x_outs[0] = outlet_new(&x->x_obj, &s_bang);
            x->x_outs[1] = outlet_new(&x->x_obj, &s_bang);
            x->x_range = 100;
            x->x_coin = 1;
        }
        else if(argv->a_type == A_SYMBOL){
            pd_error(x, "[chance]: takes only floats as arguments");
            return(NULL);
        }
    }
    else{
        x->x_ac = argc;
        x->x_bytes = x->x_ac*sizeof(t_atom);
        x->x_av = (t_atom *)getbytes(x->x_bytes);
        x->x_outs = (t_outlet **)getbytes(x->x_ac * sizeof(*outs));
        int n = 0;
        while(argc > 0){
            if(argv->a_type == A_FLOAT){
                t_float argval = atom_getfloatarg(0, argc, argv);
                SETFLOAT(x->x_av+n, x->x_range += argval);
                n++, argv++, argc--;
            }
            else if(argv->a_type == A_SYMBOL && !n){
                if(atom_getsymbolarg(0, argc, argv) == gensym("-seed")){
                    t_atom at[1];
                    SETFLOAT(at, atom_getfloat(argv+1));
                    x->x_ac-=2, argc-=2, argv+=2;
                    chance_seed(x, s, 1, at);
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        for(int i = 0; i < x->x_ac; i++)
            x->x_outs[i] = outlet_new(&x->x_obj, &s_bang);
        
    }
    if(x->x_coin)
        floatinlet_new(&x->x_obj, &x->x_chance);
    return(x);
errstate:
    pd_error(x, "[chance]: improper args");
    return(NULL);
}

void chance_setup(void){
    chance_class = class_new(gensym("chance"), (t_newmethod)chance_new,
        0, sizeof(t_chance), 0, A_GIMME, 0);
    class_addmethod(chance_class, (t_method)chance_seed, gensym("seed"), A_GIMME, 0);
    class_addlist(chance_class, chance_list);
}
