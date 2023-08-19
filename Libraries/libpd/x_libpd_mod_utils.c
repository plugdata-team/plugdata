/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>
#include <m_imp.h>
#include <g_canvas.h>
#include <g_undo.h>
#include <s_stuff.h>


#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include "x_libpd_mod_utils.h"
#include "x_libpd_extra_utils.h"

struct _instanceeditor
{
    t_binbuf *copy_binbuf;
    char *canvas_textcopybuf;
    int canvas_textcopybufsize;
    t_undofn canvas_undo_fn;         /* current undo function if any */
    int canvas_undo_whatnext;        /* whether we can now UNDO or REDO */
    void *canvas_undo_buf;           /* data private to the undo function */
    t_canvas *canvas_undo_canvas;    /* which canvas we can undo on */
    const char *canvas_undo_name;
    int canvas_undo_already_set_move;
    double canvas_upclicktime;
    int canvas_upx, canvas_upy;
    int canvas_find_index, canvas_find_wholeword;
    t_binbuf *canvas_findbuf;
    int paste_onset;
    t_canvas *paste_canvas;
    t_glist *canvas_last_glist;
    int canvas_last_glist_x, canvas_last_glist_y;
    t_canvas *canvas_cursorcanvaswas;
    unsigned int canvas_cursorwas;
};

extern int glist_getindex(t_glist* cnv, t_gobj* y);
extern void canvas_savedeclarationsto(t_canvas *x, t_binbuf *b);
extern void canvas_savetemplatesto(t_canvas *x, t_binbuf *b, int wholething);
extern void canvas_saveto(t_canvas *x, t_binbuf *b);

void libpd_get_search_paths(char** paths, int* numItems) {

    t_namelist* pathList = STUFF->st_searchpath;
    int i = 0;
    while(pathList) {
        i++;
        pathList = pathList->nl_next;
    }
    
    *numItems = i;
    
    pathList = STUFF->st_searchpath;
    i = 0;
    while(pathList) {
        paths[i] = pathList->nl_string;
        i++;
        
        pathList = pathList->nl_next;
    }
}

/* displace the selection by (dx, dy) pixels */
void libpd_moveselection(t_canvas* cnv, int dx, int dy)
{
    EDITOR->canvas_undo_already_set_move = 0;

    t_selection* y;
    int resortin = 0, resortout = 0;
    if (!EDITOR->canvas_undo_already_set_move) {
        canvas_undo_add(cnv, UNDO_MOTION, "motion", canvas_undo_set_move(cnv, 1));
        // EDITOR->canvas_undo_already_set_move = 1;
    }
    for (y = cnv->gl_editor->e_selection; y; y = y->sel_next) {

        t_class* cl = pd_class(&y->sel_what->g_pd);
        gobj_displace(y->sel_what, cnv, dx, dy);
        if (cl == vinlet_class)
            resortin = 1;
        else if (cl == voutlet_class)
            resortout = 1;
    }
    if (resortin)
        canvas_resortinlets(cnv);
    if (resortout)
        canvas_resortoutlets(cnv);
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", cnv);

    if (cnv->gl_editor->e_selection)
        canvas_dirty(cnv, 1);
}

t_pd* libpd_newest(t_canvas* cnv)
{
    // Regular pd_newest won't work because it doesn't get assigned for some gui components
    t_gobj* y;

    // Get to the last object
    for (y = cnv->gl_list; y && y->g_next; y = y->g_next) {
    }

    if (y) {
        return &y->g_pd;
    }

    return 0;
}

static void libpd_canvas_doclear(t_canvas* cnv)
{

    t_gobj *y, *y2;
    int dspstate;

    dspstate = canvas_suspend_dsp();

    /* if text is selected, deselecting it might remake the
     object. So we deselect it and hunt for a "new" object on
     the glist to reselect. */
    if (cnv->gl_editor->e_textedfor) {
        // t_gobj *selwas = x->gl_editor->e_selection->sel_what;
        pd_this->pd_newest = 0;
        glist_noselect(cnv);
        if (pd_this->pd_newest) {
            for (y = cnv->gl_list; y; y = y->g_next)
                if (&y->g_pd == pd_this->pd_newest)
                    glist_select(cnv, y);
        }
    }
    while (1) /* this is pretty weird...  should rewrite it */
    {
        for (y = cnv->gl_list; y; y = y2) {
            y2 = y->g_next;
            if (glist_isselected(cnv, y)) {
                glist_delete(cnv, y);
                goto next;
            }
        }
        goto restore;
    next:;
    }
restore:
    canvas_resume_dsp(dspstate);
    canvas_dirty(cnv, 1);
}

