
#include "m_pd.h"
#include "g_canvas.h"

static t_class *canvas_name_class;

typedef struct canvas_name{
    t_object x_obj;
    t_symbol *x_name;
    t_symbol *x_wname;
}t_canvas_name;

static void canvas_name_bang(t_canvas_name *x){
    outlet_symbol(x->x_obj.ob_outlet, x->x_name);
}

static void *canvas_name_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_canvas_name *x = (t_canvas_name *)pd_new(canvas_name_class);
    int depth = 0, env = 0, w = 0;
    t_canvas *canvas;
    while(ac && av->a_type == A_SYMBOL){
        t_symbol *cursym = atom_getsymbolarg(0, ac, av);
        if(cursym == gensym("-env")){
            env = 1;
        }
        else if(cursym == gensym("-w"))
            w = 1;
        ac--, av++;
    }
    if(ac && av->a_type == A_FLOAT){
        float f = av->a_w.w_float;
        depth = f < 0 ? 0 : (int)f;
    }
    if(env){
        canvas = canvas_getrootfor(canvas_getcurrent());
        while(depth-- && canvas->gl_owner)
            canvas = canvas_getrootfor(canvas->gl_owner);
    }
    else{
        canvas = canvas_getcurrent();
        while(depth-- && canvas->gl_owner)
            canvas = canvas->gl_owner;
    }
    char buf[MAXPDSTRING];
    if(w)
        snprintf(buf, MAXPDSTRING, ".x%lx", (long unsigned int)canvas);
    else
        snprintf(buf, MAXPDSTRING, ".x%lx.c", (long unsigned int)canvas);
    x->x_name = gensym(buf);
    outlet_new(&x->x_obj, &s_);
    return(void *)x;
}

void setup_canvas0x2ename(void){
    canvas_name_class = class_new(gensym("canvas.name"), (t_newmethod)canvas_name_new,
        0, sizeof(t_canvas_name), 0, A_GIMME,0);
    class_addbang(canvas_name_class, canvas_name_bang);
}
