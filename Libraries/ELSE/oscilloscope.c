// stolen from scope~

#include <stdlib.h>
#include <string.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "magic.h"

#define SCOPE_MINSIZE       18
#define SCOPE_MINPERIOD     2
#define SCOPE_MAXPERIOD     8192
#define SCOPE_MINBUFSIZE    8
#define SCOPE_MAXBUFSIZE    256
#define SCOPE_MINDELAY      0
#define SCOPE_SELCOLOR     "#5aadef" // select color from max
#define SCOPE_SELBDWIDTH    3
#define HANDLE_SIZE         12
#define SCOPE_GUICHUNK      128 // performance-related hacks, LATER investigate

typedef struct _edit_proxy{
    t_object        p_obj;
    t_symbol       *p_sym;
    t_clock        *p_clock;
    struct _scope *p_cnv;
}t_edit_proxy;

typedef struct _scope{
    t_object        x_obj;
    t_inlet        *x_rightinlet;
    t_glist        *x_glist;
    t_canvas       *x_cv;
    t_edit_proxy   *x_proxy;
    unsigned char   x_bg[3], x_fg[3], x_gg[3];
    float           x_xbuffer[SCOPE_MAXBUFSIZE*4];
    float           x_ybuffer[SCOPE_MAXBUFSIZE*4];
    float           x_xbuflast[SCOPE_MAXBUFSIZE*4];
    float           x_ybuflast[SCOPE_MAXBUFSIZE*4];
    float           x_min, x_max;
    float           x_trigx, x_triglevel;
    float           x_ksr;
    float           x_currx, x_curry;
    int             x_select;
    int             x_width, x_height;
    int             x_drawstyle;
    int             x_delay;
    int             x_trigmode;
    int             x_bufsize, x_lastbufsize;
    int             x_period;
    int             x_bufphase, x_precount, x_phase;
    int             x_xymode, x_frozen, x_retrigger;
    int             x_zoom;
    int             x_edit;
    t_float        *x_signalscalar;
    int             x_rcv_set;
    int             x_flag;
    int             x_r_flag;
    t_symbol       *x_receive;
    t_symbol       *x_rcv_raw;
    t_symbol       *x_bindsym;
    t_clock        *x_clock;
    t_pd           *x_handle;
}t_scope;

typedef struct _handle{
    t_pd            h_pd;
    t_scope        *h_master;
    t_symbol       *h_bindsym;
    char            h_pathname[64], h_outlinetag[64];
    int             h_dragon, h_dragx, h_dragy;
    int             h_selectedmode;
}t_handle;

static t_class *scope_class, *handle_class, *edit_proxy_class;
static t_widgetbehavior scope_widgetbehavior;

static void scope_getrect(t_gobj *z, t_glist *gl, int *xp1, int *yp1, int *xp2, int *yp2);

// ----------------- DRAW ----------------------------------------------------------------
static void scope_draw_handle(t_scope *x, int state){
    t_handle *sh = (t_handle *)x->x_handle;
    if(state){
        if(sh->h_selectedmode == 0){
            sys_vgui("canvas %s -width %d -height %d -bg %s -bd 0 -cursor bottom_right_corner\n",
                sh->h_pathname, HANDLE_SIZE, HANDLE_SIZE, SCOPE_SELCOLOR);
            sh->h_selectedmode = 1;
        }
        int x1, y1, x2, y2;
        scope_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
        sys_vgui(".x%lx.c create window %d %d -anchor nw -width %d -height %d -window %s -tags all%lx\n",
            x->x_cv,
            x2 - (HANDLE_SIZE*x->x_zoom - SCOPE_SELBDWIDTH*x->x_zoom) - 2*x->x_zoom,
            y2 - (HANDLE_SIZE*x->x_zoom - SCOPE_SELBDWIDTH*x->x_zoom) - 2*x->x_zoom,
            HANDLE_SIZE*x->x_zoom,
            HANDLE_SIZE*x->x_zoom,
            sh->h_pathname,
            x);
        sys_vgui("bind %s <Button> {pdsend [concat %s _click 1 \\;]}\n", sh->h_pathname, sh->h_bindsym->s_name);
        sys_vgui("bind %s <ButtonRelease> {pdsend [concat %s _click 0 \\;]}\n", sh->h_pathname, sh->h_bindsym->s_name);
        sys_vgui("bind %s <Motion> {pdsend [concat %s _motion %%x %%y \\;]}\n", sh->h_pathname, sh->h_bindsym->s_name);
    }
    else{
        sh->h_selectedmode = 0;
        sys_vgui("destroy %s\n", sh->h_pathname);
    }
}

static void scope_drawfg(t_scope *x, t_canvas *cv, int x1, int y1, int x2, int y2){
    float dx, dy, xx = 0, yy = 0, oldx, oldy, sc, xsc, ysc;
    float *xbp = x->x_xbuflast, *ybp = x->x_ybuflast;
    int bufsize = x->x_lastbufsize;
    if(x->x_xymode == 1){
        dx = (float)(x2 - x1) / (float)bufsize;
        oldx = x1;
        sc = ((float)x->x_height - 2.) / (float)(x->x_max - x->x_min);
    }
    else if(x->x_xymode == 2){
        dy = (float)(y2 - y1) / (float)bufsize;
        oldy = y1;
        sc = ((float)x->x_width - 2.) / (float)(x->x_max - x->x_min);
    }
    else if(x->x_xymode == 3){
        xsc = ((float)x->x_width - 2.) / (float)(x->x_max - x->x_min);
        ysc = ((float)x->x_height - 2.) / (float)(x->x_max - x->x_min);
    }
    sys_vgui(".x%lx.c create line \\\n", cv);
    for(int i = 0; i < bufsize; i++){
        if(x->x_xymode == 1){
            xx = oldx;
            yy = (y2 - 1) - sc * (*xbp++ - x->x_min);
            if(yy > y2)
                yy = y2;
            else if(yy < y1)
                yy = y1;
            oldx += dx;
        }
        else if(x->x_xymode == 2){
            yy = oldy;
            xx = (x2 - 1) - sc * (*ybp++ - x->x_min);
            if(xx > x2)
                xx = x2;
            else if(xx < x1)
                xx = x1;
            oldy += dy;
        }
        else if(x->x_xymode == 3){
            xx = x1 + xsc * (*xbp++ - x->x_min);
            yy = y2 - ysc * (*ybp++ - x->x_min);
            if(xx > x2)
                xx = x2;
            else if(xx < x1)
                xx = x1;
            if(yy > y2)
                yy = y2;
            else if(yy < y1)
                yy = y1;
        }
        sys_vgui("%d %d \\\n", (int)xx, (int)yy);
    }
    sys_vgui("-fill #%2.2x%2.2x%2.2x -width %d -tags {fg%lx all%lx}\n",
        x->x_fg[0], x->x_fg[1], x->x_fg[2], x->x_zoom, x, x);
}

