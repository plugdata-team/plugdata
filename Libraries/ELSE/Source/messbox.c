 // based on entry

#include <m_pd.h>
#include <g_canvas.h>
#include <string.h>

#include "../extra_source/compat.h"

#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#define FONT_NAME    "DejaVu Sans Mono"
//#define HANDLE_SIZE  15
#define MIN_WIDTH    60
#define MIN_HEIGHT   30
#define BORDER       5

typedef struct _messbox_proxy{
    t_object         p_ob;
    struct _messbox *p_master;
    t_symbol        *x_bind_sym;
}t_messbox_proxy;

typedef struct _messbox{
    t_object         x_obj;
    t_canvas        *x_canvas;
    t_glist         *x_glist;
    t_symbol        *x_bind_sym;
    t_messbox_proxy *x_proxy;
    t_symbol        *x_dollzero;
    int              x_flag;
    int              x_height;
    int              x_width;
    int              x_resizing;
    int              x_active;
    int              x_selected;
    char             x_fgcolor[8];
    unsigned int     x_fg[3];    // fg RGB color
    char             x_bgcolor[8];
    unsigned int     x_bg[3];    // bg RGB color
    int              x_font_size;
    int              x_zoom;
    t_symbol        *x_font_weight;
    char            *tcl_namespace;
    char            *x_cv_id;
    char            *frame_id;
    char            *text_id;
//    char            *handle_id;
    char            *window_tag;
    char            *all_tag;
}t_messbox;

static t_class *messbox_class;
static t_class *messbox_proxy_class;
static t_widgetbehavior messbox_widgetbehavior;

static void set_tk_widget_ids(t_messbox *x, t_canvas *canvas){
    char buf[MAXPDSTRING];
    x->x_canvas = canvas;
    sprintf(buf,".x%lx.c", (long unsigned int)canvas);
    x->x_cv_id = getbytes(strlen(buf)+1); // Tk ID for the current canvas this object belongs
    strcpy(x->x_cv_id, buf);
    sprintf(buf,"%s.frame%lx", x->x_cv_id, (long unsigned int)x);
    x->frame_id = getbytes(strlen(buf)+1); // Tk ID for the "frame" the other things are drawn in
    strcpy(x->frame_id, buf);
    sprintf(buf,"%s.text%lx", x->frame_id, (long unsigned int)x);
    x->text_id = getbytes(strlen(buf)+1); // Tk ID for the "text"
    strcpy(x->text_id, buf);
    sprintf(buf,"%s.window%lx", x->x_cv_id, (long unsigned int)x);
    x->window_tag = getbytes(strlen(buf)+1); // Tk ID for the resizing "window"
    strcpy(x->window_tag, buf);
//    sprintf(buf,"%s.handle%lx", x->x_cv_id, (long unsigned int)x);
//    x->handle_id = getbytes(strlen(buf)+1); // Tk ID for the resizing "handle"
//    strcpy(x->handle_id, buf);
    sprintf(buf,"all%lx", (long unsigned int)x);
    x->all_tag = getbytes(strlen(buf)+1); // Tk ID for ALL
    strcpy(x->all_tag, buf);
}

static void draw_box(t_messbox *x){
    int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
    sys_vgui("%s create rectangle %d %d %d %d -outline %s -fill \"%s\" -width %d -tags %x_outline\n",
        x->x_cv_id, xpos, ypos, xpos+x->x_width*x->x_zoom, ypos+x->x_height*x->x_zoom,
        x->x_selected ? "blue" : "black", x->x_bgcolor, x->x_zoom, x);
    sys_vgui("%s create rectangle %d %d %d %d -fill black -tags {%x_inlet %s}\n",
        x->x_cv_id, xpos, ypos, xpos+(IOWIDTH*x->x_zoom), ypos+(IHEIGHT*x->x_zoom), x, x->all_tag);
    sys_vgui("%s create rectangle %d %d %d %d -fill black -tags {%x_outlet %s}\n",
        x->x_cv_id, xpos, ypos+x->x_height*x->x_zoom-1, xpos+IOWIDTH*x->x_zoom,
        ypos+x->x_height*x->x_zoom-IHEIGHT*x->x_zoom-1, x, x->all_tag);
}

