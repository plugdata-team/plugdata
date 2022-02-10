// stolen from cyclone

#include "m_pd.h"
#include "g_canvas.h"
#include <stdio.h>
#include <string.h>

typedef struct _mouse_gui{
    t_pd       g_pd;
    t_symbol  *g_psgui;
    t_symbol  *g_psmouse;
    t_symbol  *g_pspoll;
    t_symbol  *g_psfocus;
    t_symbol  *g_psvised;
    int        g_isup;
} t_mouse_gui;

void mouse_gui_getscreen(void);
void mouse_gui_getscreenfocused(void);
void mouse_gui_getwc(t_pd *master);
void mouse_gui_bindmouse(t_pd *master);
void mouse_gui_unbindmouse(t_pd *master);
void mouse_gui_willpoll(void);
void mouse_gui_startpolling(t_pd *master, int pollmode);
void mouse_gui_stoppolling(t_pd *master);
void mouse_gui_bindfocus(t_pd *master);
void mouse_gui_unbindfocus(t_pd *master);
void mouse_gui_bindvised(t_pd *master);
void mouse_gui_unbindvised(t_pd *master);

static t_class *mouse_gui_class = 0;
static t_mouse_gui *mouse_gui_sink = 0;
static t_symbol *ps_hashmouse_gui;
static t_symbol *ps__mouse_gui;
static t_symbol *ps__up;
static t_symbol *ps__focus;
static t_symbol *ps__vised;

static void mouse_gui_anything(void){ // dummy
}

/* filtering out redundant "_up" messages */
static void mouse_gui__up(t_mouse_gui *snk, t_floatarg f){
    if (!snk->g_psmouse){
        bug("mouse_gui__up");
        return;
    }
    if ((int)f){
        if (!snk->g_isup){
            snk->g_isup = 1;
            if (snk->g_psmouse->s_thing){
                t_atom at;
                SETFLOAT(&at, 1);
                pd_typedmess(snk->g_psmouse->s_thing, ps__up, 1, &at);
            }
        }
    }
    else{
        if (snk->g_isup)
            snk->g_isup = 0;
        if (snk->g_psmouse->s_thing){
            t_atom at;
            SETFLOAT(&at, 0);
            pd_typedmess(snk->g_psmouse->s_thing, ps__up, 1, &at);
        }
    }
}

static void mouse_gui__focus(t_mouse_gui *snk, t_symbol *s, t_floatarg f){
    if (!snk->g_psfocus){
        bug("mouse_gui__focus");
        return;
    }
    if (snk->g_psfocus->s_thing){
        t_atom at[2];
        SETSYMBOL(&at[0], s);
        SETFLOAT(&at[1], f);
        pd_typedmess(snk->g_psfocus->s_thing, ps__focus, 2, at);
    }
}

static void mouse_gui__vised(t_mouse_gui *snk, t_symbol *s, t_floatarg f){
    if (!snk->g_psvised){
        bug("mouse_gui__vised");
        return;
    }
    if (snk->g_psvised->s_thing){
        t_atom at[2];
        SETSYMBOL(&at[0], s);
        SETFLOAT(&at[1], f);
        pd_typedmess(snk->g_psvised->s_thing, ps__vised, 2, at);
    }
}


static void mouse_gui_dobindmouse(t_mouse_gui *snk)
{
#if 0
    /* How to be notified about changes of button state, prior to gui objects
     in a canvas?  LATER find a reliable way -- delete if failed */
    sys_vgui("bind mouse_tag <<mouse_down>> {pdsend {%s _up 0}}\n",
             snk->g_psgui->s_name);
    sys_vgui("bind mouse_tag <<mouse_up>> {pdsend {%s _up 1}}\n",
             snk->g_psgui->s_name);
#endif
    sys_vgui("bind all <<mouse_down>> {pdsend {%s _up 0}}\n",
             snk->g_psgui->s_name);
    sys_vgui("bind all <<mouse_up>> {pdsend {%s _up 1}}\n",
             snk->g_psgui->s_name);
}

