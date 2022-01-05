// porres

#define MAX_SIZE 1024

#include <m_pd.h>
#include <g_canvas.h>
#include <math.h>

static t_class *function_class, *edit_proxy_class;
static t_widgetbehavior function_widgetbehavior;

typedef struct _edit_proxy{
    t_object    p_obj;
    t_symbol   *p_sym;
    t_clock    *p_clock;
    struct      _function *p_cnv;
}t_edit_proxy;

typedef struct _function{
    t_object        x_obj;
    t_glist        *x_glist;
    t_edit_proxy   *x_proxy;
    int             x_state;
    int             x_n_states;
    int             x_flag;
    int             x_s_flag;
    int             x_r_flag;
    int             x_sel;
    int             x_width;
    int             x_height;
    int             x_init;
    int             x_numdots;
    int             x_grabbed;      // for moving points
    int             x_shift;       
    int             x_snd_set;
    int             x_rcv_set;
    int             x_zoom;
    int             x_edit;
    t_symbol       *x_send;
    t_symbol       *x_receive;
    t_symbol       *x_snd_raw;
    t_symbol       *x_rcv_raw;
    float          *x_points;
    float          *x_dur;
    float           x_total_duration;
    float           x_min;
    float           x_max;
    float           x_min_point;
    float           x_max_point;
    float           x_pointer_x;
    float           x_pointer_y;
    unsigned char   x_fgcolor[3];
    unsigned char   x_bgcolor[3];
}t_function;

static void function_bang(t_function *x);

// DRAW FUNCTIONS //////////////////////////////////////////////////////////////////////////////
static void function_draw_in_outlet(t_function *x){
    int xpos = text_xpix(&x->x_obj, x->x_glist), ypos = text_ypix(&x->x_obj, x->x_glist);
    t_canvas *cv =  glist_getcanvas(x->x_glist);
    if(x->x_edit && x->x_receive == &s_) // Intlet
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags %lx_in\n",
            cv, xpos, ypos, xpos+IOWIDTH*x->x_zoom, ypos+IHEIGHT*x->x_zoom, x);
    if(x->x_edit && x->x_send == &s_) // Outlet
        sys_vgui(".x%lx.c create rectangle %d %d %d %d -fill black -tags %lx_out\n",
            cv, xpos, ypos+x->x_height, xpos+IOWIDTH*x->x_zoom, ypos+x->x_height-IHEIGHT*x->x_zoom, x);
}

static void function_draw_dots(t_function *x, t_glist *glist){
    float range = x->x_max - x->x_min;
    float min =  x->x_min;
    int yvalue;
    float xscale = x->x_width/x->x_dur[x->x_n_states];
    float yscale = x->x_height;
    int i = 0, xpos = text_xpix(&x->x_obj, glist), ypos = (int)(text_ypix(&x->x_obj, glist) + x->x_height);
    char fgcolor[20];
    sprintf(fgcolor, "#%2.2x%2.2x%2.2x", x->x_fgcolor[0], x->x_fgcolor[1], x->x_fgcolor[2]);
    char bgcolor[20];
    sprintf(bgcolor, "#%2.2x%2.2x%2.2x", x->x_bgcolor[0], x->x_bgcolor[1], x->x_bgcolor[2]);
    t_canvas *cv = glist_getcanvas(glist);
    for(i = 0; i <= x->x_n_states; i++){
        yvalue = (int)((x->x_points[i] - min) / range * yscale);
        sys_vgui(".x%lx.c create oval %d %d %d %d -width %d -tags %lx_dots%d -outline %s -fill %s\n",
            cv,
            xpos + (int)(x->x_dur[i]*xscale) - 3*x->x_zoom,
            ypos - yvalue - 3*x->x_zoom,
            xpos + (int)(x->x_dur[i]*xscale) + 3*x->x_zoom,
            ypos - yvalue + 3*x->x_zoom,
            2 * x->x_zoom,
            x,
            i,
            fgcolor,
            bgcolor);
    }
    x->x_numdots = i;
}

static void function_draw(t_function *x, t_glist *glist){
    float range = x->x_max - x->x_min;
    float min =  x->x_min;
    t_canvas *cv = glist_getcanvas(x->x_glist);
    int xpos = text_xpix(&x->x_obj, glist);
    int ypos = (int)text_ypix(&x->x_obj, glist);
    char bgcolor[20];
    sprintf(bgcolor, "#%2.2x%2.2x%2.2x", x->x_bgcolor[0], x->x_bgcolor[1], x->x_bgcolor[2]);
    char fgcolor[20];
    sprintf(fgcolor, "#%2.2x%2.2x%2.2x", x->x_fgcolor[0], x->x_fgcolor[1], x->x_fgcolor[2]);
    float xscale = x->x_width / x->x_dur[x->x_n_states];
    float yscale = x->x_height;
    sys_vgui(".x%lx.c create rectangle %d %d %d %d -width %d -outline black -tags %lx_rect -fill %s\n", cv, // RECTANGLE
        xpos, ypos, xpos+x->x_width, ypos+x->x_height, x->x_zoom, x, bgcolor);
    sys_vgui(".x%lx.c create line", cv); // LINES
    for(int i = 0; i <= x->x_n_states; i++)
        sys_vgui(" %d %d ", (int)(xpos + x->x_dur[i]*xscale),
                 (int)(ypos + x->x_height - (x->x_points[i]-min) / range*yscale));
    sys_vgui(" -tags %lx_line -fill %s -width %d\n", x, fgcolor, 2*x->x_zoom);
    function_draw_dots(x, glist);
    function_draw_in_outlet(x);
    if(x->x_sel)
        sys_vgui(".x%lx.c itemconfigure %lx_rect -outline blue\n", cv, x);
    else
        sys_vgui(".x%lx.c itemconfigure %lx_rect -outline black\n", cv, x);
}