void libpd_finishremove(t_canvas* cnv)
{
    canvas_undo_add(cnv, UNDO_SEQUENCE_END, "clear", 0);
}
void libpd_removeselection(t_canvas* cnv)
{
    sys_lock();
    canvas_undo_add(cnv, UNDO_SEQUENCE_START, "clear", 0);

    canvas_undo_add(cnv, UNDO_CUT, "clear",
        canvas_undo_set_cut(cnv, 2));

    libpd_canvas_doclear(cnv);
    
    sys_unlock();
}

void libpd_start_undo_sequence(t_canvas* cnv, char const* name)
{
    canvas_undo_add(cnv, UNDO_SEQUENCE_START, name, 0);
}

void libpd_end_undo_sequence(t_canvas* cnv, char const* name)
{
    canvas_undo_add(cnv, UNDO_SEQUENCE_END, name, 0);
}

static int binbuf_nextmess(int argc, t_atom const* argv)
{
    int i = 0;
    while (argc--) {
        argv++;
        i++;
        if (A_SEMI == argv->a_type) {
            return i + 1;
        }
    }
    return i;
}

int libpd_canconnect(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
{
    if (!src || !sink || sink == src) /* do source and sink exist (and are not the same)?*/
        return 0;
    if (nin >= obj_ninlets(sink) || (nout >= obj_noutlets(src))) /* do the requested iolets exist? */
        return 0;
    if (canvas_isconnected(cnv, src, nout, sink, nin)) /* are the objects already connected? */
        return 0;
    return (!obj_issignaloutlet(src, nout) || /* are the iolets compatible? */
        obj_issignalinlet(sink, nin));
}

void* libpd_setconnectionpath(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin, t_symbol* old_connection_path, t_symbol* new_connection_path)
{
    libpd_start_undo_sequence(cnv, "ConnectionPath");
    
    libpd_removeconnection(cnv, src, nout, sink, nin, old_connection_path);
    
    t_outconnect* oc = obj_connect(src, nout, sink, nin);
    if (oc) {
        outconnect_set_path_data(oc, new_connection_path);
        
        canvas_undo_add(cnv, UNDO_CONNECT, "connect", canvas_undo_set_connect(cnv, canvas_getindex(cnv, &src->ob_g), nout, canvas_getindex(cnv, &sink->ob_g), nin, new_connection_path));
        
        canvas_dirty(cnv, 1);
    }
    
    libpd_end_undo_sequence(cnv, "ConnectionPath");
    
    return oc;
}
void* libpd_tryconnect(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
{
    if (libpd_canconnect(cnv, src, nout, sink, nin)) {
        t_outconnect* oc = obj_connect(src, nout, sink, nin);
        if (oc) {
            
            canvas_undo_add(cnv, UNDO_CONNECT, "connect", canvas_undo_set_connect(cnv, canvas_getindex(cnv, &src->ob_g), nout, canvas_getindex(cnv, &sink->ob_g), nin, gensym("empty")));
            
            canvas_dirty(cnv, 1);
            return oc;
        }
    }
    return NULL;
}

static int binbuf_getpos(t_binbuf* b, int* x0, int* y0, t_symbol** type)
{
    /*
     * checks how many objects the binbuf contains and where they are located
     * for simplicity, we stop after the first object...
     * "objects" are any patchable things
     * returns: 0: no objects/...
     *          1: single object in binbuf
     *          2: more than one object in binbuf
     * (x0,y0) are the coordinates of the first object
     * (type) is the type of the first object ("obj", "msg",...)
     */
    t_atom* argv = binbuf_getvec(b);
    int argc = binbuf_getnatom(b);
    const int argc0 = argc;
    int count = 0;
    t_symbol* s;
    /* get the position of the first object in the argv binbuf */
    if (argc > 2
        && atom_getsymbol(argv + 0) == &s__N
        && atom_getsymbol(argv + 1) == gensym("canvas")) {
        int ac = argc;
        t_atom* ap = argv;
        int stack = 0;
        do {
            int off = binbuf_nextmess(argc, argv);
            if (!off)
                break;
            ac = argc;
            ap = argv;
            argc -= off;
            argv += off;
            count += off;
            if (off >= 2) {
                if (atom_getsymbol(ap + 1) == gensym("restore")
                    && atom_getsymbol(ap) == &s__X) {
                    stack--;
                }
                if (atom_getsymbol(ap + 1) == gensym("canvas")
                    && atom_getsymbol(ap) == &s__N) {
                    stack++;
                }
            }
            if (argc < 0)
                return 0;
        } while (stack > 0);
        argc = ac;
        argv = ap;
    }
    if (argc < 4 || atom_getsymbol(argv) != &s__X)
        return 0;
    /* #X obj|msg|text|floatatom|symbolatom <x> <y> ...
     * TODO: subpatches #N canvas + #X restore <x> <y>
     */
    s = atom_getsymbol(argv + 1);
    if (gensym("restore") == s
        || gensym("obj") == s
        || gensym("msg") == s
        || gensym("text") == s
        || gensym("floatatom") == s
        || gensym("symbolatom") == s) {
        if (x0)
            *x0 = atom_getfloat(argv + 2);
        if (y0)
            *y0 = atom_getfloat(argv + 3);
        if (type)
            *type = s;
    } else
        return 0;

    /* no wind the binbuf to the next message */
    while (argc--) {
        count++;
        if (A_SEMI == argv->a_type)
            break;
        argv++;
    }
    return 1 + (argc0 > count);
}

char const* libpd_copy(t_canvas* cnv, int* size)
{
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, gensym("copy"), 0, NULL);
    canvas_unsetcurrent(cnv);
    
    char const* text;
    int len;
    binbuf_gettext(pd_this->pd_gui->i_editor->copy_binbuf, &text, &len);
    *size = len;
    return text;
}

void libpd_paste(t_canvas* cnv, char const* buf)
{
    size_t len = strlen(buf);
    binbuf_text(pd_this->pd_gui->i_editor->copy_binbuf, buf, len);
    
    sys_lock();
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, gensym("paste"), 0, NULL);
    canvas_unsetcurrent(cnv);
    sys_unlock();
}

