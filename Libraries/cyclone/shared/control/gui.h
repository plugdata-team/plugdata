/* Copyright (c) 2003-2004 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __HAMMERGUI_H__
#define __HAMMERGUI_H__

typedef struct _hammergui
{
    t_pd       g_pd;
    t_symbol  *g_psgui;
    t_symbol  *g_psmouse;
    t_symbol  *g_pspoll;
    t_symbol  *g_psfocus;
    t_symbol  *g_psvised;
    int        g_isup;
  
} t_hammergui;

void hammergui_getscreen(void);
void hammergui_getscreenfocused(void);
void hammergui_getwc(t_pd *master);
void hammergui_bindmouse(t_pd *master);
void hammergui_unbindmouse(t_pd *master);
//void hammergui_localmousexy(t_symbol *s, int wx, int wy, int ww, int wh);
//void hammergui_screenmousexy(t_symbol *s);
//void hammergui_focusmousexy(t_symbol *s);
void hammergui_willpoll(void);
void hammergui_startpolling(t_pd *master, int pollmode);
void hammergui_stoppolling(t_pd *master);
void hammergui_bindfocus(t_pd *master);
void hammergui_unbindfocus(t_pd *master);
void hammergui_bindvised(t_pd *master);
void hammergui_unbindvised(t_pd *master);
#endif