static void scope_drawmargins(t_scope *x, t_canvas *cv, int x1, int y1, int x2, int y2){
    // margin lines:  mask overflow so they appear as gaps and not clipped signal values, LATER rethink
    sys_vgui(".x%lx.c create line %d %d %d %d %d %d %d %d %d %d -fill #%2.2x%2.2x%2.2x -width %d -tags {margin%lx all%lx}\n",
           cv, x1, y1 , x2, y1, x2, y2, x1, y2, x1, y1, x->x_bg[0], x->x_bg[1], x->x_bg[2], x->x_zoom, x, x);
}

static void scope_draw_grid(t_scope *x, t_canvas *cv, int x1, int y1, int x2, int y2){
    float dx = (x2-x1)*0.125, dy = (y2-y1)*0.25, xx, yy;
    int i;
    for(i = 0, xx = x1 + dx; i < 7; i++, xx += dx)
        sys_vgui(".x%lx.c create line %f %d %f %d -width %d -tags {gr%lx all%lx} -fill #%2.2x%2.2x%2.2x\n",
            cv, xx, y1, xx, y2, x->x_zoom, x, x, x->x_gg[0], x->x_gg[1], x->x_gg[2]);
    for(i = 0, yy = y1 + dy; i < 3; i++, yy += dy)
        sys_vgui(".x%lx.c create line %d %f %d %f -width %d -tags {gr%lx all%lx} -fill #%2.2x%2.2x%2.2x\n",
            cv, x1, yy, x2, yy, x->x_zoom, x, x, x->x_gg[0], x->x_gg[1], x->x_gg[2]);
}

static void scope_draw_bg(t_scope *x, t_canvas *cv, int x1, int y1, int x2, int y2){
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill #%2.2x%2.2x%2.2x -width %d -tags {bg%lx all%lx}\n",
        cv, x1, y1, x2, y2, x->x_bg[0], x->x_bg[1], x->x_bg[2], x->x_zoom, x, x);
}

static void scope_draw_inlets(t_scope *x){
    if(x->x_edit && x->x_receive == &s_){
        t_canvas *cv = glist_getcanvas(x->x_glist);
        int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags {%lx_in1 inlets%lx all%lx}\n",
            cv, xpos, ypos, xpos+(IOWIDTH*x->x_zoom), ypos+(IHEIGHT*x->x_zoom), x, x, x);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags {%lx_in2 inlets%lx all%lx}\n",
            cv, xpos+x->x_width, ypos, xpos+x->x_width-(IOWIDTH*x->x_zoom), ypos+(IHEIGHT*x->x_zoom), x, x, x);
    }
}

static void scope_draw(t_scope *x, t_canvas *cv){
    int x1, y1, x2, y2;
    scope_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
    scope_draw_bg(x, cv, x1, y1, x2, y2);
    scope_draw_grid(x, cv, x1, y1, x2, y2);
    if(x->x_xymode)
        scope_drawfg(x, cv, x1, y1, x2, y2);
    scope_drawmargins(x, cv, x1, y1, x2, y2);
    scope_draw_inlets(x);
}

static void scope_redraw(t_scope *x, t_canvas *cv){
    char chunk[32 * SCOPE_GUICHUNK];  // LATER estimate
    char *chunkp = chunk;
    int bufsize, nleft = bufsize = x->x_lastbufsize, x1, y1, x2, y2, xymode = x->x_xymode;
    scope_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
    float dx, dy, xx, yy, oldx, oldy, sc, xsc, ysc;
    float *xbp = x->x_xbuflast, *ybp = x->x_ybuflast;
    if(xymode == 1){
        dx = (float)(x2 - x1) / (float)bufsize;
        oldx = x1;
        sc = ((float)x->x_height - 2.) / (float)(x->x_max - x->x_min);
    }
    else if(xymode == 2){
        dy = (float)(y2 - y1) / (float)bufsize;
        oldy = y1;
        sc = ((float)x->x_width - 2.) / (float)(x->x_max - x->x_min);
    }
    else if(xymode == 3){
        xsc = ((float)x->x_width - 2.) / (float)(x->x_max - x->x_min);
        ysc = ((float)x->x_height - 2.) / (float)(x->x_max - x->x_min);
    }
    sys_vgui(".x%lx.c coords fg%lx\\\n", cv, x);
    while(nleft > SCOPE_GUICHUNK){
        int i = SCOPE_GUICHUNK;
        while(i--){
            if(xymode == 1){
                xx = oldx;
                yy = (y2 - 1) - sc * (*xbp++ - x->x_min);
                if(yy > y2)
                    yy = y2;
                else if(yy < y1)
                    yy = y1;
                oldx += dx;
            }
            else if(xymode == 2){
                yy = oldy;
                xx = (x2 - 1) - sc * (*ybp++ - x->x_min);
                if(xx > x2)
                    xx = x2;
                else if(xx < x1)
                    xx = x1;
                oldy += dy;
            }
            else if(xymode == 3){
                xx = x1 + xsc * (*xbp++ - x->x_min);
                yy = y2 - ysc * (*ybp++ - x->x_min);
                if(xx > x2)
                    xx = x2;
                else if(xx < x1)
                    xx = x1;
                if(yy > y2)
                    yy = y2;
                else if(yy < y1)
                    yy = y1;
            }
            sprintf(chunkp, "%d %d ", (int)xx, (int)yy);
            chunkp += strlen(chunkp);
        }
        strcpy(chunkp, "\\\n");
        sys_gui(chunk);
        chunkp = chunk;
        nleft -= SCOPE_GUICHUNK;
    }
    while(nleft--){
        if(xymode == 1){
            xx = oldx;
            yy = (y2 - 1) - sc * (*xbp++ - x->x_min);
            if(yy > y2)
                yy = y2;
            else if(yy < y1)
                yy = y1;
            oldx += dx;
        }
        else if(xymode == 2){
            yy = oldy;
            xx = (x2 - 1) - sc * (*ybp++ - x->x_min);
            if(xx > x2)
                xx = x2;
            else if(xx < x1)
                xx = x1;
            oldy += dy;
        }
        else if(xymode == 3){
            xx = x1 + xsc * (*xbp++ - x->x_min);
            yy = y2 - ysc * (*ybp++ - x->x_min);
            if(xx > x2)
                xx = x2;
            else if(xx < x1)
                xx = x1;
            if(yy > y2)
                yy = y2;
            else if(yy < y1)
                yy = y1;
        }
        sprintf(chunkp, "%d %d ", (int)xx, (int)yy);
        chunkp += strlen(chunkp);
    }
    strcpy(chunkp, "\n");
    sys_gui(chunk);
}

