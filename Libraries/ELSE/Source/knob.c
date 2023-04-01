// stolen from vanilla's knob by Antoine Rousseau
// (see https://github.com/pure-data/pure-data/pull/1738)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"

#include "g_all_guis.h"
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

#define DEFAULT_SENSITIVITY 128
#define DEFAULT_SIZE 35
#define MIN_SIZE 15
#define POS_MARGIN 0.01

typedef struct _knb{
    t_iemgui        x_gui;
    float           x_pos;  // 0-1 normalized position
    float           x_exp;
    float           x_init;
    int             x_arc_width;
    int             x_start_angle;
    int             x_end_angle;
    int             x_ticks;
    double          x_min;
    double          x_max;
    t_float         x_fval;
    int             x_acol;
    t_symbol       *x_move_mode; // "xy" or "angle"
    unsigned int    x_wiper_visible:1;
    unsigned int    x_arc_visible:1;
    unsigned int    x_center_visible:1;
}t_knb;

t_widgetbehavior knb_widgetbehavior;
static t_class *knb_class;
static t_symbol *s_k_xy, *s_k_angle;

static void knb_draw_io(t_knb *x,t_glist *glist, int old_snd_rcv_flags){
    old_snd_rcv_flags = 0;
    int xpos = text_xpix(&x->x_gui.x_obj, glist);
    int ypos = text_ypix(&x->x_gui.x_obj, glist);
    int iow = IOWIDTH * IEMGUI_ZOOM(x), ioh = IEM_GUI_IOHEIGHT * IEMGUI_ZOOM(x);
    t_canvas *canvas = glist_getcanvas(glist);
    char tag_object[128], tag[128], tag_select[128];
    char *tags[] = {tag_object, tag, tag_select};
    sprintf(tag_object, "%pOBJ", x);
    sprintf(tag_select, "%pSELECT", x);
    sprintf(tag, "%pOUTLINE", x);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if((!x->x_gui.x_fsf.x_snd_able) || (!x->x_gui.x_fsf.x_rcv_able)){
        pdgui_vmess(0, "crr iiii ri rS", canvas, "create", "rectangle",
            xpos, ypos,
            xpos + x->x_gui.x_w, ypos + x->x_gui.x_w,
            "-width", IEMGUI_ZOOM(x),
            "-tags", 3, tags);
    }
    sprintf(tag, "%pOUT%d", x, 0);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if(!x->x_gui.x_fsf.x_snd_able){
        pdgui_vmess(0, "crr iiii rs rS", canvas, "create", "rectangle",
            xpos, ypos + x->x_gui.x_w + IEMGUI_ZOOM(x) - ioh,
            xpos + iow, ypos + x->x_gui.x_w,
            "-fill", "black",
            "-tags", 2, tags);
    }
    sprintf(tag, "%pIN%d", x, 0);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if(!x->x_gui.x_fsf.x_rcv_able){
        pdgui_vmess(0, "crr iiii rs rS", canvas, "create", "rectangle",
            xpos, ypos,
            xpos + iow, ypos - IEMGUI_ZOOM(x) + ioh,
            "-fill", "black",
            "-tags", 2, tags);
    }
}

