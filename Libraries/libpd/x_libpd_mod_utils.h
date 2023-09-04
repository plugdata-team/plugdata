/*
 // Copyright (c) 2015-2023 Pierre Guillot and Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

#ifdef __cplusplus

extern "C" {
#endif

#include <m_pd.h>
#include <z_libpd.h>
#include <g_canvas.h>


// Some duplicates and modifications of pure-data functions, not a great way to do things

struct _outlet {
    t_object* o_owner;
    struct _outlet* o_next;
    t_outconnect* o_connections;
    t_symbol* o_sym;
};

union inletunion {
    t_symbol* iu_symto;
    t_gpointer* iu_pointerslot;
    t_float* iu_floatslot;
    t_symbol** iu_symslot;
    t_float iu_floatsignalvalue;
};

struct _inlet {
    t_pd i_pd;
    struct _inlet* i_next;
    t_object* i_owner;
    t_pd* i_dest;
    t_symbol* i_symfrom;
    union inletunion i_un;
};


void libpd_get_search_paths(char** paths, int* numItems);

t_pd* libpd_newest(t_canvas* cnv);

t_pd* libpd_createobj(t_canvas* cnv, t_symbol* s, int argc, t_atom* argv);
t_pd* libpd_creategraph(t_canvas* cnv, char const* name, int size, int x, int y, int drawMode, int saveContent, float minimum, float maximum);
t_pd* libpd_creategraphonparent(t_canvas* cnv, int x, int y);

void libpd_finishremove(t_canvas* cnv);
void libpd_removeobj(t_canvas* cnv, t_gobj* obj);
void libpd_renameobj(t_canvas* cnv, t_gobj* obj, char const* buf, size_t bufsize);
void libpd_moveobj(t_canvas* cnv, t_gobj* obj, int x, int y);

char const* libpd_copy(t_canvas* cnv, int* size);
void libpd_paste(t_canvas* cnv, char const*);

void libpd_duplicate(t_canvas* x);

void libpd_undo(t_canvas* cnv);
void libpd_redo(t_canvas* cnv);

void libpd_start_undo_sequence(t_canvas* cnv, char const* name);
void libpd_end_undo_sequence(t_canvas* cnv, char const* name);

// Start and end of remove action, to group them together for undo/redo
void libpd_removeselection(t_canvas* cnv);

void libpd_moveselection(t_canvas* cnv, int dx, int dy);

int libpd_hasconnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin);
void* libpd_createconnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin);
void libpd_removeconnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin, t_symbol* connection_path);

void* libpd_setconnectionpath(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin, t_symbol* old_connection_path, t_symbol* new_connection_path);

void libpd_getcontent(t_canvas* cnv, char** buf, int* bufsize);
void libpd_savetofile(t_canvas* cnv, t_symbol* filename, t_symbol* dir);

int libpd_noutlets(t_object const* x);
int libpd_ninlets(t_object const* x);

int libpd_can_undo(t_canvas* cnv);
int libpd_can_redo(t_canvas* cnv);

void libpd_tofront(t_canvas* cnv, t_gobj* obj);
void libpd_move_forward(t_canvas* cnv, t_gobj* obj);
void libpd_move_backward(t_canvas* cnv, t_gobj* obj);
void libpd_toback(t_canvas* cnv, t_gobj* obj);

void libpd_undo_apply(t_canvas* cnv, t_gobj* obj);

int libpd_issignalinlet(t_object const* x, int m);
int libpd_issignaloutlet(t_object const* x, int m);

void libpd_canvas_saveto(t_canvas* cnv, t_binbuf* b);

void* libpd_tryconnect(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin);
int libpd_canconnect(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin);

void* libpd_get_class_methods(t_class* o);

void set_class_prefix(t_symbol* dir);

#ifdef __cplusplus
}
#endif
