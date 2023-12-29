/*
 // Copyright (c) 2021-2022 Timothy Schoen.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

extern "C" {
#include <m_pd.h>
#include <m_imp.h>
#include <g_canvas.h>
#include <g_undo.h>
#include <s_stuff.h>
#include <z_libpd.h>

struct _instanceeditor {
    t_binbuf* copy_binbuf;
    char* canvas_textcopybuf;
    int canvas_textcopybufsize;
    t_undofn canvas_undo_fn;      /* current undo function if any */
    int canvas_undo_whatnext;     /* whether we can now UNDO or REDO */
    void* canvas_undo_buf;        /* data private to the undo function */
    t_canvas* canvas_undo_canvas; /* which canvas we can undo on */
    char const* canvas_undo_name;
    int canvas_undo_already_set_move;
    double canvas_upclicktime;
    int canvas_upx, canvas_upy;
    int canvas_find_index, canvas_find_wholeword;
    t_binbuf* canvas_findbuf;
    int paste_onset;
    t_canvas* paste_canvas;
    t_glist* canvas_last_glist;
    int canvas_last_glist_x, canvas_last_glist_y;
    t_canvas* canvas_cursorcanvaswas;
    unsigned int canvas_cursorwas;
};

extern int glist_getindex(t_glist* cnv, t_gobj* y);
extern void canvas_savedeclarationsto(t_canvas* x, t_binbuf* b);
extern void canvas_savetemplatesto(t_canvas* x, t_binbuf* b, int wholething);
extern void canvas_saveto(t_canvas* x, t_binbuf* b);
extern void set_class_prefix(t_symbol*);
}

namespace pd {

struct Interface {

    static t_canvas* createCanvas(char const* name, char const* path)
    {
        t_canvas* cnv = (t_canvas*)libpd_openfile(name, path);
        if (cnv) {
            canvas_vis(cnv, 1.f);
            canvas_rename(cnv, gensym(name), gensym(path));
        }
        return cnv;
    }

    static char const* getObjectClassName(t_pd* ptr)
    {
        return class_getname(pd_class(ptr));
    }

    static void getObjectText(t_object* ptr, char** text, int* size)
    {
        *text = nullptr;
        *size = 0;
        binbuf_gettext(ptr->te_binbuf, text, size);
    }

    static void getObjectBounds(t_canvas* cnv, t_gobj* ptr, int* x, int* y, int* w, int* h)
    {
        *x = 0;
        *y = 0;
        *w = 0;
        *h = 0;

        gobj_getrect(ptr, cnv, x, y, w, h);

        *w -= *x;
        *h -= *y;
    }

    static int isTextObject(t_gobj* obj)
    {
        return obj->g_pd->c_wb == &text_widgetbehavior;
    }

    static void getSearchPaths(char** paths, int* numItems)
    {

        t_namelist* pathList = STUFF->st_searchpath;
        int i = 0;
        while (pathList) {
            i++;
            pathList = pathList->nl_next;
        }

        *numItems = i;

        pathList = STUFF->st_searchpath;
        i = 0;
        while (pathList) {
            paths[i] = pathList->nl_string;
            i++;

            pathList = pathList->nl_next;
        }
    }

    static t_object* checkObject(t_pd* obj)
    {
        if (!obj)
            return nullptr;
        return pd_checkobject(obj);
    }

    static t_object* checkObject(t_gobj* obj)
    {
        if (!obj)
            return nullptr;
        return pd_checkobject(&obj->g_pd);
    }

