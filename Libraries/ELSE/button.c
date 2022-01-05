// porres 2020

#include "m_pd.h"
#include "g_canvas.h"

static t_class *button_class, *edit_proxy_class;
static t_widgetbehavior button_widgetbehavior;

typedef struct _edit_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _button *p_cnv;
}t_edit_proxy;

typedef struct _button{
    t_object        x_obj;
    t_glist        *x_glist;
    t_edit_proxy   *x_proxy;
    t_symbol       *x_bindname;
    int             x_x;
    int             x_y;
    int             x_w;
    int             x_h;
    int             x_sel;
    int             x_zoom;
    int             x_edit;
    int             x_state;
    unsigned char   x_bgcolor[3];
    unsigned char   x_fgcolor[3];
}t_button;

static void button_draw_io_let(t_button *x){
    if(x->x_edit){
        t_canvas *cv = glist_getcanvas(x->x_glist);
        int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags %lx_in\n",
            cv, xpos, ypos, xpos+(IOWIDTH*x->x_zoom), ypos+(IHEIGHT*x->x_zoom), x);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags %lx_out\n",
            cv, xpos, ypos+x->x_h, xpos+IOWIDTH*x->x_zoom, ypos+x->x_h-IHEIGHT*x->x_zoom, x);
    }
}

static void button_draw(t_button *x, t_glist *glist){
    int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -width %d -outline %s -fill #%2.2x%2.2x%2.2x -tags %lxBASE\n",
        glist_getcanvas(glist), xpos, ypos, xpos + x->x_w*x->x_zoom, ypos + x->x_h*x->x_zoom,
        x->x_zoom, x->x_sel ? "blue" : "black", x->x_bgcolor[0], x->x_bgcolor[1], x->x_bgcolor[2], x);
    button_draw_io_let(x);
}

static void button_erase(t_button* x, t_glist* glist){
    t_canvas *cv = glist_getcanvas(glist);
    sys_vgui(".x%lx.c delete %lxBASE\n", cv, x);
    sys_vgui(".x%lx.c delete %lx_in\n", cv, x);
    sys_vgui(".x%lx.c delete %lx_out\n", cv, x);
}

static void button_update(t_button *x){
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
        int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
        t_canvas *cv = glist_getcanvas(x->x_glist);
        sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n", cv, x, xpos, ypos,
            xpos + x->x_w*x->x_zoom, ypos + x->x_h*x->x_zoom);
        canvas_fixlinesfor(x->x_glist, (t_text*)x);
    }
}

static void button_vis(t_gobj *z, t_glist *glist, int vis){
    t_button* x = (t_button*)z;
    t_canvas *cv = glist_getcanvas(glist);
    if(vis){
        button_draw(x, glist);
        sys_vgui(".x%lx.c bind %lxBASE <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n", cv, x, x->x_bindname->s_name);
    }
    else
        button_erase(x, glist);
}

static void button_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void button_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_button *x = (t_button *)z;
    x->x_obj.te_xpix += dx, x->x_obj.te_ypix += dy;
    t_canvas *cv = glist_getcanvas(glist);
    sys_vgui(".x%lx.c move %lxBASE %d %d\n", cv, x, dx*x->x_zoom, dy*x->x_zoom);
    sys_vgui(".x%lx.c move %lx_in %d %d\n", cv, x, dx*x->x_zoom, dy*x->x_zoom);
    sys_vgui(".x%lx.c move %lx_out %d %d\n", cv, x, dx*x->x_zoom, dy*x->x_zoom);
    canvas_fixlinesfor(glist, (t_text*)x);
}

static void button_select(t_gobj *z, t_glist *glist, int sel){
    t_button *x = (t_button *)z;
    t_canvas *cv = glist_getcanvas(glist);
    if((x->x_sel = sel))
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline blue\n", cv, x);
    else
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline black\n", cv, x);
}

static void button_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_button *x = (t_button *)z;
    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = *xp1 + x->x_w*x->x_zoom;
    *yp2 = *yp1 + x->x_h*x->x_zoom;
}

