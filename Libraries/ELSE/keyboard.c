// Prototype By Flávio Schiavoni, developed by Alexandre Porres

#include "m_pd.h"
#include "g_canvas.h"

#define BLACK_ON    "#6666FF"
#define BLACK_OFF   "#000000"
#define WHITE_ON    "#9999FF"
#define WHITE_OFF   "#FFFFFF"
#define MIDDLE_C    "#F0FFFF" // color of Middle C when off

static t_class *keyboard_class, *edit_proxy_class;
static t_widgetbehavior keyboard_widgetbehavior;

typedef struct _edit_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _keyboard *p_cnv;
}t_edit_proxy;

typedef struct _keyboard{
    t_object       x_obj;
    t_glist       *x_glist;
    t_edit_proxy  *x_proxy;
    int           *x_tgl_notes;    // to store which notes should be played
    int            x_velocity;     // to store velocity
    int            x_last_note;    // to store last note
    float          x_vel_in;       // to store the second inlet values
    float          x_space;
    int            x_width;
    int            x_height;
    int            x_octaves;
    int            x_first_c;
    int            x_low_c;
    int            x_toggle_mode;
    int            x_norm;
    int            x_zoom;
    int            x_shift;
    int            x_xpos;
    int            x_ypos;
    int            x_snd_set;
    int            x_rcv_set;
    int            x_flag;
    int            x_s_flag;
    int            x_r_flag;
    int            x_edit;
    t_symbol      *x_receive;
    t_symbol      *x_rcv_raw;
    t_symbol      *x_send;
    t_symbol      *x_snd_raw;
    t_symbol      *x_bindsym;
    t_outlet      *x_out;
}t_keyboard;

/* ------------------------- Keyboard Play ------------------------------*/
static void keyboard_note_on(t_keyboard* x, int note){
    int i = note - x->x_first_c;
    t_canvas *cv =  glist_getcanvas(x->x_glist);
    short key = note % 12, black = (key == 1 || key == 3 || key == 6 || key == 8 || key == 10);
    sys_vgui(".x%lx.c itemconfigure %xrrk%d -fill %s\n", cv, x, i, black ? BLACK_ON : WHITE_ON);
    int ac = 2;
    t_atom at[2];
    SETFLOAT(at, note);
    SETFLOAT(at+1, x->x_velocity);
    outlet_list(x->x_out, &s_list, ac, at);
    if(x->x_send != &s_ && x->x_send->s_thing)
        pd_list(x->x_send->s_thing, &s_list, ac, at);
}

static void keyboard_note_off(t_keyboard* x, int note){
    int i = note - x->x_first_c;
    if(x->x_tgl_notes[note] == 0){
        t_canvas *cv =  glist_getcanvas(x->x_glist);
        short key = note % 12, c4 = (note == 60), black = (key == 1 || key == 3 || key == 6 || key == 8 || key == 10);
        sys_vgui(".x%lx.c itemconfigure %xrrk%d -fill %s\n", cv, x, i, black ? BLACK_OFF : c4 ? MIDDLE_C : WHITE_OFF);
    }
    int ac = 2;
    t_atom at[2];
    SETFLOAT(at, note);
    SETFLOAT(at+1, 0);
    outlet_list(x->x_out, &s_list, ac, at);
    if(x->x_send != &s_ && x->x_send->s_thing)
        pd_list(x->x_send->s_thing, &s_list, ac, at);
}

static void keyboard_get_snd_rcv(t_keyboard* x){
    t_binbuf *bb = x->x_obj.te_binbuf;
    int n_args = binbuf_getnatom(bb), i = 0; // number of arguments
    char buf[128];
    if(!x->x_snd_set){ // no send set, search arguments/flags
        if(n_args > 0){ // we have arguments, let's search them
            if(x->x_flag){ // arguments are flags actually
                if(x->x_s_flag){ // we got a search flag, let's get it
                    for(i = 0;  i < n_args; i++){
                        atom_string(binbuf_getvec(bb) + i, buf, 80);
                        if(gensym(buf) == gensym("-send")){
                            i++;
                            atom_string(binbuf_getvec(bb) + i, buf, 80);
                            x->x_snd_raw = gensym(buf);
                            break;
                        }
                    }
                }
            }
            else{ // we got no flags, let's search for argument
                int arg_n = 7; // receive argument number
                if(n_args >= arg_n){ // we have it, get it
                    atom_string(binbuf_getvec(bb) + arg_n, buf, 80);
                    x->x_snd_raw = gensym(buf);
                }
            }
        }
    }
    if(x->x_snd_raw == &s_)
        x->x_snd_raw = gensym("empty");
    if(!x->x_rcv_set){ // no receive set, search arguments
        if(n_args > 0){ // we have arguments, let's search them
            if(x->x_flag){ // arguments are flags actually
                if(x->x_r_flag){ // we got a receive flag, let's get it
                    for(i = 0;  i < n_args; i++){
                        atom_string(binbuf_getvec(bb) + i, buf, 80);
                        if(gensym(buf) == gensym("-receive")){
                            i++;
                            atom_string(binbuf_getvec(bb) + i, buf, 80);
                            x->x_rcv_raw = gensym(buf);
                            break;
                        }
                    }
                }
            }
            else{ // we got no flags, let's search for argument
                int arg_n = 8; // receive argument number
                if(n_args >= arg_n){ // we have it, get it
                    atom_string(binbuf_getvec(bb) + arg_n, buf, 80);
                    x->x_rcv_raw = gensym(buf);
                }
            }
        }
    }
    if(x->x_rcv_raw == &s_)
        x->x_rcv_raw = gensym("empty");
}