static void function_delete_dots(t_function *x, t_glist *glist){
    for(int i = 0; i <= x->x_numdots; i++)
        sys_vgui(".x%lx.c delete %lx_dots%d\n", glist_getcanvas(glist), x, i);
}

static void function_erase(t_function* x, t_glist* glist){
    t_canvas *cv = glist_getcanvas(glist);
    sys_vgui(".x%lx.c delete %lx_in\n", cv, x); // delete inlet
    sys_vgui(".x%lx.c delete %lx_out\n", cv, x); // delete outlet
    sys_vgui(".x%lx.c delete %lx_rect\n", cv, x); // delete rectangle
    sys_vgui(".x%lx.c delete %lx_line\n", cv, x); // delete lines
    function_delete_dots(x, glist);
}

static void function_update(t_function *x, t_glist *glist){
    int xpos = text_xpix(&x->x_obj, glist);
    int ypos = text_ypix(&x->x_obj, glist);
    float range = x->x_max - x->x_min;
    float min =  x->x_min;
    t_canvas *cv = glist_getcanvas(glist);
    sys_vgui(".x%lx.c coords %lx_rect %d %d %d %d\n", cv, x, xpos, ypos,
        xpos + x->x_width, ypos + x->x_height);
    float xscale = x->x_width / x->x_dur[x->x_n_states];
    float yscale = x->x_height;
    sys_vgui(".x%lx.c coords %lx_line", cv, x);
    for(int i = 0; i <= x->x_n_states; i++)
        sys_vgui(" %d %d ",(int)(xpos + x->x_dur[i]*xscale),
                 (int)(ypos + x->x_height - (x->x_points[i] - min) / range*yscale));
    sys_vgui("\n");
    function_delete_dots(x, glist);
    function_draw_dots(x, glist);
    function_draw_in_outlet(x);
}

// ------------------------ widgetbehaviour----------------------------- 
static void function_getrect(t_gobj *z, t_glist *glist, int *xp1, int *yp1, int *xp2, int *yp2){
    t_function* x = (t_function*)z;
    int xpos = text_xpix(&x->x_obj, glist), ypos = text_ypix(&x->x_obj, glist);
    *xp1 = xpos;
    *yp1 = ypos;
    *xp2 = xpos + x->x_width;
    *yp2 = ypos + x->x_height;
}

