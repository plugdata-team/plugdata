
#include <m_pd.h>
#include <string.h>

static t_class *trigger2_class;
#define TR_BANG 0
#define TR_FLOAT 1
#define TR_SYMBOL 2
#define TR_POINTER 3
#define TR_LIST 4
#define TR_ANYTHING 5
#define TR_STATIC_FLOAT 6
#define TR_STATIC_SYMBOL 7

typedef struct trigger2out
{
    int u_type;         /* outlet type from above */
    t_symbol *u_sym;     /* static value */
    t_float u_float;    /* static value */
    t_outlet *u_outlet;
} t_trigger2out;

typedef struct _trigger2
{
    t_object x_obj;
    t_int x_n;
    t_trigger2out *x_vec;
} t_trigger2;

static void *trigger2_new(t_symbol *s, int argc, t_atom *argv){
    t_trigger2 *x = (t_trigger2 *)pd_new(trigger2_class);
    s = NULL;
    t_atom defarg[2], *ap;
    t_trigger2out *u;
    int i;
    if(!argc){
        argv = defarg;
        argc = 2;
        SETSYMBOL(&defarg[0], &s_bang);
        SETSYMBOL(&defarg[1], &s_bang);
    }
    x->x_n = argc;
    x->x_vec = (t_trigger2out *)getbytes(argc * sizeof(*x->x_vec));
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
        else c = 0; // ??
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
        else{ // ??
            pd_error(x, "trigger2: %s: bad type", ap->a_w.w_symbol->s_name);
            u->u_type = TR_FLOAT, u->u_outlet = outlet_new(&x->x_obj, &s_float);
        }
    }
    return (x);
}

static void trigger2_list(t_trigger2 *x, t_symbol *s, int argc, t_atom *argv)
{
    t_trigger2out *u;
    int i;
    for (i = (int)x->x_n, u = x->x_vec + i; u--, i--;)
    {
        if (u->u_type == TR_FLOAT)
            outlet_float(u->u_outlet, (argc ? atom_getfloat(argv) : 0));
        else if (u->u_type == TR_BANG)
            outlet_bang(u->u_outlet);
        else if (u->u_type == TR_SYMBOL)
            outlet_symbol(u->u_outlet,
                          (argc ? atom_getsymbol(argv) : s));
        else if (u->u_type == TR_ANYTHING)
            outlet_anything(u->u_outlet, s, argc, argv);
        else if (u->u_type == TR_POINTER)
            pd_error(x, "trigger2: bad pointer");
        else if (u->u_type == TR_STATIC_FLOAT)
            outlet_float(u->u_outlet, u->u_float);
        else if (u->u_type == TR_STATIC_SYMBOL)
            outlet_symbol(u->u_outlet, u->u_sym);
        else outlet_list(u->u_outlet, &s_list, argc, argv);
    }
}

static void trigger2_anything(t_trigger2 *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom *av2 = NULL;
    t_trigger2out *u;
    int i;
    for (i = (int)x->x_n, u = x->x_vec + i; u--, i--;)
    {
        if (u->u_type == TR_BANG)
            outlet_bang(u->u_outlet);
        else if (u->u_type == TR_ANYTHING)
            outlet_anything(u->u_outlet, s, argc, argv);
        else if (u->u_type == TR_FLOAT)
            outlet_float(u->u_outlet, 0);
        else if (u->u_type == TR_STATIC_FLOAT)
            outlet_float(u->u_outlet, u->u_float);
        else if (u->u_type == TR_STATIC_SYMBOL)
            outlet_symbol(u->u_outlet, u->u_sym);
        else if (u->u_type == TR_SYMBOL)
            outlet_symbol(u->u_outlet, s);
        else if (u->u_type == TR_POINTER)
            pd_error(x, "trigger2: bad pointer");
        else // list
        {
            av2 = (t_atom *)getbytes((argc + 1) * sizeof(t_atom));
            SETSYMBOL(av2, s);
            for (int j = 0; j < argc; j++)
                av2[j + 1] = argv[j];
            SETSYMBOL(av2, s);
            outlet_list(u->u_outlet, &s_list, argc+1, av2);
            freebytes(av2, (argc + 1) * sizeof(t_atom));
        }
    }
}

static void trigger2_pointer(t_trigger2 *x, t_gpointer *gp)
{
    t_trigger2out *u;
    int i;
    for (i = (int)x->x_n, u = x->x_vec + i; u--, i--;)
    {
        if (u->u_type == TR_BANG)
            outlet_bang(u->u_outlet);
        else if (u->u_type == TR_POINTER || u->u_type == TR_ANYTHING)
            outlet_pointer(u->u_outlet, gp);
        else if (u->u_type == TR_STATIC_FLOAT)
            outlet_float(u->u_outlet, u->u_float);
        else pd_error(x, "trigger2: can only convert 'pointer' to 'bang'");
    }
}

static void trigger2_bang(t_trigger2 *x)
{
    trigger2_list(x, &s_bang, 0, 0);
}

static void trigger2_float(t_trigger2 *x, t_float f)
{
    t_atom at;
    SETFLOAT(&at, f);
    trigger2_list(x, &s_float, 1, &at);
}

static void trigger2_symbol(t_trigger2 *x, t_symbol *s)
{
    t_atom at;
    SETSYMBOL(&at, s);
    trigger2_list(x, &s_symbol, 1, &at);
}

static void trigger2_free(t_trigger2 *x)
{
    freebytes(x->x_vec, x->x_n * sizeof(*x->x_vec));
}

void trigger2_setup(void)
{
    trigger2_class = class_new(gensym("trigger2"), (t_newmethod)trigger2_new,
        (t_method)trigger2_free, sizeof(t_trigger2), 0, A_GIMME, 0);
    class_addlist(trigger2_class, trigger2_list);
    class_addbang(trigger2_class, trigger2_bang);
    class_addpointer(trigger2_class, trigger2_pointer);
    class_addfloat(trigger2_class, (t_method)trigger2_float);
    class_addsymbol(trigger2_class, trigger2_symbol);
    class_addanything(trigger2_class, trigger2_anything);
}