// TOGGLE MODE
static void keyboard_play_tgl(t_keyboard* x, int note){
    int i = note - x->x_first_c;
    t_canvas *cv =  glist_getcanvas(x->x_glist);
    int on = x->x_tgl_notes[note] = x->x_tgl_notes[note] ? 0 : 1;
    short key = i % 12;
    if(key == 1 || key == 3 || key == 6 || key == 8 || key == 10) // black
        sys_vgui(".x%lx.c itemconfigure %xrrk%d -fill %s\n", cv, x, i, on ? BLACK_ON : BLACK_OFF);
     else // white
        sys_vgui(".x%lx.c itemconfigure %xrrk%d -fill %s\n", cv, x, i, on ? WHITE_ON : note == 60 ? MIDDLE_C : WHITE_OFF);
    int ac = 2;
    t_atom at[2];
    SETFLOAT(at, note);
    SETFLOAT(at+1, on ? x->x_velocity : 0);
    outlet_list(x->x_out, &s_list, ac, at);
    if(x->x_send != &s_ && x->x_send->s_thing)
        pd_list(x->x_send->s_thing, &s_list, ac, at);
}

/* -------------------- MOUSE Events ----------------------------------*/
static int find_note(t_keyboard* x, float xpos, float ypos){
    int sz = (int)x->x_space;
    int b_sz = (int)(x->x_space / 3.f);
    int white_key = (int)(xpos / x->x_space) % 7;
    int white_note = (white_key * 2) - (white_key > 2);
    int oct = (int)((xpos / x->x_space) / 7);
    int i = white_note + oct*12;
    if(ypos < x->x_height*2/3){ // find black keys
        x->x_velocity = x->x_norm > 0 ? x->x_norm : (int)((ypos / (x->x_height*2/3)) * 127);
        if(x->x_velocity <= 0)
            x->x_velocity = 1;
        if(white_key == 0 || white_key == 3){ // find sharp
            if(xpos > (white_key+1 + oct*7) * sz - b_sz)
                return(x->x_first_c+i+1);
        }
        else if(white_key == 2 || white_key == 6){ // find flat
            if(xpos < (white_key + oct*7) * sz + b_sz)
                return(x->x_first_c+i-1);
        }
        else if(white_key == 1 || white_key == 4 || white_key == 5){ // find both
            if(xpos < (white_key + oct*7) * sz + b_sz)
                return(x->x_first_c+i-1);
            else if(xpos > (white_key+1 + oct*7) * sz - b_sz)
                return(x->x_first_c+i+1);
        }
    }
    x->x_velocity = x->x_norm > 0 ? x->x_norm : (int)((ypos / x->x_height) * 127);
    if(x->x_velocity <= 0)
        x->x_velocity = 1;
    return(x->x_first_c+i);
}

static void keyboard_motion(t_keyboard *x, t_floatarg dx, t_floatarg dy){
    if(x->x_toggle_mode || x->x_shift || x->x_glist->gl_edit) // ignore if toggle or edit mode!
        return;
    x->x_xpos += dx, x->x_ypos += dy;
    if(x->x_xpos < 0 || x->x_xpos >= x->x_width) // ignore if out of bounds
        return;
    int note = find_note(x, x->x_xpos, x->x_ypos);
    if(note != x->x_last_note){
        keyboard_note_off(x, x->x_last_note);
        keyboard_note_on(x, x->x_last_note = note);
    }
}

static int keyboard_click(t_keyboard *x, t_glist *gl, int xpos, int ypos, int shift, int alt, int dbl, int doit){
    alt = dbl = 0; // remove warning
    if(doit){
        x->x_xpos = xpos - text_xpix(&x->x_obj, gl), x->x_ypos = ypos - text_ypix(&x->x_obj, gl);
        glist_grab(gl, &x->x_obj.te_g, (t_glistmotionfn)keyboard_motion, 0, xpos, ypos);
        int note = find_note(x, x->x_xpos, x->x_ypos);
        x->x_toggle_mode || (x->x_shift = shift) ? keyboard_play_tgl(x, note) : keyboard_note_on(x, x->x_last_note = note);
    }
    return(1);
}

static void keyboard_mouserelease(t_keyboard* x){
    if(x->x_toggle_mode || x->x_shift || x->x_glist->gl_edit) // ignore if toggle/edit mode!
        return;
    keyboard_note_off(x, x->x_last_note);
}

/* ------------------------ GUI SHIT----------------------------- */
// Erase the GUI
static void keyboard_erase(t_keyboard *x, t_glist *glist){
    t_canvas *cv = glist_getcanvas(glist);
    sys_vgui(".x%lx.c delete %xrr\n", cv, x);
    sys_vgui(".x%lx.c delete %lx_in1\n", cv, x);
    sys_vgui(".x%lx.c delete %lx_in2\n", cv, x);
    sys_vgui(".x%lx.c delete %lx_out\n", cv, x);
}

