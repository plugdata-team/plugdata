// Stolen from Purr Data's [trigger]

#include <m_pd.h>
#include <string.h>

static t_class *t2_class;
#define TR_BANG 0
#define TR_FLOAT 1
#define TR_SYMBOL 2
#define TR_POINTER 3
#define TR_LIST 4
#define TR_ANYTHING 5

#define TR_STATIC_FLOAT 6
#define TR_STATIC_SYMBOL 7

typedef struct t2out{
    int u_type;          /* outlet type from above */
    t_symbol *u_sym;     /* static value */
    t_float u_float;     /* static value */
    t_outlet *u_outlet;
}t_t2out;

typedef struct _t2{
    t_object x_obj;
    t_int x_n;
    t_t2out *x_vec;
}t_t2;

// forward declaration
static void t2_symbol(t_t2 *x, t_symbol *s);

static void t2_list(t_t2 *x, t_symbol *s, int argc, t_atom *argv){
    t_t2out *u;
    int i;
    for(i = x->x_n, u = x->x_vec + i; u--, i--;){
        if (u->u_type == TR_FLOAT)
            outlet_float(u->u_outlet, (argc ? atom_getfloat(argv) : 0));
        else if (u->u_type == TR_BANG)
            outlet_bang(u->u_outlet);
        else if (u->u_type == TR_SYMBOL)
            outlet_symbol(u->u_outlet,
                          (argc ? atom_getsymbol(argv) : (s != NULL ? s : &s_symbol)));
        else if (u->u_type == TR_ANYTHING)
            outlet_anything(u->u_outlet, s, argc, argv);
        else if (u->u_type == TR_POINTER){
            if (!argc || argv->a_type != TR_POINTER)
                pd_error(x, "unpack: bad pointer");
            else outlet_pointer(u->u_outlet, argv->a_w.w_gpointer);
        }
        else if (u->u_type == TR_STATIC_FLOAT)
            outlet_float(u->u_outlet, u->u_float);
        else if (u->u_type == TR_STATIC_SYMBOL)
            outlet_symbol(u->u_outlet, u->u_sym);
        else outlet_list(u->u_outlet, &s_list, argc, argv);
    }
}

static void t2_anything(t_t2 *x, t_symbol *s, int argc, t_atom *argv){
    t_atom *av2 = NULL;
    t_t2out *u;
    int i, j = 0;
    for (i = x->x_n, u = x->x_vec + i; u--, i--;){
        if (u->u_type == TR_BANG)
            outlet_bang(u->u_outlet);
        else if (u->u_type == TR_ANYTHING)
            outlet_anything(u->u_outlet, s, argc, argv);
        else if (u->u_type == TR_STATIC_FLOAT)
            outlet_float(u->u_outlet, u->u_float);
        else if (u->u_type == TR_STATIC_SYMBOL)
            outlet_symbol(u->u_outlet, u->u_sym);
        else{
            // copying t2_list behavior except that here we keep
            // the outlet number and therefore avoid redundant printouts
            if (u->u_type == TR_FLOAT)
                outlet_float(u->u_outlet, (argc ? atom_getfloat(argv) : 0));
            else if (u->u_type == TR_BANG)
                outlet_bang(u->u_outlet);
            else if (u->u_type == TR_SYMBOL)
                outlet_symbol(u->u_outlet,
                              (s != NULL ? s : (argc ? atom_getsymbol(argv) : &s_symbol)));
            else if (u->u_type == TR_ANYTHING)
                outlet_anything(u->u_outlet, s, argc, argv);
            else if (u->u_type == TR_POINTER)
                if (!argc || argv->a_type != TR_POINTER)
                    pd_error(x, "unpack: bad pointer");
                else outlet_pointer(u->u_outlet, argv->a_w.w_gpointer);
            else if (u->u_type == TR_STATIC_FLOAT)
                outlet_float(u->u_outlet, u->u_float);
            else if (u->u_type == TR_STATIC_SYMBOL)
                outlet_symbol(u->u_outlet, u->u_sym);
            else{ // Ico: don't have to worry about zero element case (AFAICT)
                av2 = (t_atom *)getbytes((argc + 1) * sizeof(t_atom));
                SETSYMBOL(av2, s);
                if (argc == 0)
                    outlet_list(u->u_outlet, &s_symbol, argc+1, av2);
                else{
                    for (j = 0; j < argc; j++)
                        av2[j + 1] = argv[j];
                    SETSYMBOL(av2, s);
                    outlet_list(u->u_outlet, &s_list, argc+1, av2);
                }
                freebytes(av2, (argc + 1) * sizeof(t_atom));
            }
        }
    }
}

static void t2_bang(t_t2 *x){
    t2_list(x, &s_bang, 0, 0);
}

