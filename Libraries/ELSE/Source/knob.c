// By Porres and Tim Schoen
// Based on the knob proposal for vanilla by Ant1, Porres and others

#include "m_pd.h"
#include "g_canvas.h"
#include <math.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define HALF_PI (M_PI / 2)

#define MIN_SIZE   16

t_widgetbehavior knob_widgetbehavior;
static t_class *knob_class, *edit_proxy_class;

typedef struct _edit_proxy{
    t_object      p_obj;
    t_symbol     *p_sym;
    t_clock      *p_clock;
    struct _knob *p_cnv;
}t_edit_proxy;

typedef struct _knob{
    t_object        x_obj;
    t_edit_proxy   *x_proxy;
    t_glist        *x_glist;
    int             x_size;
    t_float         x_pos; // 0-1 normalized position
    t_float         x_exp;
    int             x_expmode;
    int             x_log;
    t_float         x_init;
    int             x_start_angle;
    int             x_end_angle;
    int             x_range;
    int             x_offset;
    int             x_ticks;
    int             x_outline;
    double          x_min;
    double          x_max;
    int             x_clicked;
    int             x_sel;
    int             x_shift;
    int             x_edit;
    int             x_jump;
    t_float         x_fval;
    t_symbol       *x_fg;
    t_symbol       *x_mg;
    t_symbol       *x_bg;
    t_symbol       *x_snd;
    t_symbol       *x_snd_raw;
    int             x_flag;
    int             x_r_flag;
    int             x_s_flag;
    int             x_rcv_set;
    int             x_snd_set;
    t_symbol       *x_rcv;
    t_symbol       *x_rcv_raw;
    int             x_circular;
    int             x_arc;
    int             x_zoom;
    int             x_discrete;
    char            x_tag_obj[128];
    char            x_tag_circle[128];
    char            x_tag_bg_arc[128];
    char            x_tag_arc[128];
    char            x_tag_center[128];
    char            x_tag_wiper[128];
    char            x_tag_wpr_c[128];
    char            x_tag_ticks[128];
    char            x_tag_outline[128];
    char            x_tag_in[128];
    char            x_tag_out[128];
    char            x_tag_sel[128];
}t_knob;

// ---------------------- Helper functions ----------------------
// get value from motion/position
static t_float knob_getfval(t_knob *x){
    t_float fval;
    t_float pos = x->x_pos;
    if(x->x_discrete){ // later just 1 tick case
        t_float ticks = (x->x_ticks < 2 ? 2 : (float)x->x_ticks) - 1;
        pos = rint(pos * ticks) / ticks;
    }
    if(x->x_log == 1){ // log
        if((x->x_min <= 0 && x->x_max >= 0) || (x->x_min >= 0 && x->x_max <= 0)){
            pd_error(x, "[knob]: range can't contain '0' in log mode");
            fval = x->x_min;
        }
        else
            fval = exp(pos * log(x->x_max / x->x_min)) * x->x_min;
    }
    else{
        if(x->x_exp != 0){
            if(x->x_exp > 0)
                pos = pow(pos, x->x_exp);
            else
                pos = 1 - pow(1 - pos, -x->x_exp);
        }
        fval = pos * (x->x_max - x->x_min) + x->x_min;
    }
    if((fval < 1.0e-10) && (fval > -1.0e-10))
        fval = 0.0;
    return(fval);
}

// get position from value
static t_float knob_getpos(t_knob *x, t_floatarg fval){
    double pos;
    if(x->x_log == 1){ // log
        if((x->x_min <= 0 && x->x_max >= 0) || (x->x_min >= 0 && x->x_max <= 0)){
            pd_error(x, "[knob]: range cannot contain '0' in log mode");
            pos = 0;
        }
        else
            pos = log(fval/x->x_min) / log(x->x_max/x->x_min);
    }
    else{
        pos = (fval - x->x_min) / (x->x_max - x->x_min);
        if(x->x_exp != 0){
            if(x->x_exp > 0)
                pos = pow(pos, 1.0/x->x_exp);
            else
                pos = 1-pow(1-pos, 1.0/(-x->x_exp));
        }
    }
    if(x->x_discrete){
        t_float ticks = (float)(x->x_ticks) -1;
        if(ticks <= 0)
            pos = (x->x_init - x->x_min) / (x->x_max - x->x_min);
        else
            pos = rint(pos * ticks) / ticks;
    }
    return(pos);
}

// ---------------------- Configure / Update GUI ----------------------

// configure colors
static void knob_config_fg(t_knob *x, t_canvas *cv){
    pdgui_vmess(0, "crs rsrs", cv, "itemconfigure",  x->x_tag_arc,
        "-outline", x->x_fg->s_name, "-fill", x->x_fg->s_name);
    pdgui_vmess(0, "crs rs", cv, "itemconfigure",  x->x_tag_ticks,
        "-fill", x->x_fg->s_name);
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_wiper,
        "-fill", x->x_fg->s_name);
    pdgui_vmess(0, "crs rsrs", cv, "itemconfigure",  x->x_tag_wpr_c,
        "-outline", x->x_fg->s_name, "-fill", x->x_fg->s_name);
}

static void knob_config_bg_arc(t_knob *x, t_canvas *cv){
    pdgui_vmess(0, "crs rsrs", cv, "itemconfigure",  x->x_tag_bg_arc,
        "-outline", x->x_mg->s_name, "-fill", x->x_mg->s_name);
}

static void knob_config_bg(t_knob *x, t_canvas *cv){
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_circle,
        "-fill", x->x_bg->s_name);
    pdgui_vmess(0, "crs rsrs", cv, "itemconfigure", x->x_tag_center,
        "-outline", x->x_bg->s_name, "-fill", x->x_bg->s_name);
}

// configure size
static void knob_config_size(t_knob *x, t_canvas *cv){
    int z = x->x_zoom;
    int x1 = text_xpix(&x->x_obj, x->x_glist);
    int y1 = text_ypix(&x->x_obj, x->x_glist);
    int x2 = x1 + x->x_size * z;
    int y2 = y1 + x->x_size * z;
// inlet, outlet, outline
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_in,
        x1, y1, x1 + IOWIDTH*z, y1 + IHEIGHT*z);
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_out,
        x1, y2 - OHEIGHT*z, x1 + IOWIDTH*z, y2);
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_outline,
        x1, y1, x2, y2);
    pdgui_vmess(0, "crs ri", cv, "itemconfigure", x->x_tag_outline,
        "-width", x->x_zoom);
