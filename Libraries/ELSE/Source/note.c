// Based on our work in comment from cyclone

#include <string.h>
#include <ctype.h>
#include "s_utf8.h"
#include "m_pd.h"
#include "g_canvas.h"

#define NOTE_MINSIZE       8
#define NOTE_HANDLE_WIDTH  8
#define NOTE_OUTBUFSIZE    16384

#if __APPLE__
char default_font[100] = "Menlo";
#else
char default_font[100] = "DejaVu Sans Mono";
#endif

static t_class *note_class, *notesink_class, *handle_class, *edit_proxy_class;
static t_widgetbehavior note_widgetbehavior;

static t_pd *notesink = 0;

static void note_getrect(t_gobj *z, t_glist *gl, int *xp1, int *yp1, int *xp2, int *yp2);

typedef struct _edit_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct _note *p_cnv;
}t_edit_proxy;

typedef struct _note{
    t_object        x_obj;
    t_edit_proxy   *x_proxy;
    t_glist        *x_glist;
    t_canvas       *x_cv;
    t_binbuf       *x_binbuf;
    char           *x_buf;      // text buf
    int             x_bufsize;  // text buf size
    int             x_keynum;
    int             x_init;
    int             x_resized;
    int             x_changed;
    int             x_edit;
    int             x_max_pixwidth;
    int             x_text_width;
    int             x_width;
    int             x_height;
    int             x_bbset;
    int             x_bbpending;
    int             x_x1;
    int             x_y1;
    int             x_x2;
    int             x_y2;
    int             x_newx2;
    int             x_dragon;
    int             x_select;
    int             x_fontsize;
    int             x_shift;
    int             x_selstart;
    int             x_start_ndx;
    int             x_end_ndx;
    int             x_selend;
    int             x_active;
    unsigned char   x_red;
    unsigned char   x_green;
    unsigned char   x_blue;
    unsigned char   x_bg[3]; // background color
    char            x_color[8];
    char            x_bgcolor[8];
    t_symbol       *x_keysym;
    t_symbol       *x_bindsym;
    t_symbol       *x_fontname;
    t_symbol       *x_receive;
    t_symbol       *x_rcv_raw;
    int             x_rcv_set;
    int             x_flag;
    int             x_r_flag;
    int             x_old;
    int             x_text_flag;
    int             x_text_n;
    int             x_text_size;
    int             x_zoom;
    int             x_fontface;
    int             x_bold;
    int             x_italic;
    int             x_underline;
    int             x_bg_flag;
    int             x_textjust; // 0: left, 1: center, 2: right
    t_pd           *x_handle;
}t_note;

typedef struct _handle{
    t_pd        h_pd;
    t_note     *h_master;
    t_symbol   *h_bindsym;
    char        h_pathname[64];
    int         h_clicked;
}t_handle;

// helper functions
static void note_initialize(t_note *x){
    t_binbuf *bb = x->x_obj.te_binbuf;
    int n_args = binbuf_getnatom(bb) - 1; // number of arguments
    if(x->x_text_flag){ // let's get the text from the attribute
        int n = x->x_text_n, ac = x->x_text_size;
        t_atom* av = (t_atom *)getbytes(ac*sizeof(t_atom));
        char buf[128];
        for(int i = 0;  i < ac; i++){
            atom_string(binbuf_getvec(bb) + n + 1 + i, buf, 128);
            SETSYMBOL(av+i, gensym(buf));
        }
        binbuf_clear(x->x_binbuf);
        binbuf_restore(x->x_binbuf, ac, av);
        freebytes(av, ac*sizeof(t_atom));
    }
    else{
        int n = x->x_old ? 8 : 14;
        if(n_args > n){
            int ac = n_args - n;
            t_atom* av = (t_atom *)getbytes(ac*sizeof(t_atom));
            char buf[128];
            for(int i = 0;  i < ac; i++){
                atom_string(binbuf_getvec(bb) + n + 1 + i, buf, 128);
                SETSYMBOL(av+i, gensym(buf));
            }
            binbuf_clear(x->x_binbuf);
            binbuf_restore(x->x_binbuf, ac, av);
            freebytes(av, ac*sizeof(t_atom));
        }
    }
    binbuf_gettext(x->x_binbuf, &x->x_buf, &x->x_bufsize);
    x->x_init = 1;
}

static void note_draw_handle(t_note *x){
//    post("draw handle");
    t_handle *ch = (t_handle *)x->x_handle;
    sys_vgui("destroy %s\n", ch->h_pathname); // always destroy, bad hack, improve
    if(x->x_edit){
//        post("note_draw_handle");
        int x1, y1, x2, y2;
        note_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
        if(x->x_resized)
            x2 = x1 + x->x_max_pixwidth;
        sys_vgui("canvas %s -width %d -height %d -bg %s -cursor sb_h_double_arrow\n",
            ch->h_pathname, NOTE_HANDLE_WIDTH, x->x_height, "black");
        sys_vgui("bind %s <Button> {pdsend [concat %s _click 1 \\;]}\n", ch->h_pathname, ch->h_bindsym->s_name);
        sys_vgui("bind %s <ButtonRelease> {pdsend [concat %s _click 0 \\;]}\n", ch->h_pathname, ch->h_bindsym->s_name);
        sys_vgui("bind %s <Motion> {pdsend [concat %s _motion %%x %%y \\;]}\n", ch->h_pathname, ch->h_bindsym->s_name);
        sys_vgui(".x%lx.c create window %d %d -anchor nw -width %d -height %d -window %s -tags [list handle%lx all%lx]\n",
            x->x_cv,
            x2 + 2*x->x_zoom,
            y1 + x->x_zoom,
            NOTE_HANDLE_WIDTH*x->x_zoom,
            x->x_height + 2*x->x_zoom,
            ch->h_pathname,
            (unsigned long)x,
            (unsigned long)x);
    }
}

static void note_draw_inlet(t_note *x){
    if(x->x_edit &&  x->x_receive == &s_){
        t_canvas *cv = glist_getcanvas(x->x_glist);
        int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lx_in all%lx]\n",
            cv, xpos, ypos, xpos+(IOWIDTH*x->x_zoom), ypos+(IHEIGHT*x->x_zoom)-x->x_zoom,
            (unsigned long)x, (unsigned long)x);
    }
}

static void note_draw_outline(t_note *x){
    if(x->x_bbset && x->x_edit){
        int x1, y1, x2, y2;
        note_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags [list %lx_outline all%lx] -width %d -outline %s\n",
            (unsigned long)x->x_cv,
            x1,
            y1,
            x2,
            y2,
            (unsigned long)x, // %lx_outline
            (unsigned long)x, // all%lx
            x->x_zoom,
            x->x_select ? "blue" : "black");
    }
}

static void note_adjust_justification(t_note *x){
    int move = 0;
    if(x->x_textjust && x->x_resized){
        move = x->x_max_pixwidth - x->x_text_width;
        if(x->x_textjust == 1) // center
            move/=2;
    }
    if(move){
//        post("move (%d) / max_pixwidth (%d) / text_width (%d)", move, x->x_max_pixwidth, x->x_text_width);
        sys_vgui(".x%lx.c moveto txt%lx  %d %d\n", x->x_cv, (unsigned long)x, x->x_x1+move*x->x_zoom, x->x_y1);
    }
}

static void note_draw(t_note *x){
//    post("note_draw");
//    post("x->x_bbset = %d", x->x_bbset);
    x->x_cv = glist_getcanvas(x->x_glist);
    if(x->x_bg_flag && x->x_bbset){ // draw bg only if initialized
//        post("draw bg");
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags [list bg%lx all%lx] -outline %s -fill %s\n",
            (unsigned long)x->x_cv,
            text_xpix((t_text *)x, x->x_glist),
            text_ypix((t_text *)x, x->x_glist),
            x->x_x2 + x->x_zoom * 2,
            x->x_y2 + x->x_zoom * 2,
            (unsigned long)x, (unsigned long)x,
            x->x_bgcolor, x->x_bgcolor);
    }
    char buf[NOTE_OUTBUFSIZE], *outbuf, *outp;
    outp = outbuf = buf;
    sprintf(outp, "%s %s .x%lx.c txt%lx all%lx %d %d {%s} -%d %s {%.*s} %d %s %s %s\n",
        x->x_underline ? "note_draw_ul" : "note_draw",
        x->x_bindsym->s_name, // %s
        (unsigned long)x->x_cv, // .x%lx.c
        (unsigned long)x, // txt%lx
        (unsigned long)x, // all%lx
        text_xpix((t_text *)x, x->x_glist) + x->x_zoom, // %d
        text_ypix((t_text *)x, x->x_glist) + x->x_zoom, // %d
        x->x_fontname->s_name, // {%s}
        x->x_fontsize, // -%d
        x->x_select ? "blue" : x->x_color, // %s
        x->x_bufsize, // %.
        x->x_buf, // *s
        x->x_max_pixwidth, // %d
        x->x_bold ? "bold" : "normal",
        x->x_italic ? "italic" : "roman", //
        x->x_textjust == 0 ? "left" : x->x_textjust == 1 ? "center" : "right");
    x->x_bbpending = 1; // bbox pending
    sys_gui(outbuf);
    if(outbuf != buf)
        freebytes(outbuf, x->x_bufsize);
    note_draw_handle(x); // fix weird bug????
    note_draw_inlet(x);
    note_draw_outline(x);
    note_adjust_justification(x);
}