static void erase_box(t_messbox *x){
    sys_vgui("%s delete %x_outline\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_inlet\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_outlet\n", x->x_cv_id, x);
}

static void messbox_draw(t_messbox *x, t_glist *glist){
    // I guess this is for fine-tuning of the rect size based on width and height
    int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    sys_vgui("namespace eval messbox%lx {}\n", x);
    sys_vgui("destroy %s\n", x->frame_id);  // Seems we gotta delete it if it exists
//    sys_vgui("destroy %s\n", x->handle_id);
    sys_vgui("frame %s\n", x->frame_id);
    sys_vgui("text %s -font {{%s} %d %s}  -highlightthickness 0 -bg \"%s\" -fg \"%s\"\n", x->text_id,
        FONT_NAME, x->x_font_size*x->x_zoom, x->x_font_weight->s_name, x->x_bgcolor, x->x_fgcolor);
    // catch ctl-v events from being propagated
    sys_vgui("bindtags %s {pre%s Text %s . all}\n", x->text_id, x->text_id, x->text_id);
    sys_vgui("::pd_bindings::bind_capslock %s $::modifier-Key v \
        {break}\n", x->text_id);
    sys_vgui("bind pre%s <KeyPress-Return> {pdsend {%s bang}\n\
        break}\n", x->text_id, x->x_bind_sym->s_name);
    sys_vgui("bind pre%s <Shift-KeyPress-Return> {#}\n", x->text_id);
    sys_vgui("pack %s -side left -fill both -expand 1\n", x->text_id);
    sys_vgui("pack %s -side bottom -fill both -expand 1\n", x->frame_id);
//    bind_button_events;
    sys_vgui("bind %s <Button> {pdtk_canvas_mouse %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b 0}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <ButtonRelease> {pdtk_canvas_mouseup %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <Button-2> {pdtk_canvas_rightclick %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <Button-3> {pdtk_canvas_rightclick %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("bind %s <Shift-Button> {pdtk_canvas_mouse %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] %%b 1}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
/* binding for editmode cursor change not working?
    sys_vgui("bind .x%lx <<EditMode>> {\n\
    set curs [expr \
    {$::editmode_button ? $cursor_editmode_nothing : \"xterm\"}]\n\
    %s configure -cursor $curs\n\
    %s configure -cursor $curs\n\
    update idletasks\n\
    }\n", glist_getcanvas(x->x_glist), x->text_id, x->frame_id); */
// mouse motion
    sys_vgui("bind %s <Motion> {pdtk_canvas_motion %s \
        [expr %%X - [winfo rootx %s]] [expr %%Y - [winfo rooty %s]] 0}\n",
        x->text_id, x->x_cv_id, x->x_cv_id, x->x_cv_id);
    sys_vgui("%s create window %d %d -anchor nw -window %s -tags {%s %s} -width %d -height %d\n",
        x->x_cv_id, xpos+BORDER, ypos+BORDER, x->frame_id, x->window_tag, x->all_tag,
        x->x_width-BORDER*2, x->x_height-BORDER*2);
    draw_box(x);
}

static void messbox_erase(t_messbox* x){
    sys_vgui("%s delete %x_outline\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_inlet\n", x->x_cv_id, x);
    sys_vgui("%s delete %x_outlet\n", x->x_cv_id, x);
    sys_vgui("destroy %s\n", x->frame_id);
    sys_vgui("%s delete %s\n", x->x_cv_id, x->all_tag);
}

// ------------------------ text widgetbehaviour------------------------------------------------
static void messbox_getrect(t_gobj *z, t_glist *owner, int *xp1, int *yp1, int *xp2, int *yp2){
    t_messbox *x = (t_messbox*)z;
    *xp1 = text_xpix(&x->x_obj, owner);
    *yp1 = text_ypix(&x->x_obj, owner);
    *xp2 = *xp1 + x->x_width*x->x_zoom;
    *yp2 = *yp1 + x->x_height*x->x_zoom;
}

