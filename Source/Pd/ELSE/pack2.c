#include <m_pd.h>

static t_class *pack2_class;
static t_class* pack2_inlet_class;

struct _pack2_inlet;

typedef struct _pack2{
    t_object            x_obj;
    t_int               x_n;
    t_atom*             x_vec;
    t_atom*             x_out;
    struct _pack2_inlet*  x_ins;
} t_pack2;

typedef struct _pack2_inlet{
    t_class*    x_pd;
    t_atom*     x_atoms;
    t_int       x_max;
    t_pack2*    x_owner;
    int         x_idx; //index of inlet
} t_pack2_inlet;

static void pack2_bang(t_pack2 *x){
    int i;
    for(i = 0; i < x->x_n; ++i)
        x->x_out[i] = x->x_vec[i];
    if(x->x_out[0].a_type == A_FLOAT)
        outlet_anything(x->x_obj.ob_outlet, &s_list, x->x_n, x->x_out);
    else{
        int n = x->x_n - 1;
        outlet_anything(x->x_obj.ob_outlet, atom_getsymbol(x->x_out), n, x->x_out + 1);
    }
}

static void pack2_copy(t_pack2 *x, int ndest, t_atom* dest, int nsrc, t_atom* src, int srcidx){
    int i, isint;
    for(i = 0; i < ndest && i < nsrc; ++i){
        if(src[i].a_type == A_FLOAT){
            if(dest[i].a_type == A_FLOAT)
                dest[i].a_w.w_float = src[i].a_w.w_float;
            else if(dest[i].a_type == A_SYMBOL){
                dest[i].a_type = A_FLOAT;
                dest[i].a_w.w_float = src[i].a_w.w_float; // symbol becomes float
            }
        }
        else if(src[i].a_type == A_SYMBOL){
            if(dest[i].a_type == A_SYMBOL)
                dest[i].a_w.w_symbol = src[i].a_w.w_symbol;
            else if(dest[i].a_type == A_FLOAT){
                dest[i].a_type = A_SYMBOL;
                dest[i].a_w.w_symbol = src[i].a_w.w_symbol; // float becomes symbol
            }
        }
    }
}


static void pack2_inlet_bang(t_pack2_inlet *x){
    pack2_bang(x->x_owner);
}

static void pack2_inlet_float(t_pack2_inlet *x, float f){
    if(x->x_atoms->a_type == A_FLOAT){
        x->x_atoms->a_w.w_float = f;
        pack2_bang(x->x_owner);
    }
    else if(x->x_atoms->a_type == A_SYMBOL){
        x->x_atoms->a_type      = A_FLOAT; // symbol becomes float
        x->x_atoms->a_w.w_float = f;
        pack2_bang(x->x_owner);
    }
}

static void pack2_inlet_symbol(t_pack2_inlet *x, t_symbol* s){
    if(x->x_atoms->a_type == A_SYMBOL){
        x->x_atoms->a_w.w_symbol = s;
        pack2_bang(x->x_owner);
    }
    else if(x->x_atoms->a_type == A_FLOAT){
        x->x_atoms->a_type      = A_SYMBOL; // float becomes symbol
        x->x_atoms->a_w.w_symbol = s;
        pack2_bang(x->x_owner);
    }
}

static void pack2_inlet_list(t_pack2_inlet *x, t_symbol* s, int argc, t_atom* argv){
    pack2_copy(x->x_owner, x->x_max, x->x_atoms, argc, argv, x->x_idx);
    pack2_bang(x->x_owner);
}

static void pack2_inlet_anything(t_pack2_inlet *x, t_symbol* s, int argc, t_atom* argv){
    if(x->x_atoms[0].a_type == A_SYMBOL)
        x->x_atoms[0].a_w.w_symbol = s;
    else if (x->x_atoms[0].a_type == A_FLOAT){
        x->x_atoms[0].a_type = A_SYMBOL;
        x->x_atoms[0].a_w.w_symbol = s; // float becomes symbol
    }
    pack2_copy(x->x_owner, x->x_max-1, x->x_atoms+1, argc, argv, x->x_idx);
    pack2_bang(x->x_owner);
}