static void function_displace(t_gobj *z, t_glist *glist, int dx, int dy){
    t_function *x = (t_function *)z;
    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    t_canvas *cv = glist_getcanvas(x->x_glist);
    sys_vgui(".x%lx.c move %lx_rect %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    sys_vgui(".x%lx.c move %lx_line %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    sys_vgui(".x%lx.c move %lx_in %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    sys_vgui(".x%lx.c move %lx_out %d %d\n", cv, x, dx * x->x_zoom, dy * x->x_zoom);
    for(int i = 0; i <= x->x_n_states; i++)
        sys_vgui(".x%lx.c move %lx_dots%d %d %d\n", cv, x, i, dx * x->x_zoom, dy * x->x_zoom);
    canvas_fixlinesfor(glist, (t_text*)x);
}

static void function_select(t_gobj *z, t_glist *glist, int selected){
    t_function *x = (t_function *)z;
    t_canvas *cv = glist_getcanvas(glist);
    if((x->x_sel = selected))
        sys_vgui(".x%lx.c itemconfigure %lx_rect -outline blue\n", cv, x);
    else
        sys_vgui(".x%lx.c itemconfigure %lx_rect -outline black\n", cv, x);
}

static void function_delete(t_gobj *z, t_glist *glist){
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void function_vis(t_gobj *z, t_glist *glist, int vis){
    vis ? function_draw((t_function*)z, glist) : function_erase((t_function*)z, glist);
}

static void function_followpointer(t_function* x,t_glist* glist){
    float dur;
    float xscale = x->x_dur[x->x_n_states] / x->x_width;
    float range = x->x_max - x->x_min;
    float min =  x->x_min;
    if((x->x_grabbed > 0) && (x->x_grabbed < x->x_n_states)){
        dur = (x->x_pointer_x - text_xpix(&x->x_obj,glist))*xscale;
        if(dur < x->x_dur[x->x_grabbed-1])
            dur = x->x_dur[x->x_grabbed-1];
        if(dur >x->x_dur[x->x_grabbed+1])
            dur = x->x_dur[x->x_grabbed+1];
        x->x_dur[x->x_grabbed] = dur;
    }
    float grabbed = 1.0f - (x->x_pointer_y - (float)text_ypix(&x->x_obj, glist)) / (float)x->x_height;
    grabbed = grabbed < 0.0 ? 0.0 : grabbed > 1.0 ? 1.0 : grabbed;
    x->x_points[x->x_grabbed] = grabbed * range + min;
    function_bang(x);
}

static void function_motion(t_function *x, t_floatarg dx, t_floatarg dy){
    if(x->x_shift)
        x->x_pointer_x += dx * 0.1, x->x_pointer_y += dy * 0.1;
    else
        x->x_pointer_x += dx, x->x_pointer_y += dy;
    function_followpointer(x, x->x_glist);
    function_update(x, x->x_glist);
}

static void function_key(t_function *x, t_floatarg f){
    if(f == 8.0 && x->x_grabbed > 0 && x->x_grabbed < x->x_n_states){
        for(int i = x->x_grabbed; i <= x->x_n_states; i++){
            x->x_dur[i] = x->x_dur[i+1];
            x->x_points[i] = x->x_points[i+1];
        }
        x->x_n_states--;
        x->x_grabbed--;
        function_update(x, x->x_glist);
        function_bang(x);
    }
}

static void function_next_dot(t_function *x, t_glist *glist, int dot_x,int dot_y){
    int i, insertpos = -1;
    float tval, minval = 100000000000000000000.0; // stupidly high number
    float range = x->x_max - x->x_min;
    float min =  x->x_min;
    float xpos = (float)text_xpix(&x->x_obj, glist), ypos = (float)text_ypix(&x->x_obj, glist);
    if(dot_x > xpos + x->x_width)
        dot_x = xpos + x->x_width;
    float xscale = x->x_width / x->x_dur[x->x_n_states];
    float yscale = x->x_height;
    for(i = 0; i <= x->x_n_states; i++){
        float dx2 = (xpos + (x->x_dur[i] * xscale)) - dot_x;
        float dy2 = (ypos + yscale - ((x->x_points[i] - min) / range * yscale)) - dot_y;
        tval = sqrt(dx2*dx2 + dy2*dy2);
        if(tval <= minval){
            minval = tval;
            insertpos = i;
        }
    }
    if(minval > 8 && insertpos >= 0){ // decide if we want to make a new one
        while(((xpos + (x->x_dur[insertpos] * xscale)) - dot_x) < 0)
            insertpos++;
        while(((xpos + (x->x_dur[insertpos-1] * xscale)) - dot_x) > 0)
            insertpos--;
        for(i = x->x_n_states; i >= insertpos; i--){
            x->x_dur[i+1] = x->x_dur[i];
            x->x_points[i+1] = x->x_points[i];
        }
        x->x_dur[insertpos] = (float)(dot_x-xpos) / x->x_width * x->x_dur[x->x_n_states++];
        x->x_pointer_x = dot_x;
        x->x_pointer_y = dot_y;
    }
    else{
        x->x_pointer_x = xpos + x->x_dur[insertpos] * x->x_width / x->x_dur[x->x_n_states];
        x->x_pointer_y = dot_y;
    }
    x->x_grabbed = insertpos;
}

static int function_click(t_function *x, t_glist *gl, int xpos, int ypos, int shift, int alt, int dbl, int doit){
    alt = dbl = 0; // remove warning
    if(doit){
        if(x->x_n_states + 1 > MAX_SIZE){
            pd_error(x, "[function]: too many lines, maximum is %d", MAX_SIZE);
            return(0);
        }
        function_next_dot(x, gl, xpos, ypos);
        glist_grab(x->x_glist, &x->x_obj.te_g, (t_glistmotionfn)function_motion, (t_glistkeyfn)function_key, xpos, ypos);
        x->x_shift = shift;
        function_followpointer(x, gl);
        function_update(x, gl);
    }
    return(1);
}

static void function_save(t_gobj *z, t_binbuf *b){
    t_function *x = (t_function *)z;
    t_binbuf *bb = x->x_obj.te_binbuf;
    int i, n_args = binbuf_getnatom(bb); // number of arguments
    char buf[80];
    if(!x->x_snd_set){ // no send set, search arguments
        if(n_args > 0){ // we have arguments, let's search them
            if(x->x_flag){ // we got flags
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
                int arg_n = 3; // receive argument number
                if(n_args >= arg_n){ // we have it, get it
                    atom_string(binbuf_getvec(bb) + arg_n, buf, 80);
                    x->x_snd_raw = gensym(buf);
                }
            }
        }
    }
    if(x->x_snd_raw == &s_) // if no symbol, set to "empty"
        x->x_snd_raw = gensym("empty");
    if(!x->x_rcv_set){ // no receive set, search arguments
        if(n_args > 0){ // we have arguments, let's search them
            if(x->x_flag){ // search for receive name in flags
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
                int arg_n = 4; // receive argument number
                if(n_args >= arg_n){ // we have it, get it
                    atom_string(binbuf_getvec(bb) + arg_n, buf, 80);
                    x->x_rcv_raw = gensym(buf);
                }
            }
        }
    }
    if(x->x_rcv_raw == &s_) // if no symbol, set to "empty"
        x->x_rcv_raw = gensym("empty");
    binbuf_addv(b,
                "ssiis",
                gensym("#X"),
                gensym("obj"),
                (int)x->x_obj.te_xpix,
                (int)x->x_obj.te_ypix,
                atom_getsymbol(binbuf_getvec(x->x_obj.te_binbuf))
                );
    binbuf_addv(b, "iissffiiiiiiiiii",
                x->x_width / x->x_zoom,
                x->x_height / x->x_zoom,
                x->x_snd_raw,
                x->x_rcv_raw,
                x->x_min,
                x->x_max,
                (int)x->x_bgcolor[0],
                (int)x->x_bgcolor[1],
                (int)x->x_bgcolor[2],
                (int)x->x_fgcolor[0],
                (int)x->x_fgcolor[1],
                (int)x->x_fgcolor[2],
                x->x_init,
                0, // placeholder
                0, // placeholder
                0  // placeholder
                );
    binbuf_addv(b, "f",
                (float)x->x_points[x->x_state = 0]
                );
                i = 1;
                int ac = x->x_n_states * 2 + 1;
                while(i < ac){
                    float dur = x->x_dur[x->x_state+1] - x->x_dur[x->x_state];
                    binbuf_addv(b, "f",
                                (float)dur
                                );
                    i++;
                    x->x_state++;
                    binbuf_addv(b, "f",
                                (float)x->x_points[x->x_state]
                                );
                    i++;
                }
    binbuf_addv(b, ";");
}

///////////////////// METHODS /////////////////////////////////////////////////////////////
static void function_bang(t_function *x){
    int ac = x->x_n_states * 2 + 1;
    t_atom* at = (t_atom *)getbytes(ac*sizeof(t_atom));
    SETFLOAT(at, x->x_min_point = x->x_max_point = x->x_points[x->x_state = 0]); // get 1st
    for(int i = 1; i < ac; i++){ // get the rest
        float dur = x->x_dur[x->x_state+1] - x->x_dur[x->x_state];
        SETFLOAT(at+i, dur); // duration
        i++, x->x_state++;
        float point = x->x_points[x->x_state];
        if(point < x->x_min_point)
            x->x_min_point = point;
        if(point > x->x_max_point)
            x->x_max_point = point;
        SETFLOAT(at+i, point);
    }
    outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
    if(x->x_send != &s_ && x->x_send->s_thing)
        pd_list(x->x_send->s_thing, &s_list, ac, at);
    freebytes(at, ac*sizeof(t_atom));
}

static void function_duration(t_function* x, t_floatarg dur){
    if(dur < 1){
        post("function: minimum duration is 1 ms");
        return;
    }
    if(dur != x->x_total_duration){
        x->x_total_duration = dur;
        float f = dur/x->x_dur[x->x_n_states];
        for(int i = 1; i <= x->x_n_states; i++)
            x->x_dur[i] *= f;
    }
}

static void function_float(t_function *x, t_floatarg f){
    float val;
    if(f <= 0){
        val = x->x_points[0];
        outlet_float(x->x_obj.ob_outlet, val);
        if(x->x_send != &s_)
            pd_float(x->x_send->s_thing, f);
        return;
    }
    if(f >= 1){
        val = x->x_points[x->x_n_states];
        outlet_float(x->x_obj.ob_outlet, val);
        if(x->x_send != &s_)
            pd_float(x->x_send->s_thing, f);
        return;
    }
    f *= x->x_dur[x->x_n_states];
    if(x->x_state > x->x_n_states)
        x->x_state = x->x_n_states;
    while((x->x_state > 0) && (f < x->x_dur[x->x_state-1]))
        x->x_state--;
    while((x->x_state <  x->x_n_states) && (x->x_dur[x->x_state] < f))
        x->x_state++;
    val = x->x_points[x->x_state-1] + (f-x->x_dur[x->x_state-1]) * (x->x_points[x->x_state] - x->x_points[x->x_state-1])
        / (x->x_dur[x->x_state] - x->x_dur[x->x_state-1]);
    outlet_float(x->x_obj.ob_outlet,val);
    if(x->x_send != &s_)
        pd_float(x->x_send->s_thing, val);
}

static float function_interpolate(t_function* x, float f){
    float point = x->x_points[x->x_state];
    float point_m1 = x->x_points[x->x_state-1];
    float dur = x->x_dur[x->x_state];
    float dur_m1 = x->x_dur[x->x_state-1];
    return(point_m1 + (f-dur_m1) * (point-point_m1)/(dur-dur_m1));
}

static void function_i(t_function *x, t_floatarg f){
    float val;
    if(f <= 0){
        val = x->x_points[0];
        outlet_float(x->x_obj.ob_outlet, val);
        if(x->x_send != &s_)
            pd_float(x->x_send->s_thing, f);
        return;
    }
    if(f >= x->x_dur[x->x_n_states]){
        val = x->x_points[x->x_n_states];
        outlet_float(x->x_obj.ob_outlet, val);
        if(x->x_send != &s_)
            pd_float(x->x_send->s_thing, f);
        return;
    }
    if(x->x_state > x->x_n_states)
        x->x_state = x->x_n_states;
    while((x->x_state > 0) && (f < x->x_dur[x->x_state-1]))
        x->x_state--;
    while((x->x_state <  x->x_n_states) && (x->x_dur[x->x_state] < f))
        x->x_state++;
    val = function_interpolate(x, f);
    outlet_float(x->x_obj.ob_outlet,val);
    if(x->x_send != &s_)
        pd_float(x->x_send->s_thing, val);
}

static void function_set_beeakpoints(t_function *x, int ac, t_atom* av){
    float tdur = 0;
    x->x_dur[0] = 0;
    x->x_n_states = ac >> 1;
    float* dur = x->x_dur;
    float* val = x->x_points;
    // get 1st value
    *val = atom_getfloat(av++);
    x->x_max_point = x->x_min_point = *val;
    *dur = 0.0;
    dur++;
    ac--;
    // get others
    while(ac--){
        tdur += atom_getfloat(av++);
        *dur++ = tdur;
        if(ac--){
            *++val = atom_getfloat(av++);
            if(*val > x->x_max_point)
                x->x_max_point = *val;
            if(*val < x->x_min_point)
                x->x_min_point = *val;
        }
        else{
            *++val = 0;
            if(*val > x->x_max_point)
                x->x_max_point = *val;
            if(*val < x->x_min_point)
                x->x_min_point = *val;
        }
    }
    if(x->x_min_point < x->x_min)
        x->x_min = x->x_min_point;
    if(x->x_max_point > x->x_max)
        x->x_max = x->x_max_point;
}

static void function_set(t_function *x, t_symbol* s, int ac,t_atom *av){
    if((ac >> 1) > MAX_SIZE){
        pd_error(x, "[function]: too many lines, maximum is %d", MAX_SIZE);
        return;
    }
    s = NULL;
    if(ac > 2 && ac % 2){
        function_set_beeakpoints(x, ac, av);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            function_update(x, x->x_glist);
    }
    else if(ac == 2){ // set a single point value
        int i = (int)av->a_w.w_float;
        av++;
        if(i < 0)
            i = 0;
        if(i > x->x_n_states)
            i = x->x_n_states;
        float v = av->a_w.w_float;
        x->x_points[i] = v;
        if(v < x->x_min_point)
            x->x_min_point = x->x_min = v;
        if(v > x->x_max_point)
            x->x_max_point = x->x_max = v;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            function_update(x, x->x_glist);
    }
    else
        post("[function] wrong format for 'set' message");
}

static void function_list(t_function *x, t_symbol* s, int ac,t_atom *av){
    if((ac >> 1) > MAX_SIZE){
        pd_error(x, "[function]: too many lines, maximum is %d", MAX_SIZE);
        return;
    }
    s = NULL;
    if(ac > 2 && ac % 2){
        function_set_beeakpoints(x, ac, av);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            function_update(x, x->x_glist);
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, av);
        if(x->x_send != &s_ && x->x_send->s_thing)
            pd_list(x->x_send->s_thing, &s_list, ac, av);
    }
    else if(ac == 2){ // set a single point value
        int i = (int)av->a_w.w_float;
        av++;
        if(i < 0)
            i = 0;
        if(i > x->x_n_states)
            i = x->x_n_states;
        float v = x->x_points[i] = av->a_w.w_float;
        if(v < x->x_min_point)
            x->x_min_point = x->x_min = v;
        if(v > x->x_max_point)
            x->x_max_point = x->x_max = v;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            function_update(x, x->x_glist);
        function_bang(x);
    }
    else
        post("[function] wrong format for 'list' message");
}

static void function_min(t_function *x, t_floatarg f){
    if(f <= x->x_min_point){
        if(x->x_min != f){
            x->x_min = f;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            function_update(x, x->x_glist);
        }
    }
}

static void function_max(t_function *x, t_floatarg f){
    if(f >= x->x_max_point){
        if(x->x_max != f){
            x->x_max = f;
            if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
                function_update(x, x->x_glist);
        }
    }
}

static void function_resize(t_function *x){
    if(x->x_max != x->x_max_point || x->x_min != x->x_min_point){
        x->x_max = x->x_max_point;
        x->x_min = x->x_min_point;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            function_update(x, x->x_glist);
    }
}

static void function_init(t_function *x, t_floatarg f){
    int init = (int)(f != 0);
    if(x->x_init != init)
        x->x_init = init;
}

static void function_height(t_function *x, t_floatarg f){
    int height = f < 20 ? 20 : (int)f;
    if(x->x_height != height){
        x->x_height = height * x->x_zoom;
        function_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            function_draw(x, x->x_glist);
            canvas_fixlinesfor(x->x_glist, (t_text*) x);
        }
    }
}

static void function_width(t_function *x, t_floatarg f){
    int width = f < 40 ? 40 : (int)f;
    if(x->x_width != width){
        x->x_width = width * x->x_zoom;
        function_erase(x, x->x_glist);
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
            function_draw(x, x->x_glist);
    }
}

static void function_send(t_function *x, t_symbol *s){
    t_symbol *snd = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
    if(x->x_send != snd){
        x->x_snd_raw = s;
        x->x_send = snd;
        x->x_snd_set = 1;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            if(x->x_send == &s_)
                function_draw_in_outlet(x);
            else
                sys_vgui(".x%lx.c delete %lx_out\n", glist_getcanvas(x->x_glist), x);
        }
            
    }
}

