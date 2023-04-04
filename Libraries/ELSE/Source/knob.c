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

typedef struct _knob{
    t_iemgui        x_gui;
    float           x_pos;  // 0-1 normalized position
    float           x_exp;
    float           x_init;
    int             x_start_angle;
    int             x_end_angle;
    int             x_range;
    int             x_offset;
    int             x_ticks;
    double          x_min;
    double          x_max;
    t_float         x_fval;
    int             x_circular;
    int             x_arc;
    int             x_discrete;
}t_knob;

t_widgetbehavior knob_widgetbehavior;
static t_class *knob_class;

static void knob_draw_io(t_knob *x,t_glist *glist, int old_snd_rcv_flags){
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

static void knob_update_knob(t_knob *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    float angle, angle0;
    int x0, y0, x1, y1;
    t_float pos = (x->x_pos - POS_MARGIN) / (1 - 2 * POS_MARGIN);
    char tag[128];
    if(pos < 0.0)
        pos = 0.0;
    else if(pos > 1.0)
        pos = 1.0;
    if(x->x_discrete){
        t_float ticks = (x->x_ticks < 2 ? 2 : (float)x->x_ticks) -1 ;
        pos = rint(pos * ticks) / ticks;
    }
    x0 = text_xpix(&x->x_gui.x_obj, glist);
    y0 = text_ypix(&x->x_gui.x_obj, glist);
    x1 = x0 + x->x_gui.x_w;
    y1 = y0 + x->x_gui.x_w;
    angle0 = (x->x_start_angle / 90.0 - 1) * M_PI / 2.0;
    angle = angle0 + pos * (x->x_end_angle - x->x_start_angle) / 180.0 * M_PI;
    if(x->x_arc){
        float zero_angle, zero_val;
        int aD, cD;
        int realw = x->x_gui.x_w / IEMGUI_ZOOM(x);
        int arcwidth = x->x_arc * rint(x->x_gui.x_w / 10);
        if((x->x_min * x->x_max) < 0){
            if(x->x_min < 0)
                zero_val = -x->x_min / (fabs(x->x_min) + fabs(x->x_max));
            else
                zero_val = -x->x_max / (fabs(x->x_min) + fabs(x->x_max));
            zero_angle = angle0 + zero_val * (x->x_end_angle - x->x_start_angle) / 180.0 * M_PI;
            angle0 = zero_angle;
        }
        if(x->x_arc)
            aD = IEMGUI_ZOOM(x);
        else
            aD = (((realw + 1)/ 2) + arcwidth) * IEMGUI_ZOOM(x) ;
        sprintf(tag, "%pARC", x);
        pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
            x0 + aD, y0 + aD, x1 - aD, y1 - aD);
        pdgui_vmess(0, "crs sf sf", canvas, "itemconfigure", tag,
            "-start", angle0 * -180.0 / M_PI,
            "-extent", (angle - angle0) * -179.99 / M_PI);
// center_visible
        sprintf(tag, "%pCENTER", x);
        cD = (arcwidth + 1) * IEMGUI_ZOOM(x);
        pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
            x0 + cD, y0 + cD, x1 - cD, y1 - cD);
    }
    #define NEAR(x) ((int)(x + 0.49))
// wiper
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

static void knob_update_ticks(t_knob *x, t_glist *glist){
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

static void knob_draw_update(t_knob *x, t_glist *glist){
    if(glist_isvisible(glist))
        knob_update_knob(x, glist);
}

static void knob_draw_config(t_knob *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    int xpos = text_xpix(&x->x_gui.x_obj, glist);
    int ypos = text_ypix(&x->x_gui.x_obj, glist);
    const int zoom = IEMGUI_ZOOM(x);
    int wiperwidth = (x->x_gui.x_w / 20);
    if(wiperwidth < 1)
        wiperwidth = 1;
    char tag[128];
    sprintf(tag, "%pARC", x);
    pdgui_vmess(0, "crs rk rk rs ri", canvas, "itemconfigure", tag,
        "-outline", x->x_gui.x_fcol,
        "-fill", x->x_gui.x_fcol,
        "-state",
        x->x_arc ? "normal" : "hidden",
        "-width",
        zoom);
    sprintf(tag, "%pCENTER", x);
    pdgui_vmess(0, "crs rk rk rs ri", canvas, "itemconfigure", tag,
        "-outline", x->x_gui.x_bcol,
        "-fill", x->x_gui.x_bcol,
        "-state",
        "normal", // always normal, not needed
        "-width",
        zoom);
    sprintf(tag, "%pWIPER", x);
    pdgui_vmess(0, "crs rk rs ri", canvas, "itemconfigure", tag,
        "-fill", x->x_gui.x_fcol,
        "-state",
        "normal", // always normal, not needed
        "-width",
        wiperwidth * zoom);
    sprintf(tag, "%pBASE", x);
    pdgui_vmess(0, "crs rk", canvas, "itemconfigure", tag,
        "-fill", x->x_gui.x_bcol);
    pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
        xpos, ypos,
        xpos + x->x_gui.x_w, ypos + x->x_gui.x_w);
    knob_update_knob(x, glist);
    knob_update_ticks(x, glist);
}

