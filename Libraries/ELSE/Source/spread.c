// porres 2020

#include "m_pd.h"

static t_class *spread_class;

typedef struct _spread{
    t_object    x_obj;
    t_atom     *x_av;
    int         x_ac;
    int         x_bytes;
    float       x_f;
    t_outlet  **x_outs;
    t_outlet   *x_out_unmatch;
}t_spread;

static void spread_float(t_spread *x, t_floatarg f){
    for(int n = 0; n < x->x_ac; n++){
        if(f < x->x_av[n].a_w.w_float){
            outlet_float(x->x_outs[n], f);
            return;
        }
    }
    outlet_float(x->x_out_unmatch, f);
}

static void *spread_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_spread *x = (t_spread *)pd_new(spread_class);
    if(!argc){
        x->x_ac = 1;
        x->x_bytes = x->x_ac*sizeof(t_atom);
        x->x_av = (t_atom *)getbytes(x->x_bytes);
        SETFLOAT(x->x_av, 0);
    }
    else{
        x->x_ac = argc;
        x->x_bytes = x->x_ac*sizeof(t_atom);
        x->x_av = (t_atom *)getbytes(x->x_bytes);
        int n = 0;
        while(argc > 0){
            if(argv->a_type == A_FLOAT){
                t_float argval = atom_getfloatarg(0, argc, argv);
                SETFLOAT(x->x_av+n, argval);
            }
            else if(argv->a_type == A_SYMBOL){
                pd_error(x, "[spread]: takes only floats as arguments");
                return (NULL);
            }
            n++;
            argv++;
            argc--;
        }
    }
    t_outlet **outs;
    x->x_outs = (t_outlet **)getbytes(x->x_ac * sizeof(*outs));
    for(int i = 0; i < x->x_ac; i++)
        x->x_outs[i] = outlet_new(&x->x_obj, &s_anything);
    x->x_out_unmatch = outlet_new(&x->x_obj, &s_anything);
    return(x);
}

void spread_setup(void){
    spread_class = class_new(gensym("spread"), (t_newmethod)spread_new,
        0, sizeof(t_spread), 0, A_GIMME, 0);
    class_addfloat(spread_class, spread_float);
}