static void mouse_gui__remouse(t_mouse_gui *snk){
    if (!snk->g_psmouse){
        bug("mouse_gui__remouse");
        return;
    }
    if (snk->g_psmouse->s_thing){
        /* if a new master was bound in a gray period, we need to
         restore gui bindings */
#if 0
        post("rebinding mouse...");
#endif
        mouse_gui_dobindmouse(snk);
    }
}

static void mouse_gui_dobindfocus(t_mouse_gui *snk){
    sys_vgui("bind Canvas <<mouse_focusin>> \
             {if {[mouse_gui_ispatcher %%W]} \
             {pdsend {%s _focus %%W 1}}}\n", snk->g_psgui->s_name);
    sys_vgui("bind Canvas <<mouse_focusout>> \
             {if {[mouse_gui_ispatcher %%W]} \
             {pdsend {%s _focus %%W 0}}}\n", snk->g_psgui->s_name);
}

static void mouse_gui__refocus(t_mouse_gui *snk){
    if (!snk->g_psfocus)
    {
        bug("mouse_gui__refocus");
        return;
    }
    if (snk->g_psfocus->s_thing)
    {
        /* if a new master was bound in a gray period, we need to
         restore gui bindings */
#if 0
        post("rebinding focus...");
#endif
        mouse_gui_dobindfocus(snk);
    }
}

static void mouse_gui_dobindvised(t_mouse_gui *snk){
    sys_vgui("bind Canvas <<mouse_vised>> \
             {if {[mouse_gui_ispatcher %%W]} \
             {pdsend {%s _vised %%W 1}}}\n", snk->g_psgui->s_name);
    sys_vgui("bind Canvas <<mouse_unvised>> \
             {if {[mouse_gui_ispatcher %%W]} \
             {pdsend {%s _vised %%W 0}}}\n", snk->g_psgui->s_name);
}

static void mouse_gui__revised(t_mouse_gui *snk){
    if (!snk->g_psvised){
        bug("mouse_gui__revised");
        return;
    }
    if (snk->g_psvised->s_thing){
        /* if a new master was bound in a gray period, we need to
         restore gui bindings */
#if 0
        post("rebinding vised events...");
#endif
        mouse_gui_dobindvised(snk);
    }
}