static void knob_draw_new(t_knob *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    char tag[128], tag_object[128], tag_select[128];
    char *tags[] = {tag_object, tag};
    char *seltags[] = {tag_object, tag, tag_select};
    sprintf(tag_object, "%pOBJ", x);
    sprintf(tag_select, "%pSELECT", x);
    sprintf(tag, "%pBASE", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "oval",
         0, 0, 0, 0, "-tags", 3, seltags);
    knob_draw_io(x, glist, 0);
    sprintf(tag, "%pARC", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "arc",
         0, 0, 0, 0, "-tags", 2, tags);
    sprintf(tag, "%pCENTER", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "oval",
         0, 0, 0, 0, "-tags", 2, tags);
    sprintf(tag, "%pWIPER", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "line",
         0, 0, 0, 0, "-tags", 2, tags);
    knob_draw_config(x, glist);
}

static void knob_draw_select(t_knob *x, t_glist *glist){
    t_canvas *canvas = glist_getcanvas(glist);
    int col = x->x_gui.x_fsf.x_selected ? IEM_GUI_COLOR_SELECTED : IEM_GUI_COLOR_NORMAL;
    char tag[128];
    sprintf(tag, "%pSELECT", x);
    pdgui_vmess(0, "crs rk", canvas, "itemconfigure", tag, "-outline", col);
}