static void knb_update_knb(t_knb *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    float angle, angle0;
    int x0, y0, x1, y1;
    t_float pos = (x->x_pos - POS_MARGIN) / (1 - 2 * POS_MARGIN);
    char tag[128];
    if(pos < 0.0)
        pos = 0.0;
    else if(pos > 1.0)
        pos = 1.0;
    x0 = text_xpix(&x->x_gui.x_obj, glist);
    y0 = text_ypix(&x->x_gui.x_obj, glist);
    x1 = x0 + x->x_gui.x_w;
    y1 = y0 + x->x_gui.x_w;
    angle0 = (x->x_start_angle / 90.0 - 1) * M_PI / 2.0;
    angle = angle0 + pos * (x->x_end_angle - x->x_start_angle) / 180.0 * M_PI;
    if(x->x_arc_visible){
        float zero_angle, zero_val;
        int arcwidth, aD, cD;
        int realw = x->x_gui.x_w / IEMGUI_ZOOM(x);
        arcwidth = x->x_arc_width;
        if(arcwidth > (realw - 1) / 2)
            arcwidth = (realw - 1) / 2;
        if(arcwidth < -((realw - 1) / 2 + 1))
            arcwidth = -((realw - 1) / 2 + 1);
        if((x->x_min * x->x_max) < 0){
            if(x->x_min < 0)
                zero_val = -x->x_min / (fabs(x->x_min) + fabs(x->x_max));
            else
                zero_val = -x->x_max / (fabs(x->x_min) + fabs(x->x_max));
            zero_angle = angle0 + zero_val * (x->x_end_angle - x->x_start_angle) / 180.0 * M_PI;
            angle0 = zero_angle;
        }
        if
            (arcwidth > 0) aD = IEMGUI_ZOOM(x);
        else
            aD = (((realw + 1)/ 2) + arcwidth) * IEMGUI_ZOOM(x) ;
        sprintf(tag, "%pARC", x);
        pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
            x0 + aD, y0 + aD, x1 - aD, y1 - aD);
        pdgui_vmess(0, "crs sf sf", canvas, "itemconfigure", tag,
            "-start", angle0 * -180.0 / M_PI,
            "-extent", (angle - angle0) * -179.99 / M_PI);
        if(x->x_center_visible){
            sprintf(tag, "%pCENTER", x);
            cD = (arcwidth + 1) * IEMGUI_ZOOM(x);
            pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
                x0 + cD, y0 + cD, x1 - cD, y1 - cD);
        }
    }
    #define NEAR(x) ((int)(x + 0.49))
    if(x->x_wiper_visible){
        float radius = x->x_gui.x_w / 2.0;
        float xc, yc, xp, yp;
        sprintf(tag, "%pWIPER", x);
        xc = (x0 + x1) / 2.0;
        yc = (y0 + y1) / 2.0;
        xp = xc + radius * cos(angle);
        yp = yc + radius * sin(angle);
        pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
            NEAR(xc), NEAR(yc), NEAR(xp), NEAR(yp));
    }
}

static void knb_update_ticks(t_knb *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    int tick;
    int x0, y0;
    float xc, yc, xTc, yTc, r1, r2;
    float alpha0, dalpha;
    char tag_object[128], tag[128];
    char *tags[] = {tag_object, tag};
    int divs = x->x_ticks;
    sprintf(tag_object, "%pOBJ", x);
    sprintf(tag, "%pTICKS", x);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if(!x->x_ticks)
        return;
    x0 = text_xpix(&x->x_gui.x_obj, glist);
    y0 = text_ypix(&x->x_gui.x_obj, glist);
    xc = x0 + x->x_gui.x_w / 2.0;
    yc = y0 + x->x_gui.x_w / 2.0;
    r1 = x->x_gui.x_w / 2.0 - IEMGUI_ZOOM(x) * 2.0;
    r2 = IEMGUI_ZOOM(x) * 1.0 ;
    if((divs > 1) && ((x->x_end_angle - x->x_start_angle + 360) % 360 != 0))
        divs = divs - 1;
    dalpha = (x->x_end_angle - x->x_start_angle) / (float)divs;
    alpha0 = x->x_start_angle;
    for(tick = 0; tick < x->x_ticks; tick++){
        float alpha = (alpha0 + dalpha * tick - 90.0) * M_PI / 180.0;
        xTc = xc + r1 * cos(alpha);
        yTc = yc + r1 * sin(alpha);
        pdgui_vmess(0, "crr iiii rk rk rS", canvas, "create", "oval",
            NEAR(xTc - r2), NEAR(yTc - r2),
            NEAR(xTc + r2), NEAR(yTc + r2),
            "-fill", x->x_gui.x_fcol,
            "-outline", x->x_gui.x_fcol,
            "-tags", 2, tags);
    }
}

static void knb_draw_update(t_knb *x, t_glist *glist){
    if(glist_isvisible(glist))
        knb_update_knb(x, glist);
}

