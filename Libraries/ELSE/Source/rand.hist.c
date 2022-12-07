// porres 2022

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <random.h>

typedef struct _rand_hist{
    t_object       x_obj;
    int            x_size;       // histogram size
    int            x_n;          // number of values with a probability weigth
    int           *x_probs;      // array with probability weigths
    int           *x_ovalues;    // array with number of times a value was output
    int           *x_candidates; // array of candidates
    int            x_id;
    int            x_u_mode;
    t_random_state x_rstate;
    t_outlet      *x_bang_outlet;
}t_rand_hist;

static t_class *rand_hist_class;

static void rand_hist_seed(t_rand_hist *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, x->x_id));
}

static void update_candidates(t_rand_hist *x){ // update candidates
    x->x_n = 0; // number of eligible values
    for(int i = 0; i < x->x_size; i++) // get number of eligible values
        x->x_n += (*(x->x_probs+i) - *(x->x_ovalues+i));
    if(!x->x_n)
        return;
    x->x_candidates = (int*)getbytes(x->x_n*sizeof(int)); // init candidates array
    for(int i = 0, n = 0; i < x->x_size; i++){ // populate candidates
        int dif = *(x->x_probs+i) - *(x->x_ovalues+i);
        while(dif--)
            *(x->x_candidates+n++) = i;
    }
}

static void rand_hist_bang(t_rand_hist *x){
    if(x->x_u_mode)
        update_candidates(x);
    if(!x->x_n){
        post("[rand.hist]: probabilities are null");
        return;
    }
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    t_float noise = (t_float)(random_frand(s1, s2, s3)) * 0.5 + 0.5;
    int n = (int)(noise * x->x_n);
    if(n >= x->x_n)
        n = x->x_n-1;
    int v = *(x->x_candidates+n);
    outlet_float(x->x_obj.ob_outlet, v);
    if(x->x_u_mode){
        *(x->x_ovalues+v) += 1;
        if(x->x_n == 1){ // end
            outlet_bang(x->x_bang_outlet);
            memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
        }
    }
}

static void rand_hist_size(t_rand_hist *x, t_float f){ // set histogram size
    int n = f < 1 ? 1 : (int)f;
    if(n != x->x_size){
        x->x_probs = (int*)getbytes((x->x_size=n)*sizeof(int));
        x->x_ovalues = (int*)getbytes((x->x_size=n)*sizeof(int));
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
        for(int i = 0; i < x->x_size; i++) // ????
            *(x->x_probs+i) = 0;
        if(!x->x_u_mode)
            update_candidates(x);
        else
            memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
    }
}

static void rand_hist_list(t_rand_hist *x, t_symbol*s, int ac, t_atom *av){
    s = NULL;
    if(!ac)
        rand_hist_bang(x);
    else{
        x->x_size = ac;
        x->x_probs = (int*)getbytes(x->x_size*sizeof(int));
        x->x_ovalues = (int*)getbytes(x->x_size*sizeof(int));
        for(int i = 0; i < x->x_size; i++){
            int v = (int)av[i].a_w.w_float;
            *(x->x_probs + i) = v < 0 ? 0 : v;
        }
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
        if(!x->x_u_mode)
            update_candidates(x);
    }
}

static void rand_hist_unrepeat(t_rand_hist *x, t_float f){
    int mode = (int)(f != 0);
    if(x->x_u_mode != mode){
        x->x_u_mode = mode;
        if(x->x_u_mode)
            memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
        else
            update_candidates(x);
    }
}

static void rand_hist_set(t_rand_hist *x, t_float f, t_float v){
    int i = (int)f;
    if(i < 0 || i >= x->x_size){
        post("[rand.hist]: %d not available", i);
        return;
    }
    *(x->x_probs+i) = v < 0 ? 0 : (int)v;
    if(!x->x_u_mode)
        update_candidates(x);
    else
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
}

static void rand_hist_inc(t_rand_hist *x, t_float f){
    int v = (int)f;
    if(v < 0 || v >= x->x_size){
        post("[rand.hist]: %d not available", v);
        return;
    }
    *(x->x_probs+v) += 1;
    if(!x->x_u_mode)
        update_candidates(x);
    else
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
}

