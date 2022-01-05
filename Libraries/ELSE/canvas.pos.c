
#include "m_pd.h"
#include "g_canvas.h"

static t_class *pos_class;

typedef struct _pos{
    t_object x_obj;
    t_canvas *x_canvas;
}t_pos;

void pos_bang(t_pos *x){
    t_atom at[2];
    SETFLOAT(at, x->x_canvas->gl_obj.te_xpix);
    SETFLOAT(at+1, x->x_canvas->gl_obj.te_ypix);
    outlet_list(x->x_obj.ob_outlet, &s_list, 2, at);
}

void *pos_new(t_floatarg f){
    t_pos *x = (t_pos *)pd_new(pos_class);
    t_glist *glist = (t_glist *)canvas_getcurrent();
    x->x_canvas = (t_canvas*)glist_getcanvas(glist);
    outlet_new(&x->x_obj, &s_list);
    if(f >= 0){
        int level = (int)f;
        while(level && x->x_canvas->gl_owner){
          x->x_canvas = x->x_canvas->gl_owner;
          level--;
        }
    }
    return(void *)x;
}

void setup_canvas0x2epos(void){
    pos_class = class_new(gensym("canvas.pos"), (t_newmethod)pos_new, 0,
        sizeof(t_pos), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addbang(pos_class, pos_bang);
}