static void knb_draw_config(t_knb *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    int xpos = text_xpix(&x->x_gui.x_obj, glist);
    int ypos = text_ypix(&x->x_gui.x_obj, glist);
    const int zoom = IEMGUI_ZOOM(x);
    char tag[128];
    x->x_arc_visible = (x->x_arc_width != 0);
    sprintf(tag, "%pARC", x);
    pdgui_vmess(0, "crs rk rk rs ri", canvas, "itemconfigure", tag,
        "-outline", x->x_acol,
        "-fill", x->x_acol,
        "-state", x->x_arc_visible ? "normal" : "hidden",
        "-width", zoom);
    x->x_center_visible = (x->x_arc_width > 0) &&
        (x->x_arc_width  + 1 < x->x_gui.x_w / (2 * zoom));
    sprintf(tag, "%pCENTER", x);
    pdgui_vmess(0, "crs rk rk rs ri", canvas, "itemconfigure", tag,
        "-outline", x->x_gui.x_bcol,
        "-fill", x->x_gui.x_bcol,
        "-state", x->x_center_visible ? "normal" : "hidden",
        "-width", zoom);
    x->x_wiper_visible = (x->x_gui.x_fcol != x->x_gui.x_bcol);
    sprintf(tag, "%pWIPER", x);
    pdgui_vmess(0, "crs rk rs ri", canvas, "itemconfigure", tag,
        "-fill", x->x_gui.x_fcol,
        "-state", x->x_wiper_visible ? "normal" : "hidden",
        "-width", 3 * zoom);
    sprintf(tag, "%pBASE", x);
    pdgui_vmess(0, "crs rk", canvas, "itemconfigure", tag,
        "-fill", x->x_gui.x_bcol);
    pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
        xpos, ypos,
        xpos + x->x_gui.x_w, ypos + x->x_gui.x_w);
    knb_update_knb(x, glist);
    knb_update_ticks(x, glist);
}

static void knb_draw_new(t_knb *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    char tag[128], tag_object[128], tag_select[128];
    char *tags[] = {tag_object, tag};
    char *seltags[] = {tag_object, tag, tag_select};
    sprintf(tag_object, "%pOBJ", x);
    sprintf(tag_select, "%pSELECT", x);
    sprintf(tag, "%pBASE", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "oval",
         0, 0, 0, 0, "-tags", 3, seltags);
    knb_draw_io(x, glist, 0);
    sprintf(tag, "%pARC", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "arc",
         0, 0, 0, 0, "-tags", 2, tags);
    sprintf(tag, "%pCENTER", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "oval",
         0, 0, 0, 0, "-tags", 2, tags);
    sprintf(tag, "%pWIPER", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "line",
         0, 0, 0, 0, "-tags", 2, tags);
    knb_draw_config(x, glist);
}

static void knb_draw_select(t_knb *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    int col = x->x_gui.x_fsf.x_selected ? IEM_GUI_COLOR_SELECTED : IEM_GUI_COLOR_NORMAL;
    char tag[128];
    sprintf(tag, "%pSELECT", x);
    pdgui_vmess(0, "crs rk", canvas, "itemconfigure", tag, "-outline", col);
}

/*static void knb_select(t_gobj *z, t_glist *glist, int sel){
    t_knb *x = (t_knb *)z;
    x->x_gui.x_fsf.x_selected = sel;
    t_canvas *cv = glist_getcanvas(glist);
    if(sel)
        sys_vgui(".x%lx.c itemconfigure %pSELECT -outline blue\n", cv, x);
    else
        sys_vgui(".x%lx.c itemconfigure %pSELECT -outline black\n", cv, x);
}*/

static void knob_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

// ------------------------ knb widgetbehaviour-----------------------------
#define GRECTRATIO 0
static void knb_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_knb *x = (t_knb *)z;
    *xp1 = text_xpix(&x->x_gui.x_obj, glist) + GRECTRATIO * x->x_gui.x_w;
    *yp1 = text_ypix(&x->x_gui.x_obj, glist) + GRECTRATIO * x->x_gui.x_w;
    *xp2 = text_xpix(&x->x_gui.x_obj, glist) + x->x_gui.x_w * (1 - GRECTRATIO);
    *yp2 = text_ypix(&x->x_gui.x_obj, glist) + x->x_gui.x_w * (1 - GRECTRATIO);
}

static void knb_save(t_gobj *z, t_binbuf *b){
    t_knb *x = (t_knb *)z;
    t_symbol *bflcol[3];
    t_symbol *srl[3];
    char acol_str[MAXPDSTRING];
    snprintf(acol_str, MAXPDSTRING-1, "#%06x", x->x_acol);
    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiifffisssiiiisssfsisiii",
        gensym("#X"), gensym("obj"),
        (t_int)x->x_gui.x_obj.te_xpix, (t_int)x->x_gui.x_obj.te_ypix,
        atom_getsymbol(binbuf_getvec(x->x_gui.x_obj.te_binbuf)),
        x->x_gui.x_w / IEMGUI_ZOOM(x),
        x->x_gui.x_h / IEMGUI_ZOOM(x),
        (float)x->x_min,
        (float)x->x_max,
        x->x_exp,
        iem_symargstoint(&x->x_gui.x_isa),
        srl[0], srl[1], srl[2],
        x->x_gui.x_ldx, x->x_gui.x_ldy,
        0, // was font style
        0, // was font size
        bflcol[0], bflcol[1], bflcol[2],
        x->x_init,
        x->x_move_mode, x->x_ticks,
        gensym(acol_str), x->x_arc_width, x->x_start_angle, x->x_end_angle);
    binbuf_addv(b, ";");
}

