// porres

#include "m_pd.h"
#include <stdlib.h>
#include <string.h>
#include <random.h>

typedef struct _rand_seq{
    t_object        x_obj;
    int             x_nvalues;  // number of values
    int            *x_probs;    // probability of a value
    int            *x_ovalues;  // number of outputs of each value
    t_random_state  x_rstate;
    t_outlet       *x_bang_outlet;
}t_rand_seq;

static unsigned int instanc_n = 0;

static t_class *rand_seq_class;

static void rand_seq_n(t_rand_seq *x, t_float f){
    int n = (int)f;
    if(n < 1)
        n = 1;
    if(n != x->x_nvalues){
        x->x_nvalues = n;
        x->x_probs = (int*) getbytes(x->x_nvalues*sizeof(int));
        if(!x->x_probs){
            pd_error(x, "[rand.seq]: could not allocate buffer");
            return;
        }
        x->x_ovalues = (int*) getbytes(x->x_nvalues*sizeof(int));
        if(!x->x_ovalues){
            pd_error(x, "[rand.seq]: could not allocate buffer");
            return;
        }
        memset(x->x_ovalues, 0x0, x->x_nvalues*sizeof(int));
        int eq = 1; // default value
        for(int i = 0; i < x->x_nvalues; i++)
            *(x->x_probs+i) = eq;
    }
}

static void rand_seq_seed(t_rand_seq *x, t_symbol *s, int ac, t_atom *av){
    random_init(&x->x_rstate, get_seed(s, ac, av, ++instanc_n));
}

static void rand_seq_bang(t_rand_seq *x){
    int *candidates;
    int ei, ci, nbcandidates = 0, nevalues = 0;
    for(ei = 0; ei < x->x_nvalues; ei++) // get number of eligible values
        nevalues += (*(x->x_probs+ei) - *(x->x_ovalues+ei));
    if(nevalues == 0){
        post("[rand.seq]: probabilities are null");
        outlet_bang(x->x_bang_outlet);
        return;
    }
    candidates = (int*)getbytes(nevalues*sizeof(int));
    for(ei = 0; ei < x->x_nvalues; ei++){ // select eligible values
        if(*(x->x_ovalues+ei) < *(x->x_probs+ei)){
            for(ci = 0; ci < *(x->x_probs+ei) - *(x->x_ovalues+ei); ci++){
                *(candidates+nbcandidates) = ei;
                nbcandidates++;
            }
        }
    }
    
//    x->x_state = x->x_state * 472940017 + 832416023;
//    int nval = ((double)nbcandidates) * ((double)x->x_state) * (1./4294967296.);
    
    uint32_t *s1 = &x->x_rstate.s1;
    uint32_t *s2 = &x->x_rstate.s2;
    uint32_t *s3 = &x->x_rstate.s3;
    t_float noise = (t_float)(random_frand(s1, s2, s3)) * 0.5 + 0.5;
    int nval = (int)(noise * nbcandidates);
    
    if(nval >= nbcandidates)
        nval = nbcandidates-1;
    int v = *(candidates+nval);
    outlet_float(x->x_obj.ob_outlet, v);
    *(x->x_ovalues+v) = *(x->x_ovalues+v) + 1;
    if(nbcandidates == 1){ // end of the serial
        outlet_bang(x->x_bang_outlet);
        memset(x->x_ovalues, 0x0, x->x_nvalues*sizeof(int));
    }
    if(candidates)
        freebytes(candidates, nevalues*sizeof(int));
}

static void rand_seq_list(t_rand_seq *x, t_symbol*s, int ac, t_atom *av){
    s = NULL;
    if(!ac)
        rand_seq_bang(x);
    else{
        if(ac > x->x_nvalues)
            ac = x->x_nvalues;
        for(int i = 0; i < ac; i++){
            int v = (int)av[i].a_w.w_float;
            *(x->x_probs + i) = v < 0 ? 0 : v;
        }
    }
}

