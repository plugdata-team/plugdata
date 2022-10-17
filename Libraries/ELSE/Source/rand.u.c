// porres 2022

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <random.h>

typedef struct _rand_u{
    t_object       x_obj;
    int            x_size;       // size
    int            x_n;          // number of values with a probability weigth
    int           *x_ovalues;    // array with number of times a value was output
    int           *x_candidates; // array of candidates
    int            x_id;
    t_random_state x_rstate;
    t_outlet      *x_bang_outlet;
}t_rand_u;

static t_class *rand_u_class;

static void rand_u_seed(t_rand_u *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static void rand_u_bang(t_rand_u *x){
    x->x_n = 0; // number of eligible values
    for(int i = 0; i < x->x_size; i++) // get number of eligible values
        x->x_n += (1 - *(x->x_ovalues+i));
    x->x_candidates = (int*)getbytes(x->x_n*sizeof(int)); // init candidates array
    for(int i = 0, n = 0; i < x->x_size; i++){ // populate candidates
        if(1 - *(x->x_ovalues+i))
            *(x->x_candidates+n++) = i;
    }
    uint32_t *s1 = &x->x_rstate.s1, *s2 = &x->x_rstate.s2, *s3 = &x->x_rstate.s3;
    t_float noise = (t_float)(random_frand(s1, s2, s3)) * 0.5 + 0.5;
    int n = (int)(noise * x->x_n);
    if(n >= x->x_n)
        n = x->x_n-1;
    int v = *(x->x_candidates+n);
    outlet_float(x->x_obj.ob_outlet, v);
    *(x->x_ovalues+v) += 1;
    if(x->x_n == 1){ // end
        outlet_bang(x->x_bang_outlet);
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
    }
}

static void rand_u_size(t_rand_u *x, t_float f){ // set uogram size
    int n = f < 1 ? 1 : (int)f;
    if(n != x->x_size){
        x->x_ovalues = (int*)getbytes((x->x_size=n)*sizeof(int));
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
    }
}

static void rand_u_clear(t_rand_u *x){
    memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
}

static t_rand_u *rand_u_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_rand_u *x = (t_rand_u *)pd_new(rand_u_class);
    x->x_id = random_get_id();
    rand_u_seed(x, s, 0, NULL);
    x->x_size = 1;
    while(ac && av[0].a_type == A_SYMBOL){
        if(av[0].a_w.w_symbol == gensym("-seed")){
            if(ac >= 2){
                if(av[1].a_type == A_FLOAT){
                    t_atom at[1];
                    SETFLOAT(at, atom_getfloat(av+1));
                    ac-=2, av+=2;
                    rand_u_seed(x, s, 1, at);
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else
            goto errstate;
    }
    if(ac && av[0].a_type == A_FLOAT){
        int size = (int)atom_getfloat(av);
        x->x_size = size < 1 ? 1 : size;
    }
    x->x_ovalues = (int*)getbytes(x->x_size*sizeof(int));
    memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("size"));
    outlet_new(&x->x_obj, &s_float);
    x->x_bang_outlet = outlet_new(&x->x_obj, &s_bang);
    return(x);
errstate:
    post("[rand.u] improper args");
    return(NULL);
}

static void rand_u_free(t_rand_u *x){
    freebytes(x->x_ovalues, x->x_size*sizeof(int));
    freebytes(x->x_candidates, x->x_n*sizeof(int));
}

void setup_rand0x2eu(void){
    rand_u_class = class_new(gensym("rand.u"), (t_newmethod)rand_u_new,
        (t_method)rand_u_free, sizeof(t_rand_u), 0, A_GIMME, 0);
    class_addbang(rand_u_class, rand_u_bang);
    class_addmethod(rand_u_class, (t_method)rand_u_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(rand_u_class, (t_method)rand_u_seed, gensym("seed"), A_GIMME, 0);
    class_addmethod(rand_u_class, (t_method)rand_u_clear, gensym("clear"), 0);
}
