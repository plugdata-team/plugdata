/* Copyright (c) 2003-2004 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* FIXME use guiconnect */

/* LATER revisit tracking the mouse state within the focusless pd-gui
   (event bindings are local only in tk8.4, and there is no other call
   to XQueryPointer() but from winfo pointer). */

//some notes on my working through this:
//basically we bind the sink to the symbol assoc with hashhammergui #hammergui
//we bind virtual events <<hammer*>> with actual events like <focus> via event add
//then we bind these virtual events <<hammer*>> with messages back to pd via pdsend 

//ALSO: redoing a lot of the way gui.c works, deferring calculations to mousestate
// in interests of multiple object independence

//-DK


#include <stdio.h>
#include <string.h>
#include "m_pd.h"
#include "g_canvas.h"
#include "control/gui.h"

#ifdef KRZYSZCZ
//#define HAMMERGUI_DEBUG
#endif

static t_class *hammergui_class = 0;
static t_hammergui *hammergui_sink = 0;
static t_symbol *ps_hashhammergui;
static t_symbol *ps__hammergui;
static t_symbol *ps__up;
static t_symbol *ps__focus;
static t_symbol *ps__vised;

static void hammergui_anything(t_hammergui *snk,
			       t_symbol *s, int ac, t_atom *av)
{
    /* Dummy method, filtering out messages from gui to the masters.  This is
       needed in order to keep Pd's message system happy in a ``gray period''
       -- after last master is unbound, and before gui bindings are cleared. */
#ifdef HAMMERGUI_DEBUG
    /* FIXME */
    startpost("%s", s->s_name);
    postatom(ac, av);
    post(" (sink %x)", (int)snk);
#endif
}

/* filtering out redundant "_up" messages */
static void hammergui__up(t_hammergui *snk, t_floatarg f)
{
//    post("hammergui__up");
#ifdef HAMMERGUI_DEBUG
    fprintf(stderr, "_up %g (sink %x)\n", f, (int)snk);
#endif
    if (!snk->g_psmouse)
    {
	bug("hammergui__up");
	return;
    }
    if ((int)f)
    {
	if (!snk->g_isup)
	{
	    snk->g_isup = 1;
	    if (snk->g_psmouse->s_thing)
	    {
		t_atom at;
		SETFLOAT(&at, 1);
		pd_typedmess(snk->g_psmouse->s_thing, ps__up, 1, &at);
	    }
	}
    }
    else
    {
	if (snk->g_isup)
	{
	    snk->g_isup = 0;
	    if (snk->g_psmouse->s_thing)
	    {
		t_atom at;
		SETFLOAT(&at, 0);
		pd_typedmess(snk->g_psmouse->s_thing, ps__up, 1, &at);
	    }
	}
    }
}

static void hammergui__focus(t_hammergui *snk, t_symbol *s, t_floatarg f)
{
//    post("hammergui__focus");
#ifdef HAMMERGUI_DEBUG
    fprintf(stderr, "_focus %s %g (sink %x)\n", (s ? s->s_name : "???"), f, (int)snk);
#endif
    if (!snk->g_psfocus){
        bug("hammergui__focus");
        return;
    }
    if (snk->g_psfocus->s_thing){
//        post("sending focus");
        t_atom at[2];
        SETSYMBOL(&at[0], s);
        SETFLOAT(&at[1], f);
        pd_typedmess(snk->g_psfocus->s_thing, ps__focus, 2, at);
    }
}