static void t2_pointer(t_t2 *x, t_gpointer *gp){
    t_atom at;
    SETPOINTER(&at, gp);
    t2_list(x, &s_list, 1, &at);
}

static void t2_float(t_t2 *x, t_float f){
    t_atom at;
    SETFLOAT(&at, f);
    t2_list(x, &s_float, 1, &at);
}

static void t2_symbol(t_t2 *x, t_symbol *s){
    t_atom at;
    SETSYMBOL(&at, s);
    t2_list(x, &s_symbol, 1, &at);
}

static void t2_free(t_t2 *x){
    freebytes(x->x_vec, x->x_n * sizeof(*x->x_vec));
}

static void *t2_new(t_symbol *s, int argc, t_atom *argv){
    t_t2 *x = (t_t2 *)pd_new(t2_class);
    t_symbol *dummy = s;
    dummy = NULL;
    t_atom defarg[2], *ap;
    t_t2out *u;
    int i;
    if(!argc){
        argv = defarg;
        argc = 2;
        SETSYMBOL(&defarg[0], &s_bang);
        SETSYMBOL(&defarg[1], &s_bang);
    }
    x->x_n = argc;
    x->x_vec = (t_t2out *)getbytes(argc * sizeof(*x->x_vec));
    for(i = 0, ap = argv, u = x->x_vec; i < argc; u++, ap++, i++){
        t_atomtype thistype = ap->a_type;
        char c;
        if (thistype == TR_SYMBOL){
            if (strcmp(ap->a_w.w_symbol->s_name, "anything") == 0
                     || strcmp(ap->a_w.w_symbol->s_name, "a") == 0)
                c = 'a';
            else if (strcmp(ap->a_w.w_symbol->s_name, "bang") == 0
                     || strcmp(ap->a_w.w_symbol->s_name, "b") == 0)
                c = 'b';
            else if (strcmp(ap->a_w.w_symbol->s_name, "float") == 0
                     || strcmp(ap->a_w.w_symbol->s_name, "f") == 0)
                c = 'f';
            else if (strcmp(ap->a_w.w_symbol->s_name, "list") == 0
                     || strcmp(ap->a_w.w_symbol->s_name, "l") == 0)
                c = 'l';
            else if (strcmp(ap->a_w.w_symbol->s_name, "pointer") == 0
                     || strcmp(ap->a_w.w_symbol->s_name, "p") == 0)
                c = 'p';
            else if (strcmp(ap->a_w.w_symbol->s_name, "symbol") == 0
                     || strcmp(ap->a_w.w_symbol->s_name, "s") == 0)
                c = 's';
            else c = 'S';
        }
        else if (thistype == TR_FLOAT)
            c = 'F';
        else c = 0;
        if (c == 'p')
            u->u_type = TR_POINTER,
            u->u_outlet = outlet_new(&x->x_obj, &s_pointer);
        else if (c == 'f')
            u->u_type = TR_FLOAT, u->u_outlet = outlet_new(&x->x_obj, &s_float);
        else if (c == 'b')
            u->u_type = TR_BANG, u->u_outlet = outlet_new(&x->x_obj, &s_bang);
        else if (c == 'l')
            u->u_type = TR_LIST, u->u_outlet = outlet_new(&x->x_obj, &s_list);
        else if (c == 's')
            u->u_type = TR_SYMBOL,
            u->u_outlet = outlet_new(&x->x_obj, &s_symbol);
        else if (c == 'a')
            u->u_type = TR_ANYTHING,
            u->u_outlet = outlet_new(&x->x_obj, &s_symbol);
        else if (c == 'F'){ // static float
            u->u_float = ap->a_w.w_float;
            u->u_type = TR_STATIC_FLOAT;
            u->u_outlet = outlet_new(&x->x_obj, &s_float);
        }
        else if (c == 'S'){ // static symbol
            u->u_sym = gensym(ap->a_w.w_symbol->s_name);
            u->u_type = TR_STATIC_SYMBOL;
            u->u_outlet = outlet_new(&x->x_obj, &s_symbol);
        }
        else{
            pd_error(x, "t2: %s: bad type", ap->a_w.w_symbol->s_name);
            u->u_type = TR_FLOAT, u->u_outlet = outlet_new(&x->x_obj, &s_float);
        }
    }
    return (x);
}

void t2_setup(void){
    t2_class = class_new(gensym("t2"), (t_newmethod)t2_new,
        (t_method)t2_free, sizeof(t_t2), 0, A_GIMME, 0);
    class_addlist(t2_class, t2_list);
    class_addbang(t2_class, t2_bang);
    class_addpointer(t2_class, t2_pointer);
    class_addfloat(t2_class, (t_method)t2_float);
    class_addsymbol(t2_class, t2_symbol);
    class_addanything(t2_class, t2_anything);
    class_sethelpsymbol(t2_class, gensym("trigger2"));
}
