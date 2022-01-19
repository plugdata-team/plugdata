/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>
#include <m_imp.h>
#include <g_canvas.h>
#include <g_all_guis.h>
#include <g_undo.h>

#include <stdlib.h>

#include <string.h>

#include "x_libpd_multi.h"
#include "x_libpd_extra_utils.h"
#include "x_libpd_mod_utils.h"

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

extern void* canvas_undo_set_disconnect(t_canvas *x, int index1, int outno, int index2, int inno);
extern int glist_getindex(t_glist *x, t_gobj *y);
extern int glist_selectionindex(t_glist *x, t_gobj *y, int selected);
extern void glist_deselectline(t_glist *x);


/* displace the selection by (dx, dy) pixels */
void libpd_moveselection(t_canvas *x, int dx, int dy)
{
    EDITOR->canvas_undo_already_set_move = 0;
    
    t_selection *y;
    int resortin = 0, resortout = 0;
    if (!EDITOR->canvas_undo_already_set_move)
    {
        canvas_undo_add(x, UNDO_MOTION, "motion", canvas_undo_set_move(x, 1));
        //EDITOR->canvas_undo_already_set_move = 1;
    }
    for (y = x->gl_editor->e_selection; y; y = y->sel_next)
    {

        t_class *cl = pd_class(&y->sel_what->g_pd);
        gobj_displace(y->sel_what, x, dx, dy);
        if (cl == vinlet_class) resortin = 1;
        else if (cl == voutlet_class) resortout = 1;
    }
    if (resortin) canvas_resortinlets(x);
    if (resortout) canvas_resortoutlets(x);
    sys_vgui("pdtk_canvas_getscroll .x%lx.c\n", x);
    
    if (x->gl_editor->e_selection)
        canvas_dirty(x, 1);
    
}


t_pd* libpd_newest(t_canvas* cnv)
{
    // Regular pd_newest won't work because it doesn't get assigned for some gui components
    t_gobj* y;
    
    // Get to the last object
    for(y = cnv->gl_list; y && y->g_next; y = y->g_next)
    {
    }
    
    if(y) {
        return &y->g_pd;
    }
    
    return 0;
}




void libpd_canvas_doclear(t_canvas *x)
{
    

    
    t_gobj *y, *y2;
    int dspstate;
    
    dspstate = canvas_suspend_dsp();
    
    /* don't clear connections, call removeconnection instead
    if (x->gl_editor && x->gl_editor->e_selectedline)
    {
        canvas_disconnect(x,
                          x->gl_editor->e_selectline_index1,
                          x->gl_editor->e_selectline_outno,
                          x->gl_editor->e_selectline_index2,
                          x->gl_editor->e_selectline_inno);
        x->gl_editor->e_selectedline=0;
    } */
    
    /* if text is selected, deselecting it might remake the
     object. So we deselect it and hunt for a "new" object on
     the glist to reselect. */
    if (x->gl_editor->e_textedfor)
    {
        //t_gobj *selwas = x->gl_editor->e_selection->sel_what;
        pd_this->pd_newest = 0;
        glist_noselect(x);
        if (pd_this->pd_newest)
        {
            for (y = x->gl_list; y; y = y->g_next)
                if (&y->g_pd == pd_this->pd_newest) glist_select(x, y);
        }
    }
    while (1)   /* this is pretty weird...  should rewrite it */
    {
        for (y = x->gl_list; y; y = y2)
        {
            y2 = y->g_next;
            if (glist_isselected(x, y))
            {
                glist_delete(x, y);
                goto next;
            }
        }
        goto restore;
    next: ;
    }
restore:
    canvas_resume_dsp(dspstate);
    canvas_dirty(x, 1);
}

void libpd_removeselection(t_canvas* x)
{
    canvas_undo_add(x, UNDO_SEQUENCE_START, "clear", 0);
    
    canvas_undo_add(x, UNDO_CUT, "clear",
                    canvas_undo_set_cut(x, 2));
    
    libpd_canvas_doclear(x);
    
    canvas_undo_add(x, UNDO_SEQUENCE_END, "clear", 0);
}

void canvas_savedeclarationsto(t_canvas *x, t_binbuf *b);


static int binbuf_nextmess(int argc, const t_atom *argv)
{
    int i=0;
    while(argc--)
    {
        argv++;
        i++;
        if (A_SEMI == argv->a_type) {
            return i+1;
        }
    }
    return i;
}