static void hammergui__vised(t_hammergui *snk, t_symbol *s, t_floatarg f)
{
//    post("hammergui__vised");
#ifdef HAMMERGUI_DEBUG
    fprintf(stderr, "_vised %s %g (sink %x)\n",
	    (s ? s->s_name : "???"), f, (int)snk);
#endif
    if (!snk->g_psvised)
    {
	bug("hammergui__vised");
	return;
    }
    if (snk->g_psvised->s_thing)
    {
	t_atom at[2];
	SETSYMBOL(&at[0], s);
	SETFLOAT(&at[1], f);
	pd_typedmess(snk->g_psvised->s_thing, ps__vised, 2, at);
    }
#if 0
    /* How to be notified about changes of button state, prior to gui objects
       in a canvas?  LATER find a reliable way -- delete if failed */
    sys_vgui("bindtags %s {hammertag %s Canvas . all}\n",
	     s->s_name, s->s_name);
#endif
}


static void hammergui_dobindmouse(t_hammergui *snk)
{
//    post("hammergui_dobindmouse");
#ifdef HAMMERGUI_DEBUG
    fprintf(stderr, "dobindmouse (sink %x)\n", (int)snk);
#endif
#if 0
    /* How to be notified about changes of button state, prior to gui objects
       in a canvas?  LATER find a reliable way -- delete if failed */
    sys_vgui("bind hammertag <<hammerdown>> {pdsend {%s _up 0}}\n",
	     snk->g_psgui->s_name);
    sys_vgui("bind hammertag <<hammerup>> {pdsend {%s _up 1}}\n",
	     snk->g_psgui->s_name);
#endif
    sys_vgui("bind all <<hammerdown>> {pdsend {%s _up 0}}\n",
	     snk->g_psgui->s_name);
    sys_vgui("bind all <<hammerup>> {pdsend {%s _up 1}}\n",
	     snk->g_psgui->s_name);
}

static void hammergui__remouse(t_hammergui *snk)
{
//    post("hammergui__remouse");
    if (!snk->g_psmouse)
    {
	bug("hammergui__remouse");
	return;
    }
    if (snk->g_psmouse->s_thing)
    {
	/* if a new master was bound in a gray period, we need to
	   restore gui bindings */
#if 1
	post("rebinding mouse...");
#endif
	hammergui_dobindmouse(snk);
    }
}

static void hammergui_dobindfocus(t_hammergui *snk)
{
//    post("hammergui_dobindfocus");
    sys_vgui("bind Canvas <<hammerfocusin>> \
 {if {[hammergui_ispatcher %%W]} \
  {pdsend {%s _focus %%W 1}}}\n", snk->g_psgui->s_name);
    sys_vgui("bind Canvas <<hammerfocusout>> \
 {if {[hammergui_ispatcher %%W]} \
  {pdsend {%s _focus %%W 0}}}\n", snk->g_psgui->s_name);
}

static void hammergui__refocus(t_hammergui *snk)
{
//    post("hammergui__refocus");
    if (!snk->g_psfocus)
    {
	bug("hammergui__refocus");
	return;
    }
    if (snk->g_psfocus->s_thing)
    {
	/* if a new master was bound in a gray period, we need to
	   restore gui bindings */
#if 1
	post("rebinding focus...");
#endif
	hammergui_dobindfocus(snk);
    }
}

static void hammergui_dobindvised(t_hammergui *snk)
{
//    post("hammergui_dobindvised");
#ifdef HAMMERGUI_DEBUG
    fprintf(stderr, "dobindvised (sink %x)\n", (int)snk);
#endif
    sys_vgui("bind Canvas <<hammervised>> \
 {if {[hammergui_ispatcher %%W]} \
  {pdsend {%s _vised %%W 1}}}\n", snk->g_psgui->s_name);
    sys_vgui("bind Canvas <<hammerunvised>> \
 {if {[hammergui_ispatcher %%W]} \
  {pdsend {%s _vised %%W 0}}}\n", snk->g_psgui->s_name);
}

static void hammergui__revised(t_hammergui *snk)
{
//    post("hammergui__revised");
    if (!snk->g_psvised)
    {
	bug("hammergui__revised");
	return;
    }
    if (snk->g_psvised->s_thing)
    {
	/* if a new master was bound in a gray period, we need to
	   restore gui bindings */
#if 1
	post("rebinding vised events...");
#endif
	hammergui_dobindvised(snk);
    }
}