void libpd_undo(t_canvas* cnv)
{
    sys_lock();
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, gensym("undo"), 0, NULL);
    glist_noselect(cnv);
    canvas_unsetcurrent(cnv);
    sys_unlock();
}

void libpd_redo(t_canvas* cnv)
{
    // Temporary fix... might cause us to miss a loadbang when recreating a canvas
    pd_this->pd_newest = 0;
    if (!cnv->gl_editor)
        return;
    
    sys_lock();
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, gensym("redo"), 0, NULL);
    glist_noselect(cnv);
    canvas_unsetcurrent(cnv);
    sys_unlock();
}

static void libpd_arrange_single_step(t_canvas* cnv, t_gobj* obj, int forward) {
    t_gobj* y_begin = cnv->gl_list;
    t_gobj* y_end = y_begin;
    
    while (y_end->g_next) {
        y_end = y_end->g_next;
    }
    
    canvas_undo_add(cnv, UNDO_ARRANGE, "arrange",
        canvas_undo_set_arrange(cnv, obj, forward));
    
    int current_idx = glist_getindex(cnv, obj);
        
    // Find the previous object
    t_gobj* prev_obj = NULL;
    int indx = 0;
    for (prev_obj = y_begin; prev_obj; prev_obj = prev_obj->g_next, indx++) {
        if (indx == current_idx - 1) {
            break;
        }
    }
    
    // If there is an object after ours
    t_gobj* next_obj = obj->g_next ? obj->g_next : NULL;
    
    if (forward) {
        // Already at the end
        if (obj == y_end) return;
        
        // Swap positions with the next object
        if (next_obj) {
            obj->g_next = next_obj->g_next;
            next_obj->g_next = obj;
            if (prev_obj) {
                prev_obj->g_next = next_obj;
            } else {
                cnv->gl_list = next_obj;
            }
        }
    } else {
        // Already at the beginning
        if (!prev_obj) return;
        
        t_gobj* prev_prev = NULL;
        int prev_indx = 0;
        for (prev_prev = y_begin; prev_prev; prev_prev = prev_prev->g_next, prev_indx++) {
            if (prev_indx == current_idx - 2) {
                break;
            }
        }

        if (prev_prev) {
            prev_prev->g_next = obj;
        } else {
            cnv->gl_list = obj;
        }

        obj->g_next = prev_obj;
        
        if (next_obj) {
            prev_obj->g_next = next_obj;
        } else {
            prev_obj->g_next = NULL;
        }
    }
    
    glist_noselect(cnv);
    canvas_dirty(cnv, 1);
}

