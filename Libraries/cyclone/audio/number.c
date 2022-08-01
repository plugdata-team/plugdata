/* Based on my_numbox.c written by Thomas Musil (c) IEM KUG Graz Austria (2000-2001)
   number~.c written by Timothy Schoen for the cyclone library (2022)
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "g_all_guis.h"

#include <common/api.h>

#include <math.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define MINDIGITS 1

typedef struct _number_tilde
{
    t_iemgui x_gui;
    t_outlet *x_signal_outlet;
    t_outlet *x_float_outlet;
    t_clock  *x_clock_reset;
    t_clock  *x_clock_wait;
    t_clock  *x_clock_repaint;
    t_float x_in_val;
    t_float x_out_val;
    t_float x_set_val;
    t_float x_ramp_val;
    t_float x_ramp_time;
    t_float x_ramp;
    t_float x_last_out;
    t_float x_maximum;
    t_float x_minimum;
    t_float  x_interval_ms;
    int      x_numwidth;
    int      x_output_mode;
    int      x_needs_update;
    char     x_buf[IEMGUI_MAX_NUM_LEN];
} t_number_tilde;


/*------------------ global functions -------------------------*/

static void number_tilde_key(void *z, t_symbol *keysym, t_floatarg fkey);
static void number_tilde_draw_update(t_gobj *client, t_glist *glist);
static void init_number_tilde_dialog(void);
static void number_tilde_interval(t_number_tilde *x, t_floatarg f);
static void number_tilde_mode(t_number_tilde *x, t_floatarg f);
static void number_tilde_bang(t_number_tilde *x);
static void number_tilde_set(t_number_tilde *x, t_floatarg f);

void iemgui_all_col2save(t_iemgui *iemgui, t_symbol**bflcol);
/* ------------ nbx gui-my number box ----------------------- */

t_widgetbehavior number_tilde_widgetbehavior;
static t_class *number_tilde_class;

/* widget helper functions */

static void number_tilde_tick_reset(t_number_tilde *x)
{
    if(x->x_gui.x_fsf.x_change && x->x_gui.x_glist)
    {
        x->x_gui.x_fsf.x_change = 0;
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    }
}

static void number_tilde_tick_wait(t_number_tilde *x)
{
    sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
}

static void number_tilde_periodic_update(t_number_tilde *x)
{
    // Make sure it's not constantly repainting when there is no audio coming in
    if(x->x_needs_update) {
        
        outlet_float(x->x_float_outlet, x->x_in_val);
        
        number_tilde_draw_update(&x->x_gui.x_obj.te_g, x->x_gui.x_glist);

        // Clear repaint flag
        x->x_needs_update = 0;
    }
    
    // Cap the repaint speed at 60fps
    clock_delay(x->x_clock_repaint, fmax(x->x_interval_ms, 15));
}

int number_tilde_check_minmax(t_number_tilde *x, t_float min, t_float max)
{
    int ret = 0;

    x->x_minimum = min;
    x->x_maximum = max;

    if(x->x_set_val < x->x_minimum)
    {
        x->x_set_val = x->x_minimum;
        ret = 1;
    }
    if(x->x_set_val > x->x_maximum)
    {
        x->x_set_val = x->x_maximum;
        ret = 1;
    }

    if(x->x_out_val < x->x_minimum)
    {
        x->x_out_val = x->x_minimum;
    }
    if(x->x_out_val > x->x_maximum)
    {
        x->x_out_val = x->x_maximum;
    }


    return(ret);
}

static void number_tilde_clip(t_number_tilde *x)
{
    if(x->x_minimum == 0 && x->x_maximum == 0) return;

    if(x->x_out_val < x->x_minimum)
        x->x_out_val = x->x_minimum;
    if(x->x_out_val > x->x_maximum)
        x->x_out_val = x->x_maximum;

    if(x->x_set_val < x->x_minimum)
        x->x_set_val = x->x_minimum;
    if(x->x_set_val > x->x_maximum)
        x->x_set_val = x->x_maximum;
}

static void number_tilde_minimum(t_number_tilde *x, t_floatarg f)
{
    if(number_tilde_check_minmax(x, f,
                                 x->x_maximum))
    {
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    }
}

static void number_tilde_maximum(t_number_tilde *x, t_floatarg f)
{
    if(number_tilde_check_minmax(x, x->x_minimum,
                                 f))
    {
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    }
}

static void number_tilde_calc_fontwidth(t_number_tilde *x)
{
    int w, f = 31;

    if(x->x_gui.x_fsf.x_font_style == 1)
        f = 27;
    else if(x->x_gui.x_fsf.x_font_style == 2)
        f = 25;
    
    w = x->x_gui.x_fontsize * f * x->x_numwidth;
    w /= 36;
    x->x_gui.x_w = (w + (x->x_gui.x_h/2)/IEMGUI_ZOOM(x) + 4) * IEMGUI_ZOOM(x);
}

static void number_tilde_ftoa(t_number_tilde *x)
{
    
    double f = x->x_output_mode ? x->x_set_val : x->x_in_val;
    int bufsize, is_exp = 0, i, idecimal;

    sprintf(x->x_buf, "%g", f);
    bufsize = (int)strlen(x->x_buf);
    
    int real_numwidth = x->x_numwidth + 1;
    if(bufsize > real_numwidth)/* if to reduce */
    {
        if(is_exp)
        {
            if(real_numwidth <= 5)
            {
                x->x_buf[0] = (f < 0.0 ? '-' : '+');
                x->x_buf[1] = 0;
            }
            i = bufsize - 4;
            for(idecimal = 0; idecimal < i; idecimal++)
                if(x->x_buf[idecimal] == '.')
                    break;
            if(idecimal > (real_numwidth - 4))
            {
                x->x_buf[0] = (f < 0.0 ? '-' : '+');
                x->x_buf[1] = 0;
            }
            else
            {
                int new_exp_index = real_numwidth - 4;
                int old_exp_index = bufsize - 4;

                for(i = 0; i < 4; i++, new_exp_index++, old_exp_index++)
                    x->x_buf[new_exp_index] = x->x_buf[old_exp_index];
                x->x_buf[x->x_numwidth] = 0;
            }
        }
        else
        {
            for(idecimal = 0; idecimal < bufsize; idecimal++)
                if(x->x_buf[idecimal] == '.')
                    break;
            if(idecimal > real_numwidth)
            {
                x->x_buf[0] = (f < 0.0 ? '-' : '+');
                x->x_buf[1] = 0;
            }
            else
                x->x_buf[real_numwidth] = 0;
        }
    }
}

static void number_tilde_draw_update(t_gobj *client, t_glist *glist)
{
    t_number_tilde *x = (t_number_tilde *)client;
    if(glist_isvisible(glist))
    {
        if(x->x_gui.x_fsf.x_change && x->x_buf[0] && x->x_output_mode)
        {
            char *cp = x->x_buf;
            int sl = (int)strlen(x->x_buf);
            
            x->x_buf[sl] = '>';
            x->x_buf[sl+1] = 0;
            
            if(sl >= x->x_numwidth)
                cp += sl - x->x_numwidth + 1;
            sys_vgui(".x%lx.c itemconfigure %lxNUMBER -fill #%06x -text {%s} \n",
                     glist_getcanvas(glist), x,
                     x->x_gui.x_fcol, cp);
            
            sys_vgui(".x%lx.c itemconfigure %lxBASE1 -width %d\n",
                     glist_getcanvas(glist), x,
                     IEMGUI_ZOOM(x) * 2);
            
            x->x_buf[sl] = 0;
        }
        else {
            number_tilde_ftoa(x);
            
            sys_vgui(".x%lx.c itemconfigure %lxNUMBER -fill #%06x -text {%s} \n",
                     glist_getcanvas(glist), x,
                     x->x_gui.x_fsf.x_selected ? IEM_GUI_COLOR_SELECTED : x->x_gui.x_fcol, x->x_buf);
            
            sys_vgui(".x%lx.c itemconfigure %lxBASE1 -width %d\n",
                     glist_getcanvas(glist), x,
                     IEMGUI_ZOOM(x));
            
            x->x_buf[0] = 0;
        }
    }
}