static int hammergui_setup(void)
{
    ps_hashhammergui = gensym("#hammergui");
    ps__hammergui = gensym("_hammergui");
    ps__up = gensym("_up");
    ps__focus = gensym("_focus");
    ps__vised = gensym("_vised");
    if (ps_hashhammergui->s_thing)
    {
	char *cname = class_getname(*ps_hashhammergui->s_thing);
#ifdef HAMMERGUI_DEBUG
	fprintf(stderr,
		"'%s' already registered as the global hammergui sink \n",
		(cname ? cname : "???"));
#endif
	if (strcmp(cname, ps__hammergui->s_name))
	{
	    /* FIXME protect against the danger of someone else
	       (e.g. receive) binding to #hammergui */
	    bug("hammergui_setup");
	    return (0);
	}
	else
	{
	    /* FIXME compatibility test */
	    hammergui_class = *ps_hashhammergui->s_thing;
	    return (1);
	}
    }
    hammergui_class = class_new(ps__hammergui, 0, 0,
				sizeof(t_hammergui),
				CLASS_PD | CLASS_NOINLET, 0);
    class_addanything(hammergui_class, hammergui_anything);
    class_addmethod(hammergui_class, (t_method)hammergui__remouse,
		    gensym("_remouse"), 0);
    class_addmethod(hammergui_class, (t_method)hammergui__refocus,
		    gensym("_refocus"), 0);
    class_addmethod(hammergui_class, (t_method)hammergui__revised,
		    gensym("_revised"), 0);
    class_addmethod(hammergui_class, (t_method)hammergui__up,
		    ps__up, A_FLOAT, 0);
    class_addmethod(hammergui_class, (t_method)hammergui__focus,
		    ps__focus, A_SYMBOL, A_FLOAT, 0);
    class_addmethod(hammergui_class, (t_method)hammergui__vised,
		    ps__vised, A_SYMBOL, A_FLOAT, 0);

    /* if older than 0.43, create an 0.43-style pdsend */
    sys_gui("if {[llength [info procs ::pdsend]] == 0} {");
    sys_gui("proc ::pdsend {args} {::pd \"[join $args { }] ;\"}}\n");
    
    /* Protect against pdCmd being called (via "Canvas <Destroy>" binding)
       during Tcl_Finalize().  FIXME this should be a standard exit handler. */
    sys_gui("proc hammergui_exithook {cmd op} {proc ::pdsend {} {}}\n");
    sys_gui("if {[info tclversion] >= 8.4} {\n\
 trace add execution exit enter hammergui_exithook}\n");

 sys_gui("proc hammergui_ispatcher {cv} {\n");
    sys_gui(" if {[string range $cv 0 1] == \".x\"");
    sys_gui("  && [string range $cv end-1 end] == \".c\"} {\n");
    sys_gui("  return 1} else {return 0}\n");
    sys_gui("}\n");

    sys_gui("proc hammergui_remouse {} {\n");
    sys_gui(" bind all <<hammerdown>> {}\n");
    sys_gui(" bind all <<hammerup>> {}\n");
    sys_gui(" pdsend {#hammergui _remouse}\n");
    sys_gui("}\n");

    
    sys_gui("proc hammergui_getscreen {} {\n");
    sys_gui(" set px [winfo pointerx .]\n");
    sys_gui(" set py [winfo pointery .]\n");
    sys_gui(" pdsend \"#hammermouse _getscreen $px $py\"\n");
    sys_gui("}\n");

    sys_gui("proc hammergui_getscreenfocused {} {\n");
    sys_gui(" set px [winfo pointerx .]\n");
    sys_gui(" set py [winfo pointery . ]\n");
    sys_gui(" set wx [winfo x $::focused_window]\n");
    sys_gui(" set wy [winfo y $::focused_window]\n");
    //sys_gui(" set ww [winfo width $::focused_window]\n");
    //sys_gui(" set wh [winfo height $::focused_window]\n");
    sys_gui(" pdsend \"#hammermouse _getscreenfocused ");
    //sys_gui("$px $py $wx $wy $ww $wh\"\n");
    sys_gui("$px $py $wx $wy\"\n");
    sys_gui("}\n");

    /* visibility hack for msw, LATER rethink */
    sys_gui("global hammergui_ispolling\n");
    sys_gui("global hammergui_px\n");
    sys_gui("global hammergui_py\n");
    sys_gui("set hammergui_ispolling 0\n");
    sys_gui("set hammergui_px 0\n");
    sys_gui("set hammergui_py 0\n");
    sys_gui("set hammergui_wx 0\n");
    sys_gui("set hammergui_wy 0\n");
    //sys_gui("set hammergui_ww 0\n");
    //sys_gui("set hammergui_wh 0\n");
    
    sys_gui("proc hammergui_poll {} {\n");
    sys_gui("global hammergui_ispolling\n");
    sys_gui("global hammergui_px\n");
    sys_gui("global hammergui_py\n");
    sys_gui("global hammergui_wx\n");
    sys_gui("global hammergui_wy\n");
    //sys_gui("global hammergui_ww\n");
    //sys_gui("global hammergui_wh\n");
    sys_gui("if {$hammergui_ispolling > 0} {\n");
    //mode0 and 1
    sys_gui("set px [winfo pointerx .]\n");
    sys_gui("set py [winfo pointery .]\n");
    sys_gui("if {$hammergui_ispolling <= 2} {\n");
    sys_gui("if {$hammergui_px != $px || $hammergui_py != $py} {\n");
    sys_gui(" pdsend \"#hammermouse _getscreen $px $py\"\n");
    sys_gui(" set hammergui_px $px\n");
    sys_gui(" set hammergui_py $py\n");
    sys_gui("}\n");
    sys_gui("} ");
    //mode2
    sys_gui("elseif {$hammergui_ispolling == 3} {\n");
    sys_gui(" set wx [winfo x $::focused_window]\n");
    sys_gui(" set wy [winfo y $::focused_window]\n");
    //sys_gui(" set ww [winfo width $::focused_window]\n");
    //sys_gui(" set wh [winfo height $::focused_window]\n");
    sys_gui("if {$hammergui_px != $px || $hammergui_py != $py ");
    sys_gui("|| $hammergui_wx != $wx || $hammergui_wy != $wy} {\n ");
    //sys_gui("|| $hammergui_ww != $ww || $hammergui_wh != $wh} {\n");
    sys_gui(" pdsend \"#hammermouse _getscreenfocused ");
    // sys_gui("$px $py $wx $wy $ww $wh\"\n");
    sys_gui("$px $py $wx $wy\"\n");
    sys_gui(" set hammergui_px $px\n");
    sys_gui(" set hammergui_py $py\n");
    sys_gui(" set hammergui_wx $wx\n");
    sys_gui(" set hammergui_wy $wy\n");
    //sys_gui(" set hammergui_ww $ww\n");
    //sys_gui(" set hammergui_wh $wh\n");
    sys_gui("}\n");
    sys_gui("}\n");
    sys_gui("after 50 hammergui_poll\n");
    sys_gui("}\n");
    sys_gui("}\n");

    sys_gui("proc hammergui_refocus {} {\n");
    sys_gui(" bind Canvas <<hammerfocusin>> {}\n");
    sys_gui(" bind Canvas <<hammerfocusout>> {}\n");
    sys_gui(" pdsend {#hammergui _refocus}\n");
    sys_gui("}\n");

    sys_gui("proc hammergui_revised {} {\n");
    sys_gui(" bind Canvas <<hammervised>> {}\n");
    sys_gui(" bind Canvas <<hammerunvised>> {}\n");
    sys_gui(" pdsend {#hammergui _revised}\n");
    sys_gui("}\n");

   return (1);
}