static void rand_hist_dec(t_rand_hist *x, t_float f){
    int v = (int)f;
    if(v < 0 || v >= x->x_size){
        post("[rand.hist]: %d not available", v);
        return;
    }
    if(*(x->x_probs+v))
        *(x->x_probs+v) -= 1;
    if(!x->x_u_mode)
        update_candidates(x);
    else
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
}

static void rand_hist_eq(t_rand_hist *x, t_float f){
    int v = f < 0 ? 0 : (int)f;
    for(int i = 0; i < x->x_size; i++)
        *(x->x_probs+i) = v;
    if(!x->x_u_mode)
        update_candidates(x);
    else
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
}

static void rand_hist_clear(t_rand_hist *x){
    for(int i = 0; i < x->x_size; i++)
        *(x->x_probs+i) = 0;
    if(!x->x_u_mode)
        update_candidates(x);
    else
        memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
}

static void rand_hist_restart(t_rand_hist *x){
    memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
}

static t_rand_hist *rand_hist_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_rand_hist *x = (t_rand_hist *)pd_new(rand_hist_class);
    x->x_id = random_get_id();
    rand_hist_seed(x, s, 0, NULL);
    x->x_size = 128;
    x->x_u_mode = 0;
    int eq = 0; // default value
    while(ac && av[0].a_type == A_SYMBOL){
        if(av[0].a_w.w_symbol == gensym("-seed")){
            if(ac >= 2){
                if(av[1].a_type == A_FLOAT){
                    t_atom at[1];
                    SETFLOAT(at, atom_getfloat(av+1));
                    ac-=2, av+=2;
                    rand_hist_seed(x, s, 1, at);
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else if(av[0].a_w.w_symbol == gensym("-size")){
            if(ac >= 2){
                if(av[1].a_type == A_FLOAT){
                    x->x_size = av[1].a_w.w_float;
                    ac-=2, av+=2;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else if(av[0].a_w.w_symbol == gensym("-eq")){
            if(ac >= 2){
                if(av[1].a_type == A_FLOAT){
                    eq = av[1].a_w.w_float;
                    ac-=2, av+=2;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
        else if(av[0].a_w.w_symbol == gensym("-u")){
            x->x_u_mode = 1;
            ac--, av++;
        }
        else
            goto errstate;
    }
    if(ac && av[0].a_type == A_FLOAT)
        x->x_size = ac;
    x->x_probs = (int*)getbytes(x->x_size*sizeof(int));
    x->x_ovalues = (int*)getbytes(x->x_size*sizeof(int));
    memset(x->x_ovalues, 0x0, x->x_size*sizeof(int));
    for(int i = 0; i < x->x_size; i++){
        if(ac){
            *(x->x_probs+i) = (int)av[i].a_w.w_float;
            ac--;
        }
        else
            *(x->x_probs+i) = eq;
    }
    if(!x->x_u_mode)
        update_candidates(x);
    outlet_new(&x->x_obj, &s_float);
    x->x_bang_outlet = outlet_new(&x->x_obj, &s_bang);
    return(x);
errstate:
    post("[rand.hist] improper args");
    return(NULL);
}

static void rand_hist_free(t_rand_hist *x){
    freebytes(x->x_probs, x->x_size*sizeof(int));
    freebytes(x->x_ovalues, x->x_size*sizeof(int));
    freebytes(x->x_candidates, x->x_n*sizeof(int));
}

void setup_rand0x2ehist(void){
    rand_hist_class = class_new(gensym("rand.hist"), (t_newmethod)rand_hist_new,
        (t_method)rand_hist_free, sizeof(t_rand_hist), 0, A_GIMME, 0);
    class_addlist(rand_hist_class, rand_hist_list);
    class_addmethod(rand_hist_class, (t_method)rand_hist_unrepeat, gensym("unrepeat"), A_FLOAT, 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_eq, gensym("eq"), A_FLOAT, 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_clear, gensym("clear"), 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_inc, gensym("inc"), A_FLOAT, 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_dec, gensym("dec"), A_FLOAT, 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_seed, gensym("seed"), A_GIMME, 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_set, gensym("set"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(rand_hist_class, (t_method)rand_hist_restart, gensym("restart"), 0);
}