int libpd_canconnect(t_canvas*x, t_object*src, int nout, t_object*sink, int nin)
{
    if (!src || !sink || sink == src) /* do source and sink exist (and are not the same)?*/
        return 0;
    if (nin >= obj_ninlets(sink) || (nout >= obj_noutlets(src))) /* do the requested iolets exist? */
        return 0;
    if (canvas_isconnected(x, src, nout, sink, nin)) /* are the objects already connected? */
        return 0;
    return (!obj_issignaloutlet(src, nout) || /* are the iolets compatible? */
            obj_issignalinlet(sink, nin));
}

int libpd_tryconnect(t_canvas*x, t_object*src, int nout, t_object*sink, int nin)
{
    if(libpd_canconnect(x, src, nout, sink, nin))
    {
        t_outconnect *oc = obj_connect(src, nout, sink, nin);
        if(oc)
        {
            int iow = IOWIDTH * x->gl_zoom;
            int iom = IOMIDDLE * x->gl_zoom;
            int x11=0, x12=0, x21=0, x22=0;
            int y11=0, y12=0, y21=0, y22=0;
            int noutlets1, ninlets, lx1, ly1, lx2, ly2;
            gobj_getrect(&src->ob_g, x, &x11, &y11, &x12, &y12);
            gobj_getrect(&sink->ob_g, x, &x21, &y21, &x22, &y22);
            
            noutlets1 = obj_noutlets(src);
            ninlets = obj_ninlets(sink);
            
            lx1 = x11 + (noutlets1 > 1 ?
                         ((x12-x11-iow) * nout)/(noutlets1-1) : 0)
            + iom;
            ly1 = y12;
            lx2 = x21 + (ninlets > 1 ?
                         ((x22-x21-iow) * nin)/(ninlets-1) : 0)
            + iom;
            ly2 = y21;
            sys_vgui(
                     ".x%lx.c create line %d %d %d %d -width %d -tags [list l%lx cord]\n",
                     glist_getcanvas(x),
                     lx1, ly1, lx2, ly2,
                     (obj_issignaloutlet(src, nout) ? 2 : 1) *
                     x->gl_zoom,
                     oc);
            canvas_undo_add(x, UNDO_CONNECT, "connect", canvas_undo_set_connect(x,
                                                                                canvas_getindex(x, &src->ob_g), nout,
                                                                                canvas_getindex(x, &sink->ob_g), nin));
            canvas_dirty(x, 1);
            return 1;
        }
    }
    return 0;
}

static int binbuf_getpos(t_binbuf*b, int *x0, int *y0, t_symbol**type)
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
    t_atom*argv = binbuf_getvec(b);
    int argc = binbuf_getnatom(b);
    const int argc0 = argc;
    int count = 0;
    t_symbol*s;
    /* get the position of the first object in the argv binbuf */
    if(argc > 2
       && atom_getsymbol(argv+0) == &s__N
       && atom_getsymbol(argv+1) == gensym("canvas"))
    {
        int ac = argc;
        t_atom*ap = argv;
        int stack = 0;
        do {
            int off = binbuf_nextmess(argc, argv);
            if(!off)
                break;
            ac = argc;
            ap = argv;
            argc-=off;
            argv+=off;
            count+=off;
            if(off >= 2)
            {
                if (atom_getsymbol(ap+1) == gensym("restore")
                    && atom_getsymbol(ap) == &s__X)
                {
                    stack--;
                }
                if (atom_getsymbol(ap+1) == gensym("canvas")
                    && atom_getsymbol(ap) == &s__N)
                {
                    stack++;
                }
            }
            if(argc<0)
                return 0;
        } while (stack>0);
        argc = ac;
        argv = ap;
    }
    if(argc < 4 || atom_getsymbol(argv) != &s__X)
        return 0;
    /* #X obj|msg|text|floatatom|symbolatom <x> <y> ...
     * TODO: subpatches #N canvas + #X restore <x> <y>
     */
    s = atom_getsymbol(argv+1);
    if(gensym("restore") == s
       || gensym("obj") == s
       || gensym("msg") == s
       || gensym("text") == s
       || gensym("floatatom") == s
       || gensym("symbolatom") == s)
    {
        if(x0)*x0=atom_getfloat(argv+2);
        if(y0)*y0=atom_getfloat(argv+3);
        if(type)*type=s;
    } else
        return 0;
    
    /* no wind the binbuf to the next message */
    while(argc--)
    {
        count++;
        if (A_SEMI == argv->a_type)
            break;
        argv++;
    }
    return 1+(argc0 > count);
}


void libpd_copy(t_canvas* x){
    pd_typedmess((t_pd*)x, gensym("copy"), 0, NULL);
}