static void note_update(t_note *x){
    char buf[NOTE_OUTBUFSIZE], *outbuf, *outp;
    unsigned long cv = (unsigned long)x->x_cv;
    outp = outbuf = buf;
    sprintf(outp, "note_update .x%lx.c txt%lx {%.*s} %d\n", cv, (unsigned long)x,
        x->x_bufsize, x->x_buf, x->x_max_pixwidth);
    outp += strlen(outp);
    if(x->x_active){
        if(x->x_selend > x->x_selstart){ // <= TEXT SELECTION!!!!
            sprintf(outp, ".x%lx.c select from txt%lx %d\n", cv, (unsigned long)x, x->x_start_ndx);
            outp += strlen(outp);
            sprintf(outp, ".x%lx.c select to txt%lx %d\n", cv, (unsigned long)x, x->x_selend);
            outp += strlen(outp);
            sprintf(outp, ".x%lx.c focus {}\n", cv);
        }
        else{
            sprintf(outp, ".x%lx.c select clear\n", cv);
            outp += strlen(outp);
            sprintf(outp, ".x%lx.c icursor txt%lx %d\n", cv, (unsigned long)x, x->x_start_ndx);
            outp += strlen(outp);
            sprintf(outp, ".x%lx.c focus txt%lx\n", cv, (unsigned long)x);
        }
        outp += strlen(outp);
    }
    sprintf(outp, "note_bbox %s .x%lx.c txt%lx\n",
        x->x_bindsym->s_name, cv, (unsigned long)x);
    x->x_bbpending = 1;
    sys_gui(outbuf);
    if(outbuf != buf)
        freebytes(outbuf, x->x_bufsize);
    note_adjust_justification(x);
}

static void note_erase(t_note *x){
    sys_vgui(".x%lx.c delete all%lx\n", x->x_cv, (unsigned long)x);
    t_handle *ch = (t_handle *)x->x_handle;
    sys_vgui("destroy %s\n", ch->h_pathname);
}

static void note_redraw(t_note *x){ // <= improve, not necessary for all cases
     if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
         note_erase(x);
         note_draw(x);
     }
}

static void note_grabbedkey(void *z, t_floatarg f){
    z = NULL, f = 0; // LATER think about replacing #key binding to float method with "grabbing"
}

static void note_dograb(t_note *x){
//    post("do grab");
// LATER investigate grab feature. Used here to prevent backspace erasing text.
// Done also when already active, because after clicked we lost our previous grab.
    glist_grab(x->x_glist, (t_gobj *)x, 0, (t_glistkeyfn)note_grabbedkey, 0, 0);
}

static void note_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
//    post("note_getrect");
    t_note *x = (t_note *)z;
    float x1, y1, x2, y2;
    x1 = text_xpix((t_text *)x, glist);
    y1 = text_ypix((t_text *)x, glist);
    if(x->x_resized)
        x->x_width = x->x_max_pixwidth;
    int min_size = NOTE_MINSIZE;
// no fucking zoom
    if(x->x_width < min_size)
        x->x_width = min_size;
    if(x->x_height < min_size)
        x->x_height = min_size;
    x2 = x1 + x->x_width;
    y2 = y1 + x->x_height;
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x2;
    *yp2 = y2;
//    post("getrect x1(%g) x2(%g) y1(%g) y2(%g) | width(%d) height(%d)", x1, x2, y1, y2, x->x_width, x->x_height);
}

static void note_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    glist = NULL;
    t_note *x = (t_note *)z;
    if(!x->x_active && !x->x_dragon){  // ???
        t_text *t = (t_text *)z;
        t->te_xpix += dx, x->x_x1 += dx, x->x_x2 += dx;
        t->te_ypix += dy, x->x_y1 += dy, x->x_y2 += dy;
        sys_vgui(".x%lx.c move all%lx %d %d\n", x->x_cv, (unsigned long)x, dx*x->x_zoom, dy*x->x_zoom);
        canvas_fixlinesfor(x->x_cv, t);
    }
}

static void note_activate(t_gobj *z, t_glist *glist, int state){
    glist = NULL;
    t_note *x = (t_note *)z;
    if(state){
        note_dograb(x);
        if(x->x_active)
            return;
        sys_vgui(".x%lx.c focus txt%lx\n", x->x_cv, (unsigned long)x);
        x->x_selstart = x->x_start_ndx = x->x_start_ndx = 0;
        x->x_selend = x->x_bufsize;
        x->x_active = 1;
        pd_bind((t_pd *)x, gensym("#key"));
        pd_bind((t_pd *)x, gensym("#keyname"));
    }
    else{
        if(!x->x_active)
            return;
        pd_unbind((t_pd *)x, gensym("#key"));
        pd_unbind((t_pd *)x, gensym("#keyname"));
        sys_vgui("selection clear .x%lx.c\n", x->x_cv);
        sys_vgui(".x%lx.c focus {}\n", x->x_cv);
        x->x_active = 0;
    }
    note_update(x);
}

static void note_select(t_gobj *z, t_glist *glist, int state){
    t_note *x = (t_note *)z;
    x->x_select = state;
    if(!state && x->x_active)
        note_activate(z, glist, 0);
    sys_vgui(".x%lx.c itemconfigure txt%lx -fill %s\n", x->x_cv, (unsigned long)x, state ? "blue" : x->x_color);
    sys_vgui(".x%lx.c itemconfigure %lx_outline -width %d -outline %s\n",
        x->x_cv, (unsigned long)x, x->x_zoom, state ? "blue" : "black");
// A regular rtext should set 'canvas_editing' variable to its canvas, we don't do it coz
// we get keys via global binding to "#key" (and coz 'canvas_editing' isn't exported).
}

static void note_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void note_vis(t_gobj *z, t_glist *glist, int vis){
//    post("vis = %d", vis);
    t_note *x = (t_note *)z;
    x->x_cv = glist_getcanvas(x->x_glist = glist);
    if(!x->x_init)
        note_initialize(x);
    if(vis){
        t_handle *ch = (t_handle *)x->x_handle;
        sprintf(ch->h_pathname, ".x%lx.h%lx", (unsigned long)x->x_cv, (unsigned long)ch);
        note_draw(x);
    }
    else
        note_erase(x);
}

static void note__bbox_callback(t_note *x, t_symbol *bindsym,
t_floatarg x1, t_floatarg y1, t_floatarg x2, t_floatarg y2){
    x->x_text_width = x2-x1;
//    post("note__bbox_callback");
    bindsym = NULL;
    if(!x->x_bbset || (x->x_height != (y2-y1))){ // redraw
//        post("bbox redraw");
        x->x_height = y2-y1, x->x_y1 = y1, x->x_y2 = y2;
        if(x->x_resized)
            x->x_width = x->x_max_pixwidth, x->x_x2 = x1 + x->x_max_pixwidth;
        else
            x->x_width = x2-x1, x->x_x2 = x2;
        x->x_x1 = x1;
        x->x_bbset = 1;
        note_redraw(x);
    }
    x->x_bbpending = 0;
}