static void number_tilde_draw_new(t_number_tilde *x, t_glist *glist)
{
    int xpos = text_xpix(&x->x_gui.x_obj, glist);
    int ypos = text_ypix(&x->x_gui.x_obj, glist);
    int w = x->x_gui.x_w, half = x->x_gui.x_h/2;
    int d = IEMGUI_ZOOM(x) + x->x_gui.x_h/(34*IEMGUI_ZOOM(x));
    int iow = IOWIDTH * IEMGUI_ZOOM(x), ioh = 3 * IEMGUI_ZOOM(x);
    t_canvas *canvas = glist_getcanvas(glist);

    sys_vgui(".x%lx.c create polygon %d %d %d %d %d %d %d %d %d %d "
             "-width %d -outline #%06x -fill #%06x -tags %lxBASE1\n",
             canvas,
             xpos, ypos,
             xpos + w, ypos,
             xpos + w, ypos + x->x_gui.x_h,
             xpos, ypos + x->x_gui.x_h,
             xpos, ypos,
             IEMGUI_ZOOM(x), IEM_GUI_COLOR_NORMAL, x->x_gui.x_bcol, x);

        sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w -font {{%s} -%d %s} -fill #%06x -tags %lxBASE2\n",
             canvas, xpos + 2*IEMGUI_ZOOM(x) + 1, ypos + half + d,
             x->x_output_mode ? "↓" : "~", x->x_gui.x_font, x->x_gui.x_fontsize * IEMGUI_ZOOM(x),
             sys_fontweight, x->x_gui.x_fcol, x);


        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxOUT%d outlet]\n",
                 canvas,
                 xpos, ypos + x->x_gui.x_h + IEMGUI_ZOOM(x) - ioh,
                 xpos + iow, ypos + x->x_gui.x_h,
                 x, 0);
        
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxOUT2%d outlet]\n",
             canvas,
             xpos + x->x_gui.x_w - iow, ypos + x->x_gui.x_h + IEMGUI_ZOOM(x) - ioh,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h,
             x, 0);
    
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxIN%d inlet]\n",
             canvas,
             xpos, ypos,
             xpos + iow, ypos - IEMGUI_ZOOM(x) + ioh,
             x, 0);
    
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags [list %lxIN2%d inlet]\n",
             canvas,
             xpos + x->x_gui.x_w - iow, ypos,
             xpos + x->x_gui.x_w, ypos - IEMGUI_ZOOM(x) + ioh,
             x, 0);

    
    number_tilde_ftoa(x);
    sys_vgui(".x%lx.c create text %d %d -text {%s} -anchor w -font {{%s} -%d %s} -fill #%06x -tags %lxNUMBER\n",
             canvas, xpos + half + 2*IEMGUI_ZOOM(x) + 3, ypos + half + d,
             x->x_buf, x->x_gui.x_font, x->x_gui.x_fontsize * IEMGUI_ZOOM(x),
             sys_fontweight, x->x_gui.x_fcol, x);
}

static void number_tilde_draw_move(t_number_tilde *x, t_glist *glist)
{
    int xpos = text_xpix(&x->x_gui.x_obj, glist);
    int ypos = text_ypix(&x->x_gui.x_obj, glist);
    int w = x->x_gui.x_w, half = x->x_gui.x_h/2;
    int d = IEMGUI_ZOOM(x) + x->x_gui.x_h / (34 * IEMGUI_ZOOM(x));
    int iow = IOWIDTH * IEMGUI_ZOOM(x), ioh = 3 * IEMGUI_ZOOM(x);
    t_canvas *canvas = glist_getcanvas(glist);

    sys_vgui(".x%lx.c coords %lxBASE1 %d %d %d %d %d %d %d %d %d %d\n",
             canvas, x,
             xpos, ypos,
             xpos + w, ypos,
             xpos + w, ypos + x->x_gui.x_h,
             xpos, ypos + x->x_gui.x_h,
             xpos, ypos);
    
    sys_vgui(".x%lx.c coords %lxBASE2 %d %d\n",
             canvas, x,
             xpos + 2*IEMGUI_ZOOM(x) + 1, ypos + half + d);

    sys_vgui(".x%lx.c coords %lxOUT%d %d %d %d %d\n",
             canvas, x, 0,
             xpos, ypos + x->x_gui.x_h + IEMGUI_ZOOM(x) - ioh,
             xpos + iow, ypos + x->x_gui.x_h);
    
    sys_vgui(".x%lx.c coords %lxOUT2%d %d %d %d %d\n",
             canvas, x, 0,
             xpos + x->x_gui.x_w - iow, ypos + x->x_gui.x_h + IEMGUI_ZOOM(x) - ioh,
             xpos + x->x_gui.x_w, ypos + x->x_gui.x_h);

    sys_vgui(".x%lx.c coords %lxIN%d %d %d %d %d\n",
             canvas, x, 0,
             xpos, ypos,
             xpos + iow, ypos - IEMGUI_ZOOM(x) + ioh);
    
    sys_vgui(".x%lx.c coords %lxIN2%d %d %d %d %d\n",
             canvas, x, 0,
             xpos + x->x_gui.x_w - iow, ypos,
             xpos + x->x_gui.x_w, ypos - IEMGUI_ZOOM(x) + ioh);
    
    sys_vgui(".x%lx.c coords %lxNUMBER %d %d\n",
             canvas, x, xpos + half + 2*IEMGUI_ZOOM(x) + 3, ypos + half + d);
}

static void number_tilde_draw_erase(t_number_tilde* x, t_glist* glist)
{
    t_canvas *canvas = glist_getcanvas(glist);

    sys_vgui(".x%lx.c delete %lxBASE1\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxBASE2\n", canvas, x);
    sys_vgui(".x%lx.c delete %lxNUMBER\n", canvas, x);

    sys_vgui(".x%lx.c delete %lxOUT%d\n", canvas, x, 0);
    sys_vgui(".x%lx.c delete %lxOUT2%d\n", canvas, x, 0);

    sys_vgui(".x%lx.c delete %lxIN%d\n", canvas, x, 0);
    sys_vgui(".x%lx.c delete %lxIN2%d\n", canvas, x, 0);
}

static void number_tilde_draw_config(t_number_tilde* x, t_glist* glist)
{
    t_canvas *canvas = glist_getcanvas(glist);

    sys_vgui(".x%lx.c itemconfigure %lxNUMBER -font {{%s} -%d %s} -fill #%06x \n",
             canvas, x, x->x_gui.x_font, x->x_gui.x_fontsize * IEMGUI_ZOOM(x), sys_fontweight,
             (x->x_gui.x_fsf.x_selected ? IEM_GUI_COLOR_SELECTED : x->x_gui.x_fcol));
    
    sys_vgui(".x%lx.c itemconfigure %lxBASE1 -fill #%06x\n", canvas,
             x, x->x_gui.x_bcol);
    sys_vgui(".x%lx.c itemconfigure %lxBASE2 -fill #%06x -font {{%s} -%d %s} \n", canvas, x,
             (x->x_gui.x_fsf.x_selected ? IEM_GUI_COLOR_SELECTED : x->x_gui.x_fcol), x->x_gui.x_font, x->x_gui.x_fontsize * IEMGUI_ZOOM(x), sys_fontweight);
}

