// By Porres and Tim Schoen

#include "m_pd.h"
#include "g_canvas.h"
#include "magic.h"
#include <stdlib.h>
#include <string.h>

#include "compat.h"

#define MINDIGITS      1
#define MAX_NUMBOX_LEN 32
#define MINSIZE        8

#if __APPLE__
char def_font[100] = "Menlo";
#else
char def_font[100] = "DejaVu Sans Mono";
#endif

typedef struct _numbox{
    t_object  x_obj;
    t_clock  *x_clock_update;
    t_symbol *x_fg;
    t_symbol *x_bg;
    t_glist  *x_glist;
    t_float   x_display;
    t_float   x_in_val;
    t_float   x_out_val;
    t_float   x_set_val;
    t_float   x_max;
    t_float   x_min;
    t_float   x_sr_khz;
    t_float   x_inc;
    t_float   x_ramp_step;
    t_float   x_ramp_val;
    int       x_ramp_ms;
    int       x_rate;
    int       x_numwidth;
    int       x_fontsize;
    int       x_clicked;
    int       x_width, x_height;
    int       x_zoom;
    int       x_outmode;
    char      x_buf[MAX_NUMBOX_LEN]; // number buffer
}t_numbox;

static t_class *numbox_class;
t_widgetbehavior numbox_widgetbehavior;

//////////////////////////////////////////////////////// Helper and Drawing functions ////////////////////////////////////////////////////////////////
char *set_x_buf(t_numbox *x){
    sprintf(x->x_buf, "~%g", x->x_display = x->x_outmode ? x->x_out_val : x->x_in_val);
    int bufsize = (int)strlen(x->x_buf), i, e;
    int real_numwidth = x->x_numwidth + 1;
    if(bufsize > real_numwidth){ // reduce
        for(i = 0; i < bufsize; i++)
            if(x->x_buf[i] == '.')
                break;
        for(e = 0; e < bufsize; e++)
            if(x->x_buf[e] == 'e' || x->x_buf[e] == 'E')
                break;
        if(i >= real_numwidth || e < bufsize)
            x->x_buf[real_numwidth-1] = '>';
        x->x_buf[real_numwidth] = 0;
    }
    return(x->x_buf);
}

static void numbox_draw_number(t_numbox *x){ // update number value
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
        t_canvas *cv = glist_getcanvas(x->x_glist);
        if(x->x_clicked && x->x_buf[0] && x->x_outmode){ // keyboard input values
            char *cp = x->x_buf;
            int sl = (int)strlen(x->x_buf);
            x->x_buf[sl] = '>';
            x->x_buf[sl+1] = 0;
            if(sl >= (x->x_numwidth + 1))
                cp += sl - x->x_numwidth + 1;
            sys_vgui(".x%lx.c itemconfigure %lxNUM -text {%s}\n", cv, x, cp);
            x->x_buf[sl] = 0;
        }
        else{ // plain update
            sys_vgui(".x%lx.c itemconfigure %lxNUM -text {%s}\n", cv, x, set_x_buf(x));
            x->x_buf[0] = 0;
        }
    }
}

static void numbox_resize(t_numbox *x){
    int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
    sys_vgui(".x%lx.c coords %lxBASE %d %d %d %d\n", glist_getcanvas(x->x_glist), x,
        xpos, ypos, xpos+x->x_width*x->x_zoom, ypos+x->x_height*x->x_zoom);
}

static void clock_update(t_numbox *x){
    t_float newdisplay = x->x_outmode ? x->x_out_val : x->x_in_val;
    if(memcmp(&newdisplay, &x->x_display, sizeof(newdisplay))) // bitwise comparison
        numbox_draw_number(x);
    clock_delay(x->x_clock_update, x->x_rate);
}

static void numbox_width_calc(t_numbox *x){
    x->x_width = (x->x_fontsize - (x->x_fontsize/2)+2) * (x->x_numwidth+2) + 2;
}