static void note__click_callback(t_note *x, t_symbol *s, int ac, t_atom *av){
//    post("note__click_callback");
    s = NULL;
    int xx, ndx;
    if(ac == 8 && av[0].a_type == A_SYMBOL && av[1].a_type == A_FLOAT
       && av[2].a_type == A_FLOAT && av[3].a_type == A_FLOAT
       && av[4].a_type == A_FLOAT && av[5].a_type == A_FLOAT
       && av[6].a_type == A_FLOAT && av[7].a_type == A_FLOAT){
            xx = (int)av[1].a_w.w_float;
            ndx = (int)av[3].a_w.w_float;
            note__bbox_callback(x, av[0].a_w.w_symbol, av[4].a_w.w_float, av[5].a_w.w_float, av[6].a_w.w_float, av[7].a_w.w_float);
    }
    else{
        post("bug [note]: note__click_callback");
        return;
    }
    char buf[NOTE_OUTBUFSIZE], *outp = buf;
    unsigned long cv = (unsigned long)x->x_cv;
    if(x->x_glist->gl_edit){
        if(x->x_active){
            if(ndx >= 0 && ndx <= x->x_bufsize){
//                post("ndx = %d", ndx);
                x->x_start_ndx = x->x_end_ndx = ndx;
                int byte_ndx = 0;
                for(int i = 0; i < ndx; i++)
                    u8_inc(x->x_buf, &byte_ndx);
                x->x_selstart = x->x_selend = byte_ndx;
                note_dograb(x);
                note_update(x);
// set selection, LATER shift-click and drag
            }
        }
        else if(xx > x->x_x2 - NOTE_HANDLE_WIDTH){ // start resizing
            sprintf(outp, ".x%lx.c bind txt%lx <ButtonRelease> {pdsend {%s _release %s}}\n",
                cv, (unsigned long)x, x->x_bindsym->s_name, x->x_bindsym->s_name);
            outp += strlen(outp);
            sprintf(outp, ".x%lx.c bind txt%lx <Motion> {pdsend {%s _motion %s %%x %%y}}\n",
                cv, (unsigned long)x, x->x_bindsym->s_name, x->x_bindsym->s_name);
            outp += strlen(outp);
            sys_gui(buf);
            x->x_newx2 = x->x_x2;
            x->x_dragon = 1;
        }
    }
}

static void handle__click_callback(t_handle *ch, t_floatarg f){
//    post("handle__click_callback");
    int click = (int)f;
    t_note *x = ch->h_master;
    if(ch->h_clicked && click == 0){ // Released the handle
        if(x->x_x2 != x->x_newx2){
            x->x_resized = 1;
            x->x_x2 = x->x_newx2;
            x->x_max_pixwidth = x->x_newx2 - x->x_x1;
            note_redraw(x); // needed to call bbox callback
        }
    }
    if(click)
        x->x_bbset = 0; // arm bbox callback redraw
    ch->h_clicked = click;
}

static void handle__motion_callback(t_handle *ch, t_floatarg f1, t_floatarg f2){
    if(ch->h_clicked){ // dragging handle
        t_note *x = ch->h_master;
        int dx = (int)f1;
        f2 = 0; // avoid warning
        int x1, y1, x2, y2;
        note_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
        x->x_x1 = x1, x->x_x2 = x2;
        x->x_y1 = y1, x->x_y2 = y2;
        int newx = x2 + dx;
        if(newx > x1 + NOTE_MINSIZE){ // update outline
            sys_vgui(".x%lx.c coords %lx_outline %d %d %d %d\n", (unsigned long)x->x_cv,
                (unsigned long)x, x->x_x1, x->x_y1, x->x_newx2 = newx, x->x_y2);
        }
    }
}

static void notesink__bbox_callback(t_pd *x, t_symbol *bindsym){
    if(bindsym->s_thing == x)
        pd_unbind(x, bindsym);  // if note gone, unbind
}

static void notesink_anything(t_pd *x, t_symbol *s, int ac, t_atom *av){ // nop: avoid warnings
    x = NULL;
    s = NULL;
    ac = (int)av;
}

static void note_get_rcv(t_note* x){
    if(!x->x_rcv_set){ // no receive set, search arguments
        t_binbuf *bb = x->x_obj.te_binbuf;
        int n_args = binbuf_getnatom(bb) - 1, i = 0; // number of arguments
        char buf[128];
        if(n_args > 0){ // we have arguments, let's search them
            if(x->x_flag){ // arguments are flags actually
                if(x->x_r_flag){ // we got a receive flag, let's get it
                    for(i = 0;  i <= n_args; i++){
                        atom_string(binbuf_getvec(bb) + i, buf, 128);
                        if(gensym(buf) == gensym("@receive")){
                            i++;
                            atom_string(binbuf_getvec(bb) + i, buf, 128);
                            x->x_rcv_raw = gensym(buf);
                            break;
                        }
                    }
                }
            }
            else{ // we got no flags, let's search for argument
                int arg_n = 4; // receive argument number
                if(n_args >= arg_n){ // we have it, get it
                    atom_string(binbuf_getvec(bb) + arg_n, buf, 128);
                    x->x_rcv_raw = gensym(buf);
                }
            }
        }
    }
    if(x->x_rcv_raw == &s_ || x->x_rcv_raw == gensym("?"))
        x->x_rcv_raw = gensym("empty");
}
    
static void note_save(t_gobj *z, t_binbuf *b){
    t_note *x = (t_note *)z;
    if(!x->x_init) // hack???
        note_initialize(x);
    t_binbuf *bb = x->x_obj.te_binbuf;
    note_get_rcv(x);
    binbuf_addv(b, "ssiisiissiiiiiiiiii",
        gensym("#X"),
        gensym("obj"),
        (int)x->x_obj.te_xpix,
        (int)x->x_obj.te_ypix,
        atom_getsymbol(binbuf_getvec(bb)),
        x->x_max_pixwidth / x->x_zoom,
        x->x_fontsize / x->x_zoom,
        x->x_fontname,
        x->x_rcv_raw,
        x->x_fontface,
        (int)x->x_red,
        (int)x->x_green,
        (int)x->x_blue,
        x->x_underline,
        (int)x->x_bg[0],
        (int)x->x_bg[1],
        (int)x->x_bg[2],
        x->x_bg_flag,
        x->x_textjust);
    binbuf_addbinbuf(b, x->x_binbuf); // the actual note
    binbuf_addv(b, ";");
}

