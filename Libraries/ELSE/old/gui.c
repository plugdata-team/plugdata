

#include "m_pd.h"

static t_class *gui_class;

typedef struct _gui{
    t_object x_obj;
}t_gui;

static void gui_anything(t_gui *x, t_symbol *s, int ac, t_atom *av){
    t_binbuf *b = binbuf_new();
    char *buf;
    int length;
    t_atom sym[1];
    SETSYMBOL(sym, s);
    binbuf_add(b, 1, sym);
    binbuf_add(b, ac, av);
    binbuf_gettext(b, &buf, &length);
    buf = t_resizebytes(buf, length, length+2);
    buf[length] = '\n';
    buf[length+1] = 0;
    sys_gui(buf);
    t_freebytes(buf, length+2);
    binbuf_free(b);
    x = NULL; // remove warnings
}

static void *gui_new( void){
    t_gui *x = (t_gui *)pd_new(gui_class);
    return(x);
}

void gui_setup(void){
    gui_class = class_new(gensym("gui"), (t_newmethod)gui_new, 0, sizeof(t_gui), 0, 0);
    class_addanything(gui_class, gui_anything);
}