static void messbox_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_messbox *x = (t_messbox *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    if(glist_isvisible(glist)){
        sys_vgui("%s move %s %d %d\n", x->x_cv_id, x->all_tag, dx*x->x_zoom, dy*x->x_zoom);
        sys_vgui("%s move %x_outline %d %d\n", x->x_cv_id, x, dx*x->x_zoom, dy*x->x_zoom);
        sys_vgui("%s move %lxRSZ %d %d\n", x->x_cv_id, (t_int)x, dx*x->x_zoom, dy*x->x_zoom);
        canvas_fixlinesfor(glist, (t_text*)x);
    }
}

static void messbox_key(void *z, t_floatarg f);

static void messbox_select(t_gobj *z, t_glist *glist, int state){
    t_messbox *x = (t_messbox *)z;
    if(state){
        sys_vgui("%s configure -fg blue -state disabled -cursor $cursor_editmode_nothing\n", x->text_id);
        sys_vgui("%s itemconfigure %x_outline -outline blue\n", x->x_cv_id, (unsigned long)x);
        x->x_selected = 1;
        glist = NULL;
/*        int x1, y1, x2, y2;
        messbox_getrect(z, glist, &x1, &y1, &x2, &y2);
        sys_vgui("canvas %s -width %d -height %d -bg #ddd -bd 0 \
            -highlightthickness 3 -highlightcolor {#f00} -cursor bottom_right_corner\n",
            x->handle_id, HANDLE_SIZE*x->x_zoom, HANDLE_SIZE*x->x_zoom);
        int handle_x1 = x2 - HANDLE_SIZE*x->x_zoom;
        int handle_y1 = y2 - HANDLE_SIZE*x->x_zoom;
        sys_vgui("%s create window %d %d -anchor nw -width %d -height %d -window %s -tags %lxRSZ\n",
            x->x_cv_id, handle_x1-2, handle_y1-2, HANDLE_SIZE*x->x_zoom, HANDLE_SIZE*x->x_zoom,
            x->handle_id, (t_int)x);
        sys_vgui("bind %s <Button> {pdsend {%s _resize 1}}\n",
            x->handle_id, x->x_bind_sym->s_name);
        sys_vgui("bind %s <ButtonRelease> {pdsend {%s _resize 0}}\n",
            x->handle_id, x->x_bind_sym->s_name);
        sys_vgui("bind %s <Motion> {pdsend {%s _motion %%x %%y }}\n",
            x->handle_id, x->x_bind_sym->s_name);
        sys_vgui("focus %s\n", x->handle_id); // because of a damn weird bug where it drew all over the canvas*/
    }
    else{
        sys_vgui("%s configure -fg %s -state normal -cursor xterm\n", x->text_id, x->x_fgcolor);
        x->x_selected = 0;
        sys_vgui("%s itemconfigure %x_outline -outline black\n", x->x_cv_id, (unsigned long)x);
//        sys_vgui("destroy %s\n", x->handle_id);
        messbox_key(z, 0);
    }
}

static void messbox_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void messbox_vis(t_gobj *z, t_glist *gl, int vis){
    t_messbox *x = (t_messbox*)z;
    vis ? messbox_draw(x, gl) : messbox_erase(x);
}

/////////// METHODS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static void messbox_list(t_messbox* x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(!ac){ // bang
        sys_vgui("pdsend \"%s [string map {\\$0 %s} [%s get 0.0 end]]\"\n",
            x->x_proxy->x_bind_sym->s_name, x->x_dollzero->s_name, x->text_id);
        return;
    }
    char buf[MAXPDSTRING];
    sprintf(buf, "\\$0 %s", x->x_dollzero->s_name);
    char symbuf[32]; // should be enough?
    size_t length = MAXPDSTRING - 1, tmplength;
    length -= strlen(buf);
    for(int i = 0; i < ac; i++) {
        sprintf(symbuf, " \\$%i ", i+1);
        tmplength = strlen(symbuf);
        length -= tmplength;
        if(length <= 0)
            break;
        strncat(buf, symbuf, tmplength);
        atom_string(av + i, symbuf, 32);
        tmplength = strlen(symbuf);
        length -= tmplength;
        if(length <= 0)
            break;
        strncat(buf, symbuf, tmplength);
    }
    sys_vgui("pdsend \"%s [string map {%s} [%s get 0.0 end]]\"\n",
        x->x_proxy->x_bind_sym->s_name, buf, x->text_id);
}