// wiper's width/circle
    int r = x->x_size * x->x_zoom / 2; // radius
    int xc = x1 + r, yc = y1 + r; // center coords
    int w_width = r / 10;   // width 1/10 of radius
    int wx1 = rint(xc - w_width), wy1 = rint(yc - w_width);
    int wx2 = rint(xc + w_width), wy2 = rint(yc + w_width);
    pdgui_vmess(0, "crs ri", cv, "itemconfigure", x->x_tag_wiper,
        "-width", w_width); // wiper width
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_wpr_c,
        wx1, wy1, wx2, wy2); // wiper center circle
// knob arc
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_bg_arc,
        x1 + x->x_zoom, y1 + x->x_zoom, x2 - x->x_zoom, y2 - x->x_zoom);
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_arc,
        x1 + x->x_zoom, y1 + x->x_zoom, x2 - x->x_zoom, y2 - x->x_zoom);
// knob circle (base)
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_circle,
        x1, y1, x2, y2);
// knob center
    int arcwidth = r / 5; // arc width is 1/5 of radius
    if(arcwidth < 1)
        arcwidth = 1;
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_center,
        x1 + arcwidth, y1 + arcwidth, x2 - arcwidth, y2 - arcwidth);
}

// configure wiper center
static void knob_config_wcenter(t_knob *x, t_canvas *cv, int active){
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_wpr_c,
                "-state", active ? "normal" : "hidden");
}

// configure inlet outlet and outline
static void knob_config_io(t_knob *x, t_canvas *cv){
    int inlet = (x->x_rcv == &s_ || x->x_rcv == gensym("empty")) && x->x_edit;
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_in,
        "-state", inlet ? "normal" : "hidden");
    int outlet = (x->x_snd == &s_ || x->x_snd == gensym("empty")) && x->x_edit;
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_out,
        "-state", outlet ? "normal" : "hidden");
    int outline = x->x_edit || x->x_outline;
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_outline,
        "-state", outline ? "normal" : "hidden");
}

// configure arc
static void knob_config_arc(t_knob *x, t_canvas *cv){
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_arc,
        "-state", x->x_arc && x->x_fval != x->x_init ? "normal" : "hidden");
    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_bg_arc,
        "-state", x->x_arc ? "normal" : "hidden");
}

// Update Arc/Wiper according to position
static void knob_update(t_knob *x, t_canvas *cv){
    t_float pos = x->x_pos;
    if(x->x_discrete){ // later just 1 tick case
        t_float ticks = (x->x_ticks < 2 ? 2 : (float)x->x_ticks) -1;
        pos = rint(pos * ticks) / ticks;
    }
    float start = (x->x_start_angle / 90.0 - 1) * HALF_PI;
    float range = x->x_range / 180.0 * M_PI;
    float angle = start + pos * range; // pos angle
// configure arc
    knob_config_arc(x, cv);
    pdgui_vmess(0, "crs sf sf", cv, "itemconfigure", x->x_tag_bg_arc,
        "-start", start * -180.0 / M_PI,
        "-extent", range * -179.99 / M_PI);
    start += (knob_getpos(x, x->x_init) * range);
    pdgui_vmess(0, "crs sf sf", cv, "itemconfigure", x->x_tag_arc,
        "-start", start * -180.0 / M_PI,
        "-extent", (angle - start) * -179.99 / M_PI);
// set wiper
    int radius = (int)(x->x_size*x->x_zoom / 2.0);
    int x0 = text_xpix(&x->x_obj, x->x_glist);
    int y0 = text_ypix(&x->x_obj, x->x_glist);
    int xc = x0 + radius; // center x coordinate
    int yc = y0 + radius; // center y coordinate
    int xp = xc + rint(radius * cos(angle)); // circle point x coordinate
    int yp = yc + rint(radius * sin(angle)); // circle point x coordinate
    pdgui_vmess(0, "crs iiii", cv, "coords", x->x_tag_wiper, xc, yc, xp, yp);
}

//---------------------- DRAW STUFF ----------------------------//

// redraw ticks
static void knob_draw_ticks(t_knob *x, t_glist *glist){
    t_canvas *cv = glist_getcanvas(glist);
    pdgui_vmess(0, "crs", cv, "delete", x->x_tag_ticks);
    if(!x->x_ticks)
        return;
    int z = x->x_zoom;
    float r = (float)x->x_size*z / 2.0; // radius
    int divs = x->x_ticks;
    if((divs > 1) && ((x->x_range + 360) % 360 != 0)) // ????
        divs = divs - 1;
    float delta_w = x->x_range / (float)divs;
    int x0 = text_xpix(&x->x_obj, x->x_glist), y0 = text_ypix(&x->x_obj, x->x_glist);
    int xc = x0 + r, yc = y0 + r; // center coords
    int start = x->x_start_angle - 90.0;
//    int odd  = x->x_ticks % 2;
//    int mid  = x->x_ticks / 2 + 1;
    if(x->x_ticks == 1){
        int width = (x->x_size / 40);
        if(width < 1)
            width = 1;
        width *= 1.5;
        float pos = knob_getpos(x, x->x_init) * x->x_range;
        float w = pos + start; // tick angle
        w *= M_PI/180.0; // in radians
        float dx = r * cos(w), dy = r * sin(w);
        int x1 = xc + (int)(dx);
        int y1 = yc + (int)(dy);
        int x2 = xc + (int)(dx * 0.65);
        int y2 = yc + (int)(dy * 0.65);
        char *tags_ticks[] = {x->x_tag_ticks, x->x_tag_obj};
        pdgui_vmess(0, "crr iiii ri rs rS",
            cv, "create", "line",
            x1, y1, x2, y2,
            "-width", width * z,
            "-fill", x->x_fg->s_name,
            "-tags", 2, tags_ticks);
    }
    else for(int t = 1; t <= x->x_ticks; t++){
//         int thicker = (t == 1 || t == x->x_ticks || (odd && t == mid));
        int thicker = (t == 1 || t == x->x_ticks);
        int width = (x->x_size / 40);
        if(width < 1)
            width = 1;
        if(thicker)
            width *= 1.5;
        float w = (t-1)*delta_w + start; // tick angle
        w *= M_PI/180.0; // in radians
        float dx = r * cos(w), dy = r * sin(w);
        int x1 = xc + (int)(dx);
        int y1 = yc + (int)(dy);
        int x2 = xc + (int)(dx * (thicker ? 0.65 : 0.75));
        int y2 = yc + (int)(dy * (thicker ? 0.65 : 0.75));
        char *tags_ticks[] = {x->x_tag_ticks, x->x_tag_obj};
        pdgui_vmess(0, "crr iiii ri rs rS",
            cv, "create", "line",
            x1, y1, x2, y2,
            "-width", width * z,
            "-fill", x->x_fg->s_name,
            "-tags", 2, tags_ticks);
    }
}