static float numbox_clip(t_numbox *x, t_floatarg val){
    if(x->x_min != 0 || x->x_max != 0)
        return(val < x->x_min ? x->x_min : val > x->x_max ? x->x_max : val);
    return(val);
}

// ------------------------ Methods-----------------------------
static void numbox_width(t_numbox *x, t_floatarg f){
    int width = f < MINDIGITS ? MINDIGITS : (int)f;
    if(x->x_numwidth != width){
        x->x_numwidth = width;
        numbox_width_calc(x);
        numbox_resize(x);
        numbox_draw_number(x);
    }
}

static void numbox_size(t_numbox *x, t_floatarg f){
    int size = f < MINSIZE ? MINSIZE : (int)f;
    if(x->x_fontsize != size){
        t_canvas *cv = glist_getcanvas(x->x_glist);
        x->x_fontsize = size;
        int oldheight = x->x_height;
        x->x_height = size + 4;
        sys_vgui(".x%lx.c move %lxOUT 0 %d\n", cv, x, (x->x_height - oldheight) * x->x_zoom);
        sys_vgui(".x%lx.c itemconfigure %lxNUM -font {{%s} -%d}\n", cv, x,
            def_font, x->x_fontsize*x->x_zoom);
        sys_vgui(".x%lx.c moveto %lxNUM %d %d\n", cv, x,
            x->x_obj.te_xpix*x->x_zoom, (x->x_obj.te_ypix+2)*x->x_zoom);
        numbox_width_calc(x);
        numbox_resize(x);
        canvas_fixlinesfor(x->x_glist, (t_text*)x);
    }
}

static void numbox_rate(t_numbox *x, t_floatarg f){
    x->x_rate = f < 15 ? 15 : (int)f;
    clock_update(x);
}

static void numbox_ramp(t_numbox *x, t_floatarg f){
    x->x_ramp_ms = f < 0 ? 0 : (int)f;
}

static void numbox_float(t_numbox *x, t_floatarg f){ // set float value and update GUI
    t_float ftocompare = f;
    if(memcmp(&ftocompare, &x->x_out_val, sizeof(ftocompare))){ // bitwise comparison
        x->x_out_val = ftocompare;
        if(x->x_outmode){
            numbox_draw_number(x);
            if(x->x_ramp_ms > 0)
                x->x_ramp_step = (x->x_out_val - x->x_ramp_val) / (x->x_ramp_ms * x->x_sr_khz);
        }
    }
}

static void numbox_set(t_numbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(!ac)
        x->x_set_val = x->x_out_val;
    else if(ac == 1 && av->a_type == A_FLOAT)
        numbox_float(x, x->x_set_val = atom_getfloat(av));
}

static void numbox_list(t_numbox *x, t_symbol *sym, int ac, t_atom *av){ // get key events
    if(ac == 1 && av->a_type == A_FLOAT){
        numbox_float(x, atom_getfloat(av));
        return;
    }
    if(x->x_glist->gl_edit || !x->x_clicked || !x->x_outmode || ac != 2)
        return;  // ignore if: edit mode / not clicked / monitor mode / wrong size list
    int flag = (int)atom_getfloat(av); // 1 for press / 0 for release
    sym = atom_getsymbol(av+1); // get key name
    if(flag && sym == gensym("Up"))
        numbox_float(x, numbox_clip(x, x->x_out_val + x->x_inc));
    if(flag && sym == gensym("Down"))
        numbox_float(x, numbox_clip(x, x->x_out_val - x->x_inc));
}

static void numbox_bg(t_numbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_symbol *bg = NULL;
    if(ac == 1 && av->a_type == A_SYMBOL)
        bg = atom_getsymbolarg(0, ac, av);
    if(x->x_bg != bg && bg != NULL){
        x->x_bg = bg;
        sys_vgui(".x%lx.c itemconfigure %lxBASE -fill %s\n", glist_getcanvas(x->x_glist), x, x->x_bg->s_name);
    }
//    else if(ac == 3 && av->a_type == A_SYMBOL)
}