static void messbox_proxy_anything(t_messbox_proxy* x, t_symbol *s, int ac,
    t_atom *av){
    outlet_anything(x->p_master->x_obj.ob_outlet, s, ac, av);
}

static void messbox_append(t_messbox* x,  t_symbol *s, int ac, t_atom *av){
    s = NULL;
    char buf[40];
    size_t length;
    sys_vgui("%s configure -state normal\n", x->text_id);
    if(ac){
        size_t pos;
        for(int i = 0; i < ac; i++){
            t_symbol *sym = atom_getsymbolarg(i, ac, av);
            if(sym == &s_)
                sys_vgui("%s insert end \"%g \"\n", x->text_id, atom_getfloatarg(i, ac , av));
            else{
                int j = 0;
                length = 39;
                for(pos = 0; pos < strlen(sym->s_name); pos++) {
                    char c = sym->s_name[pos];
                    if(c == '\\' || c == '[' || c == '$' || c == ';') {
                        length--;
                        if(length <= 0) break;
                        buf[j++] = '\\';
                    }
                    length--;
                    if(length <= 0) break;
                    buf[j++] = c;
                }
                buf[j] = '\0';
                if(sym->s_name[pos-1] == ';')
                    sys_vgui("%s insert end %s\\n\n", x->text_id, buf);
                else
                    sys_vgui("%s insert end \"%s \"\n", x->text_id, buf);
            }
        }
        sys_vgui("%s yview end-2char\n", x->text_id);
    }
    if(!x->x_active)
        sys_vgui("%s configure -state disabled\n", x->text_id);
}

static void messbox_set(t_messbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    char buf[40];
    size_t length;
    sys_vgui("%s configure -state normal\n", x->text_id);
    sys_vgui("%s delete 0.0 end \n", x->text_id);
    if(ac){
        int i;
        size_t pos;
        for(i = 0; i < ac; i++){
            t_symbol *sym = atom_getsymbolarg(i, ac, av);
            if(sym == &s_) {
                sys_vgui("%s insert end \"%g \"\n", x->text_id, atom_getfloatarg(i, ac , av));
            } else{
                int j = 0;
                length = 39;
                for(pos = 0; pos < strlen(sym->s_name); pos++) {
                    char c = sym->s_name[pos];
                    if(c == '\\' || c == '[' || c == '$' || c == ';') {
                        length--;
                        if(length <= 0) break;
                        buf[j++] = '\\';
                    }
                    length--;
                    if(length <= 0) break;
                    buf[j++] = c;
                }
                buf[j] = '\0';
                if (sym->s_name[pos-1] == ';') {
                    sys_vgui("%s insert end %s\\n\n", x->text_id, buf);
                } else
                    sys_vgui("%s insert end \"%s \"\n", x->text_id, buf);
            }
        }
        sys_vgui("%s yview end-2char\n", x->text_id);
    }
    if(!x->x_active)
        sys_vgui("%s configure -state disabled\n", x->text_id);
}

void messbox_bgcolor(t_messbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(av[0].a_type == A_FLOAT && av[1].a_type == A_FLOAT && av[2].a_type == A_FLOAT){
        float r = atom_getfloatarg(0, ac, av);
        float g = atom_getfloatarg(1, ac, av);
        float b = atom_getfloatarg(2, ac, av);
        x->x_bg[0] = r < 0 ? 0 : r > 255 ? 255 : (int)r;
        x->x_bg[1] = g < 1 ? 0 : g > 255 ? 255 : (int)g;
        x->x_bg[2] = b < 2 ? 0 : b > 255 ? 255 : (int)b;
        sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0], x->x_bg[1], x->x_bg[2]);
        sys_vgui("%s configure -background \"%s\"\n", x->text_id, x->x_bgcolor);
        sys_vgui("%s itemconfigure %x_outline -fill %s\n", x->x_cv_id, (unsigned long)x, x->x_bgcolor);
    }
}