// draw all and initialize stuff
static void knob_draw_new(t_knob *x, t_canvas *cv){
// base circle
    char *tags_circle[] = {x->x_tag_circle, x->x_tag_obj, x->x_tag_sel};
    pdgui_vmess(0, "crr iiii rS", cv, "create", "oval",
        0, 0, 0, 0,
        "-tags", 3, tags_circle);
// arc
    char *tags_arc_bg[] = {x->x_tag_bg_arc, x->x_tag_obj};
        pdgui_vmess(0, "crr iiii rS", cv, "create", "arc",
        0, 0, 0, 0,
        "-tags", 2, tags_arc_bg);
    char *tags_arc[] = {x->x_tag_arc, x->x_tag_obj};
        pdgui_vmess(0, "crr iiii rS", cv, "create", "arc",
        0, 0, 0, 0,
        "-tags", 2, tags_arc);
// center circle
    char *tags_center[] = {x->x_tag_center, x->x_tag_obj};
        pdgui_vmess(0, "crr iiii rS", cv, "create", "oval",
        0, 0, 0, 0,
        "-tags", 2, tags_center);
// wiper
    char *tags_wiper_center[] = {x->x_tag_wpr_c, x->x_tag_obj};
        pdgui_vmess(0, "crr iiii rS", cv, "create", "oval",
        0, 0, 0, 0,
        "-tags", 2, tags_wiper_center);
    char *tags_wiper[] = {x->x_tag_wiper, x->x_tag_obj};
        pdgui_vmess(0, "crr iiii rS", cv, "create", "line",
        0, 0, 0, 0,
        "-tags", 2, tags_wiper);
// inlet
    char *tags_in[] = {x->x_tag_in, x->x_tag_obj};
    pdgui_vmess(0, "crr iiii rs rS", cv, "create", "rectangle",
        0, 0, 0, 0,
        "-fill", "black",
        "-tags", 2, tags_in);
// outlet
    char *tags_out[] = {x->x_tag_out, x->x_tag_obj};
    pdgui_vmess(0, "crr iiii rs rS", cv, "create", "rectangle",
        0, 0, 0, 0,
        "-fill", "black",
        "-tags", 2, tags_out);
// outline
    char *tags_outline[] = {x->x_tag_outline, x->x_tag_obj, x->x_tag_sel};
    pdgui_vmess(0, "crr iiii rS", cv, "create", "rectangle",
        0, 0, 0, 0,
        "-tags", 3, tags_outline);
// draw ticks
    knob_draw_ticks(x, cv);

    knob_config_size(x, cv);
    knob_config_io(x, cv);
    knob_config_bg(x, cv);
    knob_config_bg_arc(x, cv);
    knob_config_fg(x, cv);
    knob_config_arc(x, cv);
    knob_config_wcenter(x, cv, x->x_clicked);
        
    knob_update(x, cv);

    pdgui_vmess(0, "crs rs", cv, "itemconfigure", x->x_tag_sel,
        "-outline", x->x_sel ? "blue" : "black"); // ??????
}

// ------------------------ knob widgetbehaviour-----------------------------
static void knob_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_knob *x = (t_knob *)z;
    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = text_xpix(&x->x_obj, glist) + x->x_size*x->x_zoom;
    *yp2 = text_ypix(&x->x_obj, glist) + x->x_size*x->x_zoom;
}

static void knob_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_knob *x = (t_knob *)z;
    x->x_obj.te_xpix += dx, x->x_obj.te_ypix += dy;
    dx *= x->x_zoom, dy *= x->x_zoom;
    pdgui_vmess(0, "crs ii", glist_getcanvas(glist), "move", x->x_tag_obj, dx, dy);
    canvas_fixlinesfor(glist, (t_text*)x);
}

static void knob_select(t_gobj *z, t_glist *glist, int sel){
    t_knob* x = (t_knob*)z;
    pdgui_vmess(0, "crs rs", glist_getcanvas(glist), "itemconfigure", x->x_tag_sel,
        "-outline", (x->x_sel = sel) ? "blue" : "black");
}

void knob_vis(t_gobj *z, t_glist *glist, int vis){
    t_knob* x = (t_knob*)z;
    t_canvas *cv = glist_getcanvas(x->x_glist = glist);
    vis ? knob_draw_new(x, cv) : pdgui_vmess(0, "crs", cv, "delete", x->x_tag_obj);
}

static void knob_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void knob_get_snd(t_knob* x){
    if(!x->x_snd_set){ // no send set, search arguments
        t_binbuf *bb = x->x_obj.te_binbuf;
        int n_args = binbuf_getnatom(bb) - 1; // number of arguments
        char buf[128];
        if(n_args > 0){ // we have arguments, let's search them
            if(x->x_flag){ // arguments are flags actually
                if(x->x_s_flag){ // we got a send flag, let's get it
                    for(int i = 0;  i <= n_args; i++){
                        atom_string(binbuf_getvec(bb) + i, buf, 128);
                        if(gensym(buf) == gensym("-send")){
                            i++;
                            atom_string(binbuf_getvec(bb) + i, buf, 128);
                            x->x_snd_raw = gensym(buf);
                            break;
                        }
                    }
                }
            }
            else{ // we got no flags, let's search for argument
                int arg_n = 6; // send argument number
                if(n_args >= arg_n){ // we have it, get it
                    atom_string(binbuf_getvec(bb) + arg_n, buf, 128);
                    x->x_snd_raw = gensym(buf);
                }
            }
        }
    }
    if(x->x_snd_raw == &s_)
        x->x_snd_raw = gensym("empty");
}