static void button_save(t_gobj *z, t_binbuf *b){
  t_button *x = (t_button *)z;
  binbuf_addv(b, "ssiisiiiiiiii", gensym("#X"),gensym("obj"),
    (int)x->x_obj.te_xpix, (int)x->x_obj.te_ypix,
    atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
    x->x_w, x->x_h,
    x->x_bgcolor[0], x->x_bgcolor[1], x->x_bgcolor[2],
    x->x_fgcolor[0], x->x_fgcolor[1], x->x_fgcolor[2]);
  binbuf_addv(b, ";");
}

static void button_mouserelease(t_button* x){
    if(!x->x_glist->gl_edit){ // ignore if toggle or edit mode!
        outlet_float(x->x_obj.ob_outlet, x->x_state = 0);
        sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%2.2x%2.2x%2.2x\n",
            glist_getcanvas(x->x_glist), x, x->x_bgcolor[0], x->x_bgcolor[1], x->x_bgcolor[2]);
    }
}

static int button_click(t_gobj *z, struct _glist *glist, int xpix, int ypix,
int shift, int alt, int dbl, int doit){
    dbl = shift = alt = 0;
    t_button* x = (t_button *)z;
    int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    x->x_x = (xpix - xpos) / x->x_zoom;
    x->x_y = x->x_h - (ypix - ypos) / x->x_zoom;
    if(doit){
        outlet_float(x->x_obj.ob_outlet, x->x_state = 1);
        sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%2.2x%2.2x%2.2x\n",
            glist_getcanvas(x->x_glist), x, x->x_fgcolor[0], x->x_fgcolor[1], x->x_fgcolor[2]);
    }
    return(1);
}

static void button_size(t_button *x, t_floatarg f){
    int s = f < 12 ? 12 : (int)f;
    if(s != x->x_w || s != x->x_h){
        x->x_w = s; x->x_h = s;
        button_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            button_draw(x, x->x_glist);
            canvas_fixlinesfor(x->x_glist, (t_text*)x);
        }
    }
}

static void button_dim(t_button *x, t_floatarg f1, t_floatarg f2){
    int w = f1 < 12 ? 12 : (int)f1, h = f2 < 12 ? 12 : (int)f2;
    if(w != x->x_w || h != x->x_h){
        x->x_w = w; x->x_h = h;
        button_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            button_draw(x, x->x_glist);
            canvas_fixlinesfor(x->x_glist, (t_text*)x);
        }
    }
}

static void button_width(t_button *x, t_floatarg f){
    int w = f < 12 ? 12 : (int)f;
    if(w != x->x_w){
        x->x_w = w;
        button_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            button_draw(x, x->x_glist);
    }
}

static void button_height(t_button *x, t_floatarg f){
    int h = f < 12 ? 12 : (int)f;
    if(h != x->x_h){
        x->x_h = h;
        button_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            button_draw(x, x->x_glist);
            canvas_fixlinesfor(x->x_glist, (t_text*)x);
        }
    }
}

static void button_fgcolor(t_button *x, t_floatarg red, t_floatarg green, t_floatarg blue){
    int r = red < 0 ? 0 : red > 255 ? 255 : (int)red;
    int g = green < 0 ? 0 : green > 255 ? 255 : (int)green;
    int b = blue < 0 ? 0 : blue > 255 ? 255 : (int)blue;
    if(x->x_fgcolor[0] != r || x->x_fgcolor[1] != g || x->x_fgcolor[2] != b){
        x->x_fgcolor[0] = r; x->x_fgcolor[1] = g; x->x_fgcolor[2] = b;
        if(x->x_state && (glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)))
            sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%2.2x%2.2x%2.2x\n", glist_getcanvas(x->x_glist), x, r, g, b);
    }
}