void messbox_fgcolor(t_messbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(av[0].a_type == A_FLOAT && av[1].a_type == A_FLOAT && av[2].a_type == A_FLOAT){
        float r = atom_getfloatarg(0, ac, av);
        float g = atom_getfloatarg(1, ac, av);
        float b = atom_getfloatarg(2, ac, av);
        x->x_fg[0] = r < 0 ? 0 : r > 255 ? 255 : (int)r;
        x->x_fg[1] = g < 1 ? 0 : g > 255 ? 255 : (int)g;
        x->x_fg[2] = b < 2 ? 0 : b > 255 ? 255 : (int)b;
        sprintf(x->x_fgcolor, "#%2.2x%2.2x%2.2x", x->x_fg[0], x->x_fg[1], x->x_fg[2]);
        sys_vgui("%s configure -foreground \"%s\"\n", x->text_id, x->x_fgcolor);
    }
}

static void messbox_bold(t_messbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(av[0].a_type == A_FLOAT){
        float bold = atom_getfloatarg(0, ac, av);
        x->x_font_weight = (int)(bold != 0) ? gensym("bold") : gensym("normal");
        sys_vgui("%s configure -font {{%s} %d %s}\n", x->text_id, FONT_NAME,
            x->x_font_size*x->x_zoom, x->x_font_weight->s_name);
    }
}

static void messbox_fontsize(t_messbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(av[0].a_type == A_FLOAT){
        float font_size = atom_getfloatarg(0, ac, av);
        x->x_font_size = font_size < 8 ? 8 : (int)font_size;
        sys_vgui("%s configure -font {{%s} %d %s}\n",
            x->text_id, FONT_NAME, x->x_font_size*x->x_zoom, x->x_font_weight->s_name);
    }
}

static void messbox_size(t_messbox *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(av[0].a_type == A_FLOAT && av[1].a_type == A_FLOAT){
        float w = atom_getfloatarg(0, ac, av), h = atom_getfloatarg(1, ac, av);
        x->x_width = w < MIN_WIDTH ? MIN_WIDTH : (int)w;
        x->x_height = h < MIN_HEIGHT ? MIN_HEIGHT : (int)h;
        if(glist_isvisible(x->x_glist)){
            sys_vgui("%s itemconfigure %s -width %d -height %d\n",
                x->x_cv_id, x->window_tag, x->x_width-BORDER*2, x->x_height-BORDER*2);
            canvas_fixlinesfor(x->x_glist, (t_text *)x);
            erase_box(x), draw_box(x);
        }
    }
}

static void messbox_key(void *z, t_floatarg f){
    if(f == 0){
        t_messbox *x = z;
        sys_vgui("%s configure -state disabled\n", x->text_id);
        sys_vgui("focus .x%lx.c\n", glist_getcanvas(x->x_glist)); // back to canvas
        x->x_active = 0;
        sys_vgui("%s itemconfigure %x_outline -width 1\n", x->x_cv_id, x);
    }
}

static int messbox_click(t_gobj *z, t_glist *glist, int xpix, int ypix, int shift, int alt, int dbl, int doit){
    xpix = ypix = alt = dbl = shift = 0;
    if(doit){
        t_messbox *x = (t_messbox *)z;
        x->x_active = 1;
        sys_vgui("%s itemconfigure %x_outline -width 2\n", x->x_cv_id, x);
        sys_vgui("%s configure -state normal\n", x->text_id);
        sys_vgui("focus %s\n", x->text_id);
        glist_grab(glist, (t_gobj *)x, 0, (t_glistkeyfn)messbox_key, 0, 0);
    }
    return(1);
}

