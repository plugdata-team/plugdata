// porres 2020

#include "m_pd.h"

static t_class *spread_class;

typedef struct _spread{
    t_object   x_obj;
    t_atom    *x_av;
    int        x_ac;
    int        x_bytes;
    int        x_mode;
    float      x_f;
    t_outlet **x_outs;
    t_outlet  *x_out_unmatch;
}t_spread;

static void spread_args(t_spread *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac > x->x_ac)
        ac = x->x_ac;
    int n = 0;
    while(ac > 0){
        if(av->a_type == A_FLOAT){
            t_float aval = atom_getfloatarg(0, ac, av);
            SETFLOAT(x->x_av+n, aval);
        }
        n++, av++, ac--;
    }
}
    
static void spread_mode(t_spread *x, t_floatarg f){
    x->x_mode = (int)(f != 0);
}

static void spread_list(t_spread *x, t_symbol*s, int ac, t_atom *av){
    s = NULL;
    if(!ac || (av)->a_type != A_FLOAT)
        return;
    for(int n = 0; n < x->x_ac; n++){
        if(x->x_mode){
            if(av[0].a_w.w_float <= x->x_av[n].a_w.w_float){
                outlet_list(x->x_outs[n], &s_list, ac, av);
                return;
            }
        }
        else{
            if(av[0].a_w.w_float < x->x_av[n].a_w.w_float){
                outlet_list(x->x_outs[n], &s_list, ac, av);
                return;
            }
        }
    }
    outlet_list(x->x_out_unmatch, &s_list, ac, av);
}

static void *spread_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_spread *x = (t_spread *)pd_new(spread_class);
    x->x_mode = 0;
    if(ac >= 2 && av->a_type == A_SYMBOL){
        t_symbol *sym = atom_getsymbolarg(0, ac, av);
        if(sym == gensym("-mode")){
            x->x_mode = (int)(atom_getfloatarg(1, ac, av) != 0);
            ac-=2, av+=2;
        }
        else
            goto errstate;
    }
    if(!ac){
        x->x_ac = 1;
        x->x_bytes = x->x_ac*sizeof(t_atom);
        x->x_av = (t_atom *)getbytes(x->x_bytes);
        SETFLOAT(x->x_av, 0);
    }
    else{
        x->x_ac = ac;
        x->x_bytes = x->x_ac*sizeof(t_atom);
        x->x_av = (t_atom *)getbytes(x->x_bytes);
        int n = 0;
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                t_float aval = atom_getfloatarg(0, ac, av);
                SETFLOAT(x->x_av+n, aval);
            }
            else if(av->a_type == A_SYMBOL)
                goto errstate;
            n++, av++, ac--;
        }
    }
    t_outlet **outs;
    x->x_outs = (t_outlet **)getbytes(x->x_ac*sizeof(*outs));
    for(int i = 0; i < x->x_ac; i++)
        x->x_outs[i] = outlet_new(&x->x_obj, &s_list);
    x->x_out_unmatch = outlet_new(&x->x_obj, &s_list);
    return(x);
errstate:
    pd_error(x, "[spread]: improper arguments");
    return(NULL);
}

void spread_setup(void){
    spread_class = class_new(gensym("spread"), (t_newmethod)spread_new, 0,
        sizeof(t_spread), 0, A_GIMME, 0);
    class_addlist(spread_class, spread_list);
    class_addmethod(spread_class, (t_method)spread_mode, gensym("mode"), A_FLOAT, 0);
    class_addmethod(spread_class, (t_method)spread_args, gensym("args"), A_GIMME, 0);
}