// draw inlet
static void keyboard_draw_io_let(t_keyboard *x){
    t_canvas *cv = glist_getcanvas(x->x_glist);
    int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
    if(x->x_edit && x->x_receive == &s_){
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags %lx_in1\n",
            cv, xpos, ypos, xpos+(IOWIDTH*x->x_zoom), ypos+(IHEIGHT*x->x_zoom), x);
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags %lx_in2\n",
            cv, xpos+x->x_width, ypos, xpos+x->x_width-(IOWIDTH*x->x_zoom), ypos+(IHEIGHT*x->x_zoom), x);
    }
    if(x->x_edit && x->x_send == &s_)
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags %lx_out\n",
            cv, xpos, ypos+x->x_height, xpos+IOWIDTH*x->x_zoom, ypos+x->x_height-IHEIGHT*x->x_zoom, x);
}

// Draw the GUI
static void keyboard_draw(t_keyboard *x, t_glist* glist){
    int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    t_canvas *cv = glist_getcanvas(x->x_glist);
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags %xrr\n", // Main rectangle
        cv, xpos, ypos, xpos + x->x_width, ypos + x->x_height, x);
    int i, wcount, bcount;
    wcount = bcount = 0;
    for(i = 0 ; i < x->x_octaves * 12 ; i++){ // white keys 1st (blacks overlay it)
        short key = i % 12;
        if(key != 1 && key != 3 && key != 6 && key != 8 && key != 10){
            int on = x->x_tgl_notes[x->x_first_c+i];
            int c4 = x->x_first_c + i == 60;
            sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags {%xrrk%d %xrr} -fill %s\n",
                     cv,
                     xpos + wcount * (int)x->x_space,
                     ypos,
                     xpos + (wcount + 1) * (int)x->x_space,
                     ypos + x->x_height,
                     x,
                     i,
                     x,
                     on ? WHITE_ON : c4 ? MIDDLE_C : WHITE_OFF);
            wcount++;
        }
    }
    for(i = 0 ; i < x->x_octaves * 12 ; i++){ // Black keys
        short key = i % 12;
        if(key == 4 || key == 11){
            bcount++;
            continue;
        }
        if(key == 1 || key == 3 || key ==6 || key == 8 || key == 10){
            int on = x->x_tgl_notes[x->x_first_c+i];
            sys_vgui(".x%lx.c create rectangle %d %d %d %d -tags {%xrrk%d %xrr} -fill %s\n",
                     cv,
                     xpos + ((bcount + 1) * (int)x->x_space) - ((int)(x->x_space / 3.f)) ,
                     ypos,
                     xpos + ((bcount + 1) * (int)x->x_space) + ((int)(x->x_space / 3.f)) ,
                     ypos + x->x_height * 2/3,
                     x,
                     i,
                     x,
                     on ? BLACK_ON : BLACK_OFF);
            bcount++;
        }
    }
    canvas_fixlinesfor(x->x_glist, (t_text *)x);
    keyboard_draw_io_let(x);
}

/* ------------------------ widgetbehaviour----------------------------- */
// GET RECT
static void keyboard_getrect(t_gobj *z, t_glist *owner, int *xp1, int *yp1, int *xp2, int *yp2){
    t_keyboard *x = (t_keyboard *)z;
    *xp1 = text_xpix(&x->x_obj, owner);
    *yp1 = text_ypix(&x->x_obj, owner) ;
    *xp2 = text_xpix(&x->x_obj, owner) + x->x_width;
    *yp2 = text_ypix(&x->x_obj, owner) + x->x_height;
}

// DISPLACE
void keyboard_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_keyboard *x = (t_keyboard *)z;
    t_canvas *cv = glist_getcanvas(glist);
    x->x_obj.te_xpix += dx, x->x_obj.te_ypix += dy;
    sys_vgui(".x%lx.c move %xrr %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    sys_vgui(".x%lx.c move %lx_in1 %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    sys_vgui(".x%lx.c move %lx_in2 %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    sys_vgui(".x%lx.c move %lx_out %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    canvas_fixlinesfor(glist, (t_text *)x);
}

// SELECT
static void keyboard_select(t_gobj *z, t_glist *gl, int sel){
    sys_vgui(".x%lx.c itemconfigure %xrr -outline %s\n", glist_getcanvas(gl), (t_keyboard *)z, sel ? "blue" : "black");
}

// Delete the GUI
static void keyboard_delete(t_gobj *z, t_glist *glist){
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist_getcanvas(glist), x);
}

// VIS
static void keyboard_vis(t_gobj *z, t_glist *glist, int vis){
    t_keyboard *x = (t_keyboard *)z;
    t_canvas *cv = glist_getcanvas(glist);
    if(vis){
        keyboard_draw(x, glist);
        sys_vgui(".x%lx.c bind %xrr <ButtonRelease> {pdsend [concat %s _mouserelease \\;]}\n", cv, x, x->x_bindsym->s_name);
    }
    else
        keyboard_erase(x, glist);
}