static void libpd_arrange(t_canvas* cnv, t_gobj* obj, int to_front)
{
    t_gobj* y_begin = cnv->gl_list;
    t_gobj* y_end = y_begin;
    
    while(y_end->g_next) {
        y_end = y_end->g_next;
    }
    
    // This will create an undo action, even if the object is already in the right position
    // Skipping the undo action if the object is alraedy in the right position would be confusing
    canvas_undo_add(cnv, UNDO_ARRANGE, "arrange",
        canvas_undo_set_arrange(cnv, obj, to_front));
    
    int oldidx = glist_getindex(cnv, obj) - 1;
    
    // Check for an object before ours
    t_gobj *oldy_prev = NULL;
    int indx = 0;
    for (oldy_prev = y_begin; oldy_prev; oldy_prev = oldy_prev->g_next, indx++) {
        if (indx == oldidx) {
            break;
        }
    }

    // If there is an object after ours
    t_gobj* oldy_next = obj->g_next ? obj->g_next : NULL;
    
    if(to_front) {
        
        // Already in the right place
        if(obj == y_end) return;
                
        // Put the object at the end of the cue
        y_end->g_next = obj;
        obj->g_next = NULL;
        
        // now fix links in the hole made in the list due to moving of the oldy
        // (we know there is oldy_next as y_end != oldy in canvas_done_popup)
        
        if (oldy_prev) // there is indeed more before the oldy position
            oldy_prev->g_next = oldy_next;
        else cnv->gl_list = oldy_next;
    }
    else {

        // Already in the right place
        if(obj == y_begin) return;
        
        // If there is an object after ours
        if (obj->g_next)
            oldy_next = obj->g_next;
        
        cnv->gl_list = obj; /* put it to the beginning of the cue */
        obj->g_next = y_begin; /* make it point to the old beginning */

            /* now fix links in the hole made in the list due to moving of the oldy
             * (we know there is oldy_prev as y_begin != oldy in canvas_done_popup)
             */
        if (oldy_prev)
        {
            if (oldy_next) /* there is indeed more after oldy position */
                oldy_prev->g_next = oldy_next;
            else oldy_prev->g_next = NULL; /* oldy was the last in the cue */
        }
    }
    
    glist_noselect(cnv);
    canvas_dirty(cnv, 1);
}
void libpd_tofront(t_canvas* cnv, t_gobj* obj)
{
    libpd_arrange(cnv, obj, 1);
}

void libpd_move_forward(t_canvas* cnv, t_gobj* obj)
{
    libpd_arrange_single_step(cnv, obj, 1);
}

void libpd_move_backward(t_canvas* cnv, t_gobj* obj)
{
    libpd_arrange_single_step(cnv, obj, 0);
}

void libpd_toback(t_canvas* cnv, t_gobj* obj)
{
    libpd_arrange(cnv, obj, 0);
}

void libpd_duplicate(t_canvas* cnv)
{
    sys_lock();
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, gensym("duplicate"), 0, NULL);
    canvas_unsetcurrent(cnv);
    sys_unlock();
}

/* save a "root" canvas to a file; cf. canvas_saveto() which saves the
 body (and which is called recursively.) */
void libpd_savetofile(t_canvas* cnv, t_symbol* filename, t_symbol* dir)
{
    t_binbuf *b = binbuf_new();
    canvas_savetemplatesto(cnv, b, 1);
    canvas_saveto(cnv, b);
    errno = 0;
    if (binbuf_write(b, filename->s_name, dir->s_name, 0))
        post("%s/%s: %s", dir->s_name, filename->s_name,
            (errno ? strerror(errno) : "write failed"));
    else
    {
            /* if not an abstraction, reset title bar and directory */
        if (!cnv->gl_owner)
        {
            canvas_rename(cnv, filename, dir);
            /* update window list in case Save As changed the window name */
            canvas_updatewindowlist();
        }
        post("saved to: %s/%s", dir->s_name, filename->s_name);
        canvas_dirty(cnv, 0);
    }
    binbuf_free(b);
}