static void number_tilde_draw_select(t_number_tilde *x, t_glist *glist)
{
    t_canvas *canvas = glist_getcanvas(glist);

    if(x->x_gui.x_fsf.x_selected)
    {
        if(x->x_gui.x_fsf.x_change)
        {
            x->x_gui.x_fsf.x_change = 0;
            clock_unset(x->x_clock_reset);
            x->x_buf[0] = 0;
            sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
        }
        sys_vgui(".x%lx.c itemconfigure %lxBASE1 -outline #%06x\n",
            canvas, x, IEM_GUI_COLOR_SELECTED);
        sys_vgui(".x%lx.c itemconfigure %lxBASE2 -fill #%06x\n",
            canvas, x, IEM_GUI_COLOR_SELECTED);
        sys_vgui(".x%lx.c itemconfigure %lxNUMBER -fill #%06x\n",
            canvas, x, IEM_GUI_COLOR_SELECTED);
    }
    else
    {
        sys_vgui(".x%lx.c itemconfigure %lxBASE1 -outline #%06x\n",
            canvas, x, IEM_GUI_COLOR_NORMAL);
        sys_vgui(".x%lx.c itemconfigure %lxBASE2 -fill #%06x\n",
            canvas, x, x->x_gui.x_fcol);
        sys_vgui(".x%lx.c itemconfigure %lxNUMBER -fill #%06x\n",
            canvas, x, x->x_gui.x_fcol);
    }
}

static void number_tilde_draw(t_number_tilde *x, t_glist *glist, int mode)
{
    if(mode == IEM_GUI_DRAW_MODE_UPDATE)
        sys_queuegui(x, glist, number_tilde_draw_update);
    else if(mode == IEM_GUI_DRAW_MODE_MOVE)
        number_tilde_draw_move(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_NEW)
        number_tilde_draw_new(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_SELECT)
        number_tilde_draw_select(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_ERASE)
        number_tilde_draw_erase(x, glist);
    else if(mode == IEM_GUI_DRAW_MODE_CONFIG)
        number_tilde_draw_config(x, glist);
}

/* ------------------------ nbx widgetbehaviour----------------------------- */

static void number_tilde_getrect(t_gobj *z, t_glist *glist,
                              int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_number_tilde* x = (t_number_tilde*)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist);
    *yp1 = text_ypix(&x->x_gui.x_obj, glist);
    *xp2 = *xp1 + x->x_gui.x_w;
    *yp2 = *yp1 + x->x_gui.x_h;
}

static void number_tilde_save(t_gobj *z, t_binbuf *b)
{
    t_number_tilde *x = (t_number_tilde *)z;
    t_symbol *bflcol[3];

    iemgui_all_col2save(&x->x_gui, bflcol);

    if(x->x_gui.x_fsf.x_change)
    {
        x->x_gui.x_fsf.x_change = 0;
        clock_unset(x->x_clock_reset);
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    }
    
    binbuf_addv(b, "ssiisiiifssfff", gensym("#X"), gensym("obj"),
                (int)x->x_gui.x_obj.te_xpix, (int)x->x_gui.x_obj.te_ypix,
                gensym("number~"), x->x_numwidth, (x->x_gui.x_h /IEMGUI_ZOOM(x)) - 5,
                x->x_output_mode, x->x_interval_ms,
                bflcol[0], bflcol[1], x->x_ramp_time, x->x_minimum, x->x_maximum);
    
    binbuf_addv(b, ";");
}