void knb_check_wh(t_knb *x, int w, int h){
    if(w < MIN_SIZE * IEMGUI_ZOOM(x))
        w = MIN_SIZE * IEMGUI_ZOOM(x);
    x->x_gui.x_w = w;
    if(h < 5)
        h = 5;
    x->x_gui.x_h = h;
}

static void knb_range(t_knb *x, t_floatarg f1, t_floatarg f2){
    if(f1 > f2){
        x->x_max = (double)f1;
        x->x_min = (double)f2;
    }
    else{
        x->x_min = (double)f1;
        x->x_max = (double)f2;
    }
}

static void knb_exp(t_knb *x, t_floatarg f){
    x->x_exp = f;
}

static void knb_properties(t_gobj *z, t_glist *owner){
    owner = NULL;
    t_knb *x = (t_knb *)z;
    t_symbol *srl[3];
    iemgui_properties(&x->x_gui, srl);
    pdgui_stub_vnew(&x->x_gui.x_obj.ob_pd, "pdtk_iemgui_dialog", x,
        "s s ffs ffs sfsfs i iss fs si sss ii ii kkk ikiii",
        "knb", // needed?
        "",
        (float)(x->x_gui.x_w / IEMGUI_ZOOM(x)),
        (float)MIN_SIZE,
        "Size:",
        x->x_exp, // exp
        0.0,
        "Exponential:", // but not needed
        "Range",
        x->x_min,
        "_", // was 'min' but wasn't used - a fucking symbol is neeeded anyway
        x->x_max,
        "_", // was 'max' but wasn't used
        0,
        0,
        "", "", // was lin/log
        x->x_init,
        x->x_move_mode->s_name,
        "",
        -1,
        srl[0] ? srl[0]->s_name : "",
        srl[1] ? srl[1]->s_name : "",
        srl[2] ? srl[2]->s_name : "",
        x->x_gui.x_ldx,
        x->x_gui.x_ldy,
        0, // was font style,
        0, // was font size,
        x->x_gui.x_bcol,
        x->x_gui.x_fcol,
        0, // was x->x_gui.x_lcol,
        x->x_ticks, x->x_acol, x->x_arc_width, x->x_start_angle, x->x_end_angle);
}

// get value from motion/position
static t_float knb_getfval(t_knb *x){
    t_float fval;
    t_float pos = (x->x_pos - POS_MARGIN) / (1 - 2 * POS_MARGIN);
    if(pos < 0.0)
        pos = 0.0;
    else if(pos > 1.0)
        pos = 1.0;
    if(x->x_exp == 0) // old log
        fval = exp(pos * log(x->x_max / x->x_min)) * x->x_min;
    else{
        if(fabs(x->x_exp) != 1){
            if(x->x_exp > 0)
                pos = pow(pos, x->x_exp);
            else
                pos = pow(1 - pos, -x->x_exp);
        }
        fval = pos * (x->x_max - x->x_min) + x->x_min;
    }
    if((fval < 1.0e-10) && (fval > -1.0e-10))
        fval = 0.0;
    return(fval);
}
    
static void knb_set(t_knb *x, t_floatarg f){
    float old = x->x_pos;
    x->x_fval = f > x->x_max ? x->x_max : f < x->x_min ? x->x_min : f;
    double pos;
    if(x->x_exp == 0) // old log
        pos = log(x->x_fval/x->x_min) / log(x->x_max/x->x_min);
    else{
        pos = (x->x_fval - x->x_min) / (x->x_max - x->x_min);
        if(fabs(x->x_exp) != 1){
            if(x->x_exp > 0)
                pos = pow(pos, 1.0/x->x_exp);
            else
                pos = 1-pow(1-pos, 1.0/(-x->x_exp));
        }
    }
    x->x_pos = pos * (1 - 2 * POS_MARGIN) + POS_MARGIN;
    if(x->x_pos != old)
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void knb_bang(t_knb *x){
    double out = x->x_fval;
    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, out);
}