static void button_bgcolor(t_button *x, t_floatarg red, t_floatarg green, t_floatarg blue){
    int r = red < 0 ? 0 : red > 255 ? 255 : (int)red;
    int g = green < 0 ? 0 : green > 255 ? 255 : (int)green;
    int b = blue < 0 ? 0 : blue > 255 ? 255 : (int)blue;
    if(x->x_bgcolor[0] != r || x->x_bgcolor[1] != g || x->x_bgcolor[2] != b){
        x->x_bgcolor[0] = r; x->x_bgcolor[1] = g; x->x_bgcolor[2] = b;
        if(!x->x_state && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            sys_vgui(".x%lx.c itemconfigure %lxBASE -fill #%2.2x%2.2x%2.2x\n", glist_getcanvas(x->x_glist), x, r, g, b);
    }
}

static void button_zoom(t_button *x, t_floatarg zoom){
    x->x_zoom = (int)zoom;
    sys_vgui(".x%lx.c itemconfigure %lxBASE -width %d\n", glist_getcanvas(x->x_glist), x, x->x_zoom);
    button_update(x);
}

static void edit_proxy_any(t_edit_proxy *p, t_symbol *s, int ac, t_atom *av){
    int edit = ac = 0;
    if(p->p_cnv){
        if(s == gensym("editmode"))
            edit = (int)(av->a_w.w_float);
         else if(s == gensym("obj") || s == gensym("msg") || s == gensym("floatatom")
        || s == gensym("symbolatom") || s == gensym("text") || s == gensym("bng")
        || s == gensym("toggle") || s == gensym("numbox") || s == gensym("vslider")
        || s == gensym("hslider") || s == gensym("vradio") || s == gensym("hradio")
        || s == gensym("vumeter") || s == gensym("mycnv") || s == gensym("selectall")){
            edit = 1;
        }
        else
            return;
        if(p->p_cnv->x_edit != edit){
            p->p_cnv->x_edit = edit;
            if(edit)
                button_draw_io_let(p->p_cnv);
            else{
                t_canvas *cv = glist_getcanvas(p->p_cnv->x_glist);
                sys_vgui(".x%lx.c delete %lx_in\n", cv, p->p_cnv);
                sys_vgui(".x%lx.c delete %lx_out\n", cv, p->p_cnv);
            }
        }
    }
}

static void button_free(t_button *x){
    pd_unbind(&x->x_obj.ob_pd, x->x_bindname);
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
    gfxstub_deleteforkey(x);
}

static void edit_proxy_free(t_edit_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_edit_proxy * edit_proxy_new(t_button *x, t_symbol *s){
    t_edit_proxy *p = (t_edit_proxy*)pd_new(edit_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)edit_proxy_free);
    return(p);
}

static void *button_new(t_symbol *s, int ac, t_atom *av){
    t_button *x = (t_button *)pd_new(button_class);
    t_canvas *cv = canvas_getcurrent();
    x->x_glist = (t_glist*)cv;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)cv);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = edit_proxy_new(x, gensym(buf));
    sprintf(buf, "#%lx", (long)x);
    pd_bind(&x->x_obj.ob_pd, x->x_bindname = gensym(buf));
    x->x_edit = cv->gl_edit;    x->x_zoom = x->x_glist->gl_zoom;
    x->x_x = x->x_y = 0;
    x->x_fgcolor[0] = 128;
    x->x_fgcolor[1] = 128;
    x->x_fgcolor[2] = 159;
    x->x_bgcolor[0] = x->x_bgcolor[1] = x->x_bgcolor[2] = 255;
    int w = 20, h = 20;
    if(ac && av->a_type == A_FLOAT){ // 1st Width
        w = av->a_w.w_float;
        ac--; av++;
        if(ac && av->a_type == A_FLOAT){ // 2nd Height
            h = av->a_w.w_float;
            ac--, av++;
            if(ac && av->a_type == A_FLOAT){ // Red
                x->x_bgcolor[0] = (unsigned char)av->a_w.w_float;
                ac--, av++;
                if(ac && av->a_type == A_FLOAT){ // Green
                    x->x_bgcolor[1] = (unsigned char)av->a_w.w_float;
                    ac--, av++;
                    if(ac && av->a_type == A_FLOAT){ // Blue
                        x->x_bgcolor[2] = (unsigned char)av->a_w.w_float;
                        ac--, av++;
                        if(ac && av->a_type == A_FLOAT){ // Red
                            x->x_fgcolor[0] = (unsigned char)av->a_w.w_float;
                            ac--, av++;
                            if(ac && av->a_type == A_FLOAT){ // Green
                                x->x_fgcolor[1] = (unsigned char)av->a_w.w_float;
                                ac--, av++;
                                if(ac && av->a_type == A_FLOAT){ // Blue
                                    x->x_fgcolor[2] = (unsigned char)av->a_w.w_float;
                                    ac--, av++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    while(ac > 0){
        if(av->a_type == A_SYMBOL){
            s = atom_getsymbolarg(0, ac, av);
            if(s == gensym("-dim")){
                if(ac >= 3 && (av+1)->a_type == A_FLOAT && (av+2)->a_type == A_FLOAT){
                    w = atom_getfloatarg(1, ac, av);
                    h = atom_getfloatarg(2, ac, av);
                    ac-=3, av+=3;
                }
                else goto errstate;
            }
            else if(s == gensym("-size")){
                post("hi");
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    w = h = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(s == gensym("-bgcolor")){
                if(ac >= 4 && (av+1)->a_type == A_FLOAT
                   && (av+2)->a_type == A_FLOAT
                   && (av+3)->a_type == A_FLOAT){
                    int r = (int)atom_getfloatarg(1, ac, av);
                    int g = (int)atom_getfloatarg(2, ac, av);
                    int b = (int)atom_getfloatarg(3, ac, av);
                    x->x_bgcolor[0] = r < 0 ? 0 : r > 255 ? 255 : r;
                    x->x_bgcolor[1] = g < 0 ? 0 : g > 255 ? 255 : g;
                    x->x_bgcolor[2] = b < 0 ? 0 : b > 255 ? 255 : b;
                    ac-=4, av+=4;
                }
                else goto errstate;
            }
            else if(s == gensym("-fgcolor")){
                if(ac >= 4 && (av+1)->a_type == A_FLOAT
                   && (av+2)->a_type == A_FLOAT
                   && (av+3)->a_type == A_FLOAT){
                    int r = (int)atom_getfloatarg(1, ac, av);
                    int g = (int)atom_getfloatarg(2, ac, av);
                    int b = (int)atom_getfloatarg(3, ac, av);
                    x->x_fgcolor[0] = r < 0 ? 0 : r > 255 ? 255 : r;
                    x->x_fgcolor[1] = g < 0 ? 0 : g > 255 ? 255 : g;
                    x->x_fgcolor[2] = b < 0 ? 0 : b > 255 ? 255 : b;
                    ac-=4, av+=4;
                }
                else goto errstate;
            }
            else goto errstate;
        }
        else goto errstate;
    }
    x->x_w = w, x->x_h = h;
    outlet_new(&x->x_obj, &s_anything);
    return(x);
    errstate:
        pd_error(x, "[button]: improper args");
        return(NULL);
}

void button_setup(void){
    button_class = class_new(gensym("button"), (t_newmethod)button_new,
        (t_method)button_free, sizeof(t_button), 0, A_GIMME, 0);
        class_addmethod(button_class, (t_method)button_dim, gensym("dim"), A_FLOAT, A_FLOAT, 0);
        class_addmethod(button_class, (t_method)button_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(button_class, (t_method)button_width, gensym("width"), A_FLOAT, 0);
    class_addmethod(button_class, (t_method)button_height, gensym("height"), A_FLOAT, 0);
    class_addmethod(button_class, (t_method)button_bgcolor, gensym("bgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(button_class, (t_method)button_fgcolor, gensym("fgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(button_class, (t_method)button_zoom, gensym("zoom"), A_CANT, 0);
    class_addmethod(button_class, (t_method)button_mouserelease, gensym("_mouserelease"), 0);
    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(edit_proxy_class, edit_proxy_any);
    button_widgetbehavior.w_getrectfn  = button_getrect;
    button_widgetbehavior.w_displacefn = button_displace;
    button_widgetbehavior.w_selectfn   = button_select;
    button_widgetbehavior.w_activatefn = NULL;
    button_widgetbehavior.w_deletefn   = button_delete;
    button_widgetbehavior.w_visfn      = button_vis;
    button_widgetbehavior.w_clickfn    = button_click;
    class_setsavefn(button_class, button_save);
    class_setwidget(button_class, &button_widgetbehavior);
}