/* ------------------------- Methods ------------------------------*/
void keyboard_float(t_keyboard *x, t_floatarg f){
    int note = (int)f;
    if(x->x_vel_in < 0)
        x->x_vel_in = 0;
    if(x->x_vel_in > 127)
        x->x_vel_in = 127;
    int on = x->x_tgl_notes[note] = x->x_vel_in > 0;
    int ac = 2;
    t_atom at[2];
    SETFLOAT(at, note);
    SETFLOAT(at+1, x->x_vel_in);
    outlet_list(x->x_out, &s_list, ac, at);
    if(x->x_send != &s_ && x->x_send->s_thing)
        pd_list(x->x_send->s_thing, &s_list, ac, at);
    if(x->x_glist->gl_havewindow){
        t_canvas *cv =  glist_getcanvas(x->x_glist);
        if(note >= x->x_first_c && note < x->x_first_c + (x->x_octaves * 12)){
            int i = note - x->x_first_c, key = i % 12;
            if(key == 1 || key == 3 || key == 6 || key == 8 || key == 10) // black
                sys_vgui(".x%lx.c itemconfigure %xrrk%d -fill %s\n", cv, x, i, on ? BLACK_ON : BLACK_OFF);
            else // white
                sys_vgui(".x%lx.c itemconfigure %xrrk%d -fill %s\n", cv, x, i, on ? WHITE_ON : note == 60 ? MIDDLE_C : WHITE_OFF);
        }
    }
}

static void keyboard_height(t_keyboard *x, t_floatarg f){
    f =  f < 10 ? 10 : (int)(f);
    if(x->x_height != f){
        x->x_height = f;
        keyboard_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            keyboard_draw(x, x->x_glist);
            canvas_fixlinesfor(x->x_glist, (t_text *)x);
        }
    }
}

static void keyboard_width(t_keyboard *x, t_floatarg f){
    f = f < 7 ? 7 : (int)(f);
    if(x->x_space != f){
        x->x_space = f;
        keyboard_erase(x, x->x_glist);
        x->x_width = ((int)(x->x_space)) * 7 * (int)x->x_octaves;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            keyboard_draw(x, x->x_glist);
    }
}

static void keyboard_8ves(t_keyboard *x, t_floatarg f){
    f = f > 10 ? 10 : f < 1 ? 1 : (int)(f);
    if(x->x_octaves != f){
        x->x_octaves = f;
        x->x_width = x->x_space * 7 * x->x_octaves;
        keyboard_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            keyboard_draw(x, x->x_glist);
    }
}

static void keyboard_low_c(t_keyboard *x, t_floatarg f){
    f = f > 8 ? 8 : f < 0 ? 0 : (int)(f);
    if(x->x_low_c != f){
        x->x_first_c = ((int)((x->x_low_c = f) * 12)) + 12;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            keyboard_erase(x, x->x_glist), keyboard_draw(x, x->x_glist);
    }
}

static void keyboard_oct(t_keyboard *x, t_floatarg f){
    keyboard_low_c(x, x->x_low_c + (int)(f));
}

static void keyboard_toggle(t_keyboard *x, t_floatarg f){
    int tgl = f != 0;
    if(tgl != x->x_toggle_mode)
        x->x_toggle_mode = tgl;
}

static void keyboard_norm(t_keyboard *x, t_floatarg f){
    int norm = f < 0 ? 0 : f > 127 ? 127 : (int)f;
    if(norm != x->x_norm)
        x->x_norm = norm;
}

// SEND
static void keyboard_send(t_keyboard *x, t_symbol *s){
    if(s != gensym("")){
        t_symbol *snd = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
        if(snd != x->x_send){
            x->x_snd_raw = s;
            x->x_send = snd;
            x->x_snd_set = 1;
            if(x->x_edit && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
                if(x->x_send == &s_)
                    keyboard_draw_io_let(x);
                else
                    sys_vgui(".x%lx.c delete %lx_out\n", glist_getcanvas(x->x_glist), x);
            }
        }
    }
}