static void knb_float(t_knb *x, t_floatarg f){
    knb_set(x, f);
    knb_bang(x);
}

static void knb_init(t_knb *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 && av->a_type == A_FLOAT)
        knb_float(x, atom_getfloat(av));
    x->x_init = x->x_fval;
}

#define SETCOLOR(a, col) do {char color[MAXPDSTRING]; snprintf(color, MAXPDSTRING-1, "#%06x", 0xffffff & col); color[MAXPDSTRING-1] = 0; SETSYMBOL(a, gensym(color));} while(0)
static void knb_apply(t_knb *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    x->x_exp = atom_getfloatarg(1, argc, argv);
    float min = atom_getfloatarg(2, argc, argv);
    float max = atom_getfloatarg(3, argc, argv);
    double init = atom_getfloatarg(4, argc, argv);
    t_symbol *movemode = atom_getsymbolarg(16, argc, argv);
    int ticks = (int)atom_getintarg(17, argc, argv);
    t_symbol *acol_sym = atom_getsymbolarg(18, argc, argv);
    int arcwidth = (int)atom_getintarg(19, argc, argv);
    int startangle = (int)atom_getintarg(20, argc, argv);
    int endangle = (int)atom_getintarg(21, argc, argv);
    int sr_flags;

    t_atom undo[23];
    iemgui_setdialogatoms(&x->x_gui, 23, undo);
    SETFLOAT(undo+2, x->x_min);
    SETFLOAT(undo+3, x->x_max);
    SETFLOAT(undo+4, x->x_init);
    SETSYMBOL(undo+16, x->x_move_mode);
    SETFLOAT(undo+17, x->x_ticks);
    SETCOLOR(undo+18, x->x_acol);
    SETFLOAT(undo+19, x->x_arc_width);
    SETFLOAT(undo+20, x->x_start_angle);
    SETFLOAT(undo+21, x->x_end_angle);
    pd_undo_set_objectstate(x->x_gui.x_glist, (t_pd*)x, gensym("dialog"),
        23, undo, argc, argv);
    x->x_move_mode = movemode;
    if(ticks < 0)
        ticks = 0;
    x->x_ticks = ticks;
    x->x_arc_width = arcwidth;
    x->x_start_angle = startangle;
    x->x_end_angle = endangle;
    if(x->x_init != init){
        knb_float(x, init);
        x->x_init = x->x_fval;
    }
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    if('#' == acol_sym->s_name[0])
        x->x_acol = (int)strtol(acol_sym->s_name+1, 0, 16);
    else
        x->x_acol = 0x00;
    int h = DEFAULT_SENSITIVITY;
    knb_check_wh(x, w * IEMGUI_ZOOM(x), h * IEMGUI_ZOOM(x));
    knb_range(x, min, max);
    knb_set(x, x->x_fval);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(x->x_gui.x_glist, (t_text *)x);
}

static void knb_motion(t_knb *x, t_floatarg dx, t_floatarg dy){
    float old = x->x_pos;
    float delta = -dy;
    if(fabs(dx) > fabs(dy))
        delta = dx;
    delta /= ((float)x->x_gui.x_h - IEMGUI_ZOOM(x));
    if(x->x_gui.x_fsf.x_finemoved)
        delta *= 0.01;
    double pos = x->x_pos + delta;
    x->x_pos = pos > 1 ? 1 : pos < 0 ? 0 : pos;
    x->x_fval = knb_getfval(x);
    if(old != x->x_pos){
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        knb_bang(x);
    }
}

static int xm, ym;

static void knb_motion_angular(t_knb *x, t_floatarg dx, t_floatarg dy){
    int xc = text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist) + x->x_gui.x_w / 2;
    int yc = text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist) + x->x_gui.x_w / 2;
    float old = x->x_pos;
    float alpha;
    float alphacenter = (x->x_end_angle + x->x_start_angle) / 2;
    xm += dx;
    ym += dy;
    alpha = atan2(xm - xc, -ym + yc) * 180.0 / M_PI;
    x->x_pos = (((int)((alpha - alphacenter + 180.0 + 360.0) * 100.0) % 36000) * 0.01
                + (alphacenter - x->x_start_angle - 180.0)) / (x->x_end_angle - x->x_start_angle);
    if(x->x_pos < 0)
        x->x_pos = 0;
    if(x->x_pos > 1.0)
        x->x_pos = 1.0;
    x->x_fval = knb_getfval(x);
    if(old != x->x_pos){
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        knb_bang(x);
    }
}