static void note_key(t_note *x){
    if(!x->x_active){
        post("key bug");
        return;
    }
    int i, newsize, ndel, n = x->x_keynum;
    if(n){
    //        post("note_float => input character = [%c], n = %d", n, n);
        if (n == '\r')
            n = '\n';
        if (n == '\b'){  // backspace
            // LATER delete the box if all text is selected...
            // this causes reentrancy problems now.
            // if ((!x->x_selstart) && (x->x_selend == x->x_bufsize)){}
            if(x->x_selstart && (x->x_selstart == x->x_selend)){
                u8_dec(x->x_buf, &x->x_selstart);
                x->x_start_ndx--, x->x_end_ndx--;
            }
        }
        else if(n == 127){    // delete
            if(x->x_selend < x->x_bufsize && (x->x_selstart == x->x_selend)){
                u8_inc(x->x_buf, &x->x_selend);
            }
        }
        ndel = x->x_selend - x->x_selstart;
        for(i = x->x_selend; i < x->x_bufsize; i++)
            x->x_buf[i-ndel] = x->x_buf[i];
        newsize = x->x_bufsize - ndel;
        x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
        x->x_bufsize = newsize;

        if(n == '\n' || (n > 31 && n < 127)){ // accepted ascii characters
            newsize = x->x_bufsize+1;
            x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
            for (i = x->x_bufsize; i > x->x_selstart; i--)
                x->x_buf[i] = x->x_buf[i-1];
            x->x_buf[x->x_selstart] = n;
            x->x_bufsize = newsize;
            x->x_selstart++, x->x_start_ndx++, x->x_end_ndx++;
        }
        else if(n > 127){ // check for unicode codepoints beyond 7-bit ASCII
            int ch_nbytes = u8_wc_nbytes(n);
            newsize = x->x_bufsize + ch_nbytes;
            x->x_buf = resizebytes(x->x_buf, x->x_bufsize, newsize);
            for(i = newsize-1; i > x->x_selstart; i--)
                x->x_buf[i] = x->x_buf[i-ch_nbytes];
            x->x_bufsize = newsize;
            // assume canvas_key() has encoded keysym as UTF-8
            strncpy(x->x_buf+x->x_selstart, x->x_keysym->s_name, ch_nbytes);
            x->x_selstart = x->x_selstart + ch_nbytes;
            x->x_start_ndx++, x->x_end_ndx++;
        }
        x->x_selend = x->x_selstart;
        x->x_glist->gl_editor->e_textdirty = 1;
    }
    else if(x->x_keysym == gensym("Home")){
        if(x->x_selend == x->x_selstart)
            x->x_selend = x->x_selstart = x->x_start_ndx = x->x_end_ndx = 0;
        else // selection
            x->x_selstart = x->x_start_ndx = x->x_end_ndx = 0;
    }
    else if(x->x_keysym == gensym("End")){
        if(x->x_selend == x->x_selstart){
            while(x->x_selstart < x->x_bufsize){
                u8_inc(x->x_buf, &x->x_selstart);
                x->x_start_ndx++, x->x_end_ndx++;
            }
            x->x_selend = x->x_selstart = x->x_bufsize;
        }
        else // selection
            x->x_selend = x->x_bufsize;
    }
    else if(x->x_keysym == gensym("Up")){ // go to start and deselect
        if(x->x_selstart){
            u8_dec(x->x_buf, &x->x_selstart);
            x->x_start_ndx--, x->x_end_ndx--;
        }
        while(x->x_selstart > 0 && x->x_buf[x->x_selstart] != '\n'){
            u8_dec(x->x_buf, &x->x_selstart);
            x->x_start_ndx--, x->x_end_ndx--;
        }
        x->x_selend = x->x_selstart;
    }
    else if(x->x_keysym == gensym("Down")){ // go to end and deselect
        while(x->x_selend < x->x_bufsize && x->x_buf[x->x_selend] != '\n'){
            u8_inc(x->x_buf, &x->x_selend);
            x->x_start_ndx++, x->x_end_ndx++;
        }
        if(x->x_selend < x->x_bufsize){
            u8_inc(x->x_buf, &x->x_selend);
            x->x_start_ndx++, x->x_end_ndx++;
        }
        x->x_selstart = x->x_selend;
    }
    else if(x->x_keysym == gensym("Right")){
        if(x->x_selend == x->x_selstart && x->x_selstart < x->x_bufsize){
            u8_inc(x->x_buf, &x->x_selstart);
            x->x_start_ndx++, x->x_end_ndx++;
            x->x_selend = x->x_selstart;
        }
        else{
//            x->x_selstart = x->x_selend;
//            post("else: x->x_selstart = x->x_selend = %d", x->x_selstart);
            while(x->x_selstart < x->x_selend){
                u8_inc(x->x_buf, &x->x_selstart);
                x->x_start_ndx++, x->x_end_ndx++;
            }
        }
    }
    else if(x->x_keysym == gensym("Left")){
        if(x->x_selend == x->x_selstart && x->x_selstart > 0){
            u8_dec(x->x_buf, &x->x_selstart);
            x->x_start_ndx--, x->x_end_ndx--;
            x->x_selend = x->x_selstart;
        }
        else
            x->x_selend = x->x_selstart;
    }
    else if(x->x_keysym == gensym("F5")){
        t_text *t = (t_text *)x;
        t_binbuf *bb = binbuf_new();
        int argc = binbuf_getnatom(x->x_binbuf);
        binbuf_addv(bb, "ii", (int)t->te_xpix + 5, (int)t->te_ypix + 5);
        binbuf_add(bb, argc, binbuf_getvec(x->x_binbuf));
        canvas_setcurrent(x->x_glist);
        typedmess((t_pd *)x->x_glist, gensym("text"), argc + 2, binbuf_getvec(bb));
        canvas_unsetcurrent(x->x_glist);
        binbuf_free(bb);
        return;
    }
    canvas_dirty(x->x_glist, 1);
    binbuf_text(x->x_binbuf, x->x_buf, x->x_bufsize);
//        post("x->x_selend = x->x_selstart = %d", x->x_selend);
    note_update(x);
}

static void note_float(t_note *x, t_float f){
    x->x_keynum = (int)f;
//    post("x->x_keynum = %d", x->x_keynum);
}

static void note_list(t_note *x, t_symbol *s, int ac, t_atom *av){
    t_symbol *dummy = s;
    dummy = NULL;
    if(ac > 1 && av->a_type == A_FLOAT && av[1].a_type == A_SYMBOL){
        int press = (int)av->a_w.w_float;
//        post("press = %d, symbol = %s", press, av[1].a_w.w_symbol->s_name);
        if(av[1].a_w.w_symbol == gensym("Shift_L")){
            x->x_shift = press;
//            post("shift = %d", press);
        }
        if(press){
            x->x_keysym = av[1].a_w.w_symbol;
            note_key(x);
        }
    }
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
            if(edit){
                note_draw_handle(p->p_cnv); // fix weird bug????
                note_draw_inlet(p->p_cnv);
                note_draw_outline(p->p_cnv);
            }
            else{
                t_canvas *cv = glist_getcanvas(p->p_cnv->x_glist);
                t_handle *ch = (t_handle *)p->p_cnv->x_handle;
                unsigned long x = (unsigned long)p->p_cnv;
                sys_vgui(".x%lx.c delete %lx_in\n", cv, x);
                sys_vgui(".x%lx.c delete %lx_outline\n", cv, x);
                sys_vgui("destroy %s\n", ch->h_pathname); // always destroy, bad hack, improve
            }
        }
    }
}

//---------------------------- METHODS ----------------------------------------
static void note_receive(t_note *x, t_symbol *s){
    if(s == gensym(""))
        s = gensym("empty");
    t_symbol *rcv = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
    if(rcv != x->x_receive){
        x->x_changed = 1;
        if(x->x_receive != &s_)
            pd_unbind(&x->x_obj.ob_pd, x->x_receive);
        x->x_rcv_set = 1;
        x->x_rcv_raw = s;
        x->x_receive = rcv;
        if(x->x_receive == &s_){
            if(x->x_edit && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
                note_draw_inlet(x);
        }
        else{
            pd_bind(&x->x_obj.ob_pd, x->x_receive);
            if(x->x_edit && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
                sys_vgui(".x%lx.c delete %lx_in\n", glist_getcanvas(x->x_glist), x);
        }
    }
}

static void note_set(t_note *x, t_symbol *s, int ac, t_atom * av){
    s = NULL;
//    post("set");
    binbuf_clear(x->x_binbuf);
    binbuf_restore(x->x_binbuf, ac, av);
    binbuf_gettext(x->x_binbuf, &x->x_buf, &x->x_bufsize);
    x->x_bbset = 0;
    note_redraw(x);
}

static void note_append(t_note *x, t_symbol *s, int ac, t_atom * av){
    s = NULL;
    if(!x->x_init) // hack???
        note_initialize(x);
    if(ac){
        int n = binbuf_getnatom(x->x_binbuf); // number of arguments
        t_atom* at = (t_atom *)getbytes((n+ac)*sizeof(t_atom));
        char buf[128];
        int i = 0;
        for(i = 0;  i < n; i++){
            atom_string(binbuf_getvec(x->x_binbuf) + i, buf, 128);
            SETSYMBOL(at+i, gensym(buf));
        }
        for(int j = 0; j < ac; j++)
            at[i+j] = av[j];
        binbuf_clear(x->x_binbuf);
        binbuf_restore(x->x_binbuf, n+ac, at);
        binbuf_gettext(x->x_binbuf, &x->x_buf, &x->x_bufsize);
        x->x_bbset = 0;
        note_redraw(x);
        freebytes(at, (n+ac)*sizeof(t_atom));
    }
}

static void note_prepend(t_note *x, t_symbol *s, int ac, t_atom * av){
//    post("prepend");
    s = NULL;
    if(!x->x_init) // hack???
        note_initialize(x);
    if(ac){
        int n = binbuf_getnatom(x->x_binbuf); // number of arguments
        t_atom* at = (t_atom *)getbytes((n+ac)*sizeof(t_atom));
        char buf[128];
        int i = 0;
        for(i = 0; i < ac; i++)
            at[i] = av[i];
        for(int j = 0;  j < n; j++){
            atom_string(binbuf_getvec(x->x_binbuf) + j, buf, 128);
            SETSYMBOL(at+i+j, gensym(buf));
        }
        binbuf_clear(x->x_binbuf);
        binbuf_restore(x->x_binbuf, n+ac, at);
        binbuf_gettext(x->x_binbuf, &x->x_buf, &x->x_bufsize);
        x->x_bbset = 0;
        note_redraw(x);
        freebytes(at, (n+ac)*sizeof(t_atom));
    }
}

static void note_textcolor(t_note *x, t_floatarg r, t_floatarg g, t_floatarg b){
    unsigned char red = r < 0 ? 0 : r > 255 ? 255 : (unsigned char)r;
    unsigned char green = g < 0 ? 0 : g > 255 ? 255 : (unsigned char)g;
    unsigned char blue = b < 0 ? 0 : b > 255 ? 255 : (unsigned char)b;
    if(x->x_red != red || x->x_green != green || x->x_blue != blue){
        sprintf(x->x_color, "#%2.2x%2.2x%2.2x", x->x_red = red, x->x_green = green, x->x_blue = blue);
        if(gobj_shouldvis((t_gobj *)x, x->x_glist) && glist_isvisible(x->x_glist))
            sys_vgui(".x%lx.c itemconfigure txt%lx -fill %s\n", x->x_cv, (unsigned long)x, x->x_color);
    }
}

static void note_bgcolor(t_note *x, t_float r, t_float g, t_float b){
    unsigned char red = r < 0 ? 0 : r > 255 ? 255 : (unsigned char)r;
    unsigned char green = g < 0 ? 0 : g > 255 ? 255 : (unsigned char)g;
    unsigned char blue = b < 0 ? 0 : b > 255 ? 255 : (unsigned char)b;
    if(!x->x_bg_flag){
        x->x_bg_flag = 1;
        sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0] = red, x->x_bg[1] = green, x->x_bg[2] = blue);
        x->x_bbset = 0;
        note_redraw(x); // why redraw???
    }
    else if(x->x_bg[0] != red || x->x_bg[1] != green || x->x_bg[2] != blue){
        sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0] = red, x->x_bg[1] = green, x->x_bg[2] = blue);
        if(gobj_shouldvis((t_gobj *)x, x->x_glist) && glist_isvisible(x->x_glist))
            sys_vgui(".x%lx.c itemconfigure bg%lx -fill %s\n", x->x_cv, (unsigned long)x, x->x_bgcolor);
    }
}