static void numbox_fg(t_numbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_symbol *fg = NULL;
    if(ac == 1 && av->a_type == A_SYMBOL)
        fg = atom_getsymbolarg(0, ac, av);
    if(x->x_fg != fg && fg != NULL){
        x->x_fg = fg;
        sys_vgui(".x%lx.c itemconfigure %lxNUM -fill %s\n", glist_getcanvas(x->x_glist), x, x->x_fg->s_name);
    }
//    else if(ac == 3 && av->a_type == A_SYMBOL)
}

static void numbox_range(t_numbox *x, t_floatarg f1, t_floatarg f2){
    if(x->x_min == f1 && x->x_max == f2)
        return;
    float temp = f1;
    if(f2 < f1){
        f1 = f2;
        f2 = temp;
    }
    x->x_min = f1, x->x_max = f2;
}

// ------------------------ widgetbehaviour-----------------------------
static void numbox_key(void *z, t_symbol *keysym, t_floatarg fkey){
    keysym = NULL; // unused, avoid warning
    t_numbox *x = z;
    char c = fkey, buf[3];
    buf[1] = 0;
    if(c == 0){ // click out
        x->x_clicked = 0;
        pd_unbind((t_pd *)x, gensym("#keyname"));
        sys_vgui(".x%lx.c itemconfigure %lxBASE -width %d\n", glist_getcanvas(x->x_glist), x, x->x_zoom);
        numbox_draw_number(x);
    }
    else if(((c >= '0') && (c <= '9')) || (c == '.') || (c == '-') ||
    (c == 'e') || (c == '+') || (c == 'E')){ // number characters
        if(strlen(x->x_buf) < (MAX_NUMBOX_LEN-2)){
            buf[0] = c;
            strcat(x->x_buf, buf);
            numbox_draw_number(x);
        }
    }
    else if((c == '\b') || (c == 127)){ // backspace / delete
        int sl = (int)strlen(x->x_buf) - 1;
        if(sl < 0)
            sl = 0;
        x->x_buf[sl] = 0;
        numbox_draw_number(x);
    }
    else if(((c == '\n') || (c == 13)) && x->x_buf[0] != 0){ // enter
        numbox_float(x, atof(x->x_buf)); // atof converts string to float
        x->x_buf[0] = 0;
    }
}

static void numbox_motion(t_numbox *x, t_floatarg dx, t_floatarg dy, t_floatarg up){
    dx = 0; // avoid warning
    if(!up) // only when mouse is pressed down
        numbox_float(x, numbox_clip(x, x->x_out_val - dy*x->x_inc));
}

static int numbox_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit){
    dbl = alt = 0;
    t_numbox* x = (t_numbox *)z;
    if(doit && x->x_outmode){
        x->x_inc = shift ? 0.01 : 1;
        x->x_clicked = 1;
        pd_bind(&x->x_obj.ob_pd, gensym("#keyname")); // listen to key events
        x->x_buf[0] = 0;
        sys_vgui(".x%lx.c itemconfigure %lxBASE -width %d\n", glist_getcanvas(glist), x, x->x_zoom*2);
        glist_grab(glist, &x->x_obj.te_g, (t_glistmotionfn)numbox_motion, numbox_key,
            (t_floatarg)xpix, (t_floatarg)ypix);
    }
    return(1);
}

static void numbox_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void numbox_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_numbox* x = (t_numbox*)z;
    *xp1 = text_xpix(&x->x_obj, glist), *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = *xp1 + x->x_width, *yp2 = *yp1 + x->x_height;
}

static void numbox_select(t_gobj *z, t_glist *glist, int sel){
    t_numbox *x = (t_numbox *)z;
    t_canvas *cv = glist_getcanvas(glist);
    if(sel){
        sys_vgui(".x%lx.c itemconfigure %lxNUM -fill blue\n", cv, x);
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline blue\n", cv, x);
    }
    else{
        sys_vgui(".x%lx.c itemconfigure %lxNUM -fill %s\n", cv, x, x->x_fg->s_name);
        sys_vgui(".x%lx.c itemconfigure %lxBASE -outline black\n", cv, x);
    }
}