/*static void knob_select(t_gobj *z, t_glist *glist, int sel){
    t_knob *x = (t_knob *)z;
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

// ------------------------ knob widgetbehaviour-----------------------------
#define GRECTRATIO 0
static void knob_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_knob *x = (t_knob *)z;
    *xp1 = text_xpix(&x->x_gui.x_obj, glist) + GRECTRATIO * x->x_gui.x_w;
    *yp1 = text_ypix(&x->x_gui.x_obj, glist) + GRECTRATIO * x->x_gui.x_w;
    *xp2 = text_xpix(&x->x_gui.x_obj, glist) + x->x_gui.x_w * (1 - GRECTRATIO);
    *yp2 = text_ypix(&x->x_gui.x_obj, glist) + x->x_gui.x_w * (1 - GRECTRATIO);
}

static void knob_save(t_gobj *z, t_binbuf *b){
    t_knob *x = (t_knob *)z;
    t_symbol *bflcol[3];
    t_symbol *srl[3];
    iemgui_save(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiis",
        gensym("#X"),
        gensym("obj"),
        (t_int)x->x_gui.x_obj.te_xpix,
        (t_int)x->x_gui.x_obj.te_ypix,
        atom_getsymbol(binbuf_getvec(x->x_gui.x_obj.te_binbuf)));
    // args
    binbuf_addv(b, "ifffssssfiiiiii", // 15 args
        x->x_gui.x_w / IEMGUI_ZOOM(x), // 01: i SIZE
        (float)x->x_min, // 02: f min
        (float)x->x_max, // 03: f max
        x->x_exp, // 04: f exp
        srl[0], // 05: s rcv
        srl[1], // 06: s snd
        bflcol[0], // 07: s bgcolor
        bflcol[1], // 08: s fgcolor
        x->x_init, // 09: f init
        x->x_circular, // 10: i circular
        x->x_ticks, // 11: i ticks
        x->x_discrete, // 12: i discrete
        x->x_arc, // 13: i arc
        x->x_range, // 14: i range
        x->x_offset); // 15: i offset
    binbuf_addv(b, ";");
}

void knob_check_size(t_knob *x, int s){
    x->x_gui.x_w = s < MIN_SIZE * IEMGUI_ZOOM(x) ? MIN_SIZE * IEMGUI_ZOOM(x) : s;
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
}

static void knob_exp(t_knob *x, t_floatarg f){
    x->x_exp = f;
}

static void knob_discrete(t_knob *x, t_floatarg f){
    x->x_discrete = (f != 0);
}

static void knob_properties(t_gobj *z, t_glist *owner){
    owner = NULL;
    t_knob *x = (t_knob *)z;
    t_symbol *srl[3];
    iemgui_properties(&x->x_gui, srl);
    pdgui_stub_vnew(&x->x_gui.x_obj.ob_pd, "pdtk_iemgui_dialog", x,
        "s s ffs ffs sfsfs i iss fi si sss ii ii kkk ikiii",
        "knob", // needed?
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
        x->x_circular,
        "",
        0, //
        srl[0] ? srl[0]->s_name : "",
        srl[1] ? srl[1]->s_name : "",
        srl[2] ? srl[2]->s_name : "",
        0, // x->x_gui.x_ldx,
        0, // x->x_gui.x_ldy,
        0, // was font style,
        0, // was font size,
        x->x_gui.x_bcol,
        x->x_gui.x_fcol,
        x->x_discrete, // was x->x_gui.x_lcol,
        x->x_ticks,
        "0x00", // x_acol 0x00
        x->x_arc,
        x->x_range,
        x->x_offset);
}

// get value from motion/position
static t_float knob_getfval(t_knob *x){
    t_float fval;
    t_float pos = (x->x_pos - POS_MARGIN) / (1 - 2 * POS_MARGIN);
    if(pos < 0.0)
        pos = 0.0;
    else if(pos > 1.0)
        pos = 1.0;
    if(x->x_discrete){
        t_float ticks = (x->x_ticks < 2 ? 2 : (float)x->x_ticks) - 1;
        pos = rint(pos * ticks) / ticks;
    }
    if(x->x_exp == 1){ // log
        if((x->x_min <= 0 && x->x_max >= 0) || (x->x_min >= 0 && x->x_max <= 0)){
            pd_error(x, "[knob]: range cannot contain '0' in log mode");
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
    
static void knob_set(t_knob *x, t_floatarg f){
    float old = x->x_pos;
    x->x_fval = f > x->x_max ? x->x_max : f < x->x_min ? x->x_min : f;
    double pos;
    if(x->x_exp == 1){ // log
        if((x->x_min <= 0 && x->x_max >= 0) || (x->x_min >= 0 && x->x_max <= 0)){
            pd_error(x, "[knob]: range cannot contain '0' in log mode");
            pos = 0;
        }
        else
            pos = log(x->x_fval/x->x_min) / log(x->x_max/x->x_min);
    }
    else{
        pos = (x->x_fval - x->x_min) / (x->x_max - x->x_min);
        if(fabs(x->x_exp) != 0){
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

static void knob_bang(t_knob *x){
    double out = x->x_fval;
    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, out);
}

static void knob_float(t_knob *x, t_floatarg f){
    knob_set(x, f);
    knob_bang(x);
}

static void knob_init(t_knob *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 1 && av->a_type == A_FLOAT)
        knob_float(x, atom_getfloat(av));
    x->x_init = x->x_fval;
}

// set start/end angles
static void knob_angle(t_knob *x, t_floatarg f1, t_floatarg f2){
    x->x_range = f1 > 360 ? 360 : f1 < 0 ? 0 : (int)f1;
    x->x_offset = f2 > 360 ? 360 : f2 < 0 ? 0 : (int)f2;
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
    if(glist_isvisible(x->x_gui.x_glist))
        knob_update_ticks(x, x->x_gui.x_glist);
    knob_draw_update(x, x->x_gui.x_glist);
}

#define SETCOLOR(a, col) do {char color[MAXPDSTRING]; snprintf(color, MAXPDSTRING-1, "#%06x", 0xffffff & col); color[MAXPDSTRING-1] = 0; SETSYMBOL(a, gensym(color));} while(0)
static void knob_apply(t_knob *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_symbol *srl[3];
    int size = (int)atom_getintarg(0, argc, argv);
    x->x_exp = atom_getfloatarg(1, argc, argv);
    float min = atom_getfloatarg(2, argc, argv);
    float max = atom_getfloatarg(3, argc, argv);
    double init = atom_getfloatarg(4, argc, argv);
    int circular = atom_getintarg(16, argc, argv);
    int ticks = atom_getintarg(17, argc, argv);
    int arc = atom_getintarg(19, argc, argv) != 0;
    int range = atom_getintarg(20, argc, argv);
    int offset = atom_getintarg(21, argc, argv);
    int sr_flags;

    t_atom undo[23];
    iemgui_setdialogatoms(&x->x_gui, 23, undo);
    SETFLOAT(undo+2, x->x_min);
    SETFLOAT(undo+3, x->x_max);
    SETFLOAT(undo+4, x->x_init);
    SETFLOAT(undo+16, x->x_circular);
    SETFLOAT(undo+17, x->x_ticks);
    SETFLOAT(undo+19, x->x_arc);
    SETFLOAT(undo+20, x->x_range);
    SETFLOAT(undo+21, x->x_offset);
    pd_undo_set_objectstate(x->x_gui.x_glist, (t_pd*)x, gensym("dialog"),
        23, undo, argc, argv);
    x->x_circular = circular;
    if(ticks < 0)
        ticks = 0;
    x->x_ticks = ticks;
    x->x_arc = arc;
    knob_angle(x, (float)range, (float)offset);
    if(x->x_init != init){
        knob_float(x, init);
        x->x_init = x->x_fval;
    }
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);
    knob_check_size(x, size * IEMGUI_ZOOM(x));
    knob_range(x, min, max);
    knob_set(x, x->x_fval);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(x->x_gui.x_glist, (t_text *)x);
}

static void knob_motion(t_knob *x, t_floatarg dx, t_floatarg dy){
    float old = x->x_pos;
    float delta = -dy;
    if(fabs(dx) > fabs(dy))
        delta = dx;
    delta /= (float)(DEFAULT_SENSITIVITY * IEMGUI_ZOOM(x));
    if(x->x_gui.x_fsf.x_finemoved)
        delta *= 0.01;
    double pos = x->x_pos + delta;
    x->x_pos = pos > 1 ? 1 : pos < 0 ? 0 : pos;
    x->x_fval = knob_getfval(x);
    if(old != x->x_pos){
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        knob_bang(x);
    }
}

static int xm, ym;

static void knob_motion_angular(t_knob *x, t_floatarg dx, t_floatarg dy){
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
    x->x_fval = knob_getfval(x);
    if(old != x->x_pos){
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        knob_bang(x);
    }
}

static void knob_click(t_knob *x, t_floatarg xpos, t_floatarg ypos, t_floatarg shift, t_floatarg ctrl, t_floatarg alt){
    alt = ctrl = shift = 0;
    knob_bang(x);
    glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
        (t_glistmotionfn)(x->x_circular ? knob_motion_angular : knob_motion), 0, xm = xpos, ym = ypos);
}

static int knob_newclick(t_gobj *z, struct _glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit){
    glist = 0;
    t_knob *x = (t_knob *)z;
    if(dbl){
        knob_set(x, x->x_init);
        knob_bang(x);
        return(1);
    }
    if(doit){
        x->x_gui.x_fsf.x_finemoved = shift;
        knob_click(x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift, 0, (t_floatarg)alt);
    }
    return(1);
}

static void knob_size(t_knob *x, t_floatarg f){
    knob_check_size(x, (int)f * IEMGUI_ZOOM(x));
    iemgui_size((void *)x, &x->x_gui);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
}

static void knob_circular(t_knob *x, t_floatarg f){
    x->x_circular = (f != 0);
}

static void knob_send(t_knob *x, t_symbol *s){
    iemgui_send(x, &x->x_gui, s);
}

static void knob_receive(t_knob *x, t_symbol *s){
    iemgui_receive(x, &x->x_gui, s);
}

static void knob_arc(t_knob *x, t_floatarg f){
    x->x_arc = f != 0;
    if(glist_isvisible(x->x_gui.x_glist)){
        knob_draw_config(x, x->x_gui.x_glist);
        knob_draw_update(x, x->x_gui.x_glist);
    }
}

static void knob_ticks(t_knob *x, t_floatarg f){
    x->x_ticks = (int)f;
    if(f <= 0)
        x->x_ticks = 0;
    if(f == 1)
        x->x_ticks = 2;
    if(f > 100)
        x->x_ticks = 100;
    if(glist_isvisible(x->x_gui.x_glist))
        knob_update_ticks(x, x->x_gui.x_glist);
}

static void knob_zoom(t_knob *x, t_floatarg f){
    iemgui_zoom(&x->x_gui, f);
}

static void *knob_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_knob *x = (t_knob *)iemgui_new(knob_class);
    float initvalue = 0, exp = 0;
    int size = 30, circular = 0, ticks = 0, discrete = 0;
    int arc = 1, range = 360, offset = 0;
    double min = 0.0, max = 127.0;
    x->x_gui.x_bcol = 0xDFDFDF, x->x_gui.x_fcol = 0x000000;
    IEMGUI_SETDRAWFUNCTIONS(x, knob);
    if(argc){
        size = atom_getintarg(0, argc, argv);
        min = (double)atom_getfloatarg(1, argc, argv);
        max = (double)atom_getfloatarg(2, argc, argv);
        exp = atom_getfloatarg(3, argc, argv);
        iemgui_new_getnames(&x->x_gui, 4, argv);
        iemgui_all_loadcolors(&x->x_gui, argv+6, argv+7, argv+7);
        initvalue = atom_getfloatarg(8, argc, argv);
        circular = atom_getintarg(9, argc, argv);
        ticks = atom_getintarg(10, argc, argv);
        discrete = atom_getintarg(11, argc, argv);
        arc = atom_getintarg(12, argc, argv);
        range = atom_getintarg(13, argc, argv);
        offset = atom_getintarg(14, argc, argv);
    }
    x->x_exp = exp;
    x->x_circular = circular;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    x->x_gui.x_fsf.x_snd_able = (x->x_gui.x_snd != 0);
    x->x_gui.x_fsf.x_rcv_able = (x->x_gui.x_rcv != 0);
    knob_ticks(x, ticks);
    x->x_discrete = discrete;
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    x->x_arc = arc;
    knob_angle(x, range, offset);
    iemgui_verify_snd_ne_rcv(&x->x_gui);
    iemgui_newzoom(&x->x_gui);
    knob_check_size(x, size);
    knob_range(x, min, max);
    knob_set(x, x->x_init = initvalue);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return(x);
}

static void knob_free(t_knob *x){
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

void knob_setup(void){
    knob_class = class_new(gensym("knob"), (t_newmethod)knob_new,
        (t_method)knob_free, sizeof(t_knob), 0, A_GIMME, 0);
    class_addbang(knob_class,knob_bang);
    class_addfloat(knob_class,knob_float);
    class_addmethod(knob_class, (t_method)knob_init, gensym("init"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_circular, gensym("circular"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_range, gensym("range"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_exp, gensym("exp"), A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_discrete, gensym("discrete"), A_FLOAT, 0);
//    class_addmethod(knob_class, (t_method)knob_color, gensym("color"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(knob_class, (t_method)knob_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(knob_class, (t_method)knob_arc, gensym("arc"), A_DEFFLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_angle, gensym("angle"), A_FLOAT, A_DEFFLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_ticks, gensym("ticks"), A_DEFFLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_zoom, gensym("zoom"), A_CANT, 0);
    class_addmethod(knob_class, (t_method)knob_apply, gensym("dialog"), A_GIMME, 0);
    class_addmethod(knob_class, (t_method)knob_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(knob_class, (t_method)knob_motion, gensym("motion"), A_FLOAT, A_FLOAT, 0);
    knob_widgetbehavior.w_getrectfn  = knob_getrect;
    knob_widgetbehavior.w_displacefn = iemgui_displace;
    knob_widgetbehavior.w_selectfn   = iemgui_select;
    knob_widgetbehavior.w_activatefn = NULL;
    knob_widgetbehavior.w_deletefn   = knob_delete;
    knob_widgetbehavior.w_visfn      = iemgui_vis;
    knob_widgetbehavior.w_clickfn    = knob_newclick;
    class_setwidget(knob_class, &knob_widgetbehavior);
    class_setsavefn(knob_class, knob_save);
    class_setpropertiesfn(knob_class, knob_properties);
}