static void note_fontname(t_note *x, t_symbol *name){
    if(name != x->x_fontname){
        x->x_fontname = name;
        x->x_bbset = 0;
        note_redraw(x);
    }
}

static void note_fontsize(t_note *x, t_floatarg f){
    int size = (int)f < 5 ? 5 : (int)f;
    if(x->x_fontsize != size){
        x->x_fontsize = size;
        x->x_bbset = 0;
        note_redraw(x);
    }
}

static void note_italic(t_note *x, t_float f){
    int italic = f != 0;
    if(italic != x->x_italic){
        x->x_italic = italic;
        x->x_bbset = 0;
        note_redraw(x);
    }
}

static void note_bold(t_note *x, t_float f){
    int bold = f != 0;
    if(bold != x->x_bold){
        x->x_bold = bold;
        x->x_bbset = 0;
        note_redraw(x);
    }
}


static void note_bg_flag(t_note *x, t_float f){
    int bgflag = (int)(f != 0);
    if(x->x_bg_flag != bgflag){
        x->x_bg_flag = bgflag;
        x->x_bbset = 0;
        note_redraw(x);
    }
}

static void note_underline(t_note *x, t_float f){
    int underline = (int)(f != 0);
    if(x->x_underline != underline){
        x->x_underline = underline;
        x->x_bbset = 0;
        note_redraw(x); // itemconfigure?
    }
}

static void note_just(t_note *x, t_float f){
    int just = f < 0 ? 0 : (f > 2 ? 2 : (int)f);
    if(just != x->x_textjust){
        x->x_textjust = just;
        x->x_bbset = 0;
        note_redraw(x); // itemconfigure?
    }
}

static void note_zoom(t_note *x, t_floatarg zoom){
    x->x_zoom = (int)zoom;
    float mul = zoom == 1. ? 0.5 : 2.;
    x->x_max_pixwidth = (int)((float)x->x_max_pixwidth * mul);
    float fontsize = (float)x->x_fontsize * mul;
    note_fontsize(x, fontsize);
}

//------------------- Properties --------------------------------------------------------
void note_properties(t_gobj *z, t_glist *gl){
    note_select(z, gl, 0);
    t_note *x = (t_note *)z;
    note_get_rcv(x);
    char buffer[512];
    sprintf(buffer, "note_properties %%s {%s} %d %d %d %d %d {%s} %d %d %d %d %d %d \n",
        x->x_fontname->s_name,
        x->x_fontsize,
        x->x_fontface,
        x->x_textjust,
        x->x_underline,
        x->x_bg_flag,
        x->x_rcv_raw->s_name,
        x->x_bg[0],
        x->x_bg[1],
        x->x_bg[2],
        x->x_red,
        x->x_green,
        x->x_blue);
    gfxstub_new(&x->x_obj.ob_pd, x, buffer);
}

static void note_ok(t_note *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    x->x_changed = 0;
    t_float temp_f;
    if(atom_getsymbolarg(0, ac, av) != x->x_fontname){
        x->x_changed = 1;
        x->x_fontname = atom_getsymbolarg(0, ac, av);
    }
    temp_f = atom_getfloatarg(1, ac, av);
    if(temp_f < 5)
        temp_f = 5;
    if(temp_f != x->x_fontsize){
        x->x_changed = 1;
        x->x_fontsize = (int)temp_f;
    }
    temp_f = atom_getfloatarg(2, ac, av);
    int face = temp_f < 0 ? 0 : temp_f > 3 ? 3  : (int)temp_f;
    if(face != x->x_fontface){
        x->x_changed = 1;
        x->x_fontface = face;
        x->x_bold = x->x_fontface == 1 || x->x_fontface == 3;
        x->x_italic = x->x_fontface > 1;
    }
    temp_f = atom_getfloatarg(3, ac, av);
    int just = temp_f < 0 ? 0 : (temp_f > 2 ? 2 : (int)temp_f);
    if(just != x->x_textjust){
        x->x_changed = 1;
        x->x_textjust = just;
    }
    int underline = (int)(atom_getfloatarg(4, ac, av) != 0);
    if(x->x_underline != underline){
        x->x_changed = 1;
        x->x_underline = underline;
    }
    int bgflag = (int)(atom_getfloatarg(5, ac, av) != 0);
    if(x->x_bg_flag != bgflag){
        x->x_bg_flag = bgflag;
        x->x_changed = 1;
    }
    temp_f = atom_getfloatarg(6, ac, av);
    unsigned int bgr = temp_f < 0 ? 0 : temp_f > 255 ? 255 : (unsigned int)temp_f;
    temp_f = atom_getfloatarg(7, ac, av);
    unsigned int bgg = temp_f < 0 ? 0 : temp_f > 255 ? 255 : (unsigned int)temp_f;
    temp_f = atom_getfloatarg(8, ac, av);
    unsigned int bgb = temp_f < 0 ? 0 : temp_f > 255 ? 255 : (unsigned int)temp_f;
    if(x->x_bg[0] != bgr || x->x_bg[1] != bgg || x->x_bg[2] != bgb){
        x->x_changed = 1;
        sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0] = bgr, x->x_bg[1] = bgg, x->x_bg[2] = bgb);
    }
    temp_f = atom_getfloatarg(9, ac, av);
    unsigned int fgr = temp_f < 0 ? 0 : temp_f > 255 ? 255 : (unsigned int)temp_f;
    temp_f = atom_getfloatarg(10, ac, av);
    unsigned int fgg = temp_f < 0 ? 0 : temp_f > 255 ? 255 : (unsigned int)temp_f;
    temp_f = atom_getfloatarg(11, ac, av);
    unsigned int fgb = temp_f < 0 ? 0 : temp_f > 255 ? 255 : (unsigned int)temp_f;
    if(x->x_red != fgr || x->x_green != fgg || x->x_blue != fgb){
        x->x_changed = 1;
        sprintf(x->x_color, "#%2.2x%2.2x%2.2x", x->x_red = fgr, x->x_green = fgg, x->x_blue = fgb);
    }
    note_receive(x, atom_getsymbolarg(12, ac, av));
    if(x->x_changed){
        canvas_dirty(x->x_glist, 1);
        note_redraw(x);
    }
}

//-------------------------------------------------------------------------------------
static void edit_proxy_free(t_edit_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_edit_proxy *edit_proxy_new(t_note *x, t_symbol *s){
    t_edit_proxy *p = (t_edit_proxy*)pd_new(edit_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)edit_proxy_free);
    return(p);
}

static void note_free(t_note *x){
    if(x->x_active){
        pd_unbind((t_pd *)x, gensym("#key"));
        pd_unbind((t_pd *)x, gensym("#keyname"));
    }
    if(x->x_receive != &s_)
        pd_unbind(&x->x_obj.ob_pd, x->x_receive);
    if(x->x_bindsym){
        pd_unbind((t_pd *)x, x->x_bindsym);
        if(!x->x_bbpending){
//            post("!x->x_bbpending: pd_unbind(notesink)");
            pd_unbind(notesink, x->x_bindsym);
        }
    }
//    if(x->x_binbuf) // init?
        binbuf_free(x->x_binbuf);
    if(x->x_handle){
        pd_unbind(x->x_handle, ((t_handle *)x->x_handle)->h_bindsym);
        pd_free(x->x_handle);
    }
    if(x->x_buf)
        freebytes(x->x_buf, x->x_bufsize);
    x->x_proxy->p_cnv = NULL;
    gfxstub_deleteforkey(x);
}