    /* displace the selection by (dx, dy) pixels */
    static void moveObjects(t_canvas* cnv, int dx, int dy, std::vector<t_gobj*> const& objects)
    {
        glist_noselect(cnv);

        for (auto* obj : objects) {
            glist_select(cnv, obj);
        }

        EDITOR->canvas_undo_already_set_move = 0;

        int resortin = 0, resortout = 0;
        if (!EDITOR->canvas_undo_already_set_move) {
            canvas_undo_add(cnv, UNDO_MOTION, "motion", canvas_undo_set_move(cnv, 1));
            // EDITOR->canvas_undo_already_set_move = 1;
        }
        for (auto* obj : objects) {
            gobj_displace(obj, cnv, dx, dy);

            t_class* cl = pd_class(&obj->g_pd);
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

        glist_noselect(cnv);

        libpd_this_instance()->pd_gui->i_editor->canvas_undo_already_set_move = 0;
    }

    static t_gobj* getNewest(t_canvas* cnv)
    {
        // Regular pd_newest won't work because it doesn't get assigned for some gui components
        t_gobj* y;

        // Get to the last object
        for (y = cnv->gl_list; y && y->g_next; y = y->g_next) {
        }

        return y;
    }

    static void finishRemove(t_canvas* cnv)
    {
        canvas_undo_add(cnv, UNDO_SEQUENCE_END, "clear", nullptr);
        glist_noselect(cnv);
    }

    static void removeObjects(t_canvas* cnv, std::vector<t_gobj*> const& objects)
    {
        canvas_undo_add(cnv, UNDO_SEQUENCE_START, "clear", nullptr);

        glist_noselect(cnv);

        for (auto* obj : objects) {
            glist_select(cnv, obj);
        }

        canvas_undo_add(cnv, UNDO_CUT, "clear",
            canvas_undo_set_cut(cnv, 2));

        doRemoveSelection(cnv);
    }

    static t_outconnect* setConnectionPath(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin, t_symbol* old_connection_path, t_symbol* new_connection_path)
    {
        canvas_undo_add(cnv, UNDO_SEQUENCE_START, "ConnectionPath", nullptr);

        removeConnection(cnv, src, nout, sink, nin, old_connection_path);

        t_outconnect* oc = obj_connect(src, nout, sink, nin);
        if (oc) {
            outconnect_set_path_data(oc, new_connection_path);

            canvas_undo_add(cnv, UNDO_CONNECT, "connect", canvas_undo_set_connect(cnv, canvas_getindex(cnv, &src->ob_g), nout, canvas_getindex(cnv, &sink->ob_g), nin, new_connection_path));

            canvas_dirty(cnv, 1);
        }

        canvas_undo_add(cnv, UNDO_SEQUENCE_END, "ConnectionPath", nullptr);

        return oc;
    }

    static char const* copy(t_canvas* cnv, int* size, std::vector<t_gobj*> const& objects)
    {
        glist_noselect(cnv);

        for (auto* obj : objects) {
            glist_select(cnv, obj);
        }

        canvas_setcurrent(cnv);
        pd_typedmess((t_pd*)cnv, gensym("copy"), 0, nullptr);
        canvas_unsetcurrent(cnv);

        char* text;
        int len;
        binbuf_gettext(libpd_this_instance()->pd_gui->i_editor->copy_binbuf, &text, &len);
        *size = len;

        glist_noselect(cnv);

        return text;
    }

    static t_symbol* getUnusedArrayName()
    {
        sys_lock();
        int gcount;
        char arraybuf[80] = { '\0' };
        for (gcount = 1; gcount < 1000; gcount++) {
            snprintf(arraybuf, 80, "array%d", gcount);
            if (!pd_findbyclass(gensym(arraybuf), garray_class))
                break;
        }
        sys_unlock();

        return gensym(arraybuf);
    }

    static void paste(t_canvas* cnv, char const* buf)
    {
        size_t len = strlen(buf);

        binbuf_text(libpd_this_instance()->pd_gui->i_editor->copy_binbuf, buf, len);

        canvas_setcurrent(cnv);
        pd_typedmess((t_pd*)cnv, gensym("paste"), 0, nullptr);
        canvas_unsetcurrent(cnv);
    }

    static void undo(t_canvas* cnv)
    {
        canvas_setcurrent(cnv);
        pd_typedmess((t_pd*)cnv, gensym("undo"), 0, nullptr);
        glist_noselect(cnv);
        canvas_unsetcurrent(cnv);
    }

    static void redo(t_canvas* cnv)
    {
        // Temporary fix... might cause us to miss a loadbang when recreating a canvas
        libpd_this_instance()->pd_newest = 0;
        if (!cnv->gl_editor)
            return;

        canvas_setcurrent(cnv);
        pd_typedmess((t_pd*)cnv, gensym("redo"), 0, nullptr);
        glist_noselect(cnv);
        canvas_unsetcurrent(cnv);
    }

    static void toFront(t_canvas* cnv, t_gobj* obj)
    {
        arrangeObject(cnv, obj, 1);
    }

    static void moveForward(t_canvas* cnv, t_gobj* obj)
    {
        arrangeObjectOneStep(cnv, obj, 1);
    }

    static void moveBackward(t_canvas* cnv, t_gobj* obj)
    {
        arrangeObjectOneStep(cnv, obj, 0);
    }

    static void toBack(t_canvas* cnv, t_gobj* obj)
    {
        arrangeObject(cnv, obj, 0);
    }

    static void duplicateSelection(t_canvas* cnv, std::vector<t_gobj*> const& objects)
    {
        glist_noselect(cnv);

        for (auto* obj : objects) {
            glist_select(cnv, obj);
        }

        canvas_setcurrent(cnv);
        pd_typedmess((t_pd*)cnv, gensym("duplicate"), 0, NULL);
        canvas_unsetcurrent(cnv);
    }

    /* save a "root" canvas to a file; cf. canvas_saveto() which saves the
     body (and which is called recursively.) */
    static void saveToFile(t_canvas* cnv, t_symbol* filename, t_symbol* dir)
    {
        t_binbuf* b = binbuf_new();
        canvas_savetemplatesto(cnv, b, 1);
        canvas_saveto(cnv, b);
        errno = 0;
        if (binbuf_write(b, filename->s_name, dir->s_name, 0))
            post("%s/%s: %s", dir->s_name, filename->s_name,
                (errno ? strerror(errno) : "write failed"));
        else {
            /* if not an abstraction, reset title bar and directory */
            if (!cnv->gl_owner) {
                canvas_rename(cnv, filename, dir);
                /* update window list in case Save As changed the window name */
                canvas_updatewindowlist();
            }
            post("saved to: %s/%s", dir->s_name, filename->s_name);
            canvas_dirty(cnv, 0);
        }
        binbuf_free(b);
    }

    static t_gobj* createObject(t_canvas* cnv, t_symbol* s, int argc, t_atom* argv)
    {
        canvas_setcurrent(cnv);
        pd_typedmess((t_pd*)cnv, s, argc, argv);

        canvas_undo_add(cnv, UNDO_CREATE, "create",
            (void*)canvas_undo_set_create(cnv));

        t_gobj* new_object = getNewest(cnv);

        if (new_object) {
            if (pd_class(&new_object->g_pd) == canvas_class)
                canvas_loadbang(reinterpret_cast<t_canvas*>(new_object));
            else if (zgetfn(&new_object->g_pd, gensym("loadbang")))
                vmess(&new_object->g_pd, gensym("loadbang"), "f", LB_LOAD);
        }

        canvas_unsetcurrent(cnv);

        glist_noselect(cnv);
        canvas_dirty(cnv, 1);

        return new_object;
    }
    
    // Can recreate any object of type t_text
    static void recreateTextObject(t_canvas* cnv, t_gobj* obj)
    {
        if(auto* textObject = checkObject(obj))
        {
            char* text = nullptr;
            int len = 0;
            
            getObjectText(textObject, &text, &len);
            renameObject(cnv, obj, text, len);
        }
    }

    static void renameObject(t_canvas* cnv, t_gobj* obj, char const* buf, size_t bufsize)
    {
        struct _fake_rtext {
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

        auto wasEditMode = cnv->gl_edit;
        canvas_editmode(cnv, 1);

        glist_noselect(cnv);
        glist_select(cnv, obj);

        auto* fuddy = reinterpret_cast<_fake_rtext*>(glist_findrtext(cnv, reinterpret_cast<t_text*>(obj)));
        cnv->gl_editor->e_textedfor = reinterpret_cast<t_rtext*>(fuddy);

        fuddy->x_buf = static_cast<char*>(resizebytes(fuddy->x_buf, fuddy->x_bufsize, bufsize));

        strncpy(fuddy->x_buf, buf, bufsize);
        fuddy->x_bufsize = bufsize;

        cnv->gl_editor->e_textdirty = 1;

        glist_deselect(cnv, obj);

        cnv->gl_editor->e_textedfor = 0;
        cnv->gl_editor->e_textdirty = 0;

        canvas_editmode(cnv, wasEditMode);

        canvas_dirty(cnv, 1);
    }

    static int getUndoSize(t_canvas* cnv)
    {
        auto* undo = canvas_undo_get(cnv)->u_queue;

        int count = 0;
        while(undo) {
            count++;
            undo = undo->next;
        }
        return count;
    }

    static int canUndo(t_canvas* cnv)
    {
        t_undo* udo = canvas_undo_get(cnv);

        if (udo && udo->u_last) {
            return strcmp(udo->u_last->name, "no") != 0;
        }

        return 0;
    }

    static int canRedo(t_canvas* cnv)
    {
        t_undo* udo = canvas_undo_get(cnv);

        if (udo && udo->u_last && udo->u_last->next) {
            return strcmp(udo->u_last->next->name, "no") != 0;
        }

        return 0;
    }

    // Can probably be used as a general purpose undo action on an object?
    static void undoApply(t_canvas* cnv, t_gobj* obj)
    {
        canvas_undo_add(cnv, UNDO_APPLY, "props",
            canvas_undo_set_apply(cnv, glist_getindex(cnv, obj)));
    }

    static void moveObject(t_canvas* cnv, t_gobj* obj, int x, int y)
    {
        if (obj->g_pd->c_wb && obj->g_pd->c_wb->w_getrectfn && obj->g_pd->c_wb && obj->g_pd->c_wb->w_displacefn) {

            int x1, y1, x2, y2;

            (*obj->g_pd->c_wb->w_getrectfn)(obj, cnv, &x1, &y1, &x2, &y2);

            (*obj->g_pd->c_wb->w_displacefn)(obj, cnv, x - x1, y - y1);
        }
    }

    static bool canConnect(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
    {
        if (!src || !sink || sink == src) /* do source and sink exist (and are not the same)?*/
            return false;
        if (nin >= obj_ninlets(sink) || (nout >= obj_noutlets(src))) /* do the requested iolets exist? */
            return false;
        if (canvas_isconnected(cnv, src, nout, sink, nin)) /* are the objects already connected? */
            return false;

        return (!obj_issignaloutlet(src, nout) || obj_issignalinlet(sink, nin)); /* are the iolets compatible? */
    }

    static t_outconnect* createConnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
    {
        t_outconnect* oc = tryConnect(cnv, src, nout, sink, nin);
        glist_noselect(cnv);
        return oc;
    }

    static int hasConnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
    {
        return canvas_isconnected(cnv, src, nout, sink, nin);
    }

    static void removeConnection(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin, t_symbol* connection_path)
    {
        if (!pd::Interface::hasConnection(cnv, src, nout, sink, nin)) {
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

    static void getCanvasContent(t_canvas* cnv, char** buf, int* bufsize)
    {
        t_binbuf* b = binbuf_new();

        t_gobj* y;
        t_linetraverser t;
        t_outconnect* oc;

        /* subpatch */
        if (cnv->gl_owner && !cnv->gl_env) {
            /* have to go to original binbuf to find out how we were named. */
            t_binbuf* bz = binbuf_new();
            t_symbol* patchsym;
            binbuf_addbinbuf(bz, cnv->gl_obj.ob_binbuf);
            patchsym = atom_getsymbolarg(1, binbuf_getnatom(bz), binbuf_getvec(bz));
            binbuf_free(bz);
            binbuf_addv(b, "ssiiiisi;", gensym("#N"), gensym("canvas"),
                (int)(cnv->gl_screenx1),
                (int)(cnv->gl_screeny1),
                (int)(cnv->gl_screenx2 - cnv->gl_screenx1),
                (int)(cnv->gl_screeny2 - cnv->gl_screeny1),
                (patchsym != gensym("") ? patchsym : gensym("(subpatch)")),
                cnv->gl_mapped);
        }
        /* root or abstraction */
        else {
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
        while ((oc = linetraverser_next(&t))) {
            int srcno = canvas_getindex(cnv, &t.tr_ob->ob_g);
            int sinkno = canvas_getindex(cnv, &t.tr_ob2->ob_g);
            if (t.outconnect_path_info == gensym("empty")) {
                binbuf_addv(b, "ssiiii;", gensym("#X"), gensym("connect"),
                    srcno, t.tr_outno, sinkno, t.tr_inno);
            } else {
                binbuf_addv(b, "ssiiiis;", gensym("#X"), gensym("connect"),
                    srcno, t.tr_outno, sinkno, t.tr_inno, t.outconnect_path_info);
            }
        }
        /* unless everything is the default (as in ordinary subpatches)
        print out a "coords" message to set up the coordinate systems */
        if (cnv->gl_isgraph || cnv->gl_x1 || cnv->gl_y1 || cnv->gl_x2 != 1.0f || cnv->gl_y2 != 1.0f || cnv->gl_pixwidth || cnv->gl_pixheight) {
            if (cnv->gl_isgraph && cnv->gl_goprect)
                /* if we have a graph-on-parent rectangle, we're new style.
                The format is arranged so
                that old versions of Pd can at least do something with it. */
                binbuf_addv(b, "ssfffffffff;", gensym("#X"), gensym("coords"),
                    cnv->gl_x1, cnv->gl_y1,
                    cnv->gl_x2, cnv->gl_y2,
                    (t_float)cnv->gl_pixwidth, (t_float)cnv->gl_pixheight,
                    (t_float)((cnv->gl_hidetext) ? 2. : 1.),
                    (t_float)cnv->gl_xmargin, (t_float)cnv->gl_ymargin);
            /* otherwise write in 0.38-compatible form */
            else
                binbuf_addv(b, "ssfffffff;", gensym("#X"), gensym("coords"),
                    cnv->gl_x1, cnv->gl_y1,
                    cnv->gl_x2, cnv->gl_y2,
                    (t_float)cnv->gl_pixwidth, (t_float)cnv->gl_pixheight,
                    (t_float)cnv->gl_isgraph);
        }

        binbuf_gettext(b, buf, bufsize);
        binbuf_free(b);
    }

    static int numOutlets(t_object const* x)
    {
        return obj_noutlets(x);
    }

    static int numInlets(t_object const* x)
    {
        return obj_ninlets(x);
    }

    static int isSignalInlet(t_object const* x, int m)
    {
        return obj_issignalinlet(x, m);
    }

    static int isSignalOutlet(t_object const* x, int m)
    {
        return obj_issignaloutlet(x, m);
    }

private:
    static void arrangeObject(t_canvas* cnv, t_gobj* obj, int to_front)
    {
        t_gobj* y_begin = cnv->gl_list;
        t_gobj* y_end = y_begin;

        while (y_end->g_next) {
            y_end = y_end->g_next;
        }

        // This will create an undo action, even if the object is already in the right position
        // Skipping the undo action if the object is alraedy in the right position would be confusing
        canvas_undo_add(cnv, UNDO_ARRANGE, "arrange",
            canvas_undo_set_arrange(cnv, obj, to_front));

        int oldidx = glist_getindex(cnv, obj) - 1;

        // Check for an object before ours
        t_gobj* oldy_prev = nullptr;
        int indx = 0;
        for (oldy_prev = y_begin; oldy_prev; oldy_prev = oldy_prev->g_next, indx++) {
            if (indx == oldidx) {
                break;
            }
        }

        // If there is an object after ours
        t_gobj* oldy_next = obj->g_next ? obj->g_next : NULL;

        if (to_front) {

            // Already in the right place
            if (obj == y_end)
                return;

            // Put the object at the end of the cue
            y_end->g_next = obj;
            obj->g_next = nullptr;

            // now fix links in the hole made in the list due to moving of the oldy
            // (we know there is oldy_next as y_end != oldy in canvas_done_popup)

            if (oldy_prev) // there is indeed more before the oldy position
                oldy_prev->g_next = oldy_next;
            else
                cnv->gl_list = oldy_next;
        } else {

            // Already in the right place
            if (obj == y_begin)
                return;

            // If there is an object after ours
            if (obj->g_next)
                oldy_next = obj->g_next;

            cnv->gl_list = obj;    /* put it to the beginning of the cue */
            obj->g_next = y_begin; /* make it point to the old beginning */

            /* now fix links in the hole made in the list due to moving of the oldy
             * (we know there is oldy_prev as y_begin != oldy in canvas_done_popup)
             */
            if (oldy_prev) {
                if (oldy_next) /* there is indeed more after oldy position */
                    oldy_prev->g_next = oldy_next;
                else
                    oldy_prev->g_next = nullptr; /* oldy was the last in the cue */
            }
        }

        glist_noselect(cnv);
        canvas_dirty(cnv, 1);
    }

    static void arrangeObjectOneStep(t_canvas* cnv, t_gobj* obj, int forward)
    {
        t_gobj* y_begin = cnv->gl_list;
        t_gobj* y_end = y_begin;

        while (y_end->g_next) {
            y_end = y_end->g_next;
        }

        canvas_undo_add(cnv, UNDO_ARRANGE, "arrange",
            canvas_undo_set_arrange(cnv, obj, forward));

        int current_idx = glist_getindex(cnv, obj);

        // Find the previous object
        t_gobj* prev_obj = nullptr;
        int indx = 0;
        for (prev_obj = y_begin; prev_obj; prev_obj = prev_obj->g_next, indx++) {
            if (indx == current_idx - 1) {
                break;
            }
        }

        // If there is an object after ours
        t_gobj* next_obj = obj->g_next ? obj->g_next : nullptr;

        if (forward) {
            // Already at the end
            if (obj == y_end)
                return;

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
            if (!prev_obj)
                return;

            t_gobj* prev_prev = nullptr;
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
                prev_obj->g_next = nullptr;
            }
        }

        glist_noselect(cnv);
        canvas_dirty(cnv, 1);
    }

    static t_outconnect* tryConnect(t_canvas* cnv, t_object* src, int nout, t_object* sink, int nin)
    {
        if (canConnect(cnv, src, nout, sink, nin)) {
            t_outconnect* oc = obj_connect(src, nout, sink, nin);
            if (oc) {

                canvas_undo_add(cnv, UNDO_CONNECT, "connect", canvas_undo_set_connect(cnv, canvas_getindex(cnv, &src->ob_g), nout, canvas_getindex(cnv, &sink->ob_g), nin, gensym("empty")));

                canvas_dirty(cnv, 1);
                return oc;
            }
        }
        return nullptr;
    }

    static void doRemoveSelection(t_canvas* cnv)
    {
        t_gobj *y, *y2;
        int dspstate;

        dspstate = canvas_suspend_dsp();

        /* if text is selected, deselecting it might remake the
         object. So we deselect it and hunt for a "new" object on
         the glist to reselect. */
        if (cnv->gl_editor->e_textedfor) {
            // t_gobj *selwas = x->gl_editor->e_selection->sel_what;
            libpd_this_instance()->pd_newest = 0;
            glist_noselect(cnv);
            if (libpd_this_instance()->pd_newest) {
                for (y = cnv->gl_list; y; y = y->g_next)
                    if (&y->g_pd == libpd_this_instance()->pd_newest)
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
};

} // namespace pd
