
#include "m_pd.h"
#include "string.h"
#include "g_canvas.h"

static t_class *args_class;

typedef struct _args{
    t_object  x_obj;
    t_canvas  *x_canvas;
    int        x_argc;
    t_atom    *x_argv;
    int             x_break;
    int             x_broken;
    char            x_separator;
}t_args;

static void copy_atoms(t_atom *src, t_atom *dst, int n){
    while(n--)
        *dst++ = *src++;
}

static void args_list(t_args *x, t_symbol *s, int argc, t_atom *argv){
    t_symbol *dummy = s;
    dummy = NULL;
    t_binbuf* b = x->x_canvas->gl_obj.te_binbuf;
    if(!x || !x->x_canvas || !b)
        return;
    t_atom name[1];
    SETSYMBOL(name, atom_getsymbol(binbuf_getvec(b)));
    binbuf_clear(b);
    binbuf_add(b, 1, name);
    binbuf_add(b, argc, argv);
    if(x->x_argc)
        freebytes(x->x_argv, x->x_argc * sizeof(x->x_argv));
    x->x_argc = argc;
    x->x_argv = getbytes(argc * sizeof(*(x->x_argv)));
    copy_atoms(argv, x->x_argv, argc);
}

static void args_break(t_args *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        outlet_anything(x->x_obj.ob_outlet, s, ac, av);
    else{
        int i = -1, first = 1, n;
        while(i < ac){
            // i is starting point & j is broken item
            int j = i + 1;
            // j starts as next item from previous iteration (and as 0 in the first iteration)
            while (j < ac){
                j++;
                if ((av+j)->a_type == A_SYMBOL && x->x_separator == (atom_getsymbol(av+j))->s_name[0]){
                    x->x_broken = 1;
                    break;
                }
            }
            n = j - i - 1; // n = # of extra elements in broken message (so we have - 1)
            if(first){
                if(av->a_type == A_SYMBOL &&
                   x->x_separator == (atom_getsymbol(av))->s_name[0])
                    outlet_anything(x->x_obj.ob_outlet, atom_getsymbol(av), n-1, av+1);
                else
                    outlet_anything(x->x_obj.ob_outlet, s, n, av);
                first = 0;
            }
            else
                outlet_anything(x->x_obj.ob_outlet, atom_getsymbol(av + i), n, av + i + 1);
            i = j;
        }
    }
}

static void args_bang(t_args *x){
    if(x->x_argv){
        if(x->x_break)
            args_break(x, &s_list, x->x_argc, x->x_argv);
        else
            outlet_list(x->x_obj.ob_outlet, &s_list, x->x_argc, x->x_argv);
    }
}

static void args_free(t_args *x){
    if(x->x_argc)
        freebytes(x->x_argv, x->x_argc * sizeof(x->x_argv));
}

static void *args_new(t_symbol *s, int ac, t_atom* av){
    t_symbol *dummy = s;
    dummy = NULL;
    t_args *x = (t_args *)pd_new(args_class);
    x->x_canvas = canvas_getrootfor(canvas_getcurrent());
    int depth = 0;
    int argc;
    t_atom *argv;
    x->x_break = x->x_broken = 0;
    if(ac && av->a_type == A_SYMBOL){
        x->x_separator = atom_getsymbol(av)->s_name[0];
        x->x_break = 1;
        ac--;
        av++;
    }
    if(ac && av->a_type == A_FLOAT)
        depth = (int)atom_getfloat(av);
    if(depth < 0)
        depth = 0;
    while(depth-- && x->x_canvas->gl_owner)
        x->x_canvas = canvas_getrootfor(x->x_canvas->gl_owner);
    canvas_setcurrent(x->x_canvas);
    canvas_getargs(&argc, &argv);
    x->x_argc = argc;
    x->x_argv = getbytes(argc * sizeof(*(x->x_argv)));
    copy_atoms(argv, x->x_argv, argc);
    canvas_unsetcurrent(x->x_canvas);
    outlet_new(&x->x_obj, 0);
    return(x);
}

void args_setup(void){
    args_class = class_new(gensym("args"), (t_newmethod)args_new,
        (t_method)args_free, sizeof(t_args), 0, A_GIMME, 0);
    class_addlist(args_class, (t_method)args_list);
    class_addbang(args_class, (t_method)args_bang);
}