t_pd* libpd_creategraphonparent(t_canvas* cnv, int x, int y)
{
    int argc = 9;

    t_atom argv[9];

    t_float x1 = 0.0f;
    t_float y1 = 0.0f;
    t_float x2 = 0;
    t_float y2 = 0;
    t_float px1 = x;
    t_float py1 = y;
    t_float px2 = x + 200.0f;
    t_float py2 = y + 140.0f;

    t_symbol* sym = gensym("graph");

    SETSYMBOL(argv, sym);
    SETFLOAT(argv + 1, x1);
    SETFLOAT(argv + 2, y1);
    SETFLOAT(argv + 3, x2);
    SETFLOAT(argv + 4, y2);

    SETFLOAT(argv + 5, px1);
    SETFLOAT(argv + 6, py1);

    SETFLOAT(argv + 7, px2);
    SETFLOAT(argv + 8, py2);

    sys_lock();
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, gensym("graph"), argc, argv);
    pd_popsym(s__X.s_thing);
    canvas_unsetcurrent(cnv);
    sys_unlock();

    glist_noselect(cnv);

    t_pd* result = libpd_newest(cnv);
    ((t_glist*)result)->gl_hidetext = 1;
    ((t_glist*)result)->gl_loading = 0;

    canvas_dirty(cnv, 1);
    
    return result;
}

t_pd* libpd_creategraph(t_canvas* cnv, char const* name, int size, int x, int y, int drawMode, int saveContent, float minimum, float maximum)
{
    int flags = saveContent + 2 * drawMode;

    t_atom argv[4];
    SETSYMBOL(argv, gensym(name));
    SETFLOAT(argv + 1, size);
    SETFLOAT(argv + 2, flags);
    SETFLOAT(argv + 3, 0);
    
    sys_lock();
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, gensym("arraydialog"), 4, argv);
    canvas_unsetcurrent(cnv);
    sys_unlock();

    glist_noselect(cnv);

    t_pd* arr = libpd_newest(cnv);

    libpd_moveobj(cnv, pd_checkobject(arr), x, y);
    
    t_garray* garray = (t_garray*)(((t_canvas*)arr)->gl_list);
    libpd_array_set_scale(garray, minimum, maximum);
    
    canvas_dirty(cnv, 1);
    
    
    return arr;
}

t_pd* libpd_createobj(t_canvas* cnv, t_symbol* s, int argc, t_atom* argv)
{
    sys_lock();
    canvas_setcurrent(cnv);
    pd_typedmess((t_pd*)cnv, s, argc, argv);
    
    canvas_undo_add(cnv, UNDO_CREATE, "create",
        (void*)canvas_undo_set_create(cnv));
    
    t_pd* new_object = libpd_newest(cnv);

    if (new_object) {
        if (pd_class(new_object) == canvas_class)
            canvas_loadbang(new_object);
        else if (zgetfn(new_object, gensym("loadbang")))
            vmess(new_object, gensym("loadbang"), "f", LB_LOAD);
    }
    sys_unlock();
    
    canvas_unsetcurrent(cnv);
    
    glist_noselect(cnv);
    canvas_dirty(cnv, 1);
    
    return new_object;
}

void libpd_removeobj(t_canvas* cnv, t_gobj* obj)
{
    
    sys_lock();
    glist_noselect(cnv);
    glist_select(cnv, obj);
    libpd_canvas_doclear(cnv);

    glist_noselect(cnv);
    sys_unlock();
}

void libpd_renameobj(t_canvas* cnv, t_gobj* obj, char const* buf, size_t bufsize)
{
    struct _rtext {
        char* x_buf;    /*-- raw byte string, assumed UTF-8 encoded (moo) --*/
        int x_bufsize;  /*-- byte length --*/
        int x_selstart; /*-- byte offset --*/
        int x_selend;   /*-- byte offset --*/
        int x_active;
        int x_dragfrom;
        int x_height;
        int x_drawnwidth;
        int x_drawnheight;
        t_text* x_text;
        t_glist* x_glist;
        char x_tag[50];
        struct _rtext* x_next;
    };
    
    sys_lock();
    canvas_editmode(cnv, 1);

    glist_noselect(cnv);
    glist_select(cnv, obj);

    t_rtext* fuddy = glist_findrtext(cnv, (t_text*)obj);
    cnv->gl_editor->e_textedfor = fuddy;

    fuddy->x_buf = resizebytes(fuddy->x_buf, fuddy->x_bufsize, bufsize);

    strncpy(fuddy->x_buf, buf, bufsize);
    fuddy->x_bufsize = bufsize;

    cnv->gl_editor->e_textdirty = 1;

    glist_deselect(cnv, obj);

    cnv->gl_editor->e_textedfor = 0;
    cnv->gl_editor->e_textdirty = 0;

    canvas_editmode(cnv, 0);
    
    canvas_dirty(cnv, 1);
    sys_unlock();
}

