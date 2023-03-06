
#ifndef COMPAT_H
#define COMPAT_H

// Definitions for Purr Data compatibility which still has an older API than
// current vanilla.

#include "m_pd.h"
#include "g_canvas.h"

#ifndef IHEIGHT
// Purr Data doesn't have these, hopefully the vanilla values will work
#define IHEIGHT 3       /* height of an inlet in pixels */
#define OHEIGHT 3       /* height of an outlet in pixels */
#endif

#ifdef PDL2ORK
/* This call is needed for the gui parts, which haven't been ported to JS yet
   anyway, so we just make this a no-op for now. XXXFIXME: Once the gui
   features have been ported, we need to figure out what exactly is needed
   here, and implement it in terms of Purr Data's undo/redo system. */
#define pd_undo_set_objectstate(canvas, x, s, undo_argc, undo_argv, redo_argc, redo_argv) /* no-op  */
#endif

#endif
