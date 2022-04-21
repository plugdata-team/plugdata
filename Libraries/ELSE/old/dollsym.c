// porres 2020

#include "m_pd.h"
#include "g_canvas.h"

static t_class *dollsym_class;

typedef struct _dollsym{
    t_object    x_obj;
    t_symbol   *x_sym;
    t_glist    *x_cv;
}t_dollsym;

static void dollsym_bang(t_dollsym *x){
    outlet_symbol(x->x_obj.ob_outlet, x->x_sym);
}

static void dollsym_symbol(t_dollsym *x, t_symbol *s){
    outlet_symbol(x->x_obj.ob_outlet, x->x_sym = canvas_realizedollar(x->x_cv, s));
}

static void dollsym_any(t_dollsym *x, t_symbol *s, int ac, t_atom* av){
    if(ac == 0){
        av = NULL;
        outlet_symbol(x->x_obj.ob_outlet, x->x_sym = canvas_realizedollar(x->x_cv, s));
    }
}

static void *dollsym_new(t_symbol *s, int ac, t_atom* av){
    s = NULL;
    t_dollsym *x = (t_dollsym *)pd_new(dollsym_class);
    int depth = 0;
    x->x_sym = &s_;
    if(ac){
        if(av->a_type == A_FLOAT){
            t_float argval = atom_getfloatarg(0, ac, av);
            depth = argval < 0 ? 0 : (int)argval;
            ac--;
            av++;
        }
    }
    x->x_cv = canvas_getrootfor(canvas_getcurrent());
    while(depth-- && x->x_cv->gl_owner)
        x->x_cv = canvas_getrootfor(x->x_cv->gl_owner);
    if(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *sym = atom_getsymbolarg(0, ac, av);
            x->x_sym = canvas_realizedollar(x->x_cv, sym);
            ac--;
            av++;
        }
    }
    outlet_new(&x->x_obj, &s_);
    return(x);
}

void dollsym_setup(void){
    dollsym_class = class_new(gensym("dollsym"), (t_newmethod)dollsym_new,
        0, sizeof(t_dollsym), 0, A_GIMME, 0);
    class_addsymbol(dollsym_class, dollsym_symbol);
    class_addanything(dollsym_class, dollsym_any);
    class_addbang(dollsym_class, dollsym_bang);
}