static void function_receive(t_function *x, t_symbol *s){
    t_symbol *rcv = s == gensym("empty") ? &s_ : canvas_realizedollar(x->x_glist, s);
    if(rcv != x->x_receive){
        x->x_rcv_raw = s;
        x->x_rcv_set = 1;
        if(x->x_receive != &s_)
            pd_unbind(&x->x_obj.ob_pd, x->x_receive);
        if(rcv != &s_)
            pd_bind(&x->x_obj.ob_pd, rcv);
        x->x_receive = rcv;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            if(x->x_receive == &s_)
                function_draw_in_outlet(x);
            else
                sys_vgui(".x%lx.c delete %lx_in\n", glist_getcanvas(x->x_glist), x);
        }
    }
}

static void function_fgcolor(t_function *x, t_floatarg red, t_floatarg green, t_floatarg blue){
    int r = red < 0 ? 0 : red > 255 ? 255 : (int)red;
    int g = green < 0 ? 0 : green > 255 ? 255 : (int)green;
    int b = blue < 0 ? 0 : blue > 255 ? 255 : (int)blue;
    if(x->x_fgcolor[0] != r || x->x_fgcolor[1] != g || x->x_fgcolor[2] != b){
        x->x_fgcolor[0] = r;
        x->x_fgcolor[1] = g;
        x->x_fgcolor[2] = b;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            t_canvas *cv = glist_getcanvas(x->x_glist);
            sys_vgui(".x%lx.c itemconfigure %lx_line -fill #%2.2x%2.2x%2.2x\n", cv,
                x, x->x_fgcolor[0] = r, x->x_fgcolor[1] = g, x->x_fgcolor[2] = b);
            for(int i = 0; i <= x->x_n_states; i++)
                sys_vgui(".x%lx.c itemconfigure %lx_dots%d -outline #%2.2x%2.2x%2.2x\n", cv,
                         x, i, x->x_fgcolor[0], x->x_fgcolor[1], x->x_fgcolor[2]);
        }
    }
}

