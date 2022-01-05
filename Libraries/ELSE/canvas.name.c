
#include "m_pd.h"
#include "g_canvas.h"

static t_class *canvas_name_class;

typedef struct canvas_name{
    t_object x_obj;
    t_symbol *x_name;
}t_canvas_name;

static void canvas_name_bang(t_canvas_name *x){
    outlet_symbol(x->x_obj.ob_outlet, x->x_name);
}

static void *canvas_name_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_canvas_name *x = (t_canvas_name *)pd_new(canvas_name_class);
    t_canvas *canvas = canvas_getcurrent();
    int depth = 0;
    if(argc && argv->a_type == A_FLOAT){
        float f = argv->a_w.w_float;
        depth = f < 0 ? 0 : (int)f;
    }
    while(depth-- && canvas->gl_owner)
        canvas = canvas->gl_owner;
    char buf[MAXPDSTRING];
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