static void *note_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_note *x = (t_note *)pd_new(note_class);
    x->x_handle = pd_new(handle_class);
    t_handle *ch = (t_handle *)x->x_handle;
    ch->h_master = x;
    char hbuf[64];
    sprintf(hbuf, "_h%lx", (unsigned long)ch);
    pd_bind(x->x_handle, ch->h_bindsym = gensym(hbuf));
    t_text *t = (t_text *)x;
    t->te_type = T_TEXT;
    x->x_glist = canvas_getcurrent();
    x->x_cv = canvas_getcurrent();
    x->x_zoom = x->x_glist->gl_zoom;
    x->x_fontname = gensym(default_font);
    x->x_edit = x->x_glist->gl_edit;
    x->x_buf = 0;
    x->x_keynum = 0;
    x->x_keysym = NULL;
    x->x_rcv_set = x->x_flag = x->x_r_flag = x->x_old = 0;
    x->x_text_n = x->x_text_size = x->x_text_width = 0;
    x->x_max_pixwidth = 425;
    x->x_width = x->x_height = 0;
    x->x_fontsize = 0;
    x->x_bbpending = 0;
    x->x_textjust = x->x_fontface = x->x_bold = x->x_italic = 0;
    x->x_red = x->x_green = x->x_blue = x->x_bufsize = 0;
    x->x_bg_flag = x->x_changed = x->x_init = x->x_resized = 0;
    x->x_bg[0] = x->x_bg[1] = x->x_bg[2] = 255;
    sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0], x->x_bg[1], x->x_bg[2]);
    x->x_bbset = x->x_select = x->x_dragon = x->x_shift = 0;
    t_symbol *rcv = x->x_receive = x->x_rcv_raw = &s_;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_cv);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = edit_proxy_new(x, gensym(buf));
    sprintf(buf, "#%lx", (long)x);
    char symbuf[32];
    sprintf(symbuf, "note%lx", (unsigned long)x);
    x->x_bindsym = gensym(symbuf);
    pd_bind((t_pd *)x, x->x_bindsym);
    if(!notesink){
//        post("new: !notesink");
        notesink = pd_new(notesink_class);
    }
    pd_bind(notesink, x->x_bindsym);
    x->x_binbuf = binbuf_new();
    t_atom at;
    SETSYMBOL(&at, gensym("note"));
    binbuf_restore(x->x_binbuf, 1, &at);