static void number_tilde_properties(t_gobj *z, t_glist *owner)
{
    // For some reason the dialog doesn't work if we initialise it in the setup...
    init_number_tilde_dialog();
    
    t_number_tilde *x = (t_number_tilde *)z;
    char buf[800];
    
    if(x->x_gui.x_fsf.x_change)
    {
        x->x_gui.x_fsf.x_change = 0;
        clock_unset(x->x_clock_reset);
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    }
    
    sprintf(buf, "::dialog_number::pdtk_number_tilde_dialog %%s \
            -------dimensions(digits)(pix):------- %d %d %d \
            %d %d %.2f \
            #%06x #%06x %.4f %.4f \n",
            x->x_numwidth, MINDIGITS, (x->x_gui.x_h /IEMGUI_ZOOM(x)) - 5, IEM_GUI_MINSIZE, x->x_output_mode, x->x_interval_ms,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, x->x_minimum, x->x_maximum);

    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

static void number_tilde_bang(t_number_tilde *x)
{
    // Repaint the number
    // Don't queue this because that will lead to too much latency
    x->x_out_val = x->x_set_val;
    x->x_ramp = (x->x_out_val - x->x_ramp_val) / (x->x_ramp_time * (sys_getsr() / 1000.0));

    number_tilde_clip(x);
}

static int iemgui_getcolorarg(int index, int argc, t_atom*argv)
{
    if(index < 0 || index >= argc)
        return 0;
    if(IS_A_FLOAT(argv,index))
        return atom_getfloatarg(index, argc, argv);
    if(IS_A_SYMBOL(argv,index))
    {
        t_symbol*s=atom_getsymbolarg(index, argc, argv);
        if ('#' == s->s_name[0])
        {
            int col = (int)strtol(s->s_name+1, 0, 16);
            return col & 0xFFFFFF;
        }
    }
    return 0;
}

static void number_tilde_dialog(t_number_tilde *x, t_symbol *s, int argc,
    t_atom *argv)
{
    
    #define SETCOLOR(a, col) do {char color[MAXPDSTRING]; snprintf(color, MAXPDSTRING-1, "#%06x", 0xffffff & col); color[MAXPDSTRING-1] = 0; SETSYMBOL(a, gensym(color));} while(0)
    

    t_symbol *srl[3];
    int w = (int)atom_getfloatarg(0, argc, argv);
    int h = (int)atom_getfloatarg(1, argc, argv) + 5;
    int mode = (int)atom_getfloatarg(2, argc, argv);
    t_float interval = (int)atom_getfloatarg(3, argc, argv);
    int bcol = (int)iemgui_getcolorarg(4, argc, argv);
    int fcol = (int)iemgui_getcolorarg(5, argc, argv);
    int min = (int)iemgui_getcolorarg(6, argc, argv);
    int max = (int)iemgui_getcolorarg(7, argc, argv);
    
    t_atom undo[9];
    
    SETFLOAT(undo+0, x->x_numwidth);
    SETFLOAT(undo+1, x->x_gui.x_h);
    SETFLOAT(undo+2, x->x_output_mode);
    SETFLOAT(undo+3, x->x_interval_ms);
    SETCOLOR(undo+4, x->x_gui.x_bcol);
    SETCOLOR(undo+5, x->x_gui.x_fcol);
    SETFLOAT(undo+6, x->x_ramp_time);
    SETFLOAT(undo+7, x->x_minimum);
    SETFLOAT(undo+8, x->x_maximum);

    pd_undo_set_objectstate(x->x_gui.x_glist, (t_pd*)x, gensym("dialog"), 9, undo, argc, argv);
    
    x->x_numwidth = w >= MINDIGITS ? w : MINDIGITS;
    x->x_gui.x_h  = (h >= IEM_GUI_MINSIZE ? h : IEM_GUI_MINSIZE) * IEMGUI_ZOOM(x);
    x->x_gui.x_fontsize = x->x_gui.x_h - 5;
    number_tilde_calc_fontwidth(x);

    number_tilde_mode(x, mode);
    number_tilde_interval(x, interval);
    number_tilde_check_minmax(x, min, max);
    
    x->x_gui.x_fcol = fcol & 0xffffff;
    x->x_gui.x_bcol = bcol & 0xffffff;
    
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(x->x_gui.x_glist, (t_text*)x);
}

static void number_tilde_motion(t_number_tilde *x, t_floatarg dx, t_floatarg dy,
    t_floatarg up)
{
    double k2 = 1.0;
    if (up != 0)
        return;

    if(x->x_gui.x_fsf.x_finemoved)
        k2 = 0.01;

    number_tilde_set(x, x->x_out_val - k2*dy);
    
    sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    number_tilde_bang(x);
    clock_unset(x->x_clock_reset);
}

static void number_tilde_click(t_number_tilde *x, t_floatarg xpos, t_floatarg ypos,
                            t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{

    int relative_x = xpos - x->x_gui.x_obj.te_xpix;
    if(relative_x < 10) {
        number_tilde_mode(x, !x->x_output_mode);
    }
    else if (x->x_output_mode) {
        glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
            (t_glistmotionfn)number_tilde_motion, number_tilde_key, xpos, ypos);
    }
}

static int number_tilde_newclick(t_gobj *z, struct _glist *glist,
    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_number_tilde* x = (t_number_tilde *)z;

    if(doit)
    {
        number_tilde_click( x, (t_floatarg)xpix, (t_floatarg)ypix,
            (t_floatarg)shift, 0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_fsf.x_finemoved = 1;
        else
            x->x_gui.x_fsf.x_finemoved = 0;
        if(!x->x_gui.x_fsf.x_change && xpix - x->x_gui.x_obj.te_xpix > 10)
        {
            clock_delay(x->x_clock_wait, 50);
            x->x_gui.x_fsf.x_change = 1;
            clock_delay(x->x_clock_reset, 3000);

            x->x_buf[0] = 0;
        }
        else
        {
            x->x_gui.x_fsf.x_change = 0;
            clock_unset(x->x_clock_reset);
            x->x_buf[0] = 0;
            sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
        }
    }
    return (1);
}

static void number_tilde_set(t_number_tilde *x, t_floatarg f)
{
    t_float ftocompare = f;
        /* bitwise comparison, suggested by Dan Borstein - to make this work
        ftocompare must be t_float type like x_val. */
    if (memcmp(&ftocompare, &x->x_out_val, sizeof(ftocompare)))
    {
        x->x_set_val = ftocompare;
        number_tilde_clip(x);
        number_tilde_draw_update(&x->x_gui.x_obj.te_g, x->x_gui.x_glist);
    }
    
    
}

static void number_tilde_float(t_number_tilde *x, t_floatarg f)
{
    number_tilde_set(x, f);
    number_tilde_bang(x);
}

static void number_tilde_size(t_number_tilde *x, t_symbol *s, int ac, t_atom *av)
{
    int h, w;

    w = (int)atom_getfloatarg(0, ac, av);
    if(w < MINDIGITS)
        w = MINDIGITS;
    x->x_numwidth = w;
    if(ac > 1)
    {
        h = (int)atom_getfloatarg(1, ac, av);
        if(h < IEM_GUI_MINSIZE)
            h = IEM_GUI_MINSIZE;
        x->x_gui.x_h = (h + 5) * IEMGUI_ZOOM(x);
        x->x_gui.x_fontsize = x->x_gui.x_h - 5;
    }
    number_tilde_calc_fontwidth(x);
    iemgui_size((void *)x, &x->x_gui);
}

static void number_tilde_delta(t_number_tilde *x, t_symbol *s, int ac, t_atom *av)
{iemgui_delta((void *)x, &x->x_gui, s, ac, av);}

static void number_tilde_pos(t_number_tilde *x, t_symbol *s, int ac, t_atom *av)
{iemgui_pos((void *)x, &x->x_gui, s, ac, av);}


static void number_tilde_color(t_number_tilde *x, t_symbol *s, int ac, t_atom *av)
{iemgui_color((void *)x, &x->x_gui, s, ac, av);}


static void number_tilde_interval(t_number_tilde *x, t_floatarg f)
{
    x->x_interval_ms = f;
}

static void number_tilde_mode(t_number_tilde *x, t_floatarg f)
{
    x->x_output_mode = f;
    
    sys_vgui(".x%lx.c itemconfigure %lxBASE2 -text {%s} \n",
             glist_getcanvas(x->x_gui.x_glist), x,
             x->x_output_mode ? "↓" : "~");
    
    x->x_gui.x_fsf.x_change = 0;
    
    sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
}

static void number_tilde_key(void *z, t_symbol *keysym, t_floatarg fkey)
{
    t_number_tilde *x = z;
    char c = fkey;
    char buf[3];
    buf[1] = 0;

    if(c == 0)
    {
        x->x_gui.x_fsf.x_change = 0;
        clock_unset(x->x_clock_reset);
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
        return;
    }
    if(((c >= '0') && (c <= '9')) || (c == '.') || (c == '-') ||
        (c == 'e') || (c == '+') || (c == 'E'))
    {
        if(strlen(x->x_buf) < (IEMGUI_MAX_NUM_LEN-2))
        {
            buf[0] = c;
            strcat(x->x_buf, buf);
            sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
        }
    }
    else if((c == '\b') || (c == 127))
    {
        int sl = (int)strlen(x->x_buf) - 1;

        if(sl < 0)
            sl = 0;
        x->x_buf[sl] = 0;
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    }
    else if(((c == '\n') || (c == 13)) && x->x_buf[0] != 0)
    {
        number_tilde_set(x, atof(x->x_buf));
        x->x_buf[0] = 0;
        x->x_gui.x_fsf.x_change = 0;
        clock_unset(x->x_clock_reset);

        number_tilde_bang(x);
        sys_queuegui(x, x->x_gui.x_glist, number_tilde_draw_update);
    }
    
    clock_delay(x->x_clock_reset, 3000);
}

static void number_tilde_list(t_number_tilde *x, t_symbol *s, int ac, t_atom *av)
{
    if(!ac) {
        number_tilde_bang(x);
    }
    else if(IS_A_FLOAT(av, 0))
    {
        number_tilde_set(x, atom_getfloatarg(0, ac, av));
        number_tilde_bang(x);
    }
}

static void *number_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    t_number_tilde *x = (t_number_tilde *)pd_new(number_tilde_class);
    int w = 4, h = 12;
    int interval = 100;
    int mode = 0;
    t_float maximum = 0;
    t_float minimum = 0;
    t_float ramp_time = 0;
    
    floatinlet_new(&x->x_gui.x_obj, &x->x_ramp_time);
    
    x->x_signal_outlet = outlet_new(&x->x_gui.x_obj,  &s_signal);
    x->x_float_outlet = outlet_new(&x->x_gui.x_obj,  &s_float);

    x->x_gui.x_bcol = 0xFCFCFC;
    x->x_gui.x_fcol = 0x00;
    x->x_gui.x_lcol = 0x00;

    if((argc >= 6)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
       &&IS_A_FLOAT(argv,2) &&IS_A_FLOAT(argv,3)&&IS_A_SYMBOL(argv,4)
       &&IS_A_SYMBOL(argv,5)&&IS_A_FLOAT(argv,6)&&IS_A_FLOAT(argv,7)&&IS_A_FLOAT(argv,8))
    {
        w = (int)atom_getfloatarg(0, argc, argv);
        h = (int)atom_getfloatarg(1, argc, argv);
        mode = atom_getfloatarg(2, argc, argv);
        interval = atom_getfloatarg(3, argc, argv);
        iemgui_all_loadcolors(&x->x_gui, argv+4, argv+5, argv+5);
        ramp_time = atom_getfloatarg(6, argc, argv);
        minimum = atom_getfloatarg(7, argc, argv);
        maximum = atom_getfloatarg(8, argc, argv);
    }
    
    x->x_gui.x_draw = (t_iemfunptr)number_tilde_draw;
    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();
    x->x_in_val = 0.0;
    x->x_out_val = 0.0;
    x->x_set_val = 0.0;
    number_tilde_check_minmax(x, minimum, maximum);
    
    if(w < MINDIGITS)
        w = MINDIGITS;
    x->x_numwidth = w;
    if(h < IEM_GUI_MINSIZE)
        h = IEM_GUI_MINSIZE;
    
    x->x_gui.x_h = h + 5;
    x->x_gui.x_fontsize = h;

    strcpy(x->x_gui.x_font, sys_font);

    x->x_buf[0] = 0;
    
    x->x_clock_reset = clock_new(x, (t_method)number_tilde_tick_reset);
    x->x_clock_wait = clock_new(x, (t_method)number_tilde_tick_wait);
    x->x_clock_repaint = clock_new(x, (t_method)number_tilde_periodic_update);
    x->x_gui.x_fsf.x_change = 0;
    iemgui_newzoom(&x->x_gui);
    number_tilde_calc_fontwidth(x);
    
    x->x_interval_ms = interval;
    x->x_output_mode = mode;
    x->x_ramp_time = ramp_time;
    
    // Start repaint clock
    clock_delay(x->x_clock_repaint, x->x_interval_ms);
    
    return (x);
}

static void number_tilde_free(t_number_tilde *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    clock_free(x->x_clock_reset);
    clock_free(x->x_clock_wait);
    clock_free(x->x_clock_repaint);
    outlet_free(x->x_signal_outlet);
    outlet_free(x->x_float_outlet);
    gfxstub_deleteforkey(x);
}

static t_int *number_tilde_perform(t_int *w)
{
    t_number_tilde *x = (t_number_tilde *)(w[1]);
    t_sample *in      = (t_sample *)(w[2]);
    t_sample *out     = (t_sample *)(w[3]);
    t_int n           = (t_int)(w[4]);
    
    // Take random sample
    t_int idx = rand() % n;
    x->x_in_val = in[idx];
    x->x_needs_update = 1;

    // Check if we need to apply a ramp
    if(x->x_ramp_val != x->x_out_val && x->x_ramp != 0) {
        for(int i = 0; i < n; i++)  {
            
            // Check if we reached our destination
            if((x->x_ramp < 0 && x->x_ramp_val <= x->x_out_val) || (x->x_ramp > 0 && x->x_ramp_val >= x->x_out_val)) {
                x->x_ramp_val = x->x_out_val;
                x->x_ramp = 0;
            }
            
            x->x_ramp_val += x->x_ramp;

            out[i] = x->x_ramp_val;
            x->x_last_out = x->x_ramp_val;
        }
    }
    // No ramp needed
    else {
        for(int i = 0; i < n; i++)  {
            out[i] = x->x_out_val;
        }
    }
    
    return (w + 5);
}

static void number_tilde_dsp(t_number_tilde *x, t_signal **sp)
{
    dsp_add(number_tilde_perform, 4, x, sp[0]->s_vec, sp[1]->s_vec, (t_int)sp[0]->s_n);
}

CYCLONE_OBJ_API void number_tilde_setup(void)
{
    number_tilde_class = class_new(gensym("number~"), (t_newmethod)number_tilde_new,
        (t_method)number_tilde_free, sizeof(t_number_tilde), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)number_tilde_new, gensym("number~"), A_GIMME, 0);

    CLASS_MAINSIGNALIN(number_tilde_class, t_number_tilde, x_in_val);

    class_addmethod(number_tilde_class, (t_method)number_tilde_dsp, gensym("dsp"), A_CANT, 0);
    
    class_addbang(number_tilde_class, number_tilde_bang);
    class_addfloat(number_tilde_class, number_tilde_float);
    class_addlist(number_tilde_class, number_tilde_list);
    
    class_addmethod(number_tilde_class, (t_method)number_tilde_click,
        gensym("click"), A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_motion,
        gensym("motion"), A_FLOAT, A_FLOAT, A_DEFFLOAT, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_dialog,
        gensym("dialog"), A_GIMME, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_set,
        gensym("set"), A_FLOAT, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_size,
        gensym("size"), A_GIMME, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_delta,
        gensym("delta"), A_GIMME, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_pos,
        gensym("pos"), A_GIMME, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_color,
        gensym("color"), A_GIMME, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_interval,
        gensym("interval"), A_FLOAT, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_mode,
        gensym("mode"), A_FLOAT, 0);
    class_addmethod(number_tilde_class, (t_method)iemgui_zoom,
        gensym("zoom"), A_CANT, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_minimum,
        gensym("minimum"), A_FLOAT, 0);
    class_addmethod(number_tilde_class, (t_method)number_tilde_maximum,
        gensym("maximum"), A_FLOAT, 0);
    
    number_tilde_widgetbehavior.w_getrectfn =    number_tilde_getrect;
    number_tilde_widgetbehavior.w_displacefn =   iemgui_displace;
    number_tilde_widgetbehavior.w_selectfn =     iemgui_select;
    number_tilde_widgetbehavior.w_activatefn =   NULL;
    number_tilde_widgetbehavior.w_deletefn =     iemgui_delete;
    number_tilde_widgetbehavior.w_visfn =        iemgui_vis;
    number_tilde_widgetbehavior.w_clickfn =      number_tilde_newclick;
    class_setwidget(number_tilde_class, &number_tilde_widgetbehavior);
    class_setsavefn(number_tilde_class, number_tilde_save);
    class_setpropertiesfn(number_tilde_class, number_tilde_properties);
}