// callback functions
/*static void messbox_resize_callback(t_messbox *x, t_floatarg f){ // ????
    x->x_resizing = (int)f;
}

static void messbox_motion_callback(t_messbox *x,
    t_floatarg f1, t_floatarg f2){
    if(x->x_resizing){
        int dx = (int)f1, dy = (int)f2, w = (x->x_width+dx/x->x_zoom), h = (x->x_height+dy/x->x_zoom);
        x->x_width = w < MIN_WIDTH ? MIN_WIDTH : w, x->x_height = h < MIN_HEIGHT ? MIN_HEIGHT : h;
        post("w (%d) = (x->x_width(%d)+dx(%d))/x->x_zoom(%d)", w, x->x_width, dx, x->x_zoom);
        t_atom av[2];
        SETFLOAT(av+0, w);
        SETFLOAT(av+1, h);
        messbox_size(x, NULL, 2, av);
        sys_vgui("%s move %lxRSZ %d %d\n", x->x_cv_id, (t_int)x, dx, dy);
    }
}*/

/////
static void messbox_save(t_gobj *z, t_binbuf *b){
    t_messbox *x = (t_messbox *)z;
    binbuf_addv(b, "ssiisiiiiiiiiii;", &s__X, gensym("obj"),
        x->x_obj.te_xpix, x->x_obj.te_ypix, atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
        x->x_width, x->x_height, x->x_bg[0], x->x_bg[1], x->x_bg[2],
        x->x_fg[0], x->x_fg[1], x->x_fg[2],
        x->x_font_weight == gensym("bold"), x->x_font_size);
}

static void messbox_zoom(t_messbox *x, t_floatarg zoom){
    x->x_zoom = (int)zoom;
}

static void messbox_free(t_messbox *x){
    pd_unbind(&x->x_obj.ob_pd, x->x_bind_sym);
    pd_unbind(&x->x_proxy->p_ob.ob_pd, x->x_proxy->x_bind_sym);
    pd_free((t_pd *)x->x_proxy);
}

static void *messbox_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_messbox *x = (t_messbox *)pd_new(messbox_class);
    char buf[MAXPDSTRING];
    x->x_glist = canvas_getcurrent();
    x->x_zoom = x->x_glist->gl_zoom;
    x->x_dollzero = binbuf_realizedollsym(gensym("$0"), 0, 0, 0);
    if(!(x->x_proxy = (t_messbox_proxy *)pd_new(messbox_proxy_class)))
        return(0);
    x->x_proxy->p_master = x;
    set_tk_widget_ids(x, glist_getcanvas(x->x_glist));
    sprintf(buf, "messbox%lx", (long unsigned int)x);
    x->tcl_namespace = getbytes(strlen(buf)+1);
    strcpy(x->tcl_namespace, buf);
    sprintf(buf,"#%s", x->tcl_namespace);
    x->x_bind_sym = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->x_bind_sym);   
    sprintf(buf, "#%s_proxy", x->tcl_namespace); 
    x->x_proxy->x_bind_sym = gensym(buf);
    pd_bind(&x->x_proxy->p_ob.ob_pd, x->x_proxy->x_bind_sym);
    x->x_font_size = glist_getfont(x->x_glist);
    x->x_selected = x->x_resizing = x->x_active = x->x_flag = 0;
    int w = x->x_font_size * 15, h = x->x_font_size * 5;
    int weight = 0;
    x->x_bg[0] = x->x_bg[1] = x->x_bg[2] = 235;
    x->x_fg[0] = x->x_fg[1] = x->x_fg[2] = 0;
    if(ac){
        if(ac == 10 && av->a_type == A_FLOAT){
               w = atom_getfloatarg(0, ac, av);
               h = atom_getfloatarg(1, ac, av);
               x->x_bg[0] = (unsigned int)atom_getfloatarg(2, ac, av);
               x->x_bg[1] = (unsigned int)atom_getfloatarg(3, ac, av);
               x->x_bg[2] = (unsigned int)atom_getfloatarg(4, ac, av);
               x->x_fg[0] = (unsigned int)atom_getfloatarg(5, ac, av);
               x->x_fg[1] = (unsigned int)atom_getfloatarg(6, ac, av);
               x->x_fg[2] = (unsigned int)atom_getfloatarg(7, ac, av);
               weight = (int)(atom_getfloatarg(8, ac, av));
               x->x_font_size = (int)(atom_getfloatarg(9, ac, av));
           }
           else{
               t_symbol *sym;
               for(int i = 0; i < ac; i++){
                   if(av[i].a_type == A_SYMBOL)
                       sym = av[i].a_w.w_symbol;
                   else goto errstate;
                   if(sym == gensym("-size")){
                       if((ac-i) >= 3){
                           x->x_flag = 1, i++;
                           if(av[i].a_type == A_FLOAT){
                               w = av[i].a_w.w_float, i++;
                               if(av[i].a_type == A_FLOAT)
                                   h = av[i].a_w.w_float;
                               else goto errstate;
                            }
                            else goto errstate;
                        }
                        else goto errstate;
                    }
                    else if(sym == gensym("-fontsize")){
                        if((ac-i) >= 2){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_font_size = av[i].a_w.w_float;
                            else goto errstate;
                        }
                        else goto errstate;                    }
                    else if(sym == gensym("-bold"))
                        x->x_flag = weight = 1;
                    else if(sym == gensym("-fgcolor")){
                        if((ac-i) >= 4){
                            x->x_flag = 1, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_fg[0] = (unsigned char)av[i].a_w.w_float, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_fg[1] = (unsigned char)av[i].a_w.w_float, i++;
                            if(av[i].a_type == A_FLOAT)
                                x->x_fg[2] = (unsigned char)av[i].a_w.w_float;
                            else goto errstate;
                        }
                        else goto errstate;
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
                    else
                        goto errstate;
                }
           }
    }
    x->x_width = w < MIN_WIDTH ? MIN_WIDTH : (int)w;
    x->x_height = w < MIN_HEIGHT ? MIN_HEIGHT : (int)h;
    if(x->x_font_size < 8)
        x->x_font_size = 8;
    sprintf(x->x_bgcolor, "#%2.2x%2.2x%2.2x", x->x_bg[0], x->x_bg[1], x->x_bg[2]);
    sprintf(x->x_fgcolor, "#%2.2x%2.2x%2.2x", x->x_fg[0], x->x_fg[1], x->x_fg[2]);
    x->x_font_weight = weight ? gensym("bold") : gensym("normal");
    outlet_new(&x->x_obj, &s_float);
    return(x);
    errstate:
        pd_error(x, "[messbox]: improper creation arguments");
        return(NULL);
}