static void function_bgcolor(t_function *x, t_floatarg red, t_floatarg green, t_floatarg blue){
    int r = red < 0 ? 0 : red > 255 ? 255 : (int)red;
    int g = green < 0 ? 0 : green > 255 ? 255 : (int)green;
    int b = blue < 0 ? 0 : blue > 255 ? 255 : (int)blue;
    if(x->x_bgcolor[0] != r || x->x_bgcolor[1] != g || x->x_bgcolor[2] != b){
        x->x_bgcolor[0] = r;
        x->x_bgcolor[1] = g;
        x->x_bgcolor[2] = b;
        if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist)){
            t_canvas *cv = glist_getcanvas(x->x_glist);
            sys_vgui(".x%lx.c itemconfigure %lx_rect -fill #%2.2x%2.2x%2.2x\n", cv,
                x, x->x_bgcolor[0] = r, x->x_bgcolor[1] = g, x->x_bgcolor[2] = b);
            for(int i = 0; i <= x->x_n_states; i++)
                sys_vgui(".x%lx.c itemconfigure %lx_dots%d -fill #%2.2x%2.2x%2.2x\n", cv,
                         x, i, x->x_bgcolor[0], x->x_bgcolor[1], x->x_bgcolor[2]);
        }
    }
}

static void function_loadbang(t_function *x, t_floatarg action){
    if(action == LB_LOAD && x->x_init)
        function_bang(x);
}