void libpd_paste(t_canvas* x) {
    pd_typedmess((t_pd*)x, gensym("paste"), 0, NULL);
}


void libpd_undo(t_canvas *x) {
    pd_typedmess((t_pd*)x, gensym("undo"), 0, NULL);
}

void libpd_redo(t_canvas *x) {
    // Temporary fix... might cause us to miss a loadbang when recreating a canvas
    pd_this->pd_newest = 0;
    if(!x->gl_editor) return;
    
    pd_typedmess((t_pd*)x, gensym("redo"), 0, NULL);
}


void libpd_duplicate(t_canvas *x)
{
    pd_typedmess((t_pd*)x, gensym("duplicate"), 0, NULL);
}


void libpd_canvas_saveto(t_canvas *x, t_binbuf *b)
{
    t_gobj *y;
    t_linetraverser t;
    t_outconnect *oc;
    
    // subpatch
    if (x->gl_owner && !x->gl_env)
    {
        // have to go to original binbuf to find out how we were named.
        t_binbuf *bz = binbuf_new();
        t_symbol *patchsym;
        binbuf_addbinbuf(bz, x->gl_obj.ob_binbuf);
        patchsym = atom_getsymbolarg(1, binbuf_getnatom(bz), binbuf_getvec(bz));
        binbuf_free(bz);
        binbuf_addv(b, "ssiiiisi;", gensym("#N"), gensym("canvas"),
                    (int)(x->gl_screenx1),
                    (int)(x->gl_screeny1),
                    (int)(x->gl_screenx2 - x->gl_screenx1),
                    (int)(x->gl_screeny2 - x->gl_screeny1),
                    (patchsym != &s_ ? patchsym: gensym("(subpatch)")),
                    x->gl_mapped);
    }
    // root or abstraction
    else
    {
        binbuf_addv(b, "ssiiiii;", gensym("#N"), gensym("canvas"),
                    (int)(x->gl_screenx1),
                    (int)(x->gl_screeny1),
                    (int)(x->gl_screenx2 - x->gl_screenx1),
                    (int)(x->gl_screeny2 - x->gl_screeny1),
                    (int)x->gl_font);
        canvas_savedeclarationsto(x, b);
    }
    for (y = x->gl_list; y; y = y->g_next)
        gobj_save(y, b);
    
    linetraverser_start(&t, x);
    while ((oc = linetraverser_next(&t)))
    {
        int srcno = canvas_getindex(x, &t.tr_ob->ob_g);
        int sinkno = canvas_getindex(x, &t.tr_ob2->ob_g);
        binbuf_addv(b, "ssiiii;", gensym("#X"), gensym("connect"),
                    srcno, t.tr_outno, sinkno, t.tr_inno);
    }
    // unless everything is the default (as in ordinary subpatches)
    // print out a "coords" message to set up the coordinate systems
    if (x->gl_isgraph || x->gl_x1 || x->gl_y1 ||
        x->gl_x2 != 1 ||  x->gl_y2 != 1 || x->gl_pixwidth || x->gl_pixheight)
    {
        if (x->gl_isgraph && x->gl_goprect)
            // if we have a graph-on-parent rectangle, we're new style.
            // The format is arranged so
            // that old versions of Pd can at least do something with it.
            binbuf_addv(b, "ssfffffffff;", gensym("#X"), gensym("coords"),
                        x->gl_x1, x->gl_y1,
                        x->gl_x2, x->gl_y2,
                        (t_float)x->gl_pixwidth, (t_float)x->gl_pixheight,
                        (t_float)((x->gl_hidetext)?2.:1.),
                        (t_float)x->gl_xmargin, (t_float)x->gl_ymargin);
        // otherwise write in 0.38-compatible form
        else binbuf_addv(b, "ssfffffff;", gensym("#X"), gensym("coords"),
                         x->gl_x1, x->gl_y1,
                         x->gl_x2, x->gl_y2,
                         (t_float)x->gl_pixwidth, (t_float)x->gl_pixheight,
                         (t_float)x->gl_isgraph);
    }
}



t_pd* libpd_creategraphonparent(t_canvas* cnv, int x, int y) {
    int argc = 9;
    
    t_atom argv[9];
    
    t_float x1 = 0.0f;
    t_float y1 = 0.0f;
    t_float x2 = x + 100.0f;
    t_float y2 = y + 100.0f;
    t_float px1 = x;
    t_float py1 = y;
    t_float px2 = 100.0f;
    t_float py2 = 100.0f;
    
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
    
    
    
    pd_typedmess((t_pd*)cnv, gensym("graph"), argc, argv);
    
    
    glist_noselect(cnv);
    
    return libpd_newest(cnv);
}

