#include "m_pd.h"

static t_class *routetype_class;

typedef struct _routetype{
    t_object     x_obj;
    int          x_b;
    int          x_f;
    int          x_s;
    int          x_l;
    int          x_a;
    int          x_p;
    int          x_r;
    t_outlet    *x_out_bang;
    t_outlet    *x_out_float;
    t_outlet    *x_out_symbol;
    t_outlet    *x_out_list;
    t_outlet    *x_out_anything;
    t_outlet    *x_out_pointer;
    t_outlet    *x_out_reject;
} t_routetype;

static void routetype_bang(t_routetype *x){
    if(x->x_b)
        outlet_bang(x->x_out_bang);
    else if(x->x_r)
        outlet_bang(x->x_out_reject);
}

static void routetype_float(t_routetype *x, t_floatarg f){
    if(x->x_f)
        outlet_float(x->x_out_float, f);
    else if(x->x_r)
        outlet_float(x->x_out_reject, f);
}

static void routetype_symbol(t_routetype *x, t_symbol *s){
    if(x->x_s)
        outlet_symbol(x->x_out_symbol, s);
    else if(x->x_r)
        outlet_symbol(x->x_out_reject, s);
}

static void routetype_list(t_routetype *x, t_symbol *sel, int argc, t_atom *argv){
    t_symbol *dummy = sel;
    dummy = NULL;
    if(argc == 0){
        routetype_bang(x);
        return;
    }
    if(argc == 1 && argv[0].a_type == A_SYMBOL){
        routetype_symbol(x, atom_getsymbol(argv));
        return;
    }
    if(argc == 1 && argv[0].a_type == A_FLOAT){
        routetype_float(x, atom_getfloat(argv));
        return;
    }
    if(x->x_l)
        outlet_list(x->x_out_list, gensym("list"), argc, argv);
    else if(x->x_r)
        outlet_list(x->x_out_reject, gensym("list"), argc, argv);
}

static void routetype_anything(t_routetype *x, t_symbol *sel, int argc, t_atom *argv){
    if(x->x_a)
        outlet_anything(x->x_out_anything, sel, argc, argv);
    else if(x->x_r)
        outlet_anything(x->x_out_reject, sel, argc, argv);
}

static void routetype_pointer(t_routetype *x, t_gpointer *gp){
    if(x->x_p)
        outlet_pointer(x->x_out_pointer, gp);
    else if(x->x_r)
        outlet_pointer(x->x_out_pointer, gp);
}

static void *routetype_new(t_symbol *s, int argc, t_atom *argv){
    t_routetype *x = (t_routetype *)pd_new(routetype_class);
    t_symbol *dummy = s;
    dummy = NULL;
    x->x_b = x->x_f = x->x_s = x->x_l = x->x_a = x->x_r = 0;
    int c = argc;
    if(!argc){
        x->x_out_reject = outlet_new(&x->x_obj, &s_anything);
        x->x_r = 1;
    }
    else{
        while(argc > 0){
            if(argv->a_type == A_SYMBOL){
                t_symbol *curarg = atom_getsymbolarg(0, argc, argv);
                if(curarg == gensym("f") || curarg == gensym("float")){
                    x->x_out_float = outlet_new(&x->x_obj, &s_float);
                    x->x_f = 1;
                }
                else if(curarg == gensym("b") || curarg == gensym("bang")){
                    x->x_out_bang = outlet_new(&x->x_obj, &s_bang);
                    x->x_b = 1;
                }
                else if(curarg == gensym("s") || curarg == gensym("symbol")){
                    x->x_out_symbol = outlet_new(&x->x_obj, &s_symbol);
                    x->x_s = 1;
                }
                else if(curarg == gensym("l") || curarg == gensym("list")){
                    x->x_out_list = outlet_new(&x->x_obj, &s_list);
                    x->x_l = 1;
                }
                else if(curarg == gensym("a") || curarg == gensym("anything")){
                    x->x_out_anything = outlet_new(&x->x_obj, &s_anything);
                    x->x_a = 1;
                }
                else if(curarg == gensym("p") || curarg == gensym("pointer")){
                    x->x_out_pointer = outlet_new(&x->x_obj, &s_pointer);
                    x->x_p = 1;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
            argv++;
            argc--;
        }
        if(c < 6){
            x->x_out_reject = outlet_new(&x->x_obj, &s_anything);
            x->x_r = 1;
        }
    }
    return(x);
errstate:
    pd_error(x, "routetype: improper args");
    return NULL;
}

void routetype_setup(void){
    routetype_class = class_new(gensym("routetype"), (t_newmethod)routetype_new,
                                0, sizeof(t_routetype), 0, A_GIMME, 0);
    class_addbang(routetype_class, routetype_bang);
    class_addfloat(routetype_class, routetype_float);
    class_addsymbol(routetype_class, routetype_symbol);
    class_addlist(routetype_class, routetype_list);
    class_addanything(routetype_class, routetype_anything);
    class_addpointer(routetype_class, routetype_pointer);
}