////////////////////////////////// GET ARGS ///////////////////////////////////////////
    if(ac){
        if(ac && av->a_type == A_FLOAT){ // 1ST Width
            x->x_max_pixwidth = (int)av->a_w.w_float;
            ac--, av++;
            if(ac && av->a_type == A_FLOAT){ // 2ND Size
                x->x_fontsize = (int)av->a_w.w_float * x->x_zoom;
                ac--, av++;
                if(ac && av->a_type == A_SYMBOL){ // 3RD type
                    x->x_fontname = av->a_w.w_symbol;
                    ac--, av++;
                    if(ac && av->a_type == A_SYMBOL){ // 4TH RECEIVE
                        if(av->a_w.w_symbol != gensym("empty")){
                            //  'empty' sets empty receive symbol
                            rcv = av->a_w.w_symbol;
                            ac--, av++;
                        }
                        else // ignore floats
                            ac--, av++;
                        if(ac && av->a_type == A_FLOAT){ // 5th face
                            x->x_fontface = (int)av->a_w.w_float;
                            ac--, av++;
                            if(ac && av->a_type == A_FLOAT){ // 6th R
                                x->x_red = (unsigned char)av->a_w.w_float;
                                ac--, av++;
                                if(ac && av->a_type == A_FLOAT){ // 7th G
                                    x->x_green = (unsigned char)av->a_w.w_float;
                                    ac--, av++;
                                    if(ac && av->a_type == A_FLOAT){ // 8th B
                                        x->x_blue = (unsigned char)av->a_w.w_float;
                                        ac--, av++;
                                        if(ac && av->a_type == A_FLOAT){ // 9th Underline
                                            x->x_underline = (int)(av->a_w.w_float != 0);
                                            ac--, av++;
                                            if(ac && av->a_type == A_FLOAT){ // 10th R
                                                x->x_bg[0] = (unsigned int)av->a_w.w_float;
                                                ac--, av++;
                                                if(ac && av->a_type == A_FLOAT){ // 11th G
                                                    x->x_bg[1] = (unsigned int)av->a_w.w_float;
                                                    ac--, av++;
                                                    if(ac && av->a_type == A_FLOAT){ // 12th B
                                                        x->x_bg[2] = (unsigned int)av->a_w.w_float;
                                                        ac--, av++;
                                                        if(ac && av->a_type == A_FLOAT){ // 13th bg flag
                                                            x->x_bg_flag = (int)(av->a_w.w_float != 0);
                                                            ac--, av++;
                                                            if(ac && av->a_type == A_FLOAT){ // 14th Justification
                                                                int textjust = (int)(av->a_w.w_float);
                                                                x->x_textjust = textjust < 0 ? 0 : textjust > 2 ? 2 : textjust;
                                                                ac--, av++;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else{ // 1st is not a float, so we must be dealing with attributes!!!
            int i, comlen = 0; // length of note list
            for(i = 0; i < ac; i++){
                if(av[i].a_type == A_SYMBOL){
                    t_symbol * sym = av[i].a_w.w_symbol;
                    if(sym == gensym("-size")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_fontsize = (int)av[i].a_w.w_float;
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-font")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_SYMBOL)
                                x->x_fontname = av[i].a_w.w_symbol;
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-receive")){
                        if((ac-i) >= 2){
                            x->x_flag = x->x_r_flag = 1, i++;
                            if(av[i].a_type == A_SYMBOL)
                                rcv = atom_getsymbolarg(i, ac, av);
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-color")){
                        if((ac-i) >= 4){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_red = (unsigned char)av[i].a_w.w_float, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_green = (unsigned char)av[i].a_w.w_float, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_blue = (unsigned char)av[i].a_w.w_float;
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-bgcolor")){
                        if((ac-i) >= 4){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_bg[0] = (unsigned char)av[i].a_w.w_float, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_bg[1] = (unsigned char)av[i].a_w.w_float, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_bg[2] = (unsigned char)av[i].a_w.w_float;
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-bold")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_bold  = (int)av[i].a_w.w_float != 0;
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-italic")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_italic  = (int)av[i].a_w.w_float != 0;
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-underline")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_underline = (int)(av[i].a_w.w_float != 0);
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-bg")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_bg_flag = (int)(av[i].a_w.w_float != 0);
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-just")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT){
                                int textjust = (int)av[i].a_w.w_float;
                                x->x_textjust = textjust < 0 ? 0 : textjust > 2 ? 2 : textjust;
                            }
                            else
                                goto errstate;
                        }
                        else
                            goto errstate;
                    }
                    else if(sym == gensym("-note")){
                        if((ac-i) >= 2){
                            x->x_flag = x->x_text_flag = 1;
                            i++, comlen++;
                            x->x_text_n = i;
                        };
                    }
                    else if(x->x_text_flag){ // treat it as a part of comlist
                        comlen++;
                    }
                    else
                        goto errstate;
                }
                else if(av[i].a_type == A_FLOAT){
                    if(x->x_text_flag)
                        comlen++;
                    else
                        goto errstate;
                }
            }
            x->x_text_size = comlen;
        }
    }
    if(x->x_fontsize < 1)
        x->x_fontsize = glist_getfont(x->x_glist);
    if(x->x_max_pixwidth <= 0)
        x->x_max_pixwidth = 425;
    if(x->x_max_pixwidth != 425) // improve
        x->x_resized = 1;
    x->x_fontsize *= x->x_zoom;
    x->x_max_pixwidth *= x->x_zoom;
    x->x_fontface = x->x_fontface < 0 ? 0 : (x->x_fontface > 3 ? 3 : x->x_fontface);
    x->x_bold = x->x_fontface == 1 || x->x_fontface == 3;
    x->x_italic = x->x_fontface > 1;
    x->x_red = x->x_red > 255 ? 255 : x->x_red < 0 ? 0 : x->x_red;
    x->x_green = x->x_green > 255 ? 255 : x->x_green < 0 ? 0 : x->x_green;
    x->x_blue = x->x_blue > 255 ? 255 : x->x_blue < 0 ? 0 : x->x_blue;
    sprintf(x->x_color, "#%2.2x%2.2x%2.2x", x->x_red, x->x_green, x->x_blue);
    x->x_bg[0] = x->x_bg[0] > 255 ? 255 : x->x_bg[0] < 0 ? 0 : x->x_bg[0];
    x->x_bg[1] = x->x_bg[1] > 255 ? 255 : x->x_bg[1] < 0 ? 0 : x->x_bg[1];
    x->x_bg[2] = x->x_bg[2] > 255 ? 255 : x->x_bg[2] < 0 ? 0 : x->x_bg[2];
    sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0], x->x_bg[1], x->x_bg[2]);
    x->x_receive = canvas_realizedollar(x->x_glist, x->x_rcv_raw = rcv);
    if(x->x_receive != &s_)
        pd_bind(&x->x_obj.ob_pd, x->x_receive);
    return(x);
    errstate:
        pd_error(x, "[note]: improper creation arguments");
        return(NULL);
}

void note_setup(void){
    note_class = class_new(gensym("note"), (t_newmethod)note_new, (t_method)note_free,
        sizeof(t_note), CLASS_DEFAULT, A_GIMME, 0);
    class_addfloat(note_class, note_float);
    class_addlist(note_class, note_list);
    class_addmethod(note_class, (t_method)note_fontname, gensym("font"), A_SYMBOL, 0);
    class_addmethod(note_class, (t_method)note_receive, gensym("receive"), A_SYMBOL, 0);
    class_addmethod(note_class, (t_method)note_fontsize, gensym("size"), A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_set, gensym("set"), A_GIMME, 0);
    class_addmethod(note_class, (t_method)note_append, gensym("append"), A_GIMME, 0);
    class_addmethod(note_class, (t_method)note_prepend, gensym("prepend"), A_GIMME, 0);
    class_addmethod(note_class, (t_method)note_bold, gensym("bold"), A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_italic, gensym("italic"), A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_underline, gensym("underline"), A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_just, gensym("just"), A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_textcolor, gensym("color"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_bgcolor, gensym("bgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_bg_flag, gensym("bg"), A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note_zoom, gensym("zoom"), A_CANT, 0);
    class_addmethod(note_class, (t_method)note_ok, gensym("ok"), A_GIMME, 0);
    class_addmethod(note_class, (t_method)note__bbox_callback, gensym("_bbox"), A_SYMBOL, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(note_class, (t_method)note__click_callback, gensym("_click"), A_GIMME, 0);
    class_setwidget(note_class, &note_widgetbehavior);
    note_widgetbehavior.w_getrectfn  = note_getrect;
    note_widgetbehavior.w_displacefn = note_displace;
    note_widgetbehavior.w_selectfn   = note_select;
    note_widgetbehavior.w_activatefn = note_activate;
    note_widgetbehavior.w_deletefn   = note_delete;
    note_widgetbehavior.w_visfn      = note_vis;
    class_setsavefn(note_class, note_save);
    class_setpropertiesfn(note_class, note_properties);
    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(edit_proxy_class, edit_proxy_any);
    notesink_class = class_new(gensym("_notesink"), 0, 0, sizeof(t_pd), CLASS_PD, 0);
    class_addanything(notesink_class, notesink_anything);
    class_addmethod(notesink_class, (t_method)notesink__bbox_callback, gensym("_bbox"), A_SYMBOL, 0); // <= ?????
    
    handle_class = class_new(gensym("_handle"), 0, 0, sizeof(t_handle), CLASS_PD, 0);
    class_addmethod(handle_class, (t_method)handle__click_callback, gensym("_click"), A_FLOAT, 0);
    class_addmethod(handle_class, (t_method)handle__motion_callback, gensym("_motion"), A_FLOAT, A_FLOAT, 0);
    sys_gui("proc note_bbox {target cvname tag} {\n\
            pdsend \"$target _bbox $target [$cvname bbox $tag]\"}\n");
// LATER think about window vs canvas coords
    sys_gui("proc note_click {target cvname x y tag} {\n\
            pdsend \"$target _click $target [$cvname canvasx $x] [$cvname canvasy $y]\
            [$cvname index $tag @$x,$y] [$cvname bbox $tag]\"}\n");
    
    sys_gui("proc note_update {cv tag tt wd} {\n\
            if {$wd > 0} {$cv itemconfig $tag -text $tt -width $wd} else {\n\
            $cv itemconfig $tag -text $tt}}\n");
    sys_gui("proc note_draw {tgt cv tag1 tag2 x y fnm fsz clr tt wd wt sl just} {\n\
            if {$wd > 0} {\n\
            $cv create text $x $y -text $tt -tags [list $tag1 $tag2] \
            -font [list $fnm $fsz $wt $sl] -justify $just -fill $clr -width $wd -anchor nw} else {\n\
            $cv create text $x $y -text $tt -tags [list $tag1 $tag2] \
            -font [list $fnm $fsz $wt $sl] -justify $just -fill $clr -anchor nw}\n\
            note_bbox $tgt $cv $tag1\n\
            $cv bind $tag1 <Button> [list note_click $tgt %W %x %y $tag1]}\n");
// later rethink how to make both into a single section:
    sys_gui("proc note_draw_ul {tgt cv tag1 tag2 x y fnm fsz clr tt wd wt sl just} {\n\
            if {$wd > 0} {\n\
            $cv create text $x $y -text $tt -tags [list $tag1 $tag2] \
            -font [list $fnm $fsz $wt $sl underline] -justify $just -fill $clr -width $wd -anchor nw} else {\n\
            $cv create text $x $y -text $tt -tags [list $tag1 $tag2] \
            -font [list $fnm $fsz $wt $sl underline] -justify $just -fill $clr -anchor nw}\n\
            note_bbox $tgt $cv $tag1\n\
            $cv bind $tag1 <Button> [list note_click $tgt %W %x %y $tag1]}\n");
// properties
    sys_vgui("if {[catch {pd}]} {\n");
    sys_vgui("    proc pd {args} {pdsend [join $args \" \"]}\n");
    sys_vgui("}\n");
    sys_vgui("proc note_ok {id} {\n");
    sys_vgui("    set vid [string trimleft $id .]\n");
    sys_vgui("    set var_name [concat var_name_$vid]\n");
    sys_vgui("    set var_size [concat var_size_$vid]\n");
    sys_vgui("    set var_face [concat var_face_$vid]\n");
    sys_vgui("    set var_just [concat var_just_$vid]\n");
    sys_vgui("    set var_underline [concat var_underline_$vid]\n");
    sys_vgui("    set var_bg_flag [concat var_bg_flag_$vid]\n");
    sys_vgui("    set var_bgr [concat var_bgr_$vid]\n");
    sys_vgui("    set var_bgg [concat var_bgg_$vid]\n");
    sys_vgui("    set var_bgb [concat var_bgb_$vid]\n");
    sys_vgui("    set var_fgr [concat var_fgr_$vid]\n");
    sys_vgui("    set var_fgg [concat var_fgg_$vid]\n");
    sys_vgui("    set var_fgb [concat var_fgb_$vid]\n");
    sys_vgui("    set var_rcv [concat var_rcv_$vid]\n");
    sys_vgui("\n");
    sys_vgui("    global $var_name\n");
    sys_vgui("    global $var_size\n");
    sys_vgui("    global $var_face\n");
    sys_vgui("    global $var_just\n");
    sys_vgui("    global $var_underline\n");
    sys_vgui("    global $var_bg_flag\n");
    sys_vgui("    global $var_bgr\n");
    sys_vgui("    global $var_bgg\n");
    sys_vgui("    global $var_bgb\n");
    sys_vgui("    global $var_fgr\n");
    sys_vgui("    global $var_fgg\n");
    sys_vgui("    global $var_fgb\n");
    sys_vgui("    global $var_rcv\n");
    sys_vgui("\n");
    sys_vgui("    set cmd [concat $id ok \\\n");
    sys_vgui("        [string map {\" \" {\\ } \";\" \"\" \",\" \"\" \"\\\\\" \"\" \"\\{\" \"\" \"\\}\" \"\"} [eval concat $$var_name]] \\\n");
    sys_vgui("        [eval concat $$var_size] \\\n");
    sys_vgui("        [eval concat $$var_face] \\\n");
    sys_vgui("        [eval concat $$var_just] \\\n");
    sys_vgui("        [eval concat $$var_underline] \\\n");
    sys_vgui("        [eval concat $$var_bg_flag] \\\n");
    sys_vgui("        [eval concat $$var_bgr] \\\n");
    sys_vgui("        [eval concat $$var_bgg] \\\n");
    sys_vgui("        [eval concat $$var_bgb] \\\n");
    sys_vgui("        [eval concat $$var_fgr] \\\n");
    sys_vgui("        [eval concat $$var_fgg] \\\n");
    sys_vgui("        [eval concat $$var_fgb] \\\n");
    sys_vgui("        [string map {\"$\" {\\$} \" \" {\\ } \";\" \"\" \",\" \"\" \"\\\\\" \"\" \"\\{\" \"\" \"\\}\" \"\"} [eval concat $$var_rcv]] \\;]\n");
    sys_vgui("    pd $cmd\n");
    sys_vgui("    note_cancel $id\n");
    sys_vgui("}\n");
    sys_vgui("proc note_cancel {id} {\n");
    sys_vgui("    set cmd [concat $id cancel \\;]\n");
    sys_vgui("    pd $cmd\n");
    sys_vgui("}\n");
    sys_vgui("proc note_properties {id name size face just underline bg_flag rcv bgr bgg bgb fgr fgg fgb} {\n");
    sys_vgui("    set vid [string trimleft $id .]\n");
    sys_vgui("    set var_name [concat var_name_$vid]\n");
    sys_vgui("    set var_size [concat var_size_$vid]\n");
    sys_vgui("    set var_face [concat var_face_$vid]\n");
    sys_vgui("    set var_just [concat var_just_$vid]\n");
    sys_vgui("    set var_underline [concat var_underline_$vid]\n");
    sys_vgui("    set var_bg_flag [concat var_bg_flag_$vid]\n");
    sys_vgui("    set var_rcv [concat var_rcv_$vid]\n");
    sys_vgui("    set var_bgr [concat var_bgr_$vid]\n");
    sys_vgui("    set var_bgg [concat var_bgg_$vid]\n");
    sys_vgui("    set var_bgb [concat var_bgb_$vid]\n");
    sys_vgui("    set var_fgr [concat var_fgr_$vid]\n");
    sys_vgui("    set var_fgg [concat var_fgg_$vid]\n");
    sys_vgui("    set var_fgb [concat var_fgb_$vid]\n");
    sys_vgui("\n");
    sys_vgui("    global $var_name\n");
    sys_vgui("    global $var_size\n");
    sys_vgui("    global $var_face\n");
    sys_vgui("    global $var_just\n");
    sys_vgui("    global $var_underline\n");
    sys_vgui("    global $var_bg_flag\n");
    sys_vgui("    global $var_rcv\n");
    sys_vgui("    global $var_bgr\n");
    sys_vgui("    global $var_bgg\n");
    sys_vgui("    global $var_bgb\n");
    sys_vgui("    global $var_fgr\n");
    sys_vgui("    global $var_fgg\n");
    sys_vgui("    global $var_fgb\n");
    sys_vgui("\n");
    sys_vgui("    set $var_name [string map {{\\ } \" \"} $name]\n"); // remove escape from space
    sys_vgui("    set $var_size $size\n");
    sys_vgui("    set $var_face $face\n");
    sys_vgui("    set $var_just $just\n");
    sys_vgui("    set $var_underline $underline\n");
    sys_vgui("    set $var_bg_flag $bg_flag\n");
    sys_vgui("    if {$rcv == \"empty\"} {set $var_rcv [format \"\"]} else {set $var_rcv [string map {{\\ } \" \"} $rcv]}\n");
    sys_vgui("    set $var_bgr $bgr\n");
    sys_vgui("    set $var_bgg $bgg\n");
    sys_vgui("    set $var_bgb $bgb\n");
    sys_vgui("    set $var_fgr $fgr\n");
    sys_vgui("    set $var_fgg $fgg\n");
    sys_vgui("    set $var_fgb $fgb\n");
    sys_vgui("\n");
    sys_vgui("    toplevel $id\n");
    sys_vgui("    wm title $id {[note] Properties}\n");
    sys_vgui("    wm protocol $id WM_DELETE_WINDOW [concat note_cancel $id]\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.name_size\n");
    sys_vgui("    pack $id.name_size -side top\n");
    sys_vgui("    label $id.name_size.lname -text \"Font Name:\"\n");
    sys_vgui("    entry $id.name_size.name -textvariable $var_name -width 30\n");
    sys_vgui("    label $id.name_size.lsize -text \"Font Size:\"\n");
    sys_vgui("    entry $id.name_size.size -textvariable $var_size -width 3\n");
    sys_vgui("    pack $id.name_size.lname $id.name_size.name $id.name_size.lsize $id.name_size.size -side left\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.face_just\n");
    sys_vgui("    pack $id.face_just -side top\n");
    sys_vgui("    label $id.face_just.lface -text \"Font Face:\"\n");
    sys_vgui("    entry $id.face_just.face -textvariable $var_face -width 1\n");
    sys_vgui("    label $id.face_just.ljust -text \"Justification:\"\n");
    sys_vgui("    entry $id.face_just.just -textvariable $var_just -width 1\n");
    sys_vgui("    pack $id.face_just.lface $id.face_just.face $id.face_just.ljust $id.face_just.just -side left\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.ul_bg\n");
    sys_vgui("    pack $id.ul_bg -side top\n");
    sys_vgui("    label $id.ul_bg.lul -text \"Underline:\"\n");
    sys_vgui("    checkbutton $id.ul_bg.ul -variable $var_underline \n");
    sys_vgui("    label $id.ul_bg.lbg -text \"Background:\"\n");
    sys_vgui("    checkbutton $id.ul_bg.bg -variable $var_bg_flag \n");
    sys_vgui("    pack $id.ul_bg.lul $id.ul_bg.ul $id.ul_bg.lbg $id.ul_bg.bg -side left\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.rcv_sym\n");
    sys_vgui("    pack $id.rcv_sym -side top\n");
    sys_vgui("    label $id.rcv_sym.lrcv -text \"Receive symbol:\"\n");
    sys_vgui("    entry $id.rcv_sym.rcv -textvariable $var_rcv -width 12\n");
    sys_vgui("    pack $id.rcv_sym.lrcv $id.rcv_sym.rcv -side left\n");
    sys_vgui("\n");
// colors
    sys_vgui("    frame $id.bg\n");
    sys_vgui("    pack $id.bg -side top\n");
    sys_vgui("    label $id.bg.lbgr -text \"BG Color: R\"\n");
    sys_vgui("    entry $id.bg.bgr -textvariable $var_bgr -width 3\n");
    sys_vgui("    label $id.bg.lbgg -text \"G\"\n");
    sys_vgui("    entry $id.bg.bgg -textvariable $var_bgg -width 3\n");
    sys_vgui("    label $id.bg.lbgb -text \"B\"\n");
    sys_vgui("    entry $id.bg.bgb -textvariable $var_bgb -width 3\n");
    sys_vgui("    pack $id.bg.lbgr $id.bg.bgr $id.bg.lbgg $id.bg.bgg $id.bg.lbgb $id.bg.bgb -side left\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.fg\n");
    sys_vgui("    pack $id.fg -side top\n");
    sys_vgui("    label $id.fg.lfgr -text \"Font Color: R\"\n");
    sys_vgui("    entry $id.fg.fgr -textvariable $var_fgr -width 3\n");
    sys_vgui("    label $id.fg.lfgg -text \"G\"\n");
    sys_vgui("    entry $id.fg.fgg -textvariable $var_fgg -width 3\n");
    sys_vgui("    label $id.fg.lfgb -text \"B\"\n");
    sys_vgui("    entry $id.fg.fgb -textvariable $var_fgb -width 3\n");
    sys_vgui("    pack $id.fg.lfgr $id.fg.fgr $id.fg.lfgg $id.fg.fgg $id.fg.lfgb $id.fg.fgb -side left\n");
    sys_vgui("\n");
    
    sys_vgui("    frame $id.buttonframe\n");
    sys_vgui("    pack $id.buttonframe -side bottom -fill x -pady 2m\n");
    sys_vgui("    button $id.buttonframe.cancel -text {Cancel} -command \"note_cancel $id\"\n");
    sys_vgui("    button $id.buttonframe.ok -text {OK} -command \"note_ok $id\"\n");
    sys_vgui("    pack $id.buttonframe.cancel -side left -expand 1\n");
    sys_vgui("    pack $id.buttonframe.ok -side left -expand 1\n");
    sys_vgui("}\n");
}