static void function_zoom(t_function *x, t_floatarg zoom){
    float mul = zoom == 1.0 ? 0.5 : 2.0;
    x->x_width = (int)((float)x->x_width * mul);
    x->x_height = (int)((float)x->x_height * mul);
    x->x_zoom = (int)zoom;
    if(glist_isvisible(x->x_glist) && gobj_shouldvis((t_gobj *)x, x->x_glist))
        function_update(x, x->x_glist);
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
            if(av->a_w.w_float == 0)
                edit = 1;
        }
        else
            return;
        if(p->p_cnv->x_edit != edit){
            p->p_cnv->x_edit = edit;
            if(edit)
                function_draw_in_outlet(p->p_cnv);
            else{
                t_canvas *cv = glist_getcanvas(p->p_cnv->x_glist);
                sys_vgui(".x%lx.c delete %lx_in\n", cv, p->p_cnv);
                sys_vgui(".x%lx.c delete %lx_out\n", cv, p->p_cnv);
            }
        }
    }
}

static void edit_proxy_free(t_edit_proxy *p){
    pd_unbind(&p->p_obj.ob_pd, p->p_sym);
    clock_free(p->p_clock);
    pd_free(&p->p_obj.ob_pd);
}

static t_edit_proxy * edit_proxy_new(t_function *x, t_symbol *s){
    t_edit_proxy *p = (t_edit_proxy*)pd_new(edit_proxy_class);
    p->p_cnv = x;
    pd_bind(&p->p_obj.ob_pd, p->p_sym = s);
    p->p_clock = clock_new(p, (t_method)edit_proxy_free);
    return(p);
}

static void function_free(t_function *x){
    if(x->x_receive != &s_)
         pd_unbind(&x->x_obj.ob_pd, x->x_receive);
    x->x_proxy->p_cnv = NULL;
    clock_delay(x->x_proxy->p_clock, 0);
    gfxstub_deleteforkey(x);
}

