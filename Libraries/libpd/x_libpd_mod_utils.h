/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#ifdef __cplusplus


extern "C"
{
#endif

#include <m_pd.h>
#include <z_libpd.h>
#include <g_canvas.h>


t_pd* libpd_newest(t_canvas* cnv);

t_pd* libpd_createobj(t_canvas* cnv, t_symbol *s, int argc, t_atom *argv, int undoable);
t_pd* libpd_creategraph(t_canvas* cnv, const char* name, int size, int x, int y);
t_pd* libpd_creategraphonparent(t_canvas* cnv, int x, int y);

void libpd_finishremove(t_canvas* cnv);
void libpd_removeobj(t_canvas* cnv, t_gobj* obj);
void libpd_renameobj(t_canvas* cnv, t_gobj* obj, const char* buf, int bufsize);
void libpd_moveobj(t_canvas* cnv, t_gobj* obj, int x, int y);


const char* libpd_copy(t_canvas* cnv);
void libpd_paste(t_canvas* cnv, const char*);

void libpd_duplicate(t_canvas *x);

void libpd_undo(t_canvas* cnv);
void libpd_redo(t_canvas* cnv);

void libpd_start_undo_sequence(t_canvas* cnv, const char* name);
void libpd_end_undo_sequence(t_canvas* cnv, const char* name);

// Start and end of remove action, to group them together for undo/redo
void libpd_removeselection(t_canvas* cnv);

void libpd_moveselection(t_canvas* cnv, int dx, int dy);

void libpd_createconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin);
void libpd_removeconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin);

void libpd_getcontent(t_canvas* cnv, char** buf, int* bufsize);
void libpd_savetofile(t_canvas* cnv, t_symbol *filename, t_symbol *dir);

int libpd_type_exists(const char* type);

int libpd_noutlets(const t_object *x);

int libpd_ninlets(const t_object *x);

int libpd_can_undo(t_canvas* cnv);
int libpd_can_redo(t_canvas* cnv);

void libpd_undo_apply(t_canvas* cnv, t_gobj* obj);

int libpd_issignalinlet(const t_object *x, int m);
int libpd_issignaloutlet(const t_object *x, int m);

void libpd_canvas_doclear(t_canvas* cnv);

void libpd_canvas_saveto(t_canvas* cnv, t_binbuf *b);

void gobj_setposition(t_gobj *x, t_glist *glist, int xpos, int ypos);

int libpd_tryconnect(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin);
int libpd_canconnect(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin);

void libpd_collecttemplatesfor(t_canvas* cnv, int *ntemplatesp,
                               t_symbol ***templatevecp);

#ifdef __cplusplus
}
#endif