static int mouse_gui_setup(void){
    ps_hashmouse_gui = gensym("#mouse_gui");
    ps__mouse_gui = gensym("_mouse_gui");
    ps__up = gensym("_up");
    ps__focus = gensym("_focus");
    ps__vised = gensym("_vised");
    if (ps_hashmouse_gui->s_thing)
    {
        if (strcmp(class_getname(*ps_hashmouse_gui->s_thing), ps__mouse_gui->s_name))
        {
            /* FIXME protect against the danger of someone else
             (e.g. receive) binding to #mouse_gui */
            bug("mouse_gui_setup");
            return (0);
        }
        else
        {
            /* FIXME compatibility test */
            mouse_gui_class = *ps_hashmouse_gui->s_thing;
            return (1);
        }
    }
    mouse_gui_class = class_new(ps__mouse_gui, 0, 0,
                                sizeof(t_mouse_gui),
                                CLASS_PD | CLASS_NOINLET, 0);
    class_addanything(mouse_gui_class, mouse_gui_anything);
    class_addmethod(mouse_gui_class, (t_method)mouse_gui__remouse,
                    gensym("_remouse"), 0);
    class_addmethod(mouse_gui_class, (t_method)mouse_gui__refocus,
                    gensym("_refocus"), 0);
    class_addmethod(mouse_gui_class, (t_method)mouse_gui__revised,
                    gensym("_revised"), 0);
    class_addmethod(mouse_gui_class, (t_method)mouse_gui__up,
                    ps__up, A_FLOAT, 0);
    class_addmethod(mouse_gui_class, (t_method)mouse_gui__focus,
                    ps__focus, A_SYMBOL, A_FLOAT, 0);
    class_addmethod(mouse_gui_class, (t_method)mouse_gui__vised,
                    ps__vised, A_SYMBOL, A_FLOAT, 0);
    
    /* Protect against pdCmd being called (via "Canvas <Destroy>" binding)
     during Tcl_Finalize().  FIXME this should be a standard exit handler. */
    sys_gui("proc mouse_gui_exithook {cmd op} {proc ::pdsend {} {}}\n");
    sys_gui("if {[info tclversion] >= 8.4} {\n\
            trace add execution exit enter mouse_gui_exithook}\n");
    
    sys_gui("proc mouse_gui_ispatcher {cv} {\n");
    sys_gui(" if {[string range $cv 0 1] == \".x\"");
    sys_gui("  && [string range $cv end-1 end] == \".c\"} {\n");
    sys_gui("  return 1} else {return 0}\n");
    sys_gui("}\n");
    
    sys_gui("proc mouse_gui_remouse {} {\n");
    sys_gui(" bind all <<mouse_down>> {}\n");
    sys_gui(" bind all <<mouse_up>> {}\n");
    sys_gui(" pdsend {#mouse_gui _remouse}\n");
    sys_gui("}\n");
    
    sys_gui("proc mouse_gui_getscreen {} {\n");
    sys_gui(" set px [winfo pointerx .]\n");
    sys_gui(" set py [winfo pointery .]\n");
    sys_gui(" pdsend \"#mouse_mouse _getscreen $px $py\"\n");
    sys_gui("}\n");
    
    sys_gui("proc mouse_gui_getscreenfocused {} {\n");
    sys_gui(" set px [winfo pointerx .]\n");
    sys_gui(" set py [winfo pointery . ]\n");
    sys_gui(" set wx [winfo x $::focused_window]\n");
    sys_gui(" set wy [winfo y $::focused_window]\n");
    sys_gui(" pdsend \"#mouse_mouse _getscreenfocused ");
    sys_gui("$px $py $wx $wy\"\n");
    sys_gui("}\n");
    
    /* visibility hack for msw, LATER rethink */
    sys_gui("global mouse_gui_ispolling\n");
    sys_gui("global mouse_gui_px\n");
    sys_gui("global mouse_gui_py\n");
    sys_gui("set mouse_gui_ispolling 0\n");
    sys_gui("set mouse_gui_px 0\n");
    sys_gui("set mouse_gui_py 0\n");
    sys_gui("set mouse_gui_wx 0\n");
    sys_gui("set mouse_gui_wy 0\n");
    
    sys_gui("proc mouse_gui_poll {} {\n");
    sys_gui("global mouse_gui_ispolling\n");
    sys_gui("global mouse_gui_px\n");
    sys_gui("global mouse_gui_py\n");
    sys_gui("global mouse_gui_wx\n");
    sys_gui("global mouse_gui_wy\n");
    sys_gui("if {$mouse_gui_ispolling > 0} {\n");
    // mode0 and 1
    sys_gui("set px [winfo pointerx .]\n");
    sys_gui("set py [winfo pointery .]\n");
    sys_gui("if {$mouse_gui_ispolling <= 2} {\n");
    sys_gui("if {$mouse_gui_px != $px || $mouse_gui_py != $py} {\n");
    sys_gui(" pdsend \"#mouse_mouse _getscreen $px $py\"\n");
    sys_gui(" set mouse_gui_px $px\n");
    sys_gui(" set mouse_gui_py $py\n");
    sys_gui("}\n");
    sys_gui("} ");
    // mode2
    sys_gui("elseif {$mouse_gui_ispolling == 3} {\n");
    sys_gui(" set wx [winfo x $::focused_window]\n");
    sys_gui(" set wy [winfo y $::focused_window]\n");
    sys_gui("if {$mouse_gui_px != $px || $mouse_gui_py != $py ");
    sys_gui("|| $mouse_gui_wx != $wx || $mouse_gui_wy != $wy} {\n ");
    sys_gui(" pdsend \"#mouse_mouse _getscreenfocused ");
    sys_gui("$px $py $wx $wy\"\n");
    sys_gui(" set mouse_gui_px $px\n");
    sys_gui(" set mouse_gui_py $py\n");
    sys_gui(" set mouse_gui_wx $wx\n");
    sys_gui(" set mouse_gui_wy $wy\n");
    sys_gui("}\n");
    sys_gui("}\n");
    sys_gui("after 50 mouse_gui_poll\n");
    sys_gui("}\n");
    sys_gui("}\n");
    
    sys_gui("proc mouse_gui_refocus {} {\n");
    sys_gui(" bind Canvas <<mouse_focusin>> {}\n");
    sys_gui(" bind Canvas <<mouse_focusout>> {}\n");
    sys_gui(" pdsend {#mouse_gui _refocus}\n");
    sys_gui("}\n");
    
    sys_gui("proc mouse_gui_revised {} {\n");
    sys_gui(" bind Canvas <<mouse_vised>> {}\n");
    sys_gui(" bind Canvas <<mouse_unvised>> {}\n");
    sys_gui(" pdsend {#mouse_gui _revised}\n");
    sys_gui("}\n");
    
    return (1);
}