//------------------ WIDGET -----------------------------------------------------------------
static void scope_getrect(t_gobj *z, t_glist *gl, int *xp1, int *yp1, int *xp2, int *yp2){
    t_scope *x = (t_scope *)z;
    float x1 = text_xpix((t_text *)x, gl), y1 = text_ypix((t_text *)x, gl);
    *xp1 = x1;
    *yp1 = y1;
    *xp2 = x1 + x->x_width;
    *yp2 = y1 + x->x_height;
}

static void scope_displace(t_gobj *z, t_glist *gl, int dx, int dy){
    t_scope *x = (t_scope *)z;
    x->x_obj.te_xpix += dx, x->x_obj.te_ypix += dy;
    t_canvas *cv = glist_getcanvas(gl);
    sys_vgui(".x%lx.c move all%lx %d %d\n", cv, x, dx*x->x_zoom, dy*x->x_zoom);
    canvas_fixlinesfor(cv, (t_text*)x);
}

static void scope_select(t_gobj *z, t_glist *glist, int state){
    t_scope *x = (t_scope *)z;
    t_canvas *cv = glist_getcanvas(glist);
    x->x_select = state;
    if(state)
        sys_vgui(".x%lx.c itemconfigure bg%lx -outline %s -width %d -fill #%2.2x%2.2x%2.2x\n",
            cv, x, SCOPE_SELCOLOR, SCOPE_SELBDWIDTH * x->x_zoom, x->x_bg[0], x->x_bg[1], x->x_bg[2]);
    else
        sys_vgui(".x%lx.c itemconfigure bg%lx -outline black -width %d -fill #%2.2x%2.2x%2.2x\n",
            cv, x, x->x_zoom, x->x_bg[0], x->x_bg[1], x->x_bg[2]);
}

static void scope_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void scope_mouserelease(t_scope* x){
    if(!x->x_edit)
        x->x_frozen = 0;
}

static void scope_draw_all(t_scope* x){
    x->x_cv = glist_getcanvas(x->x_glist);
    t_handle *sh = (t_handle *)x->x_handle;
    sprintf(sh->h_pathname, ".x%lx.h%lx", (unsigned long)x->x_cv, (unsigned long)sh);
    sys_vgui(".x%lx.c bind all%lx <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n", x->x_cv, x, x->x_bindsym->s_name);
    int bufsize = x->x_bufsize;
    x->x_bufsize = x->x_lastbufsize;
    scope_draw(x, x->x_cv);
    x->x_bufsize = bufsize;
}

static void scope_vis(t_gobj *z, t_glist *glist, int vis){
    t_scope *x = (t_scope *)z;
    x->x_cv = glist_getcanvas(glist);
    if(vis){
        t_handle *sh = (t_handle *)x->x_handle;
        sprintf(sh->h_pathname, ".x%lx.h%lx", (unsigned long)x->x_cv, (unsigned long)sh);
        sys_vgui(".x%lx.c bind all%lx <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n", x->x_cv, x, x->x_bindsym->s_name);
        int bufsize = x->x_bufsize;
        x->x_bufsize = x->x_lastbufsize;
        scope_draw(x, x->x_cv);
        x->x_bufsize = bufsize;
        scope_draw_handle(x, x->x_edit);
    }
    else
        sys_vgui(".x%lx.c delete all%lx\n", (unsigned long)x->x_cv, x);
}

static int scope_click(t_gobj *z, t_glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit){
    t_scope *x = (t_scope *)z;
    glist = NULL, shift = xpix = ypix = alt = dbl = 0;
    if(doit)
        x->x_frozen = 1;
    return(1);
}

//------------------------------ METHODS ------------------------------------------------------------------
static void scope_bufsize(t_scope *x, t_floatarg f){
    int size = f < SCOPE_MINBUFSIZE ? SCOPE_MINBUFSIZE : f > SCOPE_MAXBUFSIZE ? SCOPE_MAXBUFSIZE : (int)f;
    if(x->x_bufsize != size){
        x->x_bufsize = size;
        pd_float((t_pd *)x->x_rightinlet, x->x_bufsize);
        x->x_phase = x->x_bufphase = x->x_precount = 0;
    }
}

static void scope_period(t_scope *x, t_floatarg f){
    int period = f < 2 ? 2 : f > 8192 ? 8192 : (int)f;
    if(x->x_period != period){
        x->x_period = period;
        x->x_phase = x->x_bufphase = x->x_precount = 0;
    }
}

static void scope_range(t_scope *x, t_floatarg f1, t_floatarg f2){
    float min = f1, max = f2;
    if(min == max)
        return;
    if(min > max){
        max = f1;
        min = f2;
    }
    if(x->x_min != min || x->x_max != max)
        x->x_min = min, x->x_max = max;
}

static void scope_delay(t_scope *x, t_floatarg f){
    int delay = f < 0 ? 0 : (int)f;
    if(x->x_delay != delay)
        x->x_delay = delay;
}

static void scope_drawstyle(t_scope *x, t_floatarg f){
    int drawstyle = (int)f;
    if(x->x_drawstyle != drawstyle)
        x->x_drawstyle = drawstyle;
}

static void scope_trigger(t_scope *x, t_floatarg f){
    int trig = f < 0 ? 0 : f > 2 ? 2 : (int)f;
    if(x->x_trigmode != trig){
        x->x_trigmode = trig;
        if(x->x_trigmode == 0) // no trigger
            x->x_retrigger = 0;
    }
}

static void scope_triglevel(t_scope *x, t_floatarg f){
    if(x->x_triglevel != f)
        x->x_triglevel = f;
}

static void scope_fgcolor(t_scope *x, t_float r, t_float g, t_float b){ // scale is 0-255
    unsigned char red = (unsigned char)(r < 0 ? 0 : r > 255 ? 255 : r);
    unsigned char green = (unsigned char)(g < 0 ? 0 : g > 255 ? 255 : g);
    unsigned char blue = (unsigned char)(b < 0 ? 0 : b > 255 ? 255 : b);
    if(x->x_fg[0] != red || x->x_fg[1] != green || x->x_fg[2] != blue){
        x->x_fg[0] = red, x->x_fg[1] = green, x->x_fg[2] = blue;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            sys_vgui(".x%lx.c itemconfigure fg%lx -fill #%2.2x%2.2x%2.2x\n",
                glist_getcanvas(x->x_glist), x, x->x_fg[0], x->x_fg[1], x->x_fg[2]);
    }
}