int libpd_can_undo(t_canvas* cnv)
{

    t_undo* udo = canvas_undo_get(cnv);

    if (udo && udo->u_last) {
        return strcmp(udo->u_last->name, "no");
    }

    return 0;
}

int libpd_can_redo(t_canvas* cnv)
{

    t_undo* udo = canvas_undo_get(cnv);

    if (udo && udo->u_last && udo->u_last->next) {
        return strcmp(udo->u_last->next->name, "no");
    }

    return 0;
}

// Can probably be used as a general purpose undo action on an object?
void libpd_undo_apply(t_canvas* cnv, t_gobj* obj)
{
    canvas_undo_add(cnv, UNDO_APPLY, "props",
        canvas_undo_set_apply(cnv, glist_getindex(cnv, obj)));
}

void libpd_moveobj(t_canvas* cnv, t_gobj* obj, int x, int y)
{
    if (obj->g_pd->c_wb && obj->g_pd->c_wb->w_getrectfn && obj->g_pd->c_wb && obj->g_pd->c_wb->w_displacefn) {

        int x1, y1, x2, y2;

        (*obj->g_pd->c_wb->w_getrectfn)(obj, cnv, &x1, &y1, &x2, &y2);

        (*obj->g_pd->c_wb->w_displacefn)(obj, cnv, x - x1, y - y1);
    }
}

void* libpd_createconnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
{
    void* oc = libpd_tryconnect(cnv, src, nout, sink, nin);
    glist_noselect(cnv);
    return oc;
}

int libpd_hasconnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
{
    return canvas_isconnected(cnv, src, nout, sink, nin);
}

void libpd_removeconnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin, t_symbol* connection_path)
{

    if (!libpd_hasconnection(cnv, src, nout, sink, nin)) {
        bug("non-existent connection");
        return;
    }

    obj_disconnect(src, nout, sink, nin);

    int dest_i = canvas_getindex(cnv, &(sink->te_g));
    int src_i = canvas_getindex(cnv, &(src->te_g));

    canvas_undo_add(cnv, UNDO_DISCONNECT, "disconnect", canvas_undo_set_disconnect(cnv, src_i, nout, dest_i, nin, connection_path));
    glist_noselect(cnv);
    
    canvas_dirty(cnv, 1);
}