// RECEIVE
static void keyboard_receive(t_keyboard *x, t_symbol *s){
    if(s != gensym("")){
        t_symbol *rcv = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
        if(rcv != x->x_receive){
            if(x->x_receive != &s_)
                pd_unbind(&x->x_obj.ob_pd, x->x_receive);
            x->x_rcv_set = 1;
            x->x_rcv_raw = s;
            x->x_receive = rcv;
            if(x->x_receive == &s_){
                if(x->x_edit && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
                    keyboard_draw_io_let(x);
            }
            else{
                pd_bind(&x->x_obj.ob_pd, x->x_receive);
                if(x->x_edit && glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
                    t_canvas *cv = glist_getcanvas(x->x_glist);
                    sys_vgui(".x%lx.c delete %lx_in1\n", cv, x);
                    sys_vgui(".x%lx.c delete %lx_in2\n", cv, x);
                }
            }
        }
    }
}

// FLUSH
static void keyboard_flush(t_keyboard* x){
    t_canvas *cv =  glist_getcanvas(x->x_glist);
    int ac = 2;
    t_atom at[2];
    for(int note = 0 ; note < 256 ; note++){
        if(x->x_tgl_notes[note] > 0){
            int i = note - x->x_first_c;
            if(i >= 0 && x->x_glist->gl_havewindow){
                short key = i % 12, c4 = (i == 60), black = (key == 1 || key == 3 || key == 6 || key == 8 || key == 10);
                sys_vgui(".x%lx.c itemconfigure %xrrk%d -fill %s\n", cv, x, i, black ? BLACK_OFF : c4 ? MIDDLE_C : WHITE_OFF);
            }
            SETFLOAT(at, note);
            SETFLOAT(at+1, x->x_tgl_notes[note] = 0);
            outlet_list(x->x_out, &s_list, ac, at);
            if(x->x_send != &s_ && x->x_send->s_thing)
                pd_list(x->x_send->s_thing, &s_list, ac, at);
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
            if(edit)
                keyboard_draw_io_let(p->p_cnv);
            else{
                t_canvas *cv = glist_getcanvas(p->p_cnv->x_glist);
                sys_vgui(".x%lx.c delete %lx_in1\n", cv, p->p_cnv);
                sys_vgui(".x%lx.c delete %lx_in2\n", cv, p->p_cnv);
                sys_vgui(".x%lx.c delete %lx_out\n", cv, p->p_cnv);
            }
        }
    }
}

static void keyboard_zoom(t_keyboard *x, t_floatarg zoom){
    if(!glist_isvisible(x->x_glist) || !gobj_shouldvis((t_gobj *)x, x->x_glist))
        return;
    float mul = zoom == 1.0 ? 0.5 : 2.0;
    x->x_space = (int)((float)x->x_space * mul);
    x->x_width = (int)((float)x->x_width * mul);
    x->x_height = (int)((float)x->x_height * mul);
    x->x_zoom = (int)zoom;
    keyboard_erase(x, x->x_glist), keyboard_draw(x, x->x_glist);
}

/* ------------------------ GUI Behaviour -----------------------------*/
// Set Properties
static void keyboard_set_properties(t_keyboard *x, float space, float height, float oct,
        float low_c, float vel, float tgl){
    x->x_space = (space < 7) ? 7 : space; // key width
    x->x_height = (height < 10) ? 10 : height;
    x->x_octaves = oct < 1 ? 1 : oct > 10 ? 10 : oct;
    x->x_low_c = low_c < 0 ? 0 : low_c > 8 ? 8 : low_c;
    x->x_norm = vel < 0 ? 0 : vel > 127 ? 127 : vel;
    x->x_toggle_mode = (tgl != 0);
    x->x_width = ((int)(x->x_space)) * 7 * (int)x->x_octaves;
    x->x_first_c = ((int)(x->x_low_c * 12)) + 12;
    canvas_dirty(x->x_glist, 1);
}

// Keyboard Properties
void keyboard_properties(t_gobj *z, t_glist *owner){
    owner = NULL;
    t_keyboard *x = (t_keyboard *)z;
    keyboard_get_snd_rcv(x);
    char cmdbuf[256];
    sprintf(cmdbuf, "keyboard_properties %%s %d %d %d %d %d %d {%s} {%s}\n",
        (int)x->x_space,
        (int)x->x_height,
        (int)x->x_octaves,
        (int)x->x_low_c,
        (int)x->x_norm,
        (int)x->x_toggle_mode,
        x->x_snd_raw->s_name,
        x->x_rcv_raw->s_name);
    gfxstub_new(&x->x_obj.ob_pd, x, cmdbuf);
}

// Save Properties
static void keyboard_save(t_gobj *z, t_binbuf *b){
    t_keyboard *x = (t_keyboard *)z;
    keyboard_get_snd_rcv(x);
    binbuf_addv(b,
                "ssiisiiiiiiss",
                gensym("#X"),
                gensym("obj"),
                (int)x->x_obj.te_xpix,
                (int)x->x_obj.te_ypix,
                atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf)),
                (int)x->x_space / x->x_zoom,
                (int)x->x_height / x->x_zoom,
                (int)x->x_octaves,
                (int)x->x_low_c,
                (int)x->x_toggle_mode,
                (int)x->x_norm,
                x->x_snd_raw,
                x->x_rcv_raw);
    binbuf_addv(b, ";");
}

// Apply changes of property windows
static void keyboard_apply(t_keyboard *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    float w = atom_getfloatarg(0, ac, av);
    float h = atom_getfloatarg(1, ac, av);
    float oct = atom_getfloatarg(2, ac, av);
    float low_c = atom_getfloatarg(3, ac, av);
    float norm = atom_getfloatarg(4, ac, av);
    float tgl = atom_getfloatarg(5, ac, av);
    keyboard_send(x, atom_getsymbolarg(6, ac, av));
    keyboard_receive(x, atom_getsymbolarg(7, ac, av));
    keyboard_set_properties(x, w, h, oct, low_c, norm, tgl);
    keyboard_erase(x, x->x_glist), keyboard_draw(x, x->x_glist);
}
/* ------------------------ Free / New / Setup ------------------------------*/
// Free

static void edit_proxy_free(t_edit_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_edit_proxy * edit_proxy_new(t_keyboard *x, t_symbol *s){
    t_edit_proxy *p = (t_edit_proxy*)pd_new(edit_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)edit_proxy_free);
    return(p);
}

void keyboard_free(t_keyboard *x){
    if(x->x_receive != &s_)
        pd_unbind(&x->x_obj.ob_pd, x->x_receive);
    pd_unbind(&x->x_obj.ob_pd, x->x_bindsym);
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
    gfxstub_deleteforkey(x);
}

