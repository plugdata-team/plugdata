// porres

#include "m_pd.h"
#include "random.h"

static t_class *randf_class;

typedef struct _randf{
    t_object        x_obj;
    t_random_state  x_rstate;
    t_float         x_min;
    t_float         x_max;
    int             x_id;
}t_randf;

static void randf_seed(t_randf *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static void randf_bang(t_randf *x){
    float min = x->x_min; // Output LOW
    float max = x->x_max; // Output HIGH
    if(min > max){
        int temp = min;
        min = max;
        max = temp;
    }
    float range = max - min; // range
    float random = min;
    if(range != 0){
        uint32_t *s1 = &x->x_rstate.s1;
        uint32_t *s2 = &x->x_rstate.s2;
        uint32_t *s3 = &x->x_rstate.s3;
        t_float noise = (t_float)(random_frand(s1, s2, s3)) * 0.5 + 0.5;
        random = noise * range + min;
    }
    outlet_float(x->x_obj.ob_outlet, random);
}

static void *randf_new(t_symbol *s, int ac, t_atom *av){
    t_randf *x = (t_randf *)pd_new(randf_class);
    x->x_id = random_get_id();
    randf_seed(x, s, 0, NULL);
    x->x_min = 0;
    x->x_max = 1;
    while(ac){
        if(av->a_type == A_SYMBOL){
            if(ac >= 2 && atom_getsymbol(av) == gensym("-seed")){
                t_atom at[1];
                SETFLOAT(at, atom_getfloat(av+1));
                ac-=2, av+=2;
                randf_seed(x, s, 1, at);
            }
            else
                goto errstate;
        }
        if(ac && av->a_type == A_FLOAT){
            x->x_min = atom_getfloat(av);
            ac--, av++;
            if(ac && av->a_type == A_FLOAT){
                x->x_max = atom_getfloat(av);
                ac--, av++;
            }
        }
    }
    floatinlet_new((t_object *)x, &x->x_min);
    floatinlet_new((t_object *)x, &x->x_max);
    outlet_new(&x->x_obj, &s_float);
    return(x);
errstate:
    pd_error(x, "[rand.f]: improper args");
    return(NULL);
}

void setup_rand0x2ef(void){
    randf_class = class_new(gensym("rand.f"), (t_newmethod)randf_new, 0,
        sizeof(t_randf), 0, A_GIMME, 0);
    class_addbang(randf_class, (t_method)randf_bang);
    class_addmethod(randf_class, (t_method)randf_seed, gensym("seed"), A_GIMME, 0);
}