static int mouse_gui_validate(int dosetup)
{
    if (dosetup && !mouse_gui_sink
        && (mouse_gui_class || mouse_gui_setup()))
    {
        if (ps_hashmouse_gui->s_thing)
            mouse_gui_sink = (t_mouse_gui *)ps_hashmouse_gui->s_thing;
        else
        {
            mouse_gui_sink = (t_mouse_gui *)pd_new(mouse_gui_class);
            mouse_gui_sink->g_psgui = ps_hashmouse_gui;
            pd_bind((t_pd *)mouse_gui_sink,
                    ps_hashmouse_gui);  /* never unbound */
        }
    }
    if (mouse_gui_class && mouse_gui_sink)
        return (1);
    else
    {
        bug("mouse_gui_validate");
        return (0);
    }
}

static int mouse_gui_mousevalidate(int dosetup)
{
    if (dosetup && !mouse_gui_sink->g_psmouse)
    {
        mouse_gui_sink->g_psmouse = gensym("#mouse_mouse");
        
        sys_gui("event add <<mouse_down>> <ButtonPress>\n");
        sys_gui("event add <<mouse_up>> <ButtonRelease>\n");
    }
    if (mouse_gui_sink->g_psmouse)
        return (1);
    else
    {
        bug("mouse_gui_mousevalidate");
        return (0);
    }
}

static int mouse_gui_pollvalidate(int dosetup)
{
    if (dosetup && !mouse_gui_sink->g_pspoll)
    {
        mouse_gui_sink->g_pspoll = gensym("#mouse_poll");
        pd_bind((t_pd *)mouse_gui_sink,
                mouse_gui_sink->g_pspoll);  /* never unbound */
    }
    if (mouse_gui_sink->g_pspoll)
        return (1);
    else
    {
        bug("mouse_gui_pollvalidate");
        return (0);
    }
}

static int mouse_gui_focusvalidate(int dosetup)
{
    if (dosetup && !mouse_gui_sink->g_psfocus)
    {
        mouse_gui_sink->g_psfocus = gensym("#mouse_focus");
        sys_gui("event add <<mouse_focusin>> <FocusIn>\n");
        sys_gui("event add <<mouse_focusout>> <FocusOut>\n");
    }
    if (mouse_gui_sink->g_psfocus)
        return (1);
    else
    {
        bug("mouse_gui_focusvalidate");
        return (0);
    }
}