static void scope_bgcolor(t_scope *x, t_float r, t_float g, t_float b){ // scale: 0-255
    unsigned char red = (unsigned char)(r < 0 ? 0 : r > 255 ? 255 : r);
    unsigned char green = (unsigned char)(g < 0 ? 0 : g > 255 ? 255 : g);
    unsigned char blue = (unsigned char)(b < 0 ? 0 : b > 255 ? 255 : b);
    if(x->x_bg[0] != red || x->x_bg[1] != green || x->x_bg[2] != blue){
        x->x_bg[0] = red, x->x_bg[1] = green, x->x_bg[2] = blue;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            sys_vgui(".x%lx.c itemconfigure bg%lx -fill #%2.2x%2.2x%2.2x\n",
                glist_getcanvas(x->x_glist), x, x->x_bg[0], x->x_bg[1], x->x_bg[2]);
    }
}

static void scope_gridcolor(t_scope *x, t_float r, t_float g, t_float b){ // scale: 0-255
    unsigned char red = (unsigned char)(r < 0 ? 0 : r > 255 ? 255 : r);
    unsigned char green = (unsigned char)(g < 0 ? 0 : g > 255 ? 255 : g);
    unsigned char blue = (unsigned char)(b < 0 ? 0 : b > 255 ? 255 : b);
    if(x->x_gg[0] != red || x->x_gg[1] != green || x->x_gg[2] != blue){
        x->x_gg[0] = red, x->x_gg[1] = green, x->x_gg[2] = blue;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            sys_vgui(".x%lx.c itemconfigure gr%lx -fill #%2.2x%2.2x%2.2x\n",
                glist_getcanvas(x->x_glist), x, x->x_gg[0], x->x_gg[1], x->x_gg[2]);
    }
}

static void scope_dim(t_scope *x, t_float w, t_float h){
    int width = (int)(w < SCOPE_MINSIZE ? SCOPE_MINSIZE : w);
    int height = (int)(h < SCOPE_MINSIZE ? SCOPE_MINSIZE : h);
    if(x->x_width != width || x->x_height != height){
        x->x_width = width, x->x_height = height;
        sys_vgui(".x%lx.c delete all%lx\n", (unsigned long)glist_getcanvas(x->x_glist), x);
        if(gobj_shouldvis((t_gobj *)x, x->x_glist) && glist_isvisible(x->x_glist))
            scope_draw_all(x);
        canvas_fixlinesfor(x->x_glist, (t_text *)x);
    }
}

