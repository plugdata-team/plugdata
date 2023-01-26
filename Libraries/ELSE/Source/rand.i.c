// porres

#include "m_pd.h"
#include "random.h"

static t_class *randi_class;

typedef struct _randi{
    t_object       x_obj;
    t_random_state x_rstate;
    t_float        x_min;
    t_float        x_max;
    int            x_id;
}t_randi;

static void randi_seed(t_randi *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static void randi_bang(t_randi *x){
    int min = (int)x->x_min, max = (int)x->x_max;
    if(min > max){
        int temp = min;
        min = max;
        max = temp;
    }
    int range = (max - min);
    int random = min;
    if(range){
        uint32_t *s1 = &x->x_rstate.s1;
        uint32_t *s2 = &x->x_rstate.s2;
        uint32_t *s3 = &x->x_rstate.s3;
        t_float noise = (t_float)(random_frand(s1, s2, s3)) * 0.5 + 0.5;
        random = (int)((noise * (range + 1))) + min;
    }
    outlet_float(x->x_obj.ob_outlet, random);
}

static void *randi_new(t_symbol *s, int ac, t_atom *av){
    t_randi *x = (t_randi *)pd_new(randi_class);
    x->x_id = random_get_id();
    randi_seed(x, s, 0, NULL);
    x->x_min = 0;
    x->x_max = 1;
    while(ac){
        if(av->a_type == A_SYMBOL){
            if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
                t_atom at[1];
                SETFLOAT(at, atom_getfloat(av+1));
                ac-=2, av+=2;
                randi_seed(x, s, 1, at);
            }
            else
                goto errstate;
        }
        if(ac && av->a_type == A_FLOAT){
            x->x_min = atom_getintarg(0, ac, av);
            ac--, av++;
            if(ac && av->a_type == A_FLOAT){
                x->x_max = atom_getintarg(0, ac, av);
                ac--, av++;
            }
        }
    }
    floatinlet_new((t_object *)x, &x->x_min);
    floatinlet_new((t_object *)x, &x->x_max);
    outlet_new(&x->x_obj, &s_float);
    return(x);
errstate:
    pd_error(x, "[rand.i]: improper args");
    return(NULL);
}

void setup_rand0x2ei(void){
    randi_class = class_new(gensym("rand.i"), (t_newmethod)randi_new, 0,
        sizeof(t_randi), 0, A_GIMME, 0);
    class_addbang(randi_class, (t_method)randi_bang);
    class_addmethod(randi_class, (t_method)randi_seed, gensym("seed"), A_GIMME, 0);
}
