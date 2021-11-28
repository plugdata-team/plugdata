/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>
#include <m_imp.h>
#include <g_canvas.h>
#include <g_all_guis.h>
#include "x_libpd_multi.h"
#include "x_libpd_extra_utils.h"


void gobj_setposition(t_gobj *x, t_glist *glist, int xpos, int ypos)
{
    if (x->g_pd->c_wb && x->g_pd->c_wb->w_getrectfn && x->g_pd->c_wb && x->g_pd->c_wb->w_displacefn) {
        
        int x1, y1, x2, y2;

        (*x->g_pd->c_wb->w_getrectfn)(x, glist, &x1, &y1, &x2, &y2);
        
        (*x->g_pd->c_wb->w_displacefn)(x, glist, xpos - x1, ypos - y1);
        
    }
    

        
}

void libpd_canvas_doclear(t_canvas *x)
{
    t_gobj *y, *y2;
    int dspstate;

    dspstate = canvas_suspend_dsp();
    if (x->gl_editor->e_selectedline)
    {
        canvas_disconnect(x,
            x->gl_editor->e_selectline_index1,
            x->gl_editor->e_selectline_outno,
            x->gl_editor->e_selectline_index2,
            x->gl_editor->e_selectline_inno);
        x->gl_editor->e_selectedline=0;
    }
        /* if text is selected, deselecting it might remake the
           object. So we deselect it and hunt for a "new" object on
           the glist to reselect. */
    if (x->gl_editor->e_textedfor)
    {
        t_gobj *selwas = x->gl_editor->e_selection->sel_what;
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

void canvas_savedeclarationsto(t_canvas *x, t_binbuf *b);

static void libpd_canvas_saveto(t_canvas *x, t_binbuf *b)
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
            //canvas_undo_add(x, UNDO_CONNECT, "connect", canvas_undo_set_connect(x,
           //         canvas_getindex(x, &src->ob_g), nout,
            //        canvas_getindex(x, &sink->ob_g), nin));
            canvas_dirty(x, 1);
            return 1;
        }
    }
    return 0;
}
    
// TODO: this is stupid. move stuff from PdInstance to here eventually

t_pd* libpd_creategraphonparent(t_pd *x, int argc, t_atom *argv) {
        
    pd_typedmess(x, gensym("graph"), argc, argv);
    glist_noselect(x);
    return pd_newest();
}
    
t_pd* libpd_creategraph(t_pd *x, int argc, t_atom *argv) {
        
    pd_typedmess(x, gensym("arraydialog"), argc, argv);
    glist_noselect(x);
    return pd_newest();
}

t_pd* libpd_createobj(t_pd *x, t_symbol *s, int argc, t_atom *argv) {
    pd_typedmess(x, s, argc, argv);
    glist_noselect(x);
    return pd_newest();
    
}

void libpd_removeobj(t_canvas* cnv, t_gobj* obj)
{
    glist_select(cnv, obj);
    libpd_canvas_doclear(cnv);
    glist_noselect(cnv);
}


void libpd_renameobj(t_canvas* cnv, t_gobj* obj, const char* buf, int bufsize)
{
    glist_noselect(cnv);
    glist_select(cnv, obj);
    canvas_stowconnections(cnv); // for restoring connections when possible!
    text_setto((t_text *)obj, cnv, buf, bufsize);
    glist_noselect(cnv);
}

void libpd_moveobj(t_canvas* cnv, t_gobj* obj, int x, int y)
{
    gobj_setposition(obj, cnv,  x, y);
    glist_noselect(cnv);
}


void libpd_createconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin)
{
    libpd_tryconnect(cnv, src, nout, sink, nin);
    glist_noselect(cnv);
}

void libpd_removeconnection(t_canvas* cnv, t_object*src, int nout, t_object*sink, int nin)
{
    obj_disconnect(src, nout, sink, nin);
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