static void scope_receive(t_scope *x, t_symbol *s){
    if(s == gensym(""))
        s = gensym("empty");
    t_symbol *rcv = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
    if(rcv != x->x_receive){
        if(x->x_receive != &s_)
            pd_unbind(&x->x_obj.ob_pd, x->x_receive);
        x->x_rcv_set = 1;
        x->x_rcv_raw = s;
        x->x_receive = rcv;
        if(x->x_receive == &s_ && x->x_edit && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            scope_draw_inlets(x);
        else{
            pd_bind(&x->x_obj.ob_pd, x->x_receive);
            if(x->x_edit && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
                sys_vgui(".x%lx.c delete inlets%lx\n", glist_getcanvas(x->x_glist), x);
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
            scope_draw_handle(p->p_cnv, edit);
            if(edit)
                scope_draw_inlets(p->p_cnv);
            else
                sys_vgui(".x%lx.c delete inlets%lx\n", glist_getcanvas(p->p_cnv->x_glist), p->p_cnv);
        }
    }
}

static void scope_zoom(t_scope *x, t_floatarg zoom){
    float mul = (zoom == 1. ? 0.5 : 2.);
    scope_dim(x, (float)x->x_width*mul, (float)x->x_height*mul);
    x->x_zoom = (int)zoom;
}

// --------------------- handle ---------------------------------------------------
static void handle__click_callback(t_handle *sh, t_floatarg f){
    int click = (int)f;
    t_scope *x = sh->h_master;
    if(sh->h_dragon && click == 0){
        sys_vgui(".x%lx.c delete %s\n", x->x_cv, sh->h_outlinetag);
        scope_dim(x, x->x_width + sh->h_dragx, x->x_height + sh->h_dragy);
        scope_draw_handle(x, 1);
        scope_select((t_gobj *)x, x->x_glist, x->x_select);
    }
    else if(!sh->h_dragon && click){
        int x1, y1, x2, y2;
        scope_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -outline %s -width %d -tags %s\n",
            x->x_cv, x1, y1, x2, y2, SCOPE_SELCOLOR, SCOPE_SELBDWIDTH, sh->h_outlinetag);
        sh->h_dragx = sh->h_dragy = 0;
    }
    sh->h_dragon = click;
}

static void handle__motion_callback(t_handle *sh, t_floatarg f1, t_floatarg f2){
    if(sh->h_dragon){
        t_scope *x = sh->h_master;
        int dx = (int)f1, dy = (int)f2, x1, y1, x2, y2, newx, newy;
        scope_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
        newx = x2 + dx, newy = y2 + dy;
        if(newx < x1 + SCOPE_MINSIZE*x->x_zoom)
            newx = x1 + SCOPE_MINSIZE*x->x_zoom;
        if(newy < y1 + SCOPE_MINSIZE*x->x_zoom)
            newy = y1 + SCOPE_MINSIZE*x->x_zoom;
        sys_vgui(".x%lx.c coords %s %d %d %d %d\n", x->x_cv, sh->h_outlinetag, x1, y1, newx, newy);
        sh->h_dragx = dx, sh->h_dragy = dy;
    }
}

//------------------------------------------------------------
static t_int *scope_perform(t_int *w){
    t_scope *x = (t_scope *)(w[1]);
    if(!x->x_xymode || x->x_frozen) // do nothing
        return(w+5);
    if(!gobj_shouldvis((t_gobj *)x, x->x_glist) || !glist_isvisible(x->x_glist))
        return(w+5);
    int nblock = (int)(w[2]);
    int bufphase = x->x_bufphase;
    int bufsize = (int)*x->x_signalscalar;
    if(bufsize != x->x_bufsize){
        scope_bufsize(x, bufsize);
        bufsize = x->x_bufsize;
    }
    if(bufphase < bufsize){
        if(x->x_precount >= nblock)
            x->x_precount -= nblock;
        else{
            t_float *in1 = (t_float *)(w[3]);
            t_float *in2 = (t_float *)(w[4]);
            t_float *in;
            int phase = x->x_phase;
            int period = x->x_period;
            float freq = 1. / period;
            float *bp1;
            float *bp2;
            float currx = x->x_currx;
            float curry = x->x_curry;
            if(x->x_precount > 0){
                nblock -= x->x_precount;
                in1 += x->x_precount;
                in2 += x->x_precount;
                phase = 0;
                bufphase = 0;
                x->x_precount = 0;
            }
            if(x->x_trigmode && (x->x_xymode == 1 || x->x_xymode == 2)){
                if(x->x_xymode == 1)
                    in = in1;
                else
                    in = in2;
                while(x->x_retrigger){
                    float triglevel = x->x_triglevel;
                    if(x->x_trigmode == 1){ // Trigger Up
                        if(x->x_trigx < triglevel){
                            while(nblock--){
                                if(*in >= triglevel){
                                    x->x_retrigger = 0;
                                    phase = 0;
                                    bufphase = 0;
                                    break;
                                }
                                else in++;
                            }
                        }
                        else{
                            while(nblock--){
                                if(*in++ < triglevel){
                                    x->x_trigx = triglevel - 1.;
                                    break;
                                }
                            }
                        }
                    }
                    else{ // Trigger Down
                        if(x->x_trigx > triglevel){
                            while(nblock--){
                                if(*in <= triglevel){
                                    phase = 0;
                                    bufphase = 0;
                                    x->x_retrigger = 0;
                                    break;
                                }
                                else in++;
                            }
                        }
                        else{
                            while(nblock--){
                                if(*in++ > triglevel){
                                    x->x_trigx = triglevel + 1.;
                                    break;
                                }
                            }
                        }
                    }
                    if(nblock <= 0){
                        x->x_bufphase = bufphase;
                        x->x_phase = phase;
                        return(w+5);
                    }
                }
                if(x->x_xymode == 1)
                    in1 = in;
                else
                    in2 = in;
            }
            else if(x->x_retrigger)
                x->x_retrigger = 0;
            while(nblock--){
                bp1 = x->x_xbuffer + bufphase;
                bp2 = x->x_ybuffer + bufphase;
                if(phase){
                    t_float f1 = *in1++;
                    t_float f2 = *in2++;
                    if(x->x_xymode == 1){ // CHECKED
                        if(!x->x_drawstyle){
                            if((currx<0 && (f1<currx || f1>-currx)) || (currx>0 && (f1>currx || f1<-currx)))
                                currx = f1;
                        }
                        else if(f1 < currx)
                            currx = f1;
                        curry = 0.;
                    }
                    else if(x->x_xymode == 2){
                        if(!x->x_drawstyle){
                            if((curry<0 && (f2<curry || f2>-curry)) || (curry>0 && (f2>curry || f2<-curry)))
                                curry = f2;
                        }
                        else if(f2 < curry)
                            curry = f2;
                        currx = 0.;
                    }
                    else{
                        currx += f1;
                        curry += f2;
                    }
                }
                else{
                    currx = *in1++;
                    curry = *in2++;
                }
                if(currx != currx)
                    currx = 0.;  // CHECKED NaNs bashed to zeros
                if(curry != curry)
                    curry = 0.;
                if(++phase >= period){
                    phase = 0;
                    if(x->x_xymode == 3){
                        currx *= freq;
                        curry *= freq;
                    }
                    if(++bufphase >= bufsize){
                        *bp1 = currx;
                        *bp2 = curry;
                        bufphase = 0;
                        x->x_lastbufsize = bufsize;
                        memcpy(x->x_xbuflast, x->x_xbuffer, bufsize * sizeof(*x->x_xbuffer));
                        memcpy(x->x_ybuflast, x->x_ybuffer, bufsize * sizeof(*x->x_ybuffer));
                        x->x_retrigger = (x->x_trigmode != 0);
                        x->x_trigx = x->x_triglevel;
                        clock_delay(x->x_clock, 0);
                    }
                    else{
                        *bp1 = currx;
                        *bp2 = curry;
                    }
                }
            }
            x->x_currx = currx;
            x->x_curry = curry;
            x->x_bufphase = bufphase;
            x->x_phase = phase;
        }
    }
    return(w+5);
}

static void scope_dsp(t_scope *x, t_signal **sp){
    x->x_ksr = sp[0]->s_sr * 0.001;
    int xfeeder = magic_inlet_connection((t_object *)x, x->x_glist, 0, &s_signal);
    int yfeeder = magic_inlet_connection((t_object *)x, x->x_glist, 1, &s_signal);
    int xymode = xfeeder + 2 * yfeeder;
    if(xymode != x->x_xymode){
        x->x_xymode = xymode;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            t_canvas *cv = glist_getcanvas(x->x_glist);
            int x1, y1, x2, y2;
            scope_getrect((t_gobj *)x, x->x_glist, &x1, &y1, &x2, &y2);
            sys_vgui(".x%lx.c delete fg%lx margin%lx\n", cv, x, x);
            scope_drawmargins(x, cv, x1, y1, x2, y2);
            if(x->x_xymode)
                scope_drawfg(x, cv, x1, y1, x2, y2);
        }
        x->x_precount = 0;
    }
    dsp_add(scope_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

static void scope_tick(t_scope *x){
    if(glist_isvisible(x->x_glist)  && gobj_shouldvis((t_gobj *)x, x->x_glist) && x->x_xymode)
        scope_redraw(x, glist_getcanvas(x->x_glist));
    x->x_precount = (int)(x->x_delay * x->x_ksr);
}

static void scope_get_rcv(t_scope* x){
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
                int arg_n = 22; // receive argument number
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

static void scope_save(t_gobj *z, t_binbuf *b){
    t_scope *x = (t_scope *)z;
    t_text *t = (t_text *)x;
    scope_get_rcv(x);
    binbuf_addv(b, "ssiisiiiiiffiiifiiiiiiiiiis;", gensym("#X"), gensym("obj"), (int)t->te_xpix,
        (int)t->te_ypix, atom_getsymbol(binbuf_getvec(t->te_binbuf)), x->x_width/x->x_zoom,
        x->x_height/x->x_zoom, x->x_period, 3, x->x_bufsize, x->x_min, x->x_max, x->x_delay,
        0, x->x_trigmode, x->x_triglevel, x->x_fg[0], x->x_fg[1], x->x_fg[2], x->x_bg[0],
        x->x_bg[1], x->x_bg[2], x->x_gg[0], x->x_gg[1], x->x_gg[2], 0, x->x_rcv_raw);
}

static void scope_properties(t_gobj *z, t_glist *owner){
    owner = NULL;
    t_scope *x = (t_scope *)z;
    int bgcol = ((int)x->x_bg[0] << 16) + ((int)x->x_bg[1] << 8) + (int)x->x_bg[2];
    int grcol = ((int)x->x_gg[0] << 16) + ((int)x->x_gg[1] << 8) + (int)x->x_gg[2];
    int fgcol = ((int)x->x_fg[0] << 16) + ((int)x->x_fg[1] << 8) + (int)x->x_fg[2];
    scope_get_rcv(x);
    char buf[1000];
    sprintf(buf, "::dialog_scope::pdtk_scope_dialog %%s \
        dim %d width: %d height: \
        buf %d cal: %d bfs: \
        rng %g min: %g max: \
        del %d del: drs %d drs: \
        {%s} rcv: trg %d tmd: %g tlv: \
        dim_mins %d %d \
        cal_min_max %d %d bfs_min_max %d %d \
        del_mins %d \
        #%06x #%06x #%06x\n",
        x->x_width, x->x_height,
        x->x_period, x->x_bufsize,
        x->x_min, x->x_max,
        x->x_delay, x->x_drawstyle,
        x->x_rcv_raw->s_name, 
        x->x_trigmode, x->x_triglevel,
        SCOPE_MINSIZE, SCOPE_MINSIZE,
        SCOPE_MINPERIOD, SCOPE_MAXPERIOD,
        SCOPE_MINBUFSIZE, SCOPE_MAXBUFSIZE,
        SCOPE_MINDELAY,
        bgcol, grcol, fgcol);
    gfxstub_new(&x->x_obj.ob_pd, x, buf);
}

static int scope_getcolorarg(int index, int ac, t_atom *av){
    if((av+index)->a_type == A_SYMBOL){
        t_symbol *s = atom_getsymbolarg(index, ac, av);
        if('#' == s->s_name[0])
            return(strtol(s->s_name+1, 0, 16));
    }
    return(0);
}

static void scope_ok(t_scope *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    int width = (int)atom_getintarg(0, ac, av);
    int height = (int)atom_getintarg(1, ac, av);
    int period = (int)atom_getintarg(2, ac, av);
    int bufsize = (int)atom_getintarg(3, ac, av);
    float minval = (float)atom_getfloatarg(4, ac, av);
    float maxval = (float)atom_getfloatarg(5, ac, av);
    int delay = (int)atom_getintarg(6, ac, av);
    int drawstyle = (int)atom_getintarg(7, ac, av);
    int trigmode = (int)atom_getintarg(8, ac, av);
    float triglevel = (float)atom_getfloatarg(9, ac, av);
    int bgcol = (int)scope_getcolorarg(10, ac, av);
    int grcol = (int)scope_getcolorarg(11, ac, av);
    int fgcol = (int)scope_getcolorarg(12, ac, av);
    int bgred = (bgcol & 0xFF0000) >> 16;
    int bggreen = (bgcol & 0x00FF00) >> 8;
    int bgblue = (bgcol & 0x0000FF);
    int grred = (grcol & 0xFF0000) >> 16;
    int grgreen = (grcol & 0x00FF00) >> 8;
    int grblue = (grcol & 0x0000FF);
    int fgred = (fgcol & 0xFF0000) >> 16;
    int fggreen = (fgcol & 0x00FF00) >> 8;
    int fgblue = (fgcol & 0x0000FF);
    t_symbol *rcv = atom_getsymbolarg(13, ac, av);
    scope_period(x, period);
    scope_bufsize(x, bufsize);
    scope_range(x, minval, maxval);
    scope_delay(x, delay);
    scope_drawstyle(x, drawstyle);
    scope_receive(x, rcv);
    scope_trigger(x, trigmode);
    scope_triglevel(x, triglevel);
    scope_bgcolor(x, bgred, bggreen, bgblue);
    scope_gridcolor(x, grred, grgreen, grblue);
    scope_fgcolor(x, fgred, fggreen, fgblue);
    scope_dim(x, width, height);
    canvas_dirty(x->x_cv, 1);
}

static void edit_proxy_free(t_edit_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_edit_proxy * edit_proxy_new(t_scope *x, t_symbol *s){
    t_edit_proxy *p = (t_edit_proxy*)pd_new(edit_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)edit_proxy_free);
    return(p);
}

static void scope_free(t_scope *x){
    if(x->x_clock)
        clock_free(x->x_clock);
    if(x->x_receive != &s_)
        pd_unbind(&x->x_obj.ob_pd, x->x_receive);
    pd_unbind(&x->x_obj.ob_pd, x->x_bindsym);
    if(x->x_handle){
        pd_unbind(x->x_handle, ((t_handle *)x->x_handle)->h_bindsym);
        pd_free(x->x_handle);
    }
    clock_delay(x->x_proxy->p_clock, 0);
    gfxstub_deleteforkey(x);
}

static void *scope_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_scope *x = (t_scope *)pd_new(scope_class);
    x->x_handle = pd_new(handle_class);
    t_handle *sh = (t_handle *)x->x_handle;
    sh->h_master = x;
    char hbuf[64];
    sprintf(hbuf, "_h%lx", (unsigned long)sh);
    pd_bind(x->x_handle, sh->h_bindsym = gensym(hbuf));
    sprintf(sh->h_outlinetag, "h%lx", (unsigned long)sh);
    x->x_glist = (t_glist*)canvas_getcurrent();
    x->x_cv = glist_getcanvas(x->x_glist);
    x->x_zoom = x->x_glist->gl_zoom;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)x->x_cv);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = edit_proxy_new(x, gensym(buf));
    sprintf(buf, "#%lx", (long)x);
    pd_bind(&x->x_obj.ob_pd, x->x_bindsym = gensym(buf));
    x->x_edit = x->x_cv ->gl_edit;
    t_symbol *rcv = x->x_receive = x->x_rcv_raw = &s_;
    x->x_bufsize = x->x_xymode = x->x_frozen = x->x_precount = sh->h_selectedmode = sh->h_dragon = 0;
    x->x_flag = x->x_r_flag = x->x_rcv_set = x->x_select = 0;
    x->x_phase = x->x_bufphase = x->x_precount = 0;
    float width = 200, height = 100, period = 256, bufsize = x->x_lastbufsize = 128; // def values
    float minval = -1, maxval = 1, delay = 0, drawstyle = 0, trigger = 0, triglevel = 0; // def
    unsigned char bgred = 190, bggreen = 190, bgblue = 190;    // default bg color
    unsigned char fgred = 30, fggreen = 30, fgblue = 30; // default fg color
    unsigned char grred = 160, grgreen = 160, grblue = 160;   // default grid color
    float f_r = 0, f_g = 0, f_b = 0, b_r = 0, b_g = 0, b_b = 0, g_r = 0, g_g = 0, g_b = 0;
    int fcolset = 0, bcolset = 0, gcolset = 0; // flag for colorset
    if(ac){
        if(av->a_type == A_FLOAT){ // 1st Width
            width = av->a_w.w_float < SCOPE_MINSIZE ? SCOPE_MINSIZE : av->a_w.w_float;
            ac--; av++;
            if(ac && av->a_type == A_FLOAT){ // 2nd Height
                height = av->a_w.w_float < SCOPE_MINSIZE ? SCOPE_MINSIZE : av->a_w.w_float;
                ac--, av++;
                if(ac && av->a_type == A_FLOAT){ // 3rd period
                    period = av->a_w.w_float;
                    ac--; av++;
                    if(ac && av->a_type == A_FLOAT){ // 4th nothing really (argh/grrr)
                        ac--; av++;
                        if(ac && av->a_type == A_FLOAT){ // 5th bufsize
                            bufsize = av->a_w.w_float;
                            ac--; av++;
                            if(ac && av->a_type == A_FLOAT){ // 6th minval
                                minval = av->a_w.w_float;
                                ac--; av++;
                                if(ac && av->a_type == A_FLOAT){ // 7th maxval
                                    maxval = av->a_w.w_float;
                                    ac--; av++;
                                    if(ac && av->a_type == A_FLOAT){ // 8th delay
                                        delay = av->a_w.w_float;
                                        ac--; av++;
                                        if(ac && av->a_type == A_FLOAT){ // 9th ? nobody knows...
                                            ac--; av++;
                                            if(ac && av->a_type == A_FLOAT){ // 10th trigger
                                                trigger = av->a_w.w_float;
                                                ac--; av++;
                                                if(ac && av->a_type == A_FLOAT){ // 11th triglevel
                                                    triglevel = av->a_w.w_float;
                                                    ac--; av++;
                                                    if(ac && av->a_type == A_FLOAT){ // 12th fgred
                                                        fgred = (unsigned char)av->a_w.w_float;
                                                        if(fgred < 0)
                                                            fgred = 0;
                                                        if(fgred > 255)
                                                            fgred = 255;
                                                        ac--; av++;
                                                        if(ac && av->a_type == A_FLOAT){ // 13th fggreen
                                                            fggreen = (unsigned char)av->a_w.w_float;
                                                            if(fggreen < 0)
                                                                fggreen = 0;
                                                            if(fggreen > 255)
                                                                fggreen = 255;
                                                            ac--; av++;
                                                            if(ac && av->a_type == A_FLOAT){ // 14th fgblue
                                                                fgblue = (unsigned char)av->a_w.w_float;
                                                                if(fgblue < 0)
                                                                    fgblue = 0;
                                                                if(fgblue > 255)
                                                                    fgblue = 255;
                                                                ac--; av++;
                                                                if(ac && av->a_type == A_FLOAT){ // 15th bgred
                                                                    bgred = (unsigned char)av->a_w.w_float;
                                                                    if(bgred < 0)
                                                                        bgred = 0;
                                                                    if(bgred > 255)
                                                                        bgred = 255;
                                                                    ac--; av++;
                                                                    if(ac && av->a_type == A_FLOAT){ // 16th bggreen
                                                                        bggreen = (unsigned char)av->a_w.w_float;
                                                                        if(bggreen < 0)
                                                                            bggreen = 0;
                                                                        if(bggreen > 255)
                                                                            bggreen = 255;
                                                                        ac--; av++;
                                                                        if(ac && av->a_type == A_FLOAT){ // 17th bgblue
                                                                            bgblue = (unsigned char)av->a_w.w_float;
                                                                            if(bgblue < 0)
                                                                                bgblue = 0;
                                                                            if(fgblue > 255)
                                                                                bgblue = 255;
                                                                            ac--; av++;
                                                                            if(ac && av->a_type == A_FLOAT){ // 18th grred
                                                                                grred = (unsigned char)av->a_w.w_float;
                                                                                if(grred < 0)
                                                                                    grred = 0;
                                                                                if(grred > 255)
                                                                                    grred = 255;
                                                                                ac--; av++;
                                                                                if(ac && av->a_type == A_FLOAT){ // 19th grgreen
                                                                                    grgreen = (unsigned char)av->a_w.w_float;
                                                                                    if(grgreen < 0)
                                                                                        grgreen = 0;
                                                                                    if(grgreen > 255)
                                                                                        grgreen = 255;
                                                                                    ac--; av++;
                                                                                    if(ac && av->a_type == A_FLOAT){ // 20th grblue
                                                                                        grblue = (unsigned char)av->a_w.w_float;
                                                                                        if(grblue < 0)
                                                                                            grblue = 0;
                                                                                        if(grblue > 255)
                                                                                            grblue = 255;
                                                                                        ac--; av++;
                                                                                        if(ac && av->a_type == A_FLOAT){ // 21st empty
                                                                                            ac--; av++;
                                                                                            if(ac && av->a_type == A_SYMBOL){ // 22nd receive
                                                                                                if(av->a_w.w_symbol == gensym("empty")) // ignore empty symbol
                                                                                                    ac--, av++;
                                                                                                else{
                                                                                                    rcv = av->a_w.w_symbol;
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
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        else if(av->a_type == A_SYMBOL){
            t_symbol *sym = atom_getsymbolarg(0, ac, av);
            if(sym == gensym("-calccount") && ac >= 2){
                x->x_flag = 1;
                period = atom_getfloatarg(1, ac, av);
                ac-=2, av+=2;
            }
            else if(sym == gensym("-bufsize") && ac >= 2){
                x->x_flag = 1;
                bufsize = atom_getfloatarg(1, ac, av);
                ac-=2, av+=2;
            }
            else if(sym == gensym("-range") && ac >= 3){
                x->x_flag = 1;
                minval = atom_getfloatarg(1, ac, av);
                maxval = atom_getfloatarg(2, ac, av);
                ac-=3, av+=3;
            }
            else if(sym == gensym("-dim") && ac >= 3){
                x->x_flag = 1;
                height = atom_getfloatarg(1, ac, av);
                width = atom_getfloatarg(2, ac, av);
                ac-=3, av+=3;
            }
            else if(sym == gensym("-delay") && ac >= 2){
                x->x_flag = 1;
                delay = atom_getfloatarg(1, ac, av);
                ac-=2, av+=2;
            }
/*            else if(sym == gensym("-drawstyle") && ac >= 2){
                x->x_flag = 1;
                drawstyle = atom_getfloatarg(1, ac, av);
                ac-=2, av+=2;
            }*/
            else if(sym == gensym("-trigger") && ac >= 2){
                x->x_flag = 1;
                trigger = atom_getfloatarg(1, ac, av);
                ac-=2, av+=2;
            }
            else if(sym == gensym("-triglevel") && ac >= 2){
                x->x_flag = 1;
                triglevel = atom_getfloatarg(1, ac, av);
                ac-=2, av+=2;
            }
            else if(sym == gensym("-fgcolor") && ac >= 4){
                x->x_flag = 1;
                fgred = (unsigned char)atom_getfloatarg(1, ac, av);
                fgred = fgred < 0 ? 0 : fgred > 255 ? 255 : fgred;
                fggreen = atom_getfloatarg(2, ac, av);
                fggreen = fggreen < 0 ? 0 : fggreen > 255 ? 255 : fggreen;
                fgblue = atom_getfloatarg(3, ac, av);
                fgblue = fgblue < 0 ? 0 : fgblue > 255 ? 255 : fgblue;
                ac-=4, av+=4;
            }
            else if(sym == gensym("-bgcolor") && ac >= 4){
                x->x_flag = 1;
                bgred = (unsigned char)atom_getfloatarg(1, ac, av);
                bgred = bgred < 0 ? 0 : bgred > 255 ? 255 : bgred;
                bggreen = atom_getfloatarg(2, ac, av);
                bggreen = bggreen < 0 ? 0 : bggreen > 255 ? 255 : bggreen;
                bgblue = atom_getfloatarg(3, ac, av);
                bgblue = bgblue < 0 ? 0 : bgblue > 255 ? 255 : bgblue;
                ac-=4, av+=4;
            }
            else if(sym == gensym("-gridcolor") && ac >= 4){
                x->x_flag = 1;
                grred = (unsigned char)atom_getfloatarg(1, ac, av);
                grred = grred < 0 ? 0 : grred > 255 ? 255 : grred;
                grgreen = atom_getfloatarg(2, ac, av);
                grgreen = grgreen < 0 ? 0 : grgreen > 255 ? 255 : grgreen;
                grblue = atom_getfloatarg(3, ac, av);
                grblue = grblue < 0 ? 0 : grblue > 255 ? 255 : grblue;
                ac-=4, av+=4;
            }
            else if(sym == gensym("-receive") && ac >= 2){
                x->x_flag = x->x_r_flag = 1;
                rcv = atom_getsymbolarg(1, ac, av);
                ac-=2, av+=2;
            }
            else
                goto errstate;
        }
        else
            goto errstate;

    }
    x->x_receive = canvas_realizedollar(x->x_glist, x->x_rcv_raw = rcv);
    if(x->x_receive != &s_)
        pd_bind(&x->x_obj.ob_pd, x->x_receive);
    else
    	x->x_rcv_raw = gensym("empty");
    x->x_rightinlet = inlet_new((t_object *)x, (t_pd *)x, &s_signal, &s_signal);
    x->x_width = (int)width * x->x_zoom, x->x_height = (int)height * x->x_zoom;
    x->x_period = period < 2 ? 2 : period > 8192 ? 8192 : (int)period;
    x->x_bufsize = bufsize < SCOPE_MINBUFSIZE ? SCOPE_MINBUFSIZE : bufsize > SCOPE_MAXBUFSIZE ? SCOPE_MAXBUFSIZE : (int)bufsize;
    pd_float((t_pd *)x->x_rightinlet, x->x_bufsize);
    x->x_signalscalar = obj_findsignalscalar((t_object *)x, 1);
    if(minval > maxval)
        x->x_max = minval, x->x_min = maxval;
    else
        x->x_min = minval, x->x_max = maxval;
    x->x_delay = delay < 0 ? 0 : delay;
    x->x_drawstyle = drawstyle;
    x->x_triglevel = triglevel;
    x->x_trigmode = trigger < 0 ? 0 : trigger > 2 ? 2 : (int)trigger;
    if(x->x_trigmode == 0) // no trigger
        x->x_retrigger = 0;
    if(fcolset)
        x->x_fg[0] = f_r, x->x_fg[1] = f_g, x->x_fg[2] = f_b;
    else
        x->x_fg[0] = fgred, x->x_fg[1] = fggreen, x->x_fg[2] = fgblue;
    if(bcolset)
        x->x_bg[0] = b_r, x->x_bg[1] = b_g, x->x_bg[2] = b_b;
    else
        x->x_bg[0] = bgred, x->x_bg[1] = bggreen, x->x_bg[2] = bgblue;
    if(gcolset)
        x->x_gg[0] = g_r, x->x_gg[1] = g_g, x->x_gg[2] = g_b;
    else
        x->x_gg[0] = grred, x->x_gg[1] = grgreen, x->x_gg[2] = grblue;
    x->x_clock = clock_new(x, (t_method)scope_tick);
    return(x);
errstate:
    pd_error(x, "[scope~]: improper creation arguments");
    return(NULL);
}

void oscilloscope_tilde_setup(void){
    scope_class = class_new(gensym("oscilloscope~"), (t_newmethod)scope_new,
            (t_method)scope_free, sizeof(t_scope), 0, A_GIMME, 0);
    class_addmethod(scope_class, nullfn, gensym("signal"), 0);
    class_addmethod(scope_class, (t_method) scope_dsp, gensym("dsp"), A_CANT, 0);
    class_addfloat(scope_class, (t_method)scope_period);
    class_addmethod(scope_class, (t_method)scope_period, gensym("nsamples"), A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_bufsize, gensym("nlines"), A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_dim, gensym("dim"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_range, gensym("range"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_delay, gensym("delay"), A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_drawstyle, gensym("drawstyle"), A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_trigger, gensym("trigger"), A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_triglevel, gensym("triglevel"), A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_fgcolor, gensym("fgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_bgcolor, gensym("bgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_gridcolor, gensym("gridcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_receive, gensym("receive"), A_SYMBOL, 0);
    class_addmethod(scope_class, (t_method)scope_ok, gensym("dialog"), A_GIMME, 0);
    class_addmethod(scope_class, (t_method)scope_click, gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(scope_class, (t_method)scope_zoom, gensym("zoom"), A_CANT, 0);
    class_addmethod(scope_class, (t_method)scope_mouserelease, gensym("_mouserelease"), 0);
    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(edit_proxy_class, edit_proxy_any);
    handle_class = class_new(gensym("_handle"), 0, 0, sizeof(t_handle), CLASS_PD, 0);
    class_addmethod(handle_class, (t_method)handle__click_callback, gensym("_click"), A_FLOAT, 0);
    class_addmethod(handle_class, (t_method)handle__motion_callback, gensym("_motion"), A_FLOAT, A_FLOAT, 0);
    class_setsavefn(scope_class, scope_save);
    class_setpropertiesfn(scope_class, scope_properties);
    class_setwidget(scope_class, &scope_widgetbehavior);
    scope_widgetbehavior.w_getrectfn  = scope_getrect;
    scope_widgetbehavior.w_displacefn = scope_displace;
    scope_widgetbehavior.w_selectfn   = scope_select;
    scope_widgetbehavior.w_deletefn   = scope_delete;
    scope_widgetbehavior.w_visfn      = scope_vis;
    scope_widgetbehavior.w_clickfn    = (t_clickfn)scope_click;
    #include "oscilloscope_dialog.c"
}