static int hammergui_validate(int dosetup)
{
    if (dosetup && !hammergui_sink
	&& (hammergui_class || hammergui_setup()))
    {
	if (ps_hashhammergui->s_thing)
	    hammergui_sink = (t_hammergui *)ps_hashhammergui->s_thing;
	else
	{
	    hammergui_sink = (t_hammergui *)pd_new(hammergui_class);
	    hammergui_sink->g_psgui = ps_hashhammergui;
	    pd_bind((t_pd *)hammergui_sink,
		    ps_hashhammergui);  /* never unbound */
	}
    }
    if (hammergui_class && hammergui_sink)
	return (1);
    else
    {
	bug("hammergui_validate");
	return (0);
    }
}

static int hammergui_mousevalidate(int dosetup)
{
    if (dosetup && !hammergui_sink->g_psmouse)
    {
        hammergui_sink->g_psmouse = gensym("#hammermouse");

        sys_gui("event add <<hammerdown>> <ButtonPress>\n");
	sys_gui("event add <<hammerup>> <ButtonRelease>\n");
    }
    if (hammergui_sink->g_psmouse)
	return (1);
    else
    {
	bug("hammergui_mousevalidate");
	return (0);
    }
}

static int hammergui_pollvalidate(int dosetup)
{
    if (dosetup && !hammergui_sink->g_pspoll)
    {
	hammergui_sink->g_pspoll = gensym("#hammerpoll");
	pd_bind((t_pd *)hammergui_sink,
		hammergui_sink->g_pspoll);  /* never unbound */
    }
    if (hammergui_sink->g_pspoll)
	return (1);
    else
    {
	bug("hammergui_pollvalidate");
	return (0);
    }
}