// Tcl/Tk code for the preferences dialog, based on the default IEMGUI dialog, but with a few custom fields
static void init_number_tilde_dialog(void)
{
    sys_gui("\n"
            "package provide dialog_number 0.1\n"
            "namespace eval ::dialog_number:: {\n"
            "    variable define_min_fontsize 4\n"
            "\n"
            "    namespace export pdtk_number_tilde_dialog\n"
            "}\n"
            "\n"
            "\n"
            "proc ::dialog_number::clip_dim {mytoplevel} {\n"
            "    set vid [string trimleft $mytoplevel .]\n"
            "\n"
            "    set var_iemgui_wdt [concat iemgui_wdt_$vid]\n"
            "    global $var_iemgui_wdt\n"
            "    set var_iemgui_min_wdt [concat iemgui_min_wdt_$vid]\n"
            "    global $var_iemgui_min_wdt\n"
            "    set var_iemgui_hgt [concat iemgui_hgt_$vid]\n"
            "    global $var_iemgui_hgt\n"
            "    set var_iemgui_min_hgt [concat iemgui_min_hgt_$vid]\n"
            "    global $var_iemgui_min_hgt\n"
            "\n"
            "    if {[eval concat $$var_iemgui_wdt] < [eval concat $$var_iemgui_min_wdt]} {\n"
            "        set $var_iemgui_wdt [eval concat $$var_iemgui_min_wdt]\n"
            "        $mytoplevel.dim.w_ent configure -textvariable $var_iemgui_wdt\n"
            "    }\n"
            "    if {[eval concat $$var_iemgui_hgt] < [eval concat $$var_iemgui_min_hgt]} {\n"
            "        set $var_iemgui_hgt [eval concat $$var_iemgui_min_hgt]\n"
            "        $mytoplevel.dim.h_ent configure -textvariable $var_iemgui_hgt\n"
            "    }\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            
            "proc ::dialog_number::set_col_example {mytoplevel} {\n"
            "    set vid [string trimleft $mytoplevel .]\n"
            "\n"
            "    set var_iemgui_bcol [concat iemgui_bcol_$vid]\n"
            "    global $var_iemgui_bcol\n"
            "    set var_iemgui_fcol [concat iemgui_fcol_$vid]\n"
            "    global $var_iemgui_fcol\n"
            "\n"
            "    if { [eval concat $$var_iemgui_fcol] ne \"none\" } {\n"
            "        $mytoplevel.colors.sections.exp.fr_bk configure \\\n"
            "            -background [eval concat $$var_iemgui_bcol] \\\n"
            "            -activebackground [eval concat $$var_iemgui_bcol] \\\n"
            "            -foreground [eval concat $$var_iemgui_fcol] \\\n"
            "            -activeforeground [eval concat $$var_iemgui_fcol]\n"
            "    } else {\n"
            "        $mytoplevel.colors.sections.exp.fr_bk configure \\\n"
            "            -background [eval concat $$var_iemgui_bcol] \\\n"
            "            -activebackground [eval concat $$var_iemgui_bcol] \\\n"
            "            -foreground [eval concat $$var_iemgui_bcol] \\\n"
            "            -activeforeground [eval concat $$var_iemgui_bcol]}\n"
            "\n"
            "    # for OSX live updates\n"
            "    if {$::windowingsystem eq \"aqua\"} {\n"
            "        ::dialog_number::apply_and_rebind_return $mytoplevel\n"
            "    }\n"
            "}\n"
            "\n"
            "proc ::dialog_number::preset_col {mytoplevel presetcol} {\n"
            "    set vid [string trimleft $mytoplevel .]\n"
            "    set var_iemgui_l2_f1_b0 [concat iemgui_l2_f1_b0_$vid]\n"
            "    global $var_iemgui_l2_f1_b0\n"
            "\n"
            "    set var_iemgui_bcol [concat iemgui_bcol_$vid]\n"
            "    global $var_iemgui_bcol\n"
            "    set var_iemgui_fcol [concat iemgui_fcol_$vid]\n"
            "    global $var_iemgui_fcol\n"
            "\n"
            "    if { [eval concat $$var_iemgui_l2_f1_b0] == 0 } { set $var_iemgui_bcol $presetcol }\n"
            "    if { [eval concat $$var_iemgui_l2_f1_b0] == 1 } { set $var_iemgui_fcol $presetcol }\n"
            "    ::dialog_number::set_col_example $mytoplevel\n"
            "}\n"
            "\n"
            "proc ::dialog_number::choose_col_bkfrlb {mytoplevel} {\n"
            "    set vid [string trimleft $mytoplevel .]\n"
            "\n"
            "    set var_iemgui_bcol [concat iemgui_bcol_$vid]\n"
            "    global $var_iemgui_bcol\n"
            "    set var_iemgui_fcol [concat iemgui_fcol_$vid]\n"
            "    global $var_iemgui_fcol\n"
            "\n"
            "    if {[eval concat $$var_iemgui_l2_f1_b0] == 0} {\n"
            "        set $var_iemgui_bcol [eval concat $$var_iemgui_bcol]\n"
            "        set helpstring [tk_chooseColor -title [_ \"Background color\"] -initialcolor [eval concat $$var_iemgui_bcol]]\n"
            "        if { $helpstring ne \"\" } {\n"
            "            set $var_iemgui_bcol $helpstring }\n"
            "    }\n"
            "    if {[eval concat $$var_iemgui_l2_f1_b0] == 1} {\n"
            "        set $var_iemgui_fcol [eval concat $$var_iemgui_fcol]\n"
            "        set helpstring [tk_chooseColor -title [_ \"Foreground color\"] -initialcolor [eval concat $$var_iemgui_fcol]]\n"
            "        if { $helpstring ne \"\" } {\n"
            "            set $var_iemgui_fcol $helpstring }\n"
            "    }\n"
            "    ::dialog_number::set_col_example $mytoplevel\n"
            "}\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "proc ::dialog_number::mode {mytoplevel} {\n"
            "    set vid [string trimleft $mytoplevel .]\n"
            "\n"
            "    set var_iemgui_mode [concat iemgui_mode_$vid]\n"
            "    global $var_iemgui_mode\n"
            "\n"
            "    if {[eval concat $$var_iemgui_mode]} {\n"
            "        set $var_iemgui_mode 0\n"
            "        $mytoplevel.para.mode configure -text [_ \"Input\"]\n"
            "    } else {\n"
            "        set $var_iemgui_mode 1\n"
            "        $mytoplevel.para.mode configure -text [_ \"Output\"]\n"
            "    }\n"
            "}\n"
            "\n"
            "proc ::dialog_number::apply {mytoplevel} {\n"
            "    set vid [string trimleft $mytoplevel .]\n"
            "\n"
            "    set var_iemgui_wdt [concat iemgui_wdt_$vid]\n"
            "    global $var_iemgui_wdt\n"
            "    set var_iemgui_min_wdt [concat iemgui_min_wdt_$vid]\n"
            "    global $var_iemgui_min_wdt\n"
            "    set var_iemgui_hgt [concat iemgui_hgt_$vid]\n"
            "    global $var_iemgui_hgt\n"
            "    set var_iemgui_min_hgt [concat iemgui_min_hgt_$vid]\n"
            "    global $var_iemgui_min_hgt\n"
            "    set var_iemgui_interval [concat iemgui_interval_$vid]\n"
            "    global $var_iemgui_interval\n"
            "    set var_iemgui_mode [concat iemgui_mode_$vid]\n"
            "    global $var_iemgui_mode\n"
            "    set var_iemgui_bcol [concat iemgui_bcol_$vid]\n"
            "    global $var_iemgui_bcol\n"
            "    set var_iemgui_fcol [concat iemgui_fcol_$vid]\n"
            "    global $var_iemgui_fcol\n"
            "    set var_iemgui_min_rng [concat iemgui_min_rng_$vid]\n"
            "    global $var_iemgui_min_rng\n"
            "    set var_iemgui_max_rng [concat iemgui_max_rng_$vid]\n"
            "    global $var_iemgui_max_rng\n"
            "\n"
            "    ::dialog_number::clip_dim $mytoplevel\n"
            "\n"
            "\n"
            "\n"
            "\n"
            "    pdsend [concat $mytoplevel dialog \\\n"
            "            [eval concat $$var_iemgui_wdt] \\\n"
            "            [eval concat $$var_iemgui_hgt] \\\n"
            "            [eval concat $$var_iemgui_mode] \\\n"
            "            [eval concat $$var_iemgui_interval] \\\n"
            "            [string tolower [eval concat $$var_iemgui_bcol]] \\\n"
            "            [string tolower [eval concat $$var_iemgui_fcol]] \\\n"
            "            [eval concat $$var_iemgui_min_rng] \\\n"
            "            [eval concat $$var_iemgui_max_rng]] \\\n"

            "}\n"
            "\n"
            "\n"
            "proc ::dialog_number::cancel {mytoplevel} {\n"
            "    pdsend \"$mytoplevel cancel\"\n"
            "}\n"
            "\n"
            "proc ::dialog_number::ok {mytoplevel} {\n"
            "    ::dialog_number::apply $mytoplevel\n"
            "    ::dialog_number::cancel $mytoplevel\n"
            "}\n"
            "\n"
            "proc ::dialog_number::pdtk_number_tilde_dialog {mytoplevel dim_header \\\n"
            "                                       wdt min_wdt \\\n"
            "                                       hgt min_hgt \\\n"
            "                                       mode interval \\\n"
            "                                       bcol fcol min_rng max_rng} {\n"
            "\n"
            "    set vid [string trimleft $mytoplevel .]\n"
            "\n"
            "    set var_iemgui_wdt [concat iemgui_wdt_$vid]\n"
            "    global $var_iemgui_wdt\n"
            "    set var_iemgui_min_wdt [concat iemgui_min_wdt_$vid]\n"
            "    global $var_iemgui_min_wdt\n"
            "    set var_iemgui_hgt [concat iemgui_hgt_$vid]\n"
            "    global $var_iemgui_hgt\n"
            "    set var_iemgui_min_hgt [concat iemgui_min_hgt_$vid]\n"
            "    global $var_iemgui_min_hgt\n"
            "    set var_iemgui_interval [concat iemgui_interval_$vid]\n"
            "    global $var_iemgui_interval\n"
            "    set var_iemgui_mode [concat iemgui_mode_$vid]\n"
            "    global $var_iemgui_mode\n"
            "    set var_iemgui_bcol [concat iemgui_bcol_$vid]\n"
            "    global $var_iemgui_bcol\n"
            "    set var_iemgui_fcol [concat iemgui_fcol_$vid]\n"
            "    global $var_iemgui_fcol\n"
            "    set var_iemgui_l2_f1_b0 [concat iemgui_l2_f1_b0_$vid]\n"
            "    global $var_iemgui_l2_f1_b0\n"
            "    set var_iemgui_min_rng [concat iemgui_min_rng_$vid]\n"
            "    global $var_iemgui_min_rng\n"
            "    set var_iemgui_max_rng [concat iemgui_max_rng_$vid]\n"
            "    global $var_iemgui_max_rng\n"
            "\n"
            "    set $var_iemgui_wdt $wdt\n"
            "    set $var_iemgui_min_wdt $min_wdt\n"
            "    set $var_iemgui_hgt $hgt\n"
            "    set $var_iemgui_min_hgt $min_hgt\n"
            "    set $var_iemgui_interval $interval\n"
            "    set $var_iemgui_mode $mode\n"
            "    set $var_iemgui_bcol $bcol\n"
            "    set $var_iemgui_fcol $fcol\n"
            "    set $var_iemgui_min_rng $min_rng\n"
            "    set $var_iemgui_max_rng $max_rng\n"
            "\n"
            "    set $var_iemgui_l2_f1_b0 0\n"
            "\n"
            "    set iemgui_type [_ \"Number~\"]\n"
            "    set wdt_label [_ \"Width (digits):\"]\n"
            "    set hgt_label [_ \"Font Size:\"]\n"
            "    toplevel $mytoplevel -class DialogWindow\n"
            "    wm title $mytoplevel [format [_ \"%s Properties\"] $iemgui_type]\n"
            "    wm group $mytoplevel .\n"
            "    wm resizable $mytoplevel 0 0\n"
            "    wm transient $mytoplevel $::focused_window\n"
            "    $mytoplevel configure -menu $::dialog_menubar\n"
            "    $mytoplevel configure -padx 0 -pady 0\n"
            "    ::pd_bindings::dialog_bindings $mytoplevel \"iemgui\"\n"
            "\n"
            "    # dimensions\n"
            "    frame $mytoplevel.dim -height 7\n"
            "    pack $mytoplevel.dim -side top\n"
            "    label $mytoplevel.dim.w_lab -text [_ $wdt_label]\n"
            "    entry $mytoplevel.dim.w_ent -textvariable $var_iemgui_wdt -width 4\n"
            "    label $mytoplevel.dim.dummy1 -text \"\" -width 1\n"
            "    label $mytoplevel.dim.h_lab -text [_ $hgt_label]\n"
            "    entry $mytoplevel.dim.h_ent -textvariable $var_iemgui_hgt -width 4\n"
            "    pack $mytoplevel.dim.w_lab $mytoplevel.dim.w_ent -side left\n"
            "    if { $hgt_label ne \"empty\" } {\n"
            "        pack $mytoplevel.dim.dummy1 $mytoplevel.dim.h_lab $mytoplevel.dim.h_ent -side left }\n"
            "\n"
            "    # range\n"
            "    labelframe $mytoplevel.rng\n"
            "    pack $mytoplevel.rng -side top -fill x\n"
            "    frame $mytoplevel.rng.min\n"
            "    label $mytoplevel.rng.min.lab -text \"Minimum\"\n"
            "    entry $mytoplevel.rng.min.ent -textvariable $var_iemgui_min_rng -width 7\n"
            "    label $mytoplevel.rng.dummy1 -text \"\" -width 1\n"
            "    label $mytoplevel.rng.max_lab -text \"Maximum\"\n"
            "    entry $mytoplevel.rng.max_ent -textvariable $var_iemgui_max_rng -width 7\n"
            "    $mytoplevel.rng config -borderwidth 1 -pady 4 -text \"Output Range\"\n"
     
            "    pack $mytoplevel.rng.min\n"
            "    pack $mytoplevel.rng.min.lab $mytoplevel.rng.min.ent -side left\n"
            "    $mytoplevel.rng config -padx 26\n"
            "    pack configure $mytoplevel.rng.min -side left\n"
            "    pack $mytoplevel.rng.dummy1 $mytoplevel.rng.max_lab $mytoplevel.rng.max_ent -side left\n"
            "\n"
            "    # parameters\n"
            "    labelframe $mytoplevel.para -borderwidth 1 -padx 5 -pady 5 -text [_ \"Parameters\"]\n"
            "    pack $mytoplevel.para -side top -fill x -pady 5\n"

            "   frame $mytoplevel.para.interval\n"
            "       label $mytoplevel.para.interval.lab -text [_ \"Interval (ms)\"]\n"
            "       entry $mytoplevel.para.interval.ent -textvariable $var_iemgui_interval -width 6\n"
            "       pack $mytoplevel.para.interval.ent $mytoplevel.para.interval.lab -side right -anchor e\n"
            "    if {[eval concat $$var_iemgui_mode] == 0} {\n"
            "        button $mytoplevel.para.mode -command \"::dialog_number::mode $mytoplevel\" \\\n"
            "            -text [_ \"Input\"] }\n"
            "    if {[eval concat $$var_iemgui_mode] == 1} {\n"
            "        button $mytoplevel.para.mode -command \"::dialog_number::mode $mytoplevel\" \\\n"
            "            -text [_ \"Output\"] }\n"
            "        pack $mytoplevel.para.interval -side left -expand 1 -ipadx 10\n"
            "        pack $mytoplevel.para.mode -side left -expand 1 -ipadx 10\n"
            "    # get the current font name from the int given from C-space (gn_f)\n"
            "    set current_font $::font_family\n"
            "\n"
            "    # colors\n"
            "    labelframe $mytoplevel.colors -borderwidth 1 -text [_ \"Colors\"] -padx 5 -pady 5\n"
            "    pack $mytoplevel.colors -fill x\n"
            "\n"
            "    frame $mytoplevel.colors.select\n"
            "    pack $mytoplevel.colors.select -side top\n"
            "    radiobutton $mytoplevel.colors.select.radio0 -value 0 -variable \\\n"
            "        $var_iemgui_l2_f1_b0 -text [_ \"Background\"] -justify left\n"
            "    radiobutton $mytoplevel.colors.select.radio1 -value 1 -variable \\\n"
            "        $var_iemgui_l2_f1_b0 -text [_ \"Foreground\"] -justify left\n"
            "    radiobutton $mytoplevel.colors.select.radio2 -value 2 -variable \\\n"
            "        $var_iemgui_l2_f1_b0 -text [_ \"Text\"] -justify left\n"
            "    if { [eval concat $$var_iemgui_fcol] ne \"none\" } {\n"
            "        pack $mytoplevel.colors.select.radio0 $mytoplevel.colors.select.radio1 \\\n"
            "            -side left\n"
            "    } else {\n"
            "        pack $mytoplevel.colors.select.radio0 -side left\n"
            "    }\n"
            "\n"
            "    frame $mytoplevel.colors.sections\n"
            "    pack $mytoplevel.colors.sections -side top\n"
            "    button $mytoplevel.colors.sections.but -text [_ \"Compose color\"] \\\n"
            "        -command \"::dialog_number::choose_col_bkfrlb $mytoplevel\"\n"
            "    pack $mytoplevel.colors.sections.but -side left -anchor w -pady 5 \\\n"
            "        -expand yes -fill x\n"
            "    frame $mytoplevel.colors.sections.exp\n"
            "    pack $mytoplevel.colors.sections.exp -side right -padx 5\n"
            "    if { [eval concat $$var_iemgui_fcol] ne \"none\" } {\n"
            "        label $mytoplevel.colors.sections.exp.fr_bk -text \"o=||=o\" -width 6 \\\n"
            "            -background [eval concat $$var_iemgui_bcol] \\\n"
            "            -activebackground [eval concat $$var_iemgui_bcol] \\\n"
            "            -foreground [eval concat $$var_iemgui_fcol] \\\n"
            "            -activeforeground [eval concat $$var_iemgui_fcol] \\\n"
            "            -font [list $current_font 14 $::font_weight] -padx 2 -pady 2 -relief ridge\n"
            "    } else {\n"
            "        label $mytoplevel.colors.sections.exp.fr_bk -text \"o=||=o\" -width 6 \\\n"
            "            -background [eval concat $$var_iemgui_bcol] \\\n"
            "            -activebackground [eval concat $$var_iemgui_bcol] \\\n"
            "            -foreground [eval concat $$var_iemgui_bcol] \\\n"
            "            -activeforeground [eval concat $$var_iemgui_bcol] \\\n"
            "            -font [list $current_font 14 $::font_weight] -padx 2 -pady 2 -relief ridge\n"
            "    }\n"
            "\n"
            "    # color scheme by Mary Ann Benedetto http://piR2.org\n"
            "    foreach r {r1 r2 r3} hexcols {\n"
            "       { \"#FFFFFF\" \"#DFDFDF\" \"#BBBBBB\" \"#FFC7C6\" \"#FFE3C6\" \"#FEFFC6\" \"#C6FFC7\" \"#C6FEFF\" \"#C7C6FF\" \"#E3C6FF\" }\n"
            "       { \"#9F9F9F\" \"#7C7C7C\" \"#606060\" \"#FF0400\" \"#FF8300\" \"#FAFF00\" \"#00FF04\" \"#00FAFF\" \"#0400FF\" \"#9C00FF\" }\n"
            "       { \"#404040\" \"#202020\" \"#000000\" \"#551312\" \"#553512\" \"#535512\" \"#0F4710\" \"#0E4345\" \"#131255\" \"#2F004D\" } } \\\n"
            "    {\n"
            "       frame $mytoplevel.colors.$r\n"
            "       pack $mytoplevel.colors.$r -side top\n"
            "       foreach i { 0 1 2 3 4 5 6 7 8 9} hexcol $hexcols \\\n"
            "           {\n"
            "               label $mytoplevel.colors.$r.c$i -background $hexcol -activebackground $hexcol -relief ridge -padx 7 -pady 0 -width 1\n"
            "               bind $mytoplevel.colors.$r.c$i <Button> \"::dialog_number::preset_col $mytoplevel $hexcol\"\n"
            "           }\n"
            "       pack $mytoplevel.colors.$r.c0 $mytoplevel.colors.$r.c1 $mytoplevel.colors.$r.c2 $mytoplevel.colors.$r.c3 \\\n"
            "           $mytoplevel.colors.$r.c4 $mytoplevel.colors.$r.c5 $mytoplevel.colors.$r.c6 $mytoplevel.colors.$r.c7 \\\n"
            "           $mytoplevel.colors.$r.c8 $mytoplevel.colors.$r.c9 -side left\n"
            "    }\n"
            "\n"
            "    # buttons\n"
            "    frame $mytoplevel.cao -pady 10\n"
            "    pack $mytoplevel.cao -side top\n"
            "    button $mytoplevel.cao.cancel -text [_ \"Cancel\"] \\\n"
            "        -command \"::dialog_number::cancel $mytoplevel\"\n"
            "    pack $mytoplevel.cao.cancel -side left -expand 1 -fill x -padx 15 -ipadx 10\n"
            "    if {$::windowingsystem ne \"aqua\"} {\n"
            "        button $mytoplevel.cao.apply -text [_ \"Apply\"] \\\n"
            "            -command \"::dialog_number::apply $mytoplevel\"\n"
            "        pack $mytoplevel.cao.apply -side left -expand 1 -fill x -padx 15 -ipadx 10\n"
            "    }\n"
            "    button $mytoplevel.cao.ok -text [_ \"OK\"] \\\n"
            "        -command \"::dialog_number::ok $mytoplevel\" -default active\n"
            "    pack $mytoplevel.cao.ok -side left -expand 1 -fill x -padx 15 -ipadx 10\n"
            "\n"
            "    $mytoplevel.dim.w_ent select from 0\n"
            "    $mytoplevel.dim.w_ent select adjust end\n"
            "    focus $mytoplevel.dim.w_ent\n"
            "\n"
            "    # live widget updates on OSX in lieu of Apply button\n"
            "    if {$::windowingsystem eq \"aqua\"} {\n"
            "\n"
            "        # call apply on Return in entry boxes that are in focus & rebind Return to ok button\n"
            "        bind $mytoplevel.dim.w_ent <KeyPress-Return> \"::dialog_number::apply_and_rebind_return $mytoplevel\"\n"
            "        bind $mytoplevel.dim.h_ent <KeyPress-Return>    \"::dialog_number::apply_and_rebind_return $mytoplevel\"\n"
            "\n"
            "        # unbind Return from ok button when an entry takes focus\n"
            "        $mytoplevel.dim.w_ent config -validate focusin -vcmd \"::dialog_number::unbind_return $mytoplevel\"\n"
            "        $mytoplevel.dim.h_ent config -validate focusin -vcmd  \"::dialog_number::unbind_return $mytoplevel\"\n"
            "\n"
            "        # remove cancel button from focus list since it's not activated on Return\n"
            "        $mytoplevel.cao.cancel config -takefocus 0\n"
            "\n"
            "        # show active focus on the ok button as it *is* activated on Return\n"
            "        $mytoplevel.cao.ok config -default normal\n"
            "        bind $mytoplevel.cao.ok <FocusIn> \"$mytoplevel.cao.ok config -default active\"\n"
            "        bind $mytoplevel.cao.ok <FocusOut> \"$mytoplevel.cao.ok config -default normal\"\n"
            "\n"
            "        # since we show the active focus, disable the highlight outline\n"
            "        $mytoplevel.cao.ok config -highlightthickness 0\n"
            "        $mytoplevel.cao.cancel config -highlightthickness 0\n"
            "    }\n"
            "\n"
            "    position_over_window $mytoplevel $::focused_window\n"
            "}\n"
            "\n"
            "# for live widget updates on OSX\n"
            "proc ::dialog_number::apply_and_rebind_return {mytoplevel} {\n"
            "    ::dialog_number::apply $mytoplevel\n"
            "    bind $mytoplevel <KeyPress-Return> \"::dialog_number::ok $mytoplevel\"\n"
            "    focus $mytoplevel.cao.ok\n"
            "    return 0\n"
            "}\n"
            "\n"
            "# for live widget updates on OSX\n"
            "proc ::dialog_number::unbind_return {mytoplevel} {\n"
            "    bind $mytoplevel <KeyPress-Return> break\n"
            "    return 1\n"
            "}\n");
}