void libpd_getcontent(t_canvas* cnv, char** buf, int* bufsize)
{
    t_binbuf* b = binbuf_new();
    
    t_gobj *y;
    t_linetraverser t;
    t_outconnect *oc;

        /* subpatch */
    if (cnv->gl_owner && !cnv->gl_env)
    {
        /* have to go to original binbuf to find out how we were named. */
        t_binbuf *bz = binbuf_new();
        t_symbol *patchsym;
        binbuf_addbinbuf(bz, cnv->gl_obj.ob_binbuf);
        patchsym = atom_getsymbolarg(1, binbuf_getnatom(bz), binbuf_getvec(bz));
        binbuf_free(bz);
        binbuf_addv(b, "ssiiiisi;", gensym("#N"), gensym("canvas"),
            (int)(cnv->gl_screenx1),
            (int)(cnv->gl_screeny1),
            (int)(cnv->gl_screenx2 - cnv->gl_screenx1),
            (int)(cnv->gl_screeny2 - cnv->gl_screeny1),
            (patchsym != &s_ ? patchsym: gensym("(subpatch)")),
                    cnv->gl_mapped);
    }
        /* root or abstraction */
    else
    {
        binbuf_addv(b, "ssiiiii;", gensym("#N"), gensym("canvas"),
            (int)(cnv->gl_screenx1),
            (int)(cnv->gl_screeny1),
            (int)(cnv->gl_screenx2 - cnv->gl_screenx1),
            (int)(cnv->gl_screeny2 - cnv->gl_screeny1),
                (int)cnv->gl_font);
        canvas_savedeclarationsto(cnv, b);
    }
    for (y = cnv->gl_list; y; y = y->g_next)
        gobj_save(y, b);

    linetraverser_start(&t, cnv);
    while ((oc = linetraverser_next(&t)))
    {
        int srcno = canvas_getindex(cnv, &t.tr_ob->ob_g);
        int sinkno = canvas_getindex(cnv, &t.tr_ob2->ob_g);
        binbuf_addv(b, "ssiiiis;", gensym("#X"), gensym("connect"),
            srcno, t.tr_outno, sinkno, t.tr_inno, t.outconnect_path_info);
    }
        /* unless everything is the default (as in ordinary subpatches)
        print out a "coords" message to set up the coordinate systems */
    if (cnv->gl_isgraph || cnv->gl_x1 || cnv->gl_y1 ||
        cnv->gl_x2 != 1 ||  cnv->gl_y2 != 1 || cnv->gl_pixwidth || cnv->gl_pixheight)
    {
        if (cnv->gl_isgraph && cnv->gl_goprect)
                /* if we have a graph-on-parent rectangle, we're new style.
                The format is arranged so
                that old versions of Pd can at least do something with it. */
            binbuf_addv(b, "ssfffffffff;", gensym("#X"), gensym("coords"),
                        cnv->gl_x1, cnv->gl_y1,
                        cnv->gl_x2, cnv->gl_y2,
                (t_float)cnv->gl_pixwidth, (t_float)cnv->gl_pixheight,
                (t_float)((cnv->gl_hidetext)?2.:1.),
                (t_float)cnv->gl_xmargin, (t_float)cnv->gl_ymargin);
                    /* otherwise write in 0.38-compatible form */
        else binbuf_addv(b, "ssfffffff;", gensym("#X"), gensym("coords"),
                         cnv->gl_x1, cnv->gl_y1,
                         cnv->gl_x2, cnv->gl_y2,
                (t_float)cnv->gl_pixwidth, (t_float)cnv->gl_pixheight,
                (t_float)cnv->gl_isgraph);
    }
    
    binbuf_gettext(b, buf, bufsize);
    binbuf_free(b);
}

typedef t_pd* (*t_newgimme)(t_symbol* s, int argc, t_atom* argv);
typedef void (*t_messgimme)(t_pd* x, t_symbol* s, int argc, t_atom* argv);

typedef t_pd* (*t_fun0)(
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd* (*t_fun1)(t_int i1,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd* (*t_fun2)(t_int i1, t_int i2,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd* (*t_fun3)(t_int i1, t_int i2, t_int i3,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd* (*t_fun4)(t_int i1, t_int i2, t_int i3, t_int i4,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd* (*t_fun5)(t_int i1, t_int i2, t_int i3, t_int i4, t_int i5,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd* (*t_fun6)(t_int i1, t_int i2, t_int i3, t_int i4, t_int i5, t_int i6,
    t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);


int libpd_noutlets(t_object const* x)
{
    int n;
    t_outlet* o;
    for (o = x->ob_outlet, n = 0; o; o = o->o_next)
        n++;
    return (n);
}

int libpd_ninlets(t_object const* x)
{
    int n;
    t_inlet* i;
    for (i = x->ob_inlet, n = 0; i; i = i->i_next)
        n++;
    if (x->ob_pd->c_firstin)
        n++;
    return (n);
}

#ifdef PDINSTANCE
#    define s_signal (pd_this->pd_s_signal)
#endif

int libpd_issignalinlet(t_object const* x, int m)
{
    t_inlet* i;
    if (x->ob_pd->c_firstin) {
        if (!m)
            return (x->ob_pd->c_firstin && x->ob_pd->c_floatsignalin);
        else
            m--;
    }
    for (i = x->ob_inlet; i && m; i = i->i_next, m--)
        ;

    return (i && (i->i_symfrom == &s_signal));
}

int libpd_issignaloutlet(t_object const* x, int m)
{
    int n;
    t_outlet* o2;
    for (o2 = x->ob_outlet, n = 0; o2 && m--; o2 = o2->o_next)
        ;
    return (o2 && (o2->o_sym == &s_signal));
}

void* libpd_get_class_methods(t_class* o) {
#ifdef PDINSTANCE
    return o->c_methods[pd_this->pd_instanceno];
#else
    return o->c_methods;
#endif
}