static int hammergui_focusvalidate(int dosetup)
{
    if (dosetup && !hammergui_sink->g_psfocus)
    {
	hammergui_sink->g_psfocus = gensym("#hammerfocus");
	sys_gui("event add <<hammerfocusin>> <FocusIn>\n");
	sys_gui("event add <<hammerfocusout>> <FocusOut>\n");
    }
    if (hammergui_sink->g_psfocus)
	return (1);
    else
    {
	bug("hammergui_focusvalidate");
	return (0);
    }
}

static int hammergui_visedvalidate(int dosetup)
{
    if (dosetup && !hammergui_sink->g_psvised)
    {
	hammergui_sink->g_psvised = gensym("#hammervised");
	/* subsequent map events have to be filtered out at the caller's side,
	   LATER investigate */
	sys_gui("event add <<hammervised>> <Map>\n");
	sys_gui("event add <<hammerunvised>> <Destroy>\n");
    }
    if (hammergui_sink->g_psvised)
	return (1);
    else
    {
	bug("hammergui_visedvalidate");
	return (0);
    }
}

void hammergui_bindmouse(t_pd *master)
{
#ifdef HAMMERGUI_DEBUG
    fprintf(stderr, "bindmouse, master %x\n", (int)master);
#endif
    hammergui_validate(1);
    hammergui_mousevalidate(1);
    if (!hammergui_sink->g_psmouse->s_thing)
	hammergui_dobindmouse(hammergui_sink);
    pd_bind(master, hammergui_sink->g_psmouse);
}

void hammergui_unbindmouse(t_pd *master)
{
    if (hammergui_validate(0) && hammergui_mousevalidate(0)
	&& hammergui_sink->g_psmouse->s_thing)
    {
	pd_unbind(master, hammergui_sink->g_psmouse);
	if (!hammergui_sink->g_psmouse->s_thing)
	    sys_gui("hammergui_remouse\n");
    }
    else bug("hammergui_unbindmouse");
}