static void knob_get_rcv(t_knob* x){
    if(!x->x_rcv_set){ // no receive set, search arguments
        t_binbuf *bb = x->x_obj.te_binbuf;
        int n_args = binbuf_getnatom(bb) - 1; // number of arguments
        char buf[128];
        if(n_args > 0){ // we have arguments, let's search them
            if(x->x_flag){ // arguments are flags actually
                if(x->x_r_flag){ // we got a receive flag, let's get it
                    for(int i = 0;  i <= n_args; i++){
                        atom_string(binbuf_getvec(bb) + i, buf, 128);
                        if(gensym(buf) == gensym("-receive")){
                            i++;
                            atom_string(binbuf_getvec(bb) + i, buf, 128);
                            x->x_rcv_raw = gensym(buf);
                            break;
                        }
                    }
                }
            }
            else{ // we got no flags, let's search for argument
                int arg_n = 7; // receive argument number
                if(n_args >= arg_n){ // we have it, get it
                    atom_string(binbuf_getvec(bb) + arg_n, buf, 128);
                    x->x_rcv_raw = gensym(buf);
                }
            }
        }
    }
    if(x->x_rcv_raw == &s_)
        x->x_rcv_raw = gensym("empty");
}

static void knob_save(t_gobj *z, t_binbuf *b){
    t_knob *x = (t_knob *)z;
    binbuf_addv(b, "ssiis",
        gensym("#X"),
        gensym("obj"),
        (t_int)x->x_obj.te_xpix,
        (t_int)x->x_obj.te_ypix,
        atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)));
    knob_get_snd(x);
    knob_get_rcv(x);
    binbuf_addv(b, "iffffsssssiiiiiiii", // 18 args
        x->x_size, // 01: i SIZE
        (float)x->x_min, // 02: f min
        (float)x->x_max, // 03: f max
        x->x_log ? 1 : x->x_exp, // 04: f exp
        x->x_init, // 05: f init
        x->x_snd_raw, // 06: s snd
        x->x_rcv_raw, // 07: s rcv
        x->x_bg, // 08: s bgcolor
        x->x_mg, // 09: s mgcolor
        x->x_fg, // 10: s fgcolor
        x->x_outline, // 11: i outline
        x->x_circular, // 12: i circular
        x->x_ticks, // 13: i ticks
        x->x_discrete, // 14: i discrete
        x->x_arc, // 15: i arc
        x->x_range, // 16: i range
        x->x_offset, // 17: i offset
        x->x_jump); // 17: i offset
    binbuf_addv(b, ";");
}

// ------------------------ knob methods -----------------------------

