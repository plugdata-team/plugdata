// porres 2020

#include "m_pd.h"

static t_class *chance_class;

typedef struct _chance{
    t_object    x_obj;
    t_atom     *x_av;
    t_int       x_val;
    int         x_index;
    int         x_ac;
    int         x_bytes;
    int         x_coin;
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
                x->x_index ? outlet_float(x->x_out_index, n+1) : outlet_bang(x->x_outs[n]);
                return;
            }
        }
    }
}

static void chance_list(t_chance *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if(!x->x_coin){
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

static void chance_bang(t_chance *x){
    int val = x->x_val;
    t_float random = ((float)((val & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
    chance_output(x, x->x_range * (random + 1) / 2);
    x->x_val = val * 435898247 + 382842987;
}

static void chance_seed(t_chance *x, t_float f){
    x->x_val = (int)f * 1319;
}

static void *chance_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_chance *x = (t_chance *)pd_new(chance_class);
    static int init_seed = 54569;
    x->x_range = 0;
    x->x_index = 0;
    x->x_coin = 0;
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
            return (NULL);
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
            }
            else if(argv->a_type == A_SYMBOL){
                if(atom_getsymbolarg(0, argc, argv) == gensym("-index")){
                    x->x_index = 1;
                    x->x_ac--;
                    n--;
                }
            }
            n++;
            argv++;
            argc--;
        }
        if(!x->x_index){
            for(int i = 0; i < x->x_ac; i++)
                x->x_outs[i] = outlet_new(&x->x_obj, &s_bang);
        }
    }
    x->x_val = init_seed *= 1319; // load seed value
    if(x->x_coin)
        floatinlet_new(&x->x_obj, &x->x_chance);
    if(x->x_index)
        x->x_out_index = outlet_new(&x->x_obj, &s_float);
    return(x);
}

void chance_setup(void){
    chance_class = class_new(gensym("chance"), (t_newmethod)chance_new,
        0, sizeof(t_chance), 0, A_GIMME, 0);
    class_addmethod(chance_class, (t_method)chance_seed, gensym("seed"), A_DEFFLOAT, 0);
    class_addbang(chance_class, chance_bang);
    class_addlist(chance_class, chance_list);
}
