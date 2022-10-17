// porres 2022

#include "m_pd.h"
#include "g_canvas.h"

static t_class *canvas_file_class;

typedef struct _canvas_file{
    t_object    x_obj;
    t_canvas   *x_cv;
    t_int       x_i;
    t_outlet   *x_fail;
}t_canvas_file;

static void canvas_file_symbol(t_canvas_file *x, t_symbol *file){
    char path[MAXPDSTRING], *fn;
    int fd = canvas_open(x->x_cv, file->s_name, "", path, &fn, MAXPDSTRING, 1);
    if(fd >= 0){
        sys_close(fd);
        if(fn > path)
            fn[-1] = '/';
        outlet_symbol(x->x_obj.ob_outlet, gensym(path));
    }
    else
        outlet_symbol(x->x_fail, file);
}

static void canvas_file_anything(t_canvas_file *x, t_symbol *s, int ac, t_atom *av){
    ac = 0;
    av = NULL;
    canvas_file_symbol(x, s);
}

static void *canvas_file_new(t_floatarg f){
    t_canvas_file *x = (t_canvas_file *)pd_new(canvas_file_class);
    x->x_cv = canvas_getrootfor(canvas_getcurrent());
    int i = f < 0 ? 0 : (int)f;
    while(i-- && x->x_cv->gl_owner)
        x->x_cv = canvas_getrootfor(x->x_cv->gl_owner);
    outlet_new(&x->x_obj, &s_);
    x->x_fail = outlet_new(&x->x_obj, &s_);
    return(x);
}

void setup_canvas0x2efile(void){
    canvas_file_class = class_new(gensym("canvas.file"), (t_newmethod)canvas_file_new,
        0, sizeof(t_canvas_file), 0, A_DEFFLOAT, 0);
    class_addsymbol(canvas_file_class, canvas_file_symbol);
    class_addanything(canvas_file_class, canvas_file_anything);
}