static void numbox_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_numbox *x = (t_numbox *)z;
    x->x_obj.te_xpix += dx, x->x_obj.te_ypix += dy;
    sys_vgui(".x%lx.c move %lxALL %d %d\n", glist_getcanvas(glist), x, dx*x->x_zoom, dy*x->x_zoom);
    canvas_fixlinesfor(glist, (t_text*)x);
}

void numbox_vis(t_gobj *z, t_glist *glist, int vis){
    t_numbox* x = (t_numbox*)z;
    if(vis){ // draw it
        int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist), zoom = x->x_zoom;
        int w = x->x_width*zoom, h = x->x_height*zoom, size = x->x_fontsize*zoom;
        int half = h/2, d = zoom + h/(34*zoom), iow = IOWIDTH * zoom, ioh = 3*zoom; // d?? / why not ioheight??
        t_canvas *cv = glist_getcanvas(glist);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -width %d -outline black -fill %s -tags [list %lxBASE %lxALL]\n",
            cv, xpos, ypos, xpos+w, ypos+h, zoom, x->x_bg->s_name, x, x);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxIN %lxALL]\n",
            cv, xpos, ypos, xpos+iow, ypos-zoom+ioh, x, x);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxOUT %lxALL]\n",
            cv, xpos, ypos+h+zoom-ioh, xpos+iow, ypos+h, x, x);
        sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w -font {{%s} -%d} -fill %s -tags [list %lxNUM %lxALL]\n",
            cv, xpos+2*zoom, ypos+half+d, set_x_buf(x), def_font, size, x->x_fg->s_name, x, x, x);
    }
    else // erase
        sys_vgui(".x%lx.c delete %lxALL\n", glist_getcanvas(glist), x);
}

static void numbox_properties(t_gobj *z, t_glist *owner){ // called in right click for propeties
    owner = NULL; // not used for some reason, avoid warning
    t_numbox *x = (t_numbox *)z;
    char buf[800];
    sprintf(buf, "::dialog_numbox::pdtk_numbox_dialog %%s -------dimensions(digits)(pix):------- \
        %d %d %d %d %d %d %f %s %s %.4f %.4f\n", x->x_numwidth, MINDIGITS, x->x_fontsize, MINSIZE,
        x->x_ramp_ms, x->x_rate, x->x_set_val, x->x_bg->s_name, x->x_fg->s_name, x->x_min, x->x_max);
    gfxstub_new(&x->x_obj.ob_pd, x, buf); // no idea what this does...
}

static void numbox_dialog(t_numbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL; // we receive this when applying changes in properties
    int width = atom_getintarg(0, ac, av);
    int size = atom_getintarg(1, ac, av);
    int ramp_ms = atom_getintarg(2, ac, av);
    int rate = atom_getintarg(3, ac, av);
    int initial = atom_getintarg(4, ac, av);
    t_symbol *bgcolor = atom_getsymbolarg(5, ac, av);
    t_symbol *fgcolor = atom_getsymbolarg(6, ac, av);
    t_float min = atom_getfloatarg(7, ac, av);
    t_float max = atom_getfloatarg(8, ac, av);
    t_atom undo[9];
    SETFLOAT(undo+0, x->x_numwidth);
    SETFLOAT(undo+1, x->x_fontsize);
    SETFLOAT(undo+2, x->x_ramp_ms);
    SETFLOAT(undo+3, x->x_rate);
    SETFLOAT(undo+4, x->x_set_val);
    SETSYMBOL(undo+5, x->x_bg);
    SETSYMBOL(undo+6, x->x_fg);
    SETFLOAT(undo+7, x->x_min);
    SETFLOAT(undo+8, x->x_max);
    pd_undo_set_objectstate(x->x_glist, (t_pd*)x, gensym("dialog"), 9, undo, ac, av);
    numbox_ramp(x, ramp_ms);
    numbox_rate(x, rate);
    numbox_range(x, min, max);
    t_atom at[1];
    SETFLOAT(at,initial);
    numbox_set(x, NULL, 1, at);
    SETSYMBOL(at, bgcolor);
    numbox_bg(x, NULL, 1, at);
    SETSYMBOL(at, fgcolor);
    numbox_fg(x, NULL, 1, at);
    numbox_width(x, width);
    numbox_size(x, size);
    canvas_fixlinesfor(x->x_glist, (t_text*)x);
    canvas_dirty(x->x_glist, 1);
}