void messbox_setup(void){
    messbox_class = class_new(gensym("messbox"), (t_newmethod)messbox_new,
        (t_method)messbox_free, sizeof(t_messbox), 0, A_GIMME, 0);
    messbox_proxy_class = class_new(0, 0, 0, sizeof(t_messbox_proxy),
        CLASS_PD | CLASS_NOINLET, 0);
    class_addlist(messbox_class, (t_method)messbox_list);
    class_addanything(messbox_proxy_class, (t_method)messbox_proxy_anything);
    class_addmethod(messbox_class, (t_method)messbox_size, gensym("dim"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_fontsize, gensym("fontsize"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_bold, gensym("bold"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_set, gensym("set"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_append, gensym("append"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_bgcolor, gensym("bgcolor"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_fgcolor, gensym("fgcolor"), A_GIMME, 0);
    class_addmethod(messbox_class, (t_method)messbox_click, gensym("click"),
        A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(messbox_class, (t_method)messbox_zoom, gensym("zoom"), A_CANT, 0);
//    class_addmethod(messbox_class, (t_method)messbox_resize_callback, gensym("_resize"), A_FLOAT, 0);
//    class_addmethod(messbox_class, (t_method)messbox_motion_callback, gensym("_motion"), A_FLOAT, A_FLOAT, 0);
    class_setwidget(messbox_class, &messbox_widgetbehavior);
    class_setsavefn(messbox_class, messbox_save);
    messbox_widgetbehavior.w_getrectfn  = messbox_getrect;
    messbox_widgetbehavior.w_displacefn = messbox_displace;
    messbox_widgetbehavior.w_selectfn   = messbox_select;
    messbox_widgetbehavior.w_activatefn = NULL;
    messbox_widgetbehavior.w_deletefn   = messbox_delete;
    messbox_widgetbehavior.w_visfn      = messbox_vis;
    messbox_widgetbehavior.w_clickfn    = (t_clickfn)messbox_click;
}