static void rand_seq_set(t_rand_seq *x, t_float f, t_float v){
    int i = (int)f;
    if(i < 0 || i >= x->x_nvalues){
        post("[rand.seq]: %d not available", i);
        return;
    }
    *(x->x_probs+i) = v < 0 ? 0 : (int)v;
}

static void rand_seq_inc(t_rand_seq *x, t_float f){
    int v = (int)f;
    if(v < 0 || v >= x->x_nvalues){
        post("[rand.seq]: %d not available", v);
        return;
    }
    *(x->x_probs+v) += 1;
}

static void rand_seq_restart(t_rand_seq *x){
    memset(x->x_ovalues, 0x0, x->x_nvalues*sizeof(int));
}

static void rand_seq_eq(t_rand_seq *x, t_float f){
    int v = f < 1 ? 1 : (int)f;
    for(int ei = 0; ei < x->x_nvalues; ei++)
        *(x->x_probs+ei) = v;
    memset(x->x_ovalues, 0x0, x->x_nvalues*sizeof(int));
}

static t_rand_seq *rand_seq_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_rand_seq *x = (t_rand_seq *)pd_new(rand_seq_class);
    x->x_nvalues = 1;
    rand_seq_seed(x, s, 0, NULL);
    if(ac){
        if(av[0].a_type == A_SYMBOL){
            t_symbol * sym = av[0].a_w.w_symbol;
            if(sym == gensym("-seed")){
                if(ac >= 2){
                    if(av[1].a_type == A_FLOAT){
                        t_atom at[1];
                        SETFLOAT(at, atom_getfloat(av+1));
                        ac-=2, av+=2;
                        rand_seq_seed(x, s, 1, at);
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
        x->x_nvalues = av[0].a_w.w_float;
        if(x->x_nvalues < 1)
            x->x_nvalues = 1;
        ac--, av++;
        if(ac > x->x_nvalues) // ignore extra args
            ac = x->x_nvalues;
    }
    // common fields for new and restored rand_seqs
    x->x_probs = (int*)getbytes(x->x_nvalues*sizeof(int));
    x->x_ovalues = (int*)getbytes(x->x_nvalues*sizeof(int));
    memset(x->x_ovalues, 0x0, x->x_nvalues*sizeof(int));
    int eq = 1; // default value
    for(int i = 0; i < x->x_nvalues; i++){
        if(ac){
            *(x->x_probs + i) = (int)av[i].a_w.w_float;
            ac--;
        }
        else
            *(x->x_probs+i) = eq;
    }
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("n"));
    outlet_new(&x->x_obj, &s_float);
    x->x_bang_outlet = outlet_new(&x->x_obj, &s_bang);
    return(x);
errstate:
    post("[rand.seq] improper args");
    return(NULL);
}

static void rand_seq_free(t_rand_seq *x){
    if(x->x_probs)
        freebytes(x->x_probs, x->x_nvalues*sizeof(int));
    if(x->x_ovalues)
        freebytes(x->x_ovalues, x->x_nvalues*sizeof(int));
}

void setup_rand0x2eseq(void){
    rand_seq_class = class_new(gensym("rand.seq"), (t_newmethod)rand_seq_new,
        (t_method)rand_seq_free, sizeof(t_rand_seq), 0, A_GIMME, 0);
    class_addlist(rand_seq_class, rand_seq_list);
    class_addmethod(rand_seq_class, (t_method)rand_seq_eq, gensym("eq"), A_FLOAT, 0);
    class_addmethod(rand_seq_class, (t_method)rand_seq_inc, gensym("inc"), A_FLOAT, 0);
    class_addmethod(rand_seq_class, (t_method)rand_seq_n, gensym("n"), A_FLOAT, 0);
    class_addmethod(rand_seq_class, (t_method)rand_seq_seed, gensym("seed"), A_GIMME, 0);
    class_addmethod(rand_seq_class, (t_method)rand_seq_set, gensym("set"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(rand_seq_class, (t_method)rand_seq_restart, gensym("restart"), 0);
}