t_pd* libpd_creategraph(t_canvas* cnv, const char* name, int size, int x, int y) {
    
    int argc = 4;
    
    t_atom argv[4];
    
    SETSYMBOL(argv, gensym(name));
    
    // Set position
    SETFLOAT(argv + 1, size);
    SETFLOAT(argv + 2, x);
    SETFLOAT(argv + 3, y);
    
    
    pd_typedmess((t_pd*)cnv, gensym("arraydialog"), argc, argv);
    
    //canvas_undo_add(cnv, UNDO_CREATE, "create",
    //                (void *)canvas_undo_set_create(cnv));
    
    glist_noselect(cnv);
    
    
    return libpd_newest(cnv);
}

t_pd* libpd_createobj(t_canvas* cnv, t_symbol *s, int argc, t_atom *argv) {
        
    sys_lock();
    pd_typedmess((t_pd*)cnv, s, argc, argv);
    sys_unlock();
    
    // Needed here but not for graphs??
    canvas_undo_add(cnv, UNDO_CREATE, "create",
                    (void *)canvas_undo_set_create(cnv));
    
    glist_noselect(cnv);
    
    pd_this->pd_islocked = 0;
    
    return libpd_newest(cnv);
    
}

void libpd_removeobj(t_canvas* cnv, t_gobj* obj)
{
    
    glist_noselect(cnv);
    glist_select(cnv, obj);
    libpd_canvas_doclear(cnv);
    
    
    glist_noselect(cnv);
}

/* recursively deselect everything in a gobj "g", if it happens to be
   a glist, in preparation for deselecting g itself in glist_dselect() */
static void glist_checkanddeselectall(t_glist *gl, t_gobj *g)
{
t_glist *gl2;
t_gobj *g2;
if (pd_class(&g->g_pd) != canvas_class)
    return;
gl2 = (t_glist *)g;
for (g2 = gl2->gl_list; g2; g2 = g2->g_next)
    glist_checkanddeselectall(gl2, g2);
glist_noselect(gl2);
}


struct _rtext
{
    char *x_buf;    /*-- raw byte string, assumed UTF-8 encoded (moo) --*/
    int x_bufsize;  /*-- byte length --*/
    int x_selstart; /*-- byte offset --*/
    int x_selend;   /*-- byte offset --*/
    int x_active;
    int x_dragfrom;
    int x_height;
    int x_drawnwidth;
    int x_drawnheight;
    t_text *x_text;
    t_glist *x_glist;
    char x_tag[50];
    struct _rtext *x_next;
};

void libpd_renameobj(t_canvas* cnv, t_gobj* obj, const char* buf, int bufsize)
{
    sys_lock();
    canvas_editmode(cnv, 1);
    
    glist_noselect(cnv);
    glist_select(cnv, obj);
    
    canvas_create_editor(cnv);
    
    t_rtext *fuddy = glist_findrtext(cnv, (t_text *)obj);
    cnv->gl_editor->e_textedfor = fuddy;
    
    cnv->gl_editor->e_rtext->x_buf = strdup(buf);
    cnv->gl_editor->e_rtext->x_bufsize = bufsize + 1;
    
    cnv->gl_editor->e_textdirty = 1;
    
    glist_deselect(cnv, &((t_text *)obj)->te_g);
    
    cnv->gl_editor->e_textedfor = 0;
    cnv->gl_editor->e_textdirty = 0;
    
    canvas_destroy_editor(cnv);
    
    canvas_editmode(cnv, 0);
    sys_unlock();

}


int libpd_can_undo(t_canvas* cnv) {
    t_undo* udo = canvas_undo_get(cnv);
    
    if(udo && udo->u_last) {
        return strcmp(udo->u_last->name, "no");
    }
    
    return 0;
}

int libpd_can_redo(t_canvas* cnv) {
    
    t_undo* udo = canvas_undo_get(cnv);
    
    if(udo && udo->u_last && udo->u_last->next) {
        return strcmp(udo->u_last->next->name, "no");
    }
    
    return 0;
}

    
void libpd_moveobj(t_canvas* cnv, t_gobj* obj, int x, int y)
{
    glist_noselect(cnv);
    glist_select(cnv, obj);
    
    int x1, y1, x2, y2;
    (*obj->g_pd->c_wb->w_getrectfn)(obj, cnv, &x1, &y1, &x2, &y2);
    
    libpd_moveselection(cnv, x - x1, y - y1);
    glist_noselect(cnv);
}