/*
void hammergui_localmousexy(t_symbol *s, int wx, int wy, int ww, int wh)
{
    if (hammergui_validate(0))
	sys_vgui("hammergui_localmousexy %s %d %d %d %d\n", s->s_name, wx, wy, ww, wh);
}


void hammergui_screenmousexy(t_symbol *s)
{
    if (hammergui_validate(0))
	sys_vgui("hammergui_screenmousexy %s\n", s->s_name);
}


void hammergui_focusmousexy(t_symbol *s)
{
    if (hammergui_validate(0))
	sys_vgui("hammergui_focusmousexy %s\n", s->s_name);
}

void hammergui_getscreen(void)
{
  if(hammergui_validate(0))
    sys_gui("hammergui_getscreen\n");
}
*/

void hammergui_getscreenfocused(void)
{

  if(hammergui_validate(0))
    sys_gui("hammergui_getscreenfocused\n");
}

void hammergui_getscreen(void)
{
  if(hammergui_validate(0))
    sys_gui("hammergui_getscreen\n");
}

void hammergui_willpoll(void)
{
    hammergui_validate(1);
    hammergui_pollvalidate(1);
}

void hammergui_startpolling(t_pd *master, int pollmode)
{
    if (hammergui_validate(0) && hammergui_pollvalidate(0))
    {
	int doinit =
	    (hammergui_sink->g_pspoll->s_thing == (t_pd *)hammergui_sink);
	pd_bind(master, hammergui_sink->g_pspoll);
	if (doinit)
	{
	    /* visibility hack for msw, LATER rethink */
	    sys_gui("global hammergui_ispolling\n");
	    sys_vgui("set hammergui_ispolling %d\n", pollmode);
	    sys_gui("hammergui_poll\n");
	}
    }
}

void hammergui_stoppolling(t_pd *master)
{
    if (hammergui_validate(0) && hammergui_pollvalidate(0))
    {
	pd_unbind(master, hammergui_sink->g_pspoll);
	if (hammergui_sink->g_pspoll->s_thing == (t_pd *)hammergui_sink)
	{
	    sys_gui("global hammergui_ispolling\n");
	    sys_gui("set hammergui_ispolling 0\n");
	    sys_vgui("after cancel [hammergui_poll]\n");
	    /* visibility hack for msw, LATER rethink */
	    
	}
    }
}



void hammergui_bindfocus(t_pd *master)
{
    hammergui_validate(1);
    hammergui_focusvalidate(1);
    if (!hammergui_sink->g_psfocus->s_thing)
	hammergui_dobindfocus(hammergui_sink);
    pd_bind(master, hammergui_sink->g_psfocus);
}

void hammergui_unbindfocus(t_pd *master)
{
    if (hammergui_validate(0) && hammergui_focusvalidate(0)
	&& hammergui_sink->g_psfocus->s_thing)
    {
	pd_unbind(master, hammergui_sink->g_psfocus);
	if (!hammergui_sink->g_psfocus->s_thing)
	    sys_gui("hammergui_refocus\n");
    }
    else bug("hammergui_unbindfocus");
}

void hammergui_bindvised(t_pd *master)
{
#ifdef HAMMERGUI_DEBUG
    fprintf(stderr, "bindvised, master %x\n", (int)master);
#endif
    hammergui_validate(1);
    hammergui_visedvalidate(1);
    if (!hammergui_sink->g_psvised->s_thing)
	hammergui_dobindvised(hammergui_sink);
    pd_bind(master, hammergui_sink->g_psvised);
}

void hammergui_unbindvised(t_pd *master)
{
    if (hammergui_validate(0) && hammergui_visedvalidate(0)
	&& hammergui_sink->g_psvised->s_thing)
    {
	pd_unbind(master, hammergui_sink->g_psvised);
	if (!hammergui_sink->g_psvised->s_thing)
	    sys_gui("hammergui_revised\n");
    }
    else bug("hammergui_unbindvised");
}
