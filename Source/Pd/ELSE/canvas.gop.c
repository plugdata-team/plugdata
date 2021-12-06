
#include "m_pd.h"
#include "g_canvas.h"

static t_class *gop_class;

typedef struct _gop{
    t_object x_obj;
    t_canvas *x_canvas;
}t_gop;

void gop_bang(t_gop *x){
    int ac = 7;
    t_atom at[7];
    SETFLOAT(at, x->x_canvas->gl_isgraph);      // gop
    SETFLOAT(at+1, x->x_canvas->gl_pixwidth);   // width
    SETFLOAT(at+2, x->x_canvas->gl_pixheight);  // height
    SETFLOAT(at+3, x->x_canvas->gl_xmargin);    // x1
    SETFLOAT(at+4, x->x_canvas->gl_ymargin);    // y1
    SETFLOAT(at+5, x->x_canvas->gl_xmargin +
             x->x_canvas->gl_pixwidth);         // x2
    SETFLOAT(at+6, x->x_canvas->gl_ymargin +
             x->x_canvas->gl_pixheight);        // y2
    outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
}

void *gop_new(t_floatarg f){
    t_gop *x = (t_gop *)pd_new(gop_class);
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

void setup_canvas0x2egop(void){
    gop_class = class_new(gensym("canvas.gop"), (t_newmethod)gop_new, 0,
        sizeof(t_gop), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addbang(gop_class, gop_bang);
}