static void knb_click(t_knb *x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt){
    alt = ctrl = shift = 0;
    xm = xpos;
    ym = ypos;
    knb_bang(x);
    if(x->x_move_mode == s_k_angle)
        glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g, (t_glistmotionfn)knb_motion_angular, 0, xpos, ypos);
    else
        glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g, (t_glistmotionfn)knb_motion, 0, xpos, ypos);
}

static int knb_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit){
    glist = 0;
    t_knb *x = (t_knb *)z;
    if(dbl){
        knb_set(x, x->x_init);
        knb_bang(x);
        return(1);
    }
    if(doit){
        x->x_gui.x_fsf.x_finemoved = shift;
        knb_click(x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift, 0, (t_floatarg)alt);
    }
    return(1);
}

static void knb_size(t_knb *x, t_floatarg f){
    int w = (int)f * IEMGUI_ZOOM(x);
    int h = x->x_gui.x_h;
    knb_check_wh(x, w, h);
    iemgui_size((void *)x, &x->x_gui);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
}

static void knb_circular(t_knb *x, t_floatarg f){
//    float fval = knb_getfval(x);
    x->x_move_mode = f != 0 ? s_k_angle : s_k_xy;
//    knb_set(x, fval);
}

/* from g_all_guis.c: */
extern int iemgui_compatible_colorarg(int index, int argc, t_atom* argv);

static void knb_color(t_knb *x, t_symbol *s, int ac, t_atom *av){
    x->x_acol = iemgui_compatible_colorarg(3, ac, av);
    iemgui_color((void *)x, &x->x_gui, s, ac, av);
}

static void knb_send(t_knb *x, t_symbol *s){
    iemgui_send(x, &x->x_gui, s);
}

static void knb_receive(t_knb *x, t_symbol *s){
    iemgui_receive(x, &x->x_gui, s);
}

static void knb_arc(t_knb *x, t_floatarg arcwidth){
    int realw = x->x_gui.x_w / IEMGUI_ZOOM(x);
    if(arcwidth > (realw - 1) / 2)
        arcwidth = (realw - 1) / 2;
    if(arcwidth < -((realw - 1) / 2 + 1))
        arcwidth = -((realw - 1) / 2 + 1);
    x->x_arc_width = arcwidth;
    if(glist_isvisible(x->x_gui.x_glist)){
        knb_draw_config(x, x->x_gui.x_glist);
        knb_draw_update(x, x->x_gui.x_glist);
    }
}

static void knb_ticks(t_knb *x, t_floatarg f){
    x->x_ticks = (int)f;
    if(f <= 0)
        x->x_ticks = 0;
    if(glist_isvisible(x->x_gui.x_glist))
        knb_update_ticks(x, x->x_gui.x_glist);
}

// set start/end angles
static void knb_angle(t_knb *x, t_floatarg start, t_floatarg end){
    float tmp;
    if(start < -360)
        start = -360;
    else if(start > 360)
        start = 360;
    if(end < -360)
        end = -360;
    else if(end > 360)
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
    knb_set(x, x->x_fval);
    if(glist_isvisible(x->x_gui.x_glist))
        knb_update_ticks(x, x->x_gui.x_glist);
    knb_draw_update(x, x->x_gui.x_glist);
}

static void knb_zoom(t_knb *x, t_floatarg f){
    iemgui_zoom(&x->x_gui, f);
}