static void pack2_inlet_set(t_pack2_inlet *x, t_symbol* s, int argc, t_atom* argv){
    pack2_copy(x->x_owner, x->x_max, x->x_atoms, argc, argv, x->x_idx);
}


static void *pack2_new(t_symbol *s, int argc, t_atom *argv){
    int i;
    t_pack2 *x = (t_pack2 *)pd_new(pack2_class);
    t_atom defarg[2];
    if(!argc){
        argv = defarg;
        argc = 2;
        SETFLOAT(&defarg[0], 0);
        SETFLOAT(&defarg[1], 0);
    }
    x->x_n   = argc;
    x->x_vec = (t_atom *)getbytes(argc * sizeof(*x->x_vec));
    x->x_out = (t_atom *)getbytes(argc * sizeof(*x->x_out));
    x->x_ins = (t_pack2_inlet *)getbytes(argc * sizeof(*x->x_ins));
    for(i = 0; i < x->x_n; ++i){
        if(argv[i].a_type == A_FLOAT){
            x->x_vec[i].a_type      = A_FLOAT;
            x->x_vec[i].a_w.w_float = argv[i].a_w.w_float;
            x->x_ins[i].x_pd    = pack2_inlet_class;
            x->x_ins[i].x_atoms = x->x_vec+i;
            x->x_ins[i].x_max   = x->x_n-i;
            x->x_ins[i].x_owner = x;
            x->x_ins[i].x_idx = i;
            inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
        }
        else if(argv[i].a_type == A_SYMBOL){
            if(argv[i].a_w.w_symbol == gensym("f") ||
               argv[i].a_w.w_symbol == gensym("float")){ 
                x->x_vec[i].a_type      = A_FLOAT;
                x->x_vec[i].a_w.w_float = 0.f;
                x->x_ins[i].x_pd    = pack2_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                x->x_ins[i].x_idx = i;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
                
            }
            else{
                x->x_vec[i].a_type       = A_SYMBOL;
                x->x_vec[i].a_w.w_symbol = argv[i].a_w.w_symbol; // loads symbol from arg
                x->x_ins[i].x_pd    = pack2_inlet_class;
                x->x_ins[i].x_atoms = x->x_vec+i;
                x->x_ins[i].x_max   = x->x_n-i;
                x->x_ins[i].x_owner = x;
                x->x_ins[i].x_idx = i;
                inlet_new((t_object *)x, &(x->x_ins[i].x_pd), 0, 0);
            }
        }
    }
    outlet_new(&x->x_obj, &s_list);
    return (x);
}

static void pack2_free(t_pack2 *x){
    freebytes(x->x_vec, x->x_n * sizeof(*x->x_vec));
    freebytes(x->x_out, x->x_n * sizeof(*x->x_out));
    freebytes(x->x_ins, x->x_n * sizeof(*x->x_ins));
}

extern void pack2_setup(void){
    t_class* c = NULL;
    c = class_new(gensym("pack2-inlet"), 0, 0, sizeof(t_pack2_inlet), CLASS_PD, 0);
    if(c){
        class_addbang(c,    (t_method)pack2_inlet_bang);
        class_addfloat(c,   (t_method)pack2_inlet_float);
        class_addsymbol(c,  (t_method)pack2_inlet_symbol);
        class_addlist(c,    (t_method)pack2_inlet_list);
        class_addanything(c,(t_method)pack2_inlet_anything);
        class_addmethod(c,  (t_method)pack2_inlet_set, gensym("set"), A_GIMME, 0);
    }
    pack2_inlet_class = c;
    c = class_new(gensym("pack2"), (t_newmethod)pack2_new, (t_method)pack2_free,
            sizeof(t_pack2), CLASS_NOINLET, A_GIMME, 0);
    pack2_class = c;
}