///////////////////// NEW / FREE / SETUP /////////////////////
static void *function_new(t_symbol *s, int ac, t_atom* av){
    t_function *x = (t_function *)pd_new(function_class);
    outlet_new(&x->x_obj, &s_anything);
    t_symbol *sym = s; // get rid of warning
    x->x_state = 0;
    x->x_grabbed = 0;
    t_canvas *cv = canvas_getcurrent();
    x->x_glist = (t_glist*)cv;
    char buf[MAXPDSTRING];
    snprintf(buf, MAXPDSTRING-1, ".x%lx", (unsigned long)cv);
    buf[MAXPDSTRING-1] = 0;
    x->x_proxy = edit_proxy_new(x, gensym(buf));
    x->x_edit = cv->gl_edit;
    x->x_zoom = x->x_glist->gl_zoom;
    x->x_points = getbytes((MAX_SIZE+1)*sizeof(float));
    x->x_dur = getbytes((MAX_SIZE+1)*sizeof(float));
    int envset = 0;
    float initialDuration = 0;
// Default Args
    x->x_width = 200;
    x->x_height = 100;
    x->x_rcv_set = x->x_snd_set = x->x_flag = x->x_s_flag = x->x_r_flag = 0;
    x->x_receive = x->x_rcv_raw = x->x_send = x->x_snd_raw = &s_;
    x->x_init = 0;
    x->x_min = 0;
    x->x_max = 1;
    x->x_bgcolor[0] = x->x_bgcolor[1] = x->x_bgcolor[2] = 220;
    x->x_fgcolor[0] = x->x_fgcolor[1] = x->x_fgcolor[2] = 50;
    t_atom a[3];
    SETFLOAT(a, 0);
    SETFLOAT(a+1, 1000);
    SETFLOAT(a+2, 0);
////////////////////////////////// GET ARGS ///////////////////////////////////////////
    if(ac && av->a_type == A_FLOAT){ // 1ST Width
        int w = (int)av->a_w.w_float;
        x->x_width = w < 40 ? 40 : w; // min width is 40
        ac--, av++;
        if(ac && av->a_type == A_FLOAT){ // 2ND Height
            int h = (int)av->a_w.w_float;
            x->x_height = h < 20 ? 20 : h; // min height is 20
            ac--, av++;
            if(ac && av->a_type == A_SYMBOL){ // 3RD Send
                if(av->a_w.w_symbol == gensym("empty")){ //  sets empty symbol
                    ac--, av++;
                }
                else{
                    x->x_send = av->a_w.w_symbol;
                    ac--, av++;
                }
                if(ac && av->a_type == A_SYMBOL){ // 4TH Receive
                    if(av->a_w.w_symbol == gensym("empty")){ //  sets empty symbol
                        ac--, av++;
                    }
                    else{
                        pd_bind(&x->x_obj.ob_pd, x->x_receive = av->a_w.w_symbol);
                        ac--, av++;
                    }
                    if(ac && av->a_type == A_FLOAT){ // 5TH Min
                        x->x_min = av->a_w.w_float;
                        ac--, av++;
                        if(ac && av->a_type == A_FLOAT){ // 6TH Max
                            x->x_max = av->a_w.w_float;
                            ac--, av++;
                            if(ac && av->a_type == A_FLOAT){ // BG Red
                                x->x_bgcolor[0] = (unsigned char)av->a_w.w_float;
                                ac--, av++;
                                if(ac && av->a_type == A_FLOAT){ // BG Green
                                    x->x_bgcolor[1] = (unsigned char)av->a_w.w_float;
                                    ac--, av++;
                                    if(ac && av->a_type == A_FLOAT){ // BG Blue
                                        x->x_bgcolor[2] = (unsigned char)av->a_w.w_float;
                                        ac--, av++;
                                        if(ac && av->a_type == A_FLOAT){ // FG Red
                                            x->x_fgcolor[0] = (unsigned char)av->a_w.w_float;
                                            ac--, av++;
                                            if(ac && av->a_type == A_FLOAT){ // FG Green
                                                x->x_fgcolor[1] = (unsigned char)av->a_w.w_float;
                                                ac--, av++;
                                                if(ac && av->a_type == A_FLOAT){ // FG Blue
                                                    x->x_fgcolor[2] = (unsigned char)av->a_w.w_float;
                                                    ac--, av++;
                                                    if(ac && av->a_type == A_FLOAT){ // Init
                                                        x->x_init = (int)(av->a_w.w_float != 0);
                                                        ac--, av++;
                                                        if(ac && av->a_type == A_FLOAT){ // placeholder
                                                            ac--, av++;
                                                            if(ac && av->a_type == A_FLOAT){ // placeholder
                                                                ac--, av++;
                                                                if(ac && av->a_type == A_FLOAT){ // placeholder
                                                                    ac--, av++;
                                                                    if(ac && av->a_type == A_FLOAT){ // Set Env
                                                                        int i = 0;
                                                                        int j = ac;
                                                                        while(j && (av+i)->a_type == A_FLOAT){
                                                                            i++; j--;
                                                                        }
                                                                        if(i % 2 == 0)
                                                                            pd_error(x, "[function]: needs an odd list of floats");
                                                                        else if(i >> 1 > MAX_SIZE){
                                                                            pd_error(x, "[function]: too many lines, maximum is %d", MAX_SIZE);
                                                                            goto errstate;
                                                                        }
                                                                        else{
                                                                            envset = 1;
                                                                            function_set_beeakpoints(x, i, av);
                                                                        }
                                                                        av+=i, ac-=i;
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
    while(ac > 0){
        if(av->a_type == A_SYMBOL){
            sym = atom_getsymbolarg(0, ac, av);
            if(sym == gensym("-duration")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_flag = 1;
                    float curfloat = atom_getfloatarg(1, ac, av);
                    initialDuration = curfloat < 0 ? 0 : curfloat;
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-init")){
                x->x_flag = 1;
                x->x_init = 1;
                ac--, av++;
            }
            else if(sym == gensym("-width")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_flag = 1;
                    float curfloat = atom_getfloatarg(1, ac, av);
                    x->x_width = curfloat < 40 ? 40 : curfloat; // min width is 40
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-height")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_flag = 1;
                    float curfloat = atom_getfloatarg(1, ac, av);
                    x->x_height = curfloat < 20 ? 20 : curfloat; // min width is 20
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-send")){
                if(ac >= 2 && (av+1)->a_type == A_SYMBOL){
                    x->x_flag = x->x_s_flag = 1;
                    x->x_send = atom_getsymbolarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-receive")){
                if(ac >= 2 && (av+1)->a_type == A_SYMBOL){
                    x->x_flag = x->x_r_flag = 1;
                    pd_bind(&x->x_obj.ob_pd, x->x_receive = atom_getsymbolarg(1, ac, av));
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-min")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_flag = 1;
                    x->x_min = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-max")){
                if(ac >= 2 && (av+1)->a_type == A_FLOAT){
                    x->x_flag = 1;
                    x->x_max = atom_getfloatarg(1, ac, av);
                    ac-=2, av+=2;
                }
                else goto errstate;
            }
            else if(sym == gensym("-bgcolor")){
                if(ac >= 4 && (av+1)->a_type == A_FLOAT
                   && (av+2)->a_type == A_FLOAT
                   && (av+3)->a_type == A_FLOAT){
                    x->x_flag = 1;
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
            else if(sym == gensym("-fgcolor")){
                if(ac >= 4 && (av+1)->a_type == A_FLOAT
                   && (av+2)->a_type == A_FLOAT
                   && (av+3)->a_type == A_FLOAT){
                    x->x_flag = 1;
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
            else if(sym == gensym("-set")){
                if(ac >= 4 && (av+1)->a_type == A_FLOAT
                   && (av+2)->a_type == A_FLOAT
                   && (av+3)->a_type == A_FLOAT){
                    x->x_flag = 1;
                    ac--, av++;
                    int i = 0, j = ac;
                    while(j && (av+i)->a_type == A_FLOAT){
                        i++;
                        j--;
                    }
                    if(i % 2 == 0)
                        pd_error(x, "[function]: needs an odd list of floats");
                    else if(i >> 1 > MAX_SIZE){
                        pd_error(x, "[function]: too many lines, maximum is %d", MAX_SIZE);
                        goto errstate;
                    }
                    else{
                        envset = 1;
                        function_set_beeakpoints(x, i, av);
                    }
                    av+=i, ac -= i;
                }
                else goto errstate;
            }
            else goto errstate;
        }
        else goto errstate;
    };
    x->x_width *= x->x_zoom;
    x->x_height *= x->x_zoom;
    if(!envset)
        function_set_beeakpoints(x, 3, a);
// set min/max
    float temp = x->x_max;
    if(x->x_min > x->x_max){
        x->x_max = x->x_min;
        x->x_min = temp;
    }
    else if(x->x_max == x->x_min){
        if(x->x_max == 0) // max & min = 0
            x->x_max = 1;
        else{
            if(x->x_max > 0){
                x->x_min = 0; // set min to 0
                if(x->x_max < 1)
                    x->x_max = 1; // set max to 1
            }
            else
                x->x_max = 0;
        }
    }
    if(initialDuration > 0)
        function_duration(x, initialDuration);
     return(x);
errstate:
    pd_error(x, "[function]: improper args");
    return(NULL);
}

void function_setup(void){
    function_class = class_new(gensym("function"), (t_newmethod)function_new,
        (t_method)function_free, sizeof(t_function), 0, A_GIMME,0);
    class_addbang(function_class, function_bang);
    class_addfloat(function_class, function_float);
    class_addlist(function_class, function_list);
    class_addmethod(function_class, (t_method)function_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
    class_addmethod(function_class, (t_method)function_resize, gensym("resize"), 0);
    class_addmethod(function_class, (t_method)function_set, gensym("set"), A_GIMME, 0);
    class_addmethod(function_class, (t_method)function_i, gensym("i"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_height, gensym("height"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_width, gensym("width"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_min, gensym("min"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_max, gensym("max"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_duration, gensym("duration"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_send, gensym("send"), A_SYMBOL, 0);
    class_addmethod(function_class, (t_method)function_receive, gensym("receive"), A_SYMBOL, 0);
    class_addmethod(function_class, (t_method)function_bgcolor, gensym("bgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_fgcolor, gensym("fgcolor"), A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_motion, gensym("motion"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_key, gensym("key"), A_FLOAT, 0);
    class_addmethod(function_class, (t_method)function_zoom, gensym("zoom"), A_CANT, 0);
    edit_proxy_class = class_new(0, 0, 0, sizeof(t_edit_proxy), CLASS_NOINLET | CLASS_PD, 0);
    class_addanything(edit_proxy_class, edit_proxy_any);
    class_setwidget(function_class, &function_widgetbehavior);
    class_setsavefn(function_class, function_save);
    function_widgetbehavior.w_getrectfn  = function_getrect;
    function_widgetbehavior.w_displacefn = function_displace;
    function_widgetbehavior.w_selectfn   = function_select;
    function_widgetbehavior.w_deletefn   = function_delete;
    function_widgetbehavior.w_visfn      = function_vis;
    function_widgetbehavior.w_clickfn    = (t_clickfn)function_click;
}