static void *knb_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_knb *x = (t_knb *)iemgui_new(knb_class);
    int width = IEM_GUI_DEFAULTSIZE * 2, height = DEFAULT_SENSITIVITY;
    int ldx = 0, ldy = -8 * IEM_GUI_DEFAULTSIZE_SCALE;
    float v = 0;
    float exp = 1;
    t_symbol *movemode = s_k_xy;
    int ticks = 0, arcwidth = 0, start_angle = -135, end_angle = 135;
    t_symbol *acol_sym = gensym("#00");
    double min = 0.0, max = (double)(DEFAULT_SENSITIVITY - 1);
    iem_inttosymargs(&x->x_gui.x_isa, 0);
    x->x_gui.x_bcol = 0xDFDFDF;
    x->x_gui.x_fcol = 0x00;
    x->x_acol = 0x00;
    IEMGUI_SETDRAWFUNCTIONS(x, knb);
    if((argc >= 17)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
            &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
            &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
            &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
            &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
            &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
            &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
            &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,16))
    {
        width = (int)atom_getintarg(0, argc, argv);
        height = (int)atom_getintarg(1, argc, argv);
        min = (double)atom_getfloatarg(2, argc, argv);
        max = (double)atom_getfloatarg(3, argc, argv);
        exp = atom_getfloatarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = (int)atom_getintarg(9, argc, argv);
        ldy = (int)atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(11, argc, argv));
        iemgui_all_loadcolors(&x->x_gui, argv+13, argv+14, argv+15);
        v = atom_getfloatarg(16, argc, argv);
    }
    else
        iemgui_new_getnames(&x->x_gui, 6, 0);
    argc -= 17; argv += 17;
    if((argc > 5) && (IS_A_FLOAT(argv,1))
        && (IS_A_SYMBOL(argv,2)) && (IS_A_FLOAT(argv,3))
        && (IS_A_FLOAT(argv,4)) && (IS_A_FLOAT(argv,5)))
    {
        if(IS_A_SYMBOL(argv, 0))
            movemode = atom_getsymbol(argv);
        argv++;
        ticks = (int)atom_getint(argv++);
        acol_sym = atom_getsymbol(argv++);
        arcwidth = (int)atom_getint(argv++);
        start_angle = (int)atom_getint(argv++);
        end_angle = (int)atom_getint(argv++);
    }
    x->x_gui.x_fsf.x_snd_able = (0 != x->x_gui.x_snd);
    x->x_gui.x_fsf.x_rcv_able = (0 != x->x_gui.x_rcv);
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    x->x_move_mode = movemode;
    x->x_exp = exp;
    if(ticks < 0)
        ticks = 0;
    if(ticks > 100)
        ticks = 100;
    x->x_ticks = ticks;
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;
    x->x_arc_width = arcwidth;
    x->x_start_angle = start_angle;
    x->x_end_angle = end_angle;
    if('#' == acol_sym->s_name[0])
        x->x_acol = (int)strtol(acol_sym->s_name+1, 0, 16);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    knb_check_wh(x, width, height);
    knb_range(x, min, max);
    iemgui_newzoom(&x->x_gui);
    knb_set(x, x->x_init = v);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return(x);}

static void knb_free(t_knb *x){
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void knob_setup(void){
    s_k_xy = gensym("xy");
    s_k_angle = gensym("angle");
    knb_class = class_new(gensym("knob"), (t_newmethod)knb_new,
        (t_method)knb_free, sizeof(t_knb), 0, A_GIMME, 0);
    class_addbang(knb_class,knb_bang);
    class_addfloat(knb_class,knb_float);
    class_addmethod(knb_class, (t_method)knb_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_motion, gensym("motion"),
        A_FLOAT, A_FLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_apply, gensym("dialog"), A_GIMME, 0);
    class_addmethod(knb_class, (t_method)knb_init, gensym("init"), A_GIMME, 0);
    class_addmethod(knb_class, (t_method)knb_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_circular, gensym("circular"), A_FLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_range, gensym("range"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_exp, gensym("exp"), A_FLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_color, gensym("color"), A_GIMME, 0);
    class_addmethod(knb_class, (t_method)knb_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(knb_class, (t_method)knb_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(knb_class, (t_method)knb_arc, gensym("arc"), A_DEFFLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_angle, gensym("angle"), A_FLOAT, A_DEFFLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_ticks, gensym("ticks"), A_DEFFLOAT, 0);
    class_addmethod(knb_class, (t_method)knb_zoom, gensym("zoom"), A_CANT, 0);
    knb_widgetbehavior.w_getrectfn  = knb_getrect;
    knb_widgetbehavior.w_displacefn = iemgui_displace;
    knb_widgetbehavior.w_selectfn   = iemgui_select;
    knb_widgetbehavior.w_activatefn = NULL;
    knb_widgetbehavior.w_deletefn   = knob_delete;
    knb_widgetbehavior.w_visfn      = iemgui_vis;
    knb_widgetbehavior.w_clickfn    = knb_newclick;
    class_setwidget(knb_class, &knb_widgetbehavior);
    class_setsavefn(knb_class, knb_save);
    class_setpropertiesfn(knb_class, knb_properties);
}