static int mouse_gui_visedvalidate(int dosetup)
{
    if (dosetup && !mouse_gui_sink->g_psvised)
    {
        mouse_gui_sink->g_psvised = gensym("#mouse_vised");
        /* subsequent map events have to be filtered out at the caller's side,
         LATER investigate */
        sys_gui("event add <<mouse_vised>> <Map>\n");
        sys_gui("event add <<mouse_unvised>> <Destroy>\n");
    }
    if (mouse_gui_sink->g_psvised)
        return (1);
    else
    {
        bug("mouse_gui_visedvalidate");
        return (0);
    }
}

void mouse_gui_bindmouse(t_pd *master)
{
    mouse_gui_validate(1);
    mouse_gui_mousevalidate(1);
    if (!mouse_gui_sink->g_psmouse->s_thing)
        mouse_gui_dobindmouse(mouse_gui_sink);
    pd_bind(master, mouse_gui_sink->g_psmouse);
}

void mouse_gui_unbindmouse(t_pd *master)
{
    if (mouse_gui_validate(0) && mouse_gui_mousevalidate(0)
        && mouse_gui_sink->g_psmouse->s_thing)
    {
        pd_unbind(master, mouse_gui_sink->g_psmouse);
        if (!mouse_gui_sink->g_psmouse->s_thing)
            sys_gui("mouse_gui_remouse\n");
    }
    else bug("mouse_gui_unbindmouse");
}

void mouse_gui_getscreenfocused(void)
{
    if(mouse_gui_validate(0))
        sys_gui("mouse_gui_getscreenfocused\n");
}

void mouse_gui_getscreen(void)
{
    if(mouse_gui_validate(0))
        sys_gui("mouse_gui_getscreen\n");
}

void mouse_gui_willpoll(void)
{
    mouse_gui_validate(1);
    mouse_gui_pollvalidate(1);
}

void mouse_gui_startpolling(t_pd *master, int pollmode)
{
    if (mouse_gui_validate(0) && mouse_gui_pollvalidate(0))
    {
        int doinit =
        (mouse_gui_sink->g_pspoll->s_thing == (t_pd *)mouse_gui_sink);
        pd_bind(master, mouse_gui_sink->g_pspoll);
        if (doinit)
        {
            /* visibility hack for msw, LATER rethink */
            sys_gui("global mouse_gui_ispolling\n");
            sys_vgui("set mouse_gui_ispolling %d\n", pollmode);
            sys_gui("mouse_gui_poll\n");
        }
    }
}

void mouse_gui_stoppolling(t_pd *master)
{
    if (mouse_gui_validate(0) && mouse_gui_pollvalidate(0))
    {
        pd_unbind(master, mouse_gui_sink->g_pspoll);
        if (mouse_gui_sink->g_pspoll->s_thing == (t_pd *)mouse_gui_sink)
        {
            sys_gui("global mouse_gui_ispolling\n");
            sys_gui("set mouse_gui_ispolling 0\n");
            sys_vgui("after cancel [mouse_gui_poll]\n");
            /* visibility hack for msw, LATER rethink */
        }
    }
}

void mouse_gui_bindfocus(t_pd *master){
    mouse_gui_validate(1);
    mouse_gui_focusvalidate(1);
    if(!mouse_gui_sink->g_psfocus->s_thing)
        mouse_gui_dobindfocus(mouse_gui_sink);
    pd_bind(master, mouse_gui_sink->g_psfocus);
}

void mouse_gui_unbindfocus(t_pd *master){
    if (mouse_gui_validate(0) && mouse_gui_focusvalidate(0) && mouse_gui_sink->g_psfocus->s_thing){
        pd_unbind(master, mouse_gui_sink->g_psfocus);
        if(!mouse_gui_sink->g_psfocus->s_thing)
            sys_gui("mouse_gui_refocus\n");
    }
    else
        bug("mouse_gui_unbindfocus");
}

void mouse_gui_bindvised(t_pd *master)
{
    mouse_gui_validate(1);
    mouse_gui_visedvalidate(1);
    if (!mouse_gui_sink->g_psvised->s_thing)
        mouse_gui_dobindvised(mouse_gui_sink);
    pd_bind(master, mouse_gui_sink->g_psvised);
}