static void knob_set(t_knob *x, t_floatarg f){
    float old = x->x_pos;
    x->x_fval = f > x->x_max ? x->x_max : f < x->x_min ? x->x_min : f;
    x->x_pos = knob_getpos(x, x->x_fval);
    x->x_fval = knob_getfval(x);
    if(x->x_pos != old){
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_update(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_bang(t_knob *x){
    outlet_float(x->x_obj.ob_outlet, x->x_fval);
    if(x->x_snd->s_thing)
        pd_float(x->x_snd->s_thing, x->x_fval);
}

static void knob_float(t_knob *x, t_floatarg f){
    knob_set(x, f);
    knob_bang(x);
}

static void knob_init(t_knob *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(!ac)
        x->x_init = x->x_fval;
    else if(ac == 1 && av->a_type == A_FLOAT){
        float f = atom_getfloat(av);
        x->x_init = f < x->x_min ? x->x_min : f > x->x_max ? x->x_max : f;
        x->x_pos = knob_getpos(x, x->x_fval = x->x_init);
    }
    else
        return;
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
        knob_update(x, glist_getcanvas(x->x_glist));
        if(x->x_ticks == 1)
            knob_draw_ticks(x, glist_getcanvas(x->x_glist));
    }
}

// set start/end angles
static void knob_start_end(t_knob *x, t_floatarg f1, t_floatarg f2){
    int range = f1 > 360 ? 360 : f1 < 0 ? 0 : (int)f1;
    int offset = f2 > 360 ? 360 : f2 < 0 ? 0 : (int)f2;
    if(x->x_range == range && x->x_offset == offset)
        return;
    x->x_range = range;
    x->x_offset = offset;
    int start = -(x->x_range/2) + x->x_offset;
    int end = x->x_range/2 + x->x_offset;
    float tmp;
    while(start < -360)
        start = -360;
    while (start > 360)
        start = 360;
    while(end < -360)
        end = -360;
    while(end > 360)
        end = 360;
    if(end < start){
        tmp = start;
        start = end;
        end = tmp;
    }
    if((end - start) > 360)
        end = start + 360;
    if(end == start)
        end = start + 1;
    x->x_start_angle = start;
    x->x_end_angle = end;
    knob_set(x, x->x_fval);
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
        knob_draw_ticks(x, glist_getcanvas(x->x_glist));
        knob_update(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_angle(t_knob *x, t_floatarg f){
    knob_start_end(x, f, x->x_offset);
}

static void knob_offset(t_knob *x, t_floatarg f){
    knob_start_end(x, x->x_range, f);
}

static void knob_size(t_knob *x, t_floatarg f){
    float size = f < MIN_SIZE ? MIN_SIZE : f;
    if(x->x_size != size){
        x->x_size = size;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            t_canvas *cv = glist_getcanvas(x->x_glist);
            knob_config_size(x, cv);
            knob_draw_ticks(x, cv);
            knob_update(x, cv);
            canvas_fixlinesfor(x->x_glist, (t_text *)x);
        }
    }
}

static void knob_arc(t_knob *x, t_floatarg f){
    int arc = f != 0;
    if(x->x_arc != arc){
        x->x_arc = arc;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_config_arc(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_circular(t_knob *x, t_floatarg f){
    x->x_circular = (f != 0);
}

static void knob_discrete(t_knob *x, t_floatarg f){
    x->x_discrete = (f != 0);
}

static void knob_ticks(t_knob *x, t_floatarg f){
    int ticks = f < 0 ? 0 : (int)f;
    if(x->x_ticks != ticks){
        x->x_ticks = ticks;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_draw_ticks(x, glist_getcanvas(x->x_glist));
    }
}

static t_symbol *getcolor(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if((av)->a_type == A_SYMBOL)
        return(atom_getsymbol(av));
    else{
        int r = atom_getintarg(0, ac, av);
        int g = atom_getintarg(1, ac, av);
        int b = atom_getintarg(2, ac, av);
        r = r < 0 ? 0 : r > 255 ? 255 : r;
        g = g < 0 ? 0 : g > 255 ? 255 : g;
        b = b < 0 ? 0 : b > 255 ? 255 : b;
        char color[20];
        sprintf(color, "#%2.2x%2.2x%2.2x", r, g, b);
        return(gensym(color));
    }
}

static void knob_bgcolor(t_knob *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    t_symbol *color = getcolor(s, ac, av);
    if(x->x_bg != color){
        x->x_bg = color;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_config_bg(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_mgcolor(t_knob *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    t_symbol *color = getcolor(s, ac, av);
    if(x->x_mg != color){
        x->x_mg = color;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_config_bg_arc(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_fgcolor(t_knob *x, t_symbol *s, int ac, t_atom *av){
    if(!ac)
        return;
    t_symbol *color = getcolor(s, ac, av);
    if(x->x_fg != color){
        x->x_fg = color;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_config_fg(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_send(t_knob *x, t_symbol *s){
    if(s == gensym(""))
        s = gensym("empty");
    t_symbol *snd = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
    if(snd != x->x_snd){
        x->x_snd_set = 1;
        x->x_snd_raw = s;
        x->x_snd = snd;
        knob_config_io(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_receive(t_knob *x, t_symbol *s){
    if(s == gensym(""))
        s = gensym("empty");
    t_symbol *rcv = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
    if(rcv != x->x_rcv){
        x->x_rcv_set = 1;
        t_symbol *old_rcv = x->x_rcv;
        x->x_rcv_raw = s;
        x->x_rcv = rcv;
        if(old_rcv != &s_ && old_rcv != gensym("empty"))
            pd_unbind(&x->x_obj.ob_pd, old_rcv);
        if(x->x_rcv != &s_)
            pd_bind(&x->x_obj.ob_pd, x->x_rcv);
        knob_config_io(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_range(t_knob *x, t_floatarg f1, t_floatarg f2){
    if(f1 > f2){
        x->x_max = (double)f1;
        x->x_min = (double)f2;
    }
    else{
        x->x_min = (double)f1;
        x->x_max = (double)f2;
    }
    if(x->x_fval < x->x_min)
        x->x_fval = x->x_min;
    if(x->x_fval > x->x_max)
        x->x_fval = x->x_max;
    if(x->x_init < x->x_min)
        x->x_init = x->x_min;
    if(x->x_init > x->x_max)
        x->x_init = x->x_max;
    x->x_pos = knob_getpos(x, x->x_fval);
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
        knob_update(x, glist_getcanvas(x->x_glist));
}

static void knob_log(t_knob *x, t_floatarg f){
    x->x_log = (f != 0);
    if(x->x_log)
        x->x_expmode = 1; // log
    else{
        if(x->x_exp == 0)
            x->x_expmode = 0; // lin
        else
            x->x_expmode = 2; // exp
    }
    x->x_pos = knob_getpos(x, x->x_fval);
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
        knob_update(x, glist_getcanvas(x->x_glist));
}

static void knob_jump(t_knob *x, t_floatarg f){
    x->x_jump = (f != 0);
}

static void knob_exp(t_knob *x, t_floatarg f){
    x->x_exp = f;
    if(fabs(x->x_exp) == 1)
        x->x_exp = 0; // lin
    if(x->x_log)
        x->x_expmode = 1; // log
    else{
        if(x->x_exp == 0)
            x->x_expmode = 0; // lin
        else
            x->x_expmode = 2; // exp
    }
    x->x_pos = knob_getpos(x, x->x_fval);
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
        knob_update(x, glist_getcanvas(x->x_glist));
}

static void knob_outline(t_knob *x, t_floatarg f){
    x->x_outline = (int)f;
    knob_config_io(x, glist_getcanvas(x->x_glist));
}

static void knob_zoom(t_knob *x, t_floatarg f){
    x->x_zoom = (int)f;
}

// --------------- properties stuff --------------------
static void knob_properties(t_gobj *z, t_glist *owner){
    owner = NULL;
    t_knob *x = (t_knob *)z;
    char buffer[512];
    sprintf(buffer, "knob_dialog %%s %g %g %g %g %d {%s} {%s} %d %g %d {%s} {%s} {%s} %d %d %d %d %d %d\n",
        (float)(x->x_size / x->x_zoom),
        x->x_min,
        x->x_max,
        x->x_init,
        x->x_circular,
        x->x_snd_raw->s_name,
        x->x_rcv_raw->s_name,
        x->x_expmode,
        x->x_exp,
        x->x_jump,
        x->x_bg->s_name,
        x->x_fg->s_name,
        x->x_mg->s_name,
        x->x_discrete,
        x->x_ticks,
        x->x_arc,
        x->x_range,
        x->x_offset,
        x->x_outline
);
    gfxstub_new(&x->x_obj.ob_pd, x, buffer);
}

static void knob_apply(t_knob *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_atom undo[19];
    SETFLOAT(undo+0, x->x_size);
    SETFLOAT(undo+1, x->x_min);
    SETFLOAT(undo+2, x->x_max);
    SETFLOAT(undo+3, x->x_init);
    SETSYMBOL(undo+4, x->x_snd);
    SETSYMBOL(undo+5, x->x_rcv);
    SETFLOAT(undo+6, x->x_outline);
    SETFLOAT(undo+7, x->x_log ? 1 : x->x_exp);
    SETFLOAT(undo+8, x->x_expmode);
    SETFLOAT(undo+9, x->x_jump);
    SETSYMBOL(undo+10, x->x_bg);
    SETSYMBOL(undo+11, x->x_mg);
    SETSYMBOL(undo+12, x->x_fg);
    SETFLOAT(undo+13, x->x_circular);
    SETFLOAT(undo+14, x->x_ticks);
    SETFLOAT(undo+15, x->x_discrete);
    SETFLOAT(undo+16, x->x_arc);
    SETFLOAT(undo+17, x->x_range);
    SETFLOAT(undo+18, x->x_offset);
    pd_undo_set_objectstate(x->x_glist, (t_pd*)x, gensym("dialog"), 19, undo, ac, av);
    int size = (int)atom_getintarg(0, ac, av);
    float min = atom_getfloatarg(1, ac, av);
    float max = atom_getfloatarg(2, ac, av);
    double init = atom_getfloatarg(3, ac, av);
    t_symbol* snd = atom_getsymbolarg(4, ac, av);
    t_symbol* rcv = atom_getsymbolarg(5, ac, av);
    x->x_outline = atom_getintarg(6, ac, av);
    float exp = atom_getfloatarg(7, ac, av);
    int expmode = atom_getintarg(8, ac, av);
    x->x_jump = atom_getintarg(9, ac, av);
    t_symbol *bg = atom_getsymbolarg(10, ac, av);
    t_symbol *mg = atom_getsymbolarg(11, ac, av);
    t_symbol *fg = atom_getsymbolarg(12, ac, av);
    x->x_circular = atom_getintarg(13, ac, av);
    int ticks = atom_getintarg(14, ac, av);
    x->x_discrete = atom_getintarg(15, ac, av);
    int arc = atom_getintarg(16, ac, av) != 0;
    int range = atom_getintarg(17, ac, av);
    int offset = atom_getintarg(18, ac, av);
    knob_config_io(x, glist_getcanvas(x->x_glist)); // for outline
    if(expmode == 0){
        knob_log(x, 0);
        knob_exp(x, 0.0f);
    }
    if(expmode == 1)
        knob_log(x, 1);
    if(expmode == 2){
        knob_log(x, 0);
        knob_exp(x, exp);
    }
    knob_range(x, min, max);
    knob_ticks(x, ticks);
    t_atom at[1];
    SETSYMBOL(at, bg);
    knob_bgcolor(x, NULL, 1, at);
    SETSYMBOL(at, fg);
    knob_fgcolor(x, NULL, 1, at);
    SETSYMBOL(at, mg);
    knob_mgcolor(x, NULL, 1, at);
    knob_start_end(x, (float)range, (float)offset);
    knob_arc(x, (float)arc);
    knob_size(x, (float)size);
    knob_send(x, snd);
    knob_receive(x, rcv);
    if(x->x_init != init){
        SETFLOAT(at, init);
        knob_init(x, NULL, 1, at);
    }
}

// --------------- click + motion stuff --------------------

static int xm, ym;

static void knob_arrow_motion(t_knob *x, t_floatarg dir){
    float old = x->x_pos, pos;
    if(x->x_discrete){
        t_float ticks = (x->x_ticks < 2 ? 2 : (float)x->x_ticks) - 1;
        old = rint(old * ticks) / ticks;
        pos = old + dir/ticks;
    }
    else{
        float delta = dir;
        delta /= (float)(x->x_size) * x->x_zoom * 2; // 2 is 'sensitivity'
        if(x->x_shift)
            delta *= 0.01;
        pos = x->x_pos + delta;
    }
    if(x->x_circular){
        if(pos > 1)
            pos = 0;
        if(pos < 0)
            pos = 1;
    }
    else
        pos = pos > 1 ? 1 : pos < 0 ? 0 : pos;
    x->x_pos = pos;
    t_float fval = x->x_fval;
    x->x_fval = knob_getfval(x);
    if(fval != x->x_fval)
        knob_bang(x);
    if(old != x->x_pos){
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_update(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_list(t_knob *x, t_symbol *sym, int ac, t_atom *av){ // get key events
    if(ac == 1 && av->a_type == A_FLOAT){ // also implement bang (to do)
        knob_float(x, atom_getfloat(av));
        return;
    }
    if(!x->x_clicked || ac != 2 || x->x_edit)
        return;
    int dir = 0;
    int flag = (int)atom_getfloat(av); // 1 for press, 0 for release
    sym = atom_getsymbol(av+1); // get key name
    if(flag && (sym == gensym("Up") || (sym == gensym("Right"))))
        dir = 1;
    else if(flag && (sym == gensym("Down") || (sym == gensym("Left"))))
        dir = -1;
    else
        return;
    knob_arrow_motion(x, dir);
}

static void knob_motion(t_knob *x, t_floatarg dx, t_floatarg dy){
    if(dx == 0 && dy == 0)
        return;
    float old = x->x_pos, pos;
    if(!x->x_circular){ // normal mode
        float delta = -dy;
        if(fabs(dx) > fabs(dy))
            delta = dx;
        delta /= (float)(x->x_size) * x->x_zoom * 2; // 2 is 'sensitivity'
        if(x->x_shift)
            delta *= 0.01;
        pos = x->x_pos + delta;
    }
    else{ // circular mode
        xm += dx, ym += dy;
        int xc = text_xpix(&x->x_obj, x->x_glist) + x->x_size / 2;
        int yc = text_ypix(&x->x_obj, x->x_glist) + x->x_size / 2;
        float alphacenter = (x->x_end_angle + x->x_start_angle) / 2;
        float alpha = atan2(xm - xc, -ym + yc) * 180.0 / M_PI;
        pos = (((int)((alpha - alphacenter + 180.0 + 360.0) * 100.0) % 36000) * 0.01
            + (alphacenter - x->x_start_angle - 180.0)) / x->x_range;
    }
    x->x_pos = pos > 1 ? 1 : pos < 0 ? 0 : pos; // should go up in non circular (to do)
    t_float fval = x->x_fval;
    x->x_fval = knob_getfval(x);
    if(fval != x->x_fval)
        knob_bang(x);
    if(old != x->x_pos){
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            knob_update(x, glist_getcanvas(x->x_glist));
    }
}

static void knob_key(void *z, t_symbol *keysym, t_floatarg fkey){
    keysym = NULL; // unused, avoid warning
    t_knob *x = z;
    char c = fkey, buf[3];
    buf[1] = 0;
    if(c == 0){ // click out
        pd_unbind((t_pd *)x, gensym("#keyname"));
        knob_config_wcenter(x, glist_getcanvas(x->x_glist), x->x_clicked = 0);
    }
    else if(((c == '\n') || (c == 13))) // enter
        knob_float(x, x->x_fval);
}

static int knob_click(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit){
    alt = 0;
    t_knob *x = (t_knob *)z;
    if(dbl){
        knob_set(x, x->x_init);
        knob_bang(x);
        return(1);
    }
    if(doit){
        pd_bind(&x->x_obj.ob_pd, gensym("#keyname")); // listen to key events
        knob_config_wcenter(x, glist_getcanvas(x->x_glist), x->x_clicked = 1);
        x->x_shift = shift;
        if(x->x_circular){
//            if(x->x_jump){
                xm = xpix;
                ym = ypix;
/*            }
            else{ // did not work
                float start = (x->x_start_angle / 90.0 - 1) * HALF_PI;
                float range = x->x_range / 180.0 * M_PI;
                float angle = start + x->x_pos * range; // pos angle
                int radius = (int)(x->x_size*x->x_zoom / 2.0);
                int x0 = text_xpix(&x->x_obj, x->x_glist), y0 = text_ypix(&x->x_obj, x->x_glist);
                int xp = x0 + radius + rint(radius * cos(angle)); // circle point x coordinate
                int yp = y0 + radius + rint(radius * sin(angle)); // circle point x coordinate
                xm = xp;
                ym = yp;
            }*/
        }
        else{
            if(x->x_jump){
                int xc = text_xpix(&x->x_obj, x->x_glist) + x->x_size / 2;
                int yc = text_ypix(&x->x_obj, x->x_glist) + x->x_size / 2;
                float alphacenter = (x->x_end_angle + x->x_start_angle) / 2;
                float alpha = atan2(xpix - xc, -ypix + yc) * 180.0 / M_PI;
                float pos = (((int)((alpha - alphacenter + 180.0 + 360.0) * 100.0) % 36000) * 0.01
                    + (alphacenter - x->x_start_angle - 180.0)) / x->x_range;
                x->x_pos = pos > 1 ? 1 : pos < 0 ? 0 : pos;
                x->x_fval = knob_getfval(x);
                knob_update(x, glist_getcanvas(x->x_glist));
            }
        }
        knob_bang(x);
        glist_grab(glist, &x->x_obj.te_g, (t_glistmotionfn)(knob_motion), knob_key, xpix, ypix);
    }
    return(1);
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
            knob_config_io(p->p_cnv, glist_getcanvas(p->p_cnv->x_glist));
        }
    }
}

// ---------------------- new / free / setup ----------------------

static void edit_proxy_free(t_edit_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_edit_proxy *edit_proxy_new(t_knob *x, t_symbol *s){
    t_edit_proxy *p = (t_edit_proxy*)pd_new(edit_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)edit_proxy_free);
    return(p);
}

static void knob_free(t_knob *x){
    if(x->x_clicked)
        pd_unbind((t_pd *)x, gensym("#keyname"));
    if(x->x_rcv != gensym("empty"))
        pd_unbind(&x->x_obj.ob_pd, x->x_rcv);
    x->x_proxy->p_cnv = NULL;
    gfxstub_deleteforkey(x);
}

static void *knob_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_knob *x = (t_knob *)pd_new(knob_class);
    float initvalue = 0.0, exp = 0.0, min = 0.0, max = 127.0;
    t_symbol *snd = gensym("empty"), *rcv = gensym("empty");
    int size = 50, circular = 0, ticks = 0, discrete = 0;
    int arc = 1, angle = 320, offset = 0;
    x->x_bg = gensym("#dfdfdf"), x->x_mg = gensym("#7c7c7c"), x->x_fg = gensym("black");
    x->x_clicked = x->x_log = 0;
    x->x_outline = x->x_jump = 0;
    x->x_glist = (t_glist *)canvas_getcurrent();
    x->x_zoom = x->x_glist->gl_zoom;
    x->x_flag = 0;
    x->x_snd_set = x->x_s_flag = 0;
    x->x_rcv_set = x->x_r_flag = 0;
    if(ac){
        if(av->a_type == A_FLOAT){
            size = atom_getintarg(0, ac, av); // 01: i SIZE
            min = atom_getfloatarg(1, ac, av); // 02: f min
            max = atom_getfloatarg(2, ac, av); // 03: f max
            exp = atom_getfloatarg(3, ac, av); // 04: f exp
            initvalue = atom_getfloatarg(4, ac, av); // 05: f init
            snd = atom_getsymbolarg(5, ac, av); // 06: s snd
            rcv = atom_getsymbolarg(6, ac, av); // 07: s rcv
            x->x_bg = atom_getsymbolarg(7, ac, av); // 08: s bgcolor
            x->x_mg = atom_getsymbolarg(8, ac, av); // 09: s mgcolor
            x->x_fg = atom_getsymbolarg(9, ac, av); // 10: s fgcolor
            x->x_outline = atom_getintarg(10, ac, av); // 11: i outline
            circular = atom_getintarg(11, ac, av); // 12: i circular
            ticks = atom_getintarg(12, ac, av); // 13: i ticks
            discrete = atom_getintarg(13, ac, av); // 14: i discrete
            arc = atom_getintarg(14, ac, av); // 15: i arc
            angle = atom_getintarg(15, ac, av); // 16: i range
            offset = atom_getintarg(16, ac, av); // 17: i offset
            x->x_jump = atom_getintarg(17, ac, av); // 18: i jump
        }
        else{
            while(ac){
                t_symbol *sym = atom_getsymbol(av);
                if(sym == gensym("-size")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_FLOAT){
                            size = atom_getint(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-range")){
                    if(ac >= 3){
                        x->x_flag = 1, av++, ac--;
                        min = atom_getfloat(av);
                        av++, ac--;
                        max = atom_getfloat(av);
                        av++, ac--;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-exp")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_FLOAT){
                            exp = atom_getfloat(av);
                            if(fabs(exp) == 1)
                                exp = 0;
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-log")){
                    x->x_flag = 1, av++, ac--;
                    exp = 1;
                }
                else if(sym == gensym("-send")){
                    if(ac >= 2){
                        x->x_flag = x->x_r_flag = 1, av++, ac--;
                        if(av->a_type == A_SYMBOL){
                            snd = atom_getsymbol(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-receive")){
                    if(ac >= 2){
                        x->x_flag = x->x_r_flag = 1, av++, ac--;
                        if(av->a_type == A_SYMBOL){
                            rcv = atom_getsymbol(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-bgcolor")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_SYMBOL){
                            x->x_bg = atom_getsymbol(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-mgcolor")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_SYMBOL){
                            x->x_mg = atom_getsymbol(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-fgcolor")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_SYMBOL){
                            x->x_fg = atom_getsymbol(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-init")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_FLOAT){
                            initvalue = atom_getfloat(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-circular")){
                    x->x_flag = 1, av++, ac--;
                    circular = 1;
                }
                else if(sym == gensym("-jump")){
                    x->x_flag = 1, av++, ac--;
                    x->x_jump = 1;
                }
                else if(sym == gensym("-ticks")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_FLOAT){
                            ticks = atom_getint(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-discrete")){
                    if(ac >= 1){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_FLOAT)
                            discrete = 1;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-arc")){
                    if(ac >= 1){
                        x->x_flag = 1, av++, ac--;
                        arc = 1;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-angle")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_FLOAT){
                            angle = atom_getint(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else if(sym == gensym("-offset")){
                    if(ac >= 2){
                        x->x_flag = 1, av++, ac--;
                        if(av->a_type == A_FLOAT){
                            offset = atom_getint(av);
                            av++, ac--;
                        }
                        else
                            goto errstate;
                    }
                    else
                        goto errstate;
                }
                else
                    goto errstate;
            }
        }
    }
    x->x_snd = canvas_realizedollar(x->x_glist, x->x_snd_raw = snd);
    x->x_rcv = canvas_realizedollar(x->x_glist, x->x_rcv_raw = rcv);
    x->x_size = size < MIN_SIZE ? MIN_SIZE : size;
    knob_range(x, min, max);
    if(exp == 1)
        knob_log(x, 1);
    else
        knob_exp(x, exp);
    x->x_circular = circular;
    x->x_ticks = ticks < 0 ? 0 : ticks;
    x->x_discrete = discrete;
    x->x_arc = arc;
    x->x_range = angle < 0 ? 0 : angle > 360 ? 360 : angle;
    x->x_offset = offset < 0 ? 0 : offset > 360 ? 360 : offset;
    x->x_start_angle = -(x->x_range/2) + x->x_offset;
    x->x_end_angle = x->x_range/2 + x->x_offset;
    x->x_fval = x->x_init = initvalue;
    x->x_pos = knob_getpos(x, x->x_fval);
    x->x_edit = x->x_glist->gl_edit;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_glist);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = edit_proxy_new(x, gensym(buf));
    sprintf(x->x_tag_obj, "%pOBJ", x);
    sprintf(x->x_tag_circle, "%pCIRCLE", x);
    sprintf(x->x_tag_sel, "%pSEL", x);
    sprintf(x->x_tag_arc, "%pARC", x);
    sprintf(x->x_tag_bg_arc, "%pBGARC", x);
    sprintf(x->x_tag_ticks, "%pTICKS", x);
    sprintf(x->x_tag_wiper, "%pWIPER", x);
    sprintf(x->x_tag_wpr_c, "%pWIPERC", x);
    sprintf(x->x_tag_center, "%pCENTER", x);
    sprintf(x->x_tag_outline, "%pOUTLINE", x);
    sprintf(x->x_tag_in, "%pIN", x);
    sprintf(x->x_tag_out, "%pOUT", x);
    if(x->x_rcv != gensym("empty"))
        pd_bind(&x->x_obj.ob_pd, x->x_rcv);
    outlet_new(&x->x_obj, &s_float);
    return(x);
errstate:
    pd_error(x, "[knob]: improper creation arguments");
    return(NULL);
}

void knob_setup(void){
    knob_class = class_new(gensym("knob"), (t_newmethod)knob_new,
        (t_method)knob_free, sizeof(t_knob), 0, A_GIMME, 0);
    class_addbang(knob_class,knob_bang);
    class_addfloat(knob_class, knob_float);
    class_addlist(knob_class, knob_list); // used for float and keypresses
    class_addmethod(knob_class, (t_method)knob_init, gensym("init"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_circular, gensym("circular"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_range, gensym("range"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_jump, gensym("jump"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_exp, gensym("exp"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_log, gensym("log"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_discrete, gensym("discrete"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_bgcolor, gensym("bgcolor"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_mgcolor, gensym("arccolor"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_fgcolor, gensym("fgcolor"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(knob_class, (t_method)knob_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(knob_class, (t_method)knob_arc, gensym("arc"), A_DEFFLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_angle, gensym("angle"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_offset, gensym("offset"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_ticks, gensym("ticks"), A_DEFFLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_zoom, gensym("zoom"), A_CANT, 0);
    class_addmethod(knob_class, (t_method)knob_apply, gensym("dialog"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_motion, gensym("motion"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_outline, gensym("outline"), A_DEFFLOAT, 0);
    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(edit_proxy_class, edit_proxy_any);
    knob_widgetbehavior.w_getrectfn  = knob_getrect;
    knob_widgetbehavior.w_displacefn = knob_displace;
    knob_widgetbehavior.w_selectfn   = knob_select;
    knob_widgetbehavior.w_activatefn = NULL;
    knob_widgetbehavior.w_deletefn   = knob_delete;
    knob_widgetbehavior.w_visfn      = knob_vis;
    knob_widgetbehavior.w_clickfn    = knob_click;
    class_setwidget(knob_class, &knob_widgetbehavior);
    class_setsavefn(knob_class, knob_save);
    class_setpropertiesfn(knob_class, knob_properties);
    #include "../extra_source/knob_dialog.h"
}