void libpd_createconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin)
{
    libpd_tryconnect(cnv, src, nout, sink, nin);
    glist_noselect(cnv);
}

/* ------- specific undo methods: 1. connect -------- */
typedef struct _undo_connect
{
    int u_index1;
    int u_outletno;
    int u_index2;
    int u_inletno;
} t_undo_connect;



void libpd_removeconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin)
{
    obj_disconnect(src, nout, sink, nin);
    
    int dest_i = canvas_getindex(cnv, &(sink->te_g));
    int src_i = canvas_getindex(cnv, &(src->te_g));
    
    canvas_undo_add(cnv, UNDO_DISCONNECT, "disconnect", canvas_undo_set_disconnect(cnv,
                                                                                   src_i, nout, dest_i, nin));
    glist_noselect(cnv);
}


void libpd_getcontent(t_canvas* cnv, char** buf, int* bufsize) {
    t_binbuf* b = binbuf_new();
    libpd_canvas_saveto(cnv, b);
    binbuf_gettext(b, buf, bufsize);
    
}

typedef t_pd *(*t_newgimme)(t_symbol *s, int argc, t_atom *argv);
typedef void(*t_messgimme)(t_pd *x, t_symbol *s, int argc, t_atom *argv);

typedef t_pd *(*t_fun0)(
                        t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun1)(t_int i1,
                        t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun2)(t_int i1, t_int i2,
                        t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun3)(t_int i1, t_int i2, t_int i3,
                        t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun4)(t_int i1, t_int i2, t_int i3, t_int i4,
                        t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun5)(t_int i1, t_int i2, t_int i3, t_int i4, t_int i5,
                        t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);
typedef t_pd *(*t_fun6)(t_int i1, t_int i2, t_int i3, t_int i4, t_int i5, t_int i6,
                        t_floatarg d1, t_floatarg d2, t_floatarg d3, t_floatarg d4, t_floatarg d5);


int libpd_type_exists(const char* type) {
    int i;
    t_class* c = pd_objectmaker;
    t_methodentry *mlist, *m;
    
#ifdef PDINSTANCE
    mlist = c->c_methods[pd_this->pd_instanceno];
#else
    mlist = c->c_methods;
#endif
    
    for (i = c->c_nmethod, m = mlist; i--; m++)
        if (m->me_name == gensym(type))
        {
            return 1;
        }
    
    return 0;
}


//typedef struct _outlet t_outlet;

// Some duplicates and modifications of pure-data functions
// We do this so we can keep pure-data and libpd intact and easily updatable


struct _outlet
{
    t_object *o_owner;
    struct _outlet *o_next;
    t_outconnect *o_connections;
    t_symbol *o_sym;
};

union inletunion
{
    t_symbol *iu_symto;
    t_gpointer *iu_pointerslot;
    t_float *iu_floatslot;
    t_symbol **iu_symslot;
    t_float iu_floatsignalvalue;
};

struct _inlet
{
    t_pd i_pd;
    struct _inlet *i_next;
    t_object *i_owner;
    t_pd *i_dest;
    t_symbol *i_symfrom;
    union inletunion i_un;
};


int libpd_noutlets(const t_object *x)
{
    int n;
    t_outlet *o;
    for (o = x->ob_outlet, n = 0; o; o = o->o_next) n++;
    return (n);
}

int libpd_ninlets(const t_object *x)
{
    int n;
    t_inlet *i;
    for (i = x->ob_inlet, n = 0; i; i = i->i_next) n++;
    if (x->ob_pd->c_firstin) n++;
    return (n);
}

#define s_anything  (pd_this->pd_s_anything)
#define s_signal    (pd_this->pd_s_signal)

int libpd_issignalinlet(const t_object *x, int m)
{
    t_inlet *i;
    if (x->ob_pd->c_firstin)
    {
        if (!m)
            return (x->ob_pd->c_firstin && x->ob_pd->c_floatsignalin);
        else m--;
    }
    for (i = x->ob_inlet; i && m; i = i->i_next, m--)
        ;
    
    
    return (i && (i->i_symfrom == &s_signal));
    
}

int libpd_issignaloutlet(const t_object *x, int m)
{
    int n;
    t_outlet *o2;
    for (o2 = x->ob_outlet, n = 0; o2 && m--; o2 = o2->o_next);
    return (o2 && (o2->o_sym == &s_signal));
}