// New
void * keyboard_new(t_symbol *s, int ac, t_atom* av){
    s = NULL;
    t_keyboard *x = (t_keyboard *) pd_new(keyboard_class);
    t_canvas *cv = canvas_getcurrent();
    x->x_glist = (t_glist*)cv;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)cv);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = edit_proxy_new(x, gensym(buf));
    sprintf(buf, "#%lx", (long)x);
    pd_bind(&x->x_obj.ob_pd, x->x_bindsym = gensym(buf));
    x->x_edit = cv->gl_edit;
    x->x_zoom = x->x_glist->gl_zoom;
    x->x_last_note = -1;
    x->x_norm = x->x_velocity = 0;
    x->x_send = x->x_snd_raw = x->x_receive = x->x_rcv_raw = &s_;
    int tgl = 0;
    int vel = 0;
    float init_space = 17;
    float init_height = 80;
    float init_8ves = 4;
    float init_low_c = 3;
    t_symbol *snd = &s_;
    t_symbol *rcv = &s_;
    if(ac && av->a_type == A_FLOAT){ // 1st Width
        init_space = av->a_w.w_float;
        ac--; av++;
        if(ac && av->a_type == A_FLOAT){ // 2nd Height
            init_height = av->a_w.w_float;
            ac--, av++;
            if(ac && av->a_type == A_FLOAT){ // 3rd Octaves
                init_8ves = av->a_w.w_float;
                ac--; av++;
                if(ac && av->a_type == A_FLOAT){ // 4th Low C
                    init_low_c = av->a_w.w_float;
                    ac--; av++;
                    if(ac && av->a_type == A_FLOAT){ // 5th Toggle
                        tgl = (int)(av->a_w.w_float != 0);
                        ac--; av++;
                        if(ac && av->a_type == A_FLOAT){ // 6th Norm
                            vel = (int)(av->a_w.w_float);
                            ac--; av++;
                            if(ac && av->a_type == A_SYMBOL){ // 7th send
                                if(av->a_w.w_symbol == gensym("empty")) // ignore empty symbol
                                    ac--, av++;
                                else{
                                    snd = av->a_w.w_symbol;
                                    ac--, av++;
                                }
                                if(ac && av->a_type == A_SYMBOL){ // 8th receive
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
    t_symbol *cursym = NULL, *sym = NULL;
    while(ac > 0){
        if(av->a_type == A_SYMBOL){
            cursym = atom_getsymbolarg(0, ac, av);
            if(cursym == gensym("-width")){
                x->x_flag = 1;
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    init_space = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-height")){
                x->x_flag = 1;
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    init_height = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-oct")){
                x->x_flag = 1;
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    init_8ves = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            
            else if(cursym == gensym("-lowc")){
                x->x_flag = 1;
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    init_low_c = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-tgl"))
                x->x_flag = tgl = 1;
            else if(cursym == gensym("-norm")){
                x->x_flag = 1;
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    vel = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-send")){
                if(ac >= 2 && (av+1)->a_type == A_SYMBOL){
                    sym = atom_getsymbolarg(1, ac, av);
                    x->x_flag = x->x_s_flag = 1;
                    if(sym != gensym("empty"))
                        snd = sym;
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(cursym == gensym("-receive")){
                if(ac >= 2 && (av+1)->a_type == A_SYMBOL){
                    sym = atom_getsymbolarg(1, ac, av);
                    x->x_flag = x->x_r_flag = 1;
                    if(sym != gensym("empty"))
                        rcv = sym;
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else goto errstate;
        }
        else goto errstate;
    };
    x->x_snd_raw = snd;
    x->x_send = canvas_realizedollar(x->x_glist, x->x_snd_raw);
    x->x_rcv_raw = rcv;
    x->x_receive = canvas_realizedollar(x->x_glist, x->x_rcv_raw);
    if(x->x_receive != &s_)
        pd_bind(&x->x_obj.ob_pd, x->x_receive);
    x->x_space = (init_space < 7) ? 7 : init_space; // key width
    x->x_height = (init_height < 10) ? 10 : init_height;
    x->x_octaves = init_8ves < 1 ? 1 : init_8ves > 10 ? 10 : init_8ves;
    x->x_low_c = init_low_c < 0 ? 0 : init_low_c > 8 ? 8 : init_low_c;
    x->x_norm = vel < 0 ? 0 : vel > 127 ? 127 : vel;
    x->x_toggle_mode = (tgl != 0);
    x->x_width = ((int)(x->x_space)) * 7 * (int)x->x_octaves;
    x->x_first_c = ((int)(x->x_low_c * 12)) + 12;
    x->x_tgl_notes = getbytes(sizeof(int) * 256);
    for(int i = 0; i < 256; i++)
        x->x_tgl_notes[i] = 0;
    x->x_out = outlet_new(&x->x_obj, &s_list);
    floatinlet_new(&x->x_obj, &x->x_vel_in);
    return(void *)x;
    errstate:
        pd_error(x, "[keyboard]: improper args");
        return(NULL);
}

// Setup
void keyboard_setup(void){
    keyboard_class = class_new(gensym("keyboard"), (t_newmethod) keyboard_new, (t_method) keyboard_free,
        sizeof (t_keyboard), CLASS_DEFAULT, A_GIMME, 0);
    class_addfloat(keyboard_class, keyboard_float);
    class_addmethod(keyboard_class, (t_method)keyboard_8ves, gensym("8ves"), A_DEFFLOAT, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_width, gensym("width"), A_DEFFLOAT, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_height, gensym("height"), A_DEFFLOAT, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_oct, gensym("oct"), A_DEFFLOAT, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_low_c, gensym("lowc"), A_DEFFLOAT, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_toggle, gensym("toggle"), A_DEFFLOAT, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_norm, gensym("norm"), A_DEFFLOAT, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_flush, gensym("flush"), 0);
    class_addmethod(keyboard_class, (t_method)keyboard_send, gensym("send"), A_DEFSYMBOL, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_receive, gensym("receive"), A_DEFSYMBOL, 0);
    class_addmethod(keyboard_class, (t_method)keyboard_zoom, gensym("zoom"), A_CANT, 0);
    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(edit_proxy_class, edit_proxy_any);
// Methods to receive TCL/TK events
    class_addmethod(keyboard_class, (t_method)keyboard_mouserelease, gensym("_mouserelease"), 0);
    class_addmethod(keyboard_class, (t_method)keyboard_apply, gensym("apply"), A_GIMME, 0);
// GUI
    class_setwidget(keyboard_class, &keyboard_widgetbehavior);
    keyboard_widgetbehavior.w_getrectfn  = keyboard_getrect;
    keyboard_widgetbehavior.w_displacefn = keyboard_displace;
    keyboard_widgetbehavior.w_selectfn   = keyboard_select;
    keyboard_widgetbehavior.w_deletefn   = keyboard_delete;
    keyboard_widgetbehavior.w_visfn      = keyboard_vis;
    keyboard_widgetbehavior.w_clickfn    = (t_clickfn)keyboard_click;
    class_setsavefn(keyboard_class, keyboard_save);
    class_setpropertiesfn(keyboard_class, keyboard_properties);
    
    sys_vgui("if {[catch {pd}] } {\n");
    sys_vgui("    proc pd {args} {pdsend [join $args \" \"]}\n");
    sys_vgui("}\n");

    sys_vgui("proc keyboard_ok {id} {\n");
    sys_vgui("    keyboard_apply $id\n");
    sys_vgui("    keyboard_cancel $id\n");
    sys_vgui("}\n");
    
    sys_vgui("proc keyboard_cancel {id} {\n");
    sys_vgui("    set cmd [concat $id cancel \\;]\n");
    sys_vgui("    pd $cmd\n");
    sys_vgui("}\n");
    
    sys_vgui("proc keyboard_apply {id} {\n");
    sys_vgui("    set vid [string trimleft $id .]\n");
    sys_vgui("    set var_width [concat var_width_$vid]\n");
    sys_vgui("    set var_height [concat var_height_$vid]\n");
    sys_vgui("    set var_octaves [concat var_octaves_$vid]\n");
    sys_vgui("    set var_low_c [concat var_low_c_$vid]\n");
    sys_vgui("    set var_norm [concat var_norm_$vid]\n");
    sys_vgui("    set var_tgl [concat var_tgl_$vid]\n");
    sys_vgui("    set var_snd [concat var_snd_$vid]\n");
    sys_vgui("    set var_rcv [concat var_rcv_$vid]\n");
    sys_vgui("\n");
    sys_vgui("    global $var_width\n");
    sys_vgui("    global $var_height\n");
    sys_vgui("    global $var_octaves\n");
    sys_vgui("    global $var_low_c\n");
    sys_vgui("    global $var_norm\n");
    sys_vgui("    global $var_tgl\n");
    sys_vgui("    global $var_snd\n");
    sys_vgui("    global $var_rcv\n");
    sys_vgui("\n");
    sys_vgui("    set cmd [concat $id apply \\\n");
    sys_vgui("        [eval concat $$var_width] \\\n");
    sys_vgui("        [eval concat $$var_height] \\\n");
    sys_vgui("        [eval concat $$var_octaves] \\\n");
    sys_vgui("        [eval concat $$var_low_c] \\\n");
    sys_vgui("        [eval concat $$var_norm] \\\n");
    sys_vgui("        [eval concat $$var_tgl] \\\n");
    sys_vgui("        [string map {\"$\" {\\$} \" \" {\\ } \";\" \"\" \",\" \"\" \"\\\\\" \"\" \"\\{\" \"\" \"\\}\" \"\"} [eval concat $$var_snd]] \\\n");
    sys_vgui("        [string map {\"$\" {\\$} \" \" {\\ } \";\" \"\" \",\" \"\" \"\\\\\" \"\" \"\\{\" \"\" \"\\}\" \"\"} [eval concat $$var_rcv]] \\;]\n");
    sys_vgui("    pd $cmd\n");
    sys_vgui("}\n");
    sys_vgui("proc keyboard_properties {id width height octaves low_c norm tgl snd rcv} {\n");
    sys_vgui("    set vid [string trimleft $id .]\n");
    sys_vgui("    set var_width [concat var_width_$vid]\n");
    sys_vgui("    set var_height [concat var_height_$vid]\n");
    sys_vgui("    set var_octaves [concat var_octaves_$vid]\n");
    sys_vgui("    set var_low_c [concat var_low_c_$vid]\n");
    sys_vgui("    set var_norm [concat var_norm_$vid]\n");
    sys_vgui("    set var_tgl [concat var_tgl_$vid]\n");
    sys_vgui("    set var_snd [concat var_snd_$vid]\n");
    sys_vgui("    set var_rcv [concat var_rcv_$vid]\n");
    sys_vgui("\n");
    sys_vgui("    global $var_width\n");
    sys_vgui("    global $var_height\n");
    sys_vgui("    global $var_octaves\n");
    sys_vgui("    global $var_low_c\n");
    sys_vgui("    global $var_norm\n");
    sys_vgui("    global $var_tgl\n");
    sys_vgui("    global $var_snd\n");
    sys_vgui("    global $var_rcv\n");
    sys_vgui("\n");
    sys_vgui("    set $var_width $width\n");
    sys_vgui("    set $var_height $height\n");
    sys_vgui("    set $var_octaves $octaves\n");
    sys_vgui("    set $var_low_c $low_c\n");
    sys_vgui("    set $var_norm $norm\n");
    sys_vgui("    set $var_tgl $tgl\n");
    sys_vgui("    set $var_snd [string map {{\\ } \" \"} $snd]\n"); // remove escape from space
    sys_vgui("    set $var_rcv [string map {{\\ } \" \"} $rcv]\n"); // remove escape from space
    sys_vgui("\n");
    sys_vgui("    toplevel $id\n");
    sys_vgui("    wm title $id {Keyboard}\n");
    sys_vgui("    wm protocol $id WM_DELETE_WINDOW [concat keyboard_cancel $id]\n");
    sys_vgui("\n");
    sys_vgui("    label $id.label -text {Keyboard properties}\n");
    sys_vgui("    pack $id.label -side top\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.size\n");
    sys_vgui("    pack $id.size -side top\n");
    sys_vgui("    label $id.size.lwidth -text \"Key Width:\"\n");
    sys_vgui("    entry $id.size.width -textvariable $var_width -width 7\n");
    sys_vgui("    label $id.size.lheight -text \"Height:\"\n");
    sys_vgui("    entry $id.size.height -textvariable $var_height -width 7\n");
    sys_vgui("    pack $id.size.lwidth $id.size.width $id.size.lheight $id.size.height -side left\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.notes\n");
    sys_vgui("    pack $id.notes -side top\n");
    sys_vgui("    label $id.notes.loctaves -text \"Octaves:\"\n");
    sys_vgui("    entry $id.notes.octaves -textvariable $var_octaves -width 7\n");
    sys_vgui("    label $id.notes.llow_c -text \"Low C:\"\n");
    sys_vgui("    entry $id.notes.low_c -textvariable $var_low_c -width 7\n");
    sys_vgui("    pack $id.notes.loctaves $id.notes.octaves $id.notes.llow_c $id.notes.low_c -side left\n");
    sys_vgui("\n");
// new tgl mode
    sys_vgui("    frame $id.mode\n");
    sys_vgui("    pack $id.mode -side top\n");
    sys_vgui("    label $id.mode.lnorm -text \"Velocity Normalization:\"\n");
    sys_vgui("    entry $id.mode.norm -textvariable $var_norm -width 4\n");
    sys_vgui("    label $id.mode.ltgl -text \"Toggle Mode:\"\n");
    sys_vgui("    checkbutton $id.mode.tgl -variable $var_tgl \n");
    sys_vgui("    pack $id.mode.lnorm $id.mode.norm $id.mode.ltgl $id.mode.tgl -side left\n");
    sys_vgui("\n");
// send / receive
    sys_vgui("    frame $id.snd_rcv\n");
    sys_vgui("    pack $id.snd_rcv -side top\n");
    sys_vgui("    label $id.snd_rcv.lsnd -text \"Send symbol:\"\n");
    sys_vgui("    entry $id.snd_rcv.snd -textvariable $var_snd -width 12\n");
    sys_vgui("    label $id.snd_rcv.lrcv -text \"Receive symbol:\"\n");
    sys_vgui("    entry $id.snd_rcv.rcv -textvariable $var_rcv -width 12\n");
    sys_vgui("    pack $id.snd_rcv.lsnd $id.snd_rcv.snd $id.snd_rcv.lrcv $id.snd_rcv.rcv -side left\n");
    sys_vgui("\n");
    sys_vgui("    frame $id.buttonframe\n");
    sys_vgui("    pack $id.buttonframe -side bottom -fill x -pady 2m\n");
    sys_vgui("    button $id.buttonframe.cancel -text {Cancel} -command \"keyboard_cancel $id\"\n");
    sys_vgui("    button $id.buttonframe.apply -text {Apply} -command \"keyboard_apply $id\"\n");
    sys_vgui("    button $id.buttonframe.ok -text {OK} -command \"keyboard_ok $id\"\n");
    sys_vgui("    pack $id.buttonframe.cancel -side left -expand 1\n");
    sys_vgui("    pack $id.buttonframe.apply -side left -expand 1\n");
    sys_vgui("    pack $id.buttonframe.ok -side left -expand 1\n");
    sys_vgui("\n");
    sys_vgui("    focus $id.size.width\n");
    sys_vgui("}\n");
}