void mouse_gui_unbindvised(t_pd *master)
{
    if (mouse_gui_validate(0) && mouse_gui_visedvalidate(0)
        && mouse_gui_sink->g_psvised->s_thing)
    {
        pd_unbind(master, mouse_gui_sink->g_psvised);
        if (!mouse_gui_sink->g_psvised->s_thing)
            sys_gui("mouse_gui_revised\n");
    }
    else bug("mouse_gui_unbindvised");
}

////////////////////// MOUSE /////////////////////////////

typedef struct _mouse{
    t_object   x_obj;
    int        x_hzero;
    int        x_vzero;
    int        x_zero;
    int        x_wx;
    int        x_wy;
    t_glist   *x_glist;
    t_outlet  *x_horizontal;
    t_outlet  *x_vertical;
}t_mouse;

static t_class *mouse_class;

static void mouse_anything(void){ // dummy
}

static void mouse_updatepos(t_mouse *x){ // update position
    x->x_wx = x->x_glist->gl_screenx1;
    x->x_wy = x->x_glist->gl_screeny1;
}

static void mouse_doup(t_mouse *x, t_floatarg f){
    outlet_float(((t_object *)x)->ob_outlet, ((int)f ? 0 : 1));
}

static void mouse_dobang(t_mouse *x, t_floatarg h, t_floatarg v){
    outlet_float(x->x_vertical, (int)v - x->x_vzero);
    outlet_float(x->x_horizontal, (int)h - x->x_hzero);
}

static void mouse_dozero(t_mouse *x, t_floatarg f1, t_floatarg f2){
    if(x->x_zero){
        x->x_hzero = (int)f1;
        x->x_vzero = (int)f2;
        x->x_zero = 0;
    };
}

static void mouse__getscreen(t_mouse *x, t_float screenx, t_float screeny){
    // callback from tcl for requesting screen coords
    if(x->x_zero == 1)
        mouse_dozero(x, screenx, screeny);
    mouse_dobang(x, screenx, screeny);
}

static void mouse_zero(t_mouse *x){
    x->x_zero = 1;
    mouse_gui_getscreen();
}

static void mouse_reset(t_mouse *x){
    x->x_hzero = x->x_vzero = 0;
    mouse_gui_getscreen();
}

static void mouse_free(t_mouse *x){
    mouse_gui_stoppolling((t_pd *)x);
    mouse_gui_unbindmouse((t_pd *)x);
}

static void *mouse_new(void){
    t_mouse *x = (t_mouse *)pd_new(mouse_class);
    x->x_zero = 0;    outlet_new((t_object *)x, &s_float);
    x->x_horizontal = outlet_new((t_object *)x, &s_float);
    x->x_vertical = outlet_new((t_object *)x, &s_float);
    x->x_glist = (t_glist *)canvas_getcurrent();
    mouse_gui_bindmouse((t_pd *)x);
    mouse_gui_willpoll();
    mouse_reset(x);
    mouse_updatepos(x);
    mouse_gui_startpolling((t_pd *)x, 1);
    return(x);
}

void mouse_setup(void){
    mouse_class = class_new(gensym("mouse"), (t_newmethod)mouse_new,
            (t_method)mouse_free, sizeof(t_mouse), 0, 0);
    class_addanything(mouse_class, mouse_anything);
    class_addmethod(mouse_class, (t_method)mouse_doup, gensym("_up"), A_FLOAT, 0);
    class_addmethod(mouse_class, (t_method)mouse__getscreen, gensym("_getscreen"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(mouse_class, (t_method)mouse_dobang, gensym("_bang"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(mouse_class, (t_method)mouse_dozero, gensym("_zero"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(mouse_class, (t_method)mouse_zero, gensym("zero"), 0);
    class_addmethod(mouse_class, (t_method)mouse_reset, gensym("reset"), 0);
}