static void numbox_save(t_gobj *z, t_binbuf *b){
    t_numbox *x = (t_numbox *)z;
    binbuf_addv(b, "ssiisiiissifff", gensym("#X"), gensym("obj"), (int)x->x_obj.te_xpix,
        (int)x->x_obj.te_ypix, atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)), x->x_numwidth,
        x->x_fontsize, x->x_rate, x->x_bg, x->x_fg, x->x_ramp_ms, x->x_min, x->x_max, x->x_set_val);
    binbuf_addv(b, ";");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static t_int *numbox_perform(t_int *w){
    t_numbox *x   = (t_numbox *)(w[1]);
    t_sample *in  = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    t_int n       = (t_int)(w[4]);
    if(!x->x_outmode)
        x->x_in_val = in[0];
    for(int i = 0; i < n; i++){
        if(x->x_ramp_val != x->x_out_val && x->x_ramp_ms != 0 && x->x_outmode){ // apply a ramp
                if((x->x_ramp_step < 0 && x->x_ramp_val <= x->x_out_val) ||
                (x->x_ramp_step > 0 && x->x_ramp_val >= x->x_out_val)){ // reached destination
                    x->x_ramp_val = x->x_out_val;
                    x->x_ramp_step = 0;
                }
                out[i] = (x->x_ramp_val += x->x_ramp_step);
            }
        else // No ramp
            out[i] = x->x_outmode ? x->x_out_val : in[i];
    }
    return(w+5);
}

static void numbox_dsp(t_numbox *x, t_signal **sp){
    x->x_outmode = !else_magic_inlet_connection((t_object *)x, x->x_glist, 0, &s_signal);
    x->x_sr_khz = sp[0]->s_sr * 0.001;
    dsp_add(numbox_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void numbox_zoom(t_numbox *x, t_floatarg zoom){
    x->x_zoom = (int)zoom;
}

static void numbox_free(t_numbox *x){
    if(x->x_clicked)
        pd_unbind((t_pd *)x, gensym("#keyname"));
    clock_free(x->x_clock_update);
    gfxstub_deleteforkey(x);
}

static void *numbox_new(t_symbol *sym, int ac, t_atom *av){
    t_numbox *x = (t_numbox *)pd_new(numbox_class);;
    x->x_glist = (t_glist *)canvas_getcurrent();
    x->x_zoom = x->x_glist->gl_zoom;
    x->x_in_val = x->x_set_val = x->x_out_val = 0.0;
    x->x_buf[0] = 0; // ??
    x->x_clicked = 0, x->x_outmode = 1;
    int width = 6, size = glist_getfont(x->x_glist), rate = 100, ramp_ms = 10;
    t_float minimum = 0, maximum = 0;
    x->x_bg = gensym("#C0C0C4"), x->x_fg = gensym("#440008");
    if(ac == 9 && av->a_type == A_FLOAT){
        width = atom_getintarg(0, ac, av);
        size = atom_getintarg(1, ac, av);
        rate = atom_getintarg(2, ac, av);
        x->x_bg = atom_getsymbolarg(3, ac, av);
        x->x_fg = atom_getsymbolarg(4, ac, av);
        ramp_ms = atom_getintarg(5, ac, av);
        minimum = atom_getfloatarg(6, ac, av);
        maximum = atom_getfloatarg(7, ac, av);
        x->x_set_val = atom_getfloatarg(8, ac, av);
    }
    else while(ac > 0){
        if(av->a_type == A_SYMBOL){
            sym = atom_getsymbolarg(0, ac, av);
            if(sym == gensym("-width")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    width = atom_getintarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-size")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    size = atom_getintarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-rate")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    rate = atom_getintarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-ramp")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    ramp_ms = atom_getintarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-min")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    minimum = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-max")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    maximum = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-set")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_set_val = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-bgcolor")){
                if(ac >= 2 && (av+1)->a_type == A_SYMBOL){
                    x->x_bg = atom_getsymbolarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-fgcolor")){
                if(ac >= 2 && (av+1)->a_type == A_SYMBOL){
                    x->x_fg = atom_getsymbolarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else goto errstate;
        }
        else goto errstate;
    }
    x->x_numwidth = width < MINDIGITS ? MINDIGITS : width;
    x->x_fontsize = size < MINSIZE ? MINSIZE : size;
    numbox_width_calc(x);
    x->x_height = x->x_fontsize + 4;
    numbox_range(x, minimum, maximum);
    x->x_rate = rate < 15 ? 15 : (int)rate;
    x->x_ramp_ms = ramp_ms < 0 ? 0 : ramp_ms;    
    x->x_out_val = x->x_ramp_val = x->x_set_val;
    x->x_sr_khz = sys_getsr() * 0.001;
    x->x_clock_update = clock_new(x, (t_method)clock_update);
    clock_delay(x->x_clock_update, x->x_rate); // Start repaint clock
    outlet_new(&x->x_obj, &s_signal);
    return(x);
errstate:
    pd_error(x, "[numbox~]: improper args");
    return(NULL);
}

void numbox_tilde_setup(void){
    numbox_class = class_new(gensym("numbox~"), (t_newmethod)numbox_new,
        (t_method)numbox_free, sizeof(t_numbox), 0, A_GIMME, 0);
    class_addlist(numbox_class, numbox_list); // used for float and keypresses
    class_addmethod(numbox_class, nullfn, gensym("signal"), 0);
    class_addmethod(numbox_class, (t_method)numbox_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(numbox_class, (t_method)numbox_rate, gensym("rate"), A_FLOAT, 0);
    class_addmethod(numbox_class, (t_method)numbox_ramp, gensym("ramp"), A_FLOAT, 0);
    class_addmethod(numbox_class, (t_method)numbox_width, gensym("width"), A_FLOAT, 0);
    class_addmethod(numbox_class, (t_method)numbox_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(numbox_class, (t_method)numbox_range, gensym("range"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(numbox_class, (t_method)numbox_set, gensym("set"), A_GIMME, 0);
    class_addmethod(numbox_class, (t_method)numbox_bg, gensym("bgcolor"), A_GIMME, 0);
    class_addmethod(numbox_class, (t_method)numbox_fg, gensym("fgcolor"), A_GIMME, 0);
    class_addmethod(numbox_class, (t_method)numbox_zoom, gensym("zoom"), A_CANT, 0);
    class_addmethod(numbox_class, (t_method)numbox_dialog, gensym("dialog"), A_GIMME, 0);
    numbox_widgetbehavior.w_getrectfn  = numbox_getrect;
    numbox_widgetbehavior.w_displacefn = numbox_displace;
    numbox_widgetbehavior.w_selectfn   = numbox_select;
    numbox_widgetbehavior.w_deletefn   = numbox_delete;
    numbox_widgetbehavior.w_clickfn    = numbox_newclick;
    numbox_widgetbehavior.w_visfn      = numbox_vis;
    numbox_widgetbehavior.w_activatefn = NULL;
    class_setwidget(numbox_class, &numbox_widgetbehavior);
    class_setsavefn(numbox_class, numbox_save);
    class_setpropertiesfn(numbox_class, numbox_properties);
    #include "numbox~_dialog.c"
}
