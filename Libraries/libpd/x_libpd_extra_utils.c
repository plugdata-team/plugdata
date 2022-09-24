/*
 // Copyright (c) 2015-2018 Pierre Guillot.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <string.h>
#include <stdlib.h>

#include <m_pd.h>
#include <m_imp.h>
#include <g_canvas.h>
#include <s_stuff.h>
#include <m_imp.h>
#include <g_all_guis.h>
#include "x_libpd_multi.h"

// False GARRAY
typedef struct _fake_garray {
    t_gobj x_gobj;
    t_scalar* x_scalar;
    t_glist* x_glist;
    t_symbol* x_name;
    t_symbol* x_realname;
    unsigned int x_usedindsp : 1;   /* 1 if some DSP routine is using this */
    unsigned int x_saveit : 1;      /* we should save this with parent */
    unsigned int x_savesize : 1;    /* save size too */
    unsigned int x_listviewing : 1; /* list view window is open */
    unsigned int x_hidename : 1;    /* don't print name above graph */
    unsigned int x_edit : 1;        /* we can edit the array */
} t_fake_garray;

typedef struct _gatom {
    t_text a_text;
    int a_flavor;          /* A_FLOAT, A_SYMBOL, or A_LIST */
    t_glist* a_glist;      /* owning glist */
    t_float a_toggle;      /* value to toggle to */
    t_float a_draghi;      /* high end of drag range */
    t_float a_draglo;      /* low end of drag range */
    t_symbol* a_label;     /* symbol to show as label next to box */
    t_symbol* a_symfrom;   /* "receive" name -- bind ourselves to this */
    t_symbol* a_symto;     /* "send" name -- send to this on output */
    t_binbuf* a_revertbuf; /* binbuf to revert to if typing canceled */
    int a_dragindex;       /* index of atom being dragged */
    int a_fontsize;
    unsigned int a_shift : 1;         /* was shift key down when drag started? */
    unsigned int a_wherelabel : 2;    /* 0-3 for left, right, above, below */
    unsigned int a_grabbed : 1;       /* 1 if we've grabbed keyboard */
    unsigned int a_doubleclicked : 1; /* 1 if dragging from a double click */
    t_symbol* a_expanded_to;          /* a_symto after $0, $1, ...  expansion */
} t_fake_gatom;

void* libpd_create_canvas(char const* name, char const* path)
{
    t_canvas* cnv = (t_canvas*)libpd_openfile(name, path);
    if (cnv) {
        canvas_vis(cnv, 1.f);
        canvas_rename(cnv, gensym(name), gensym(path));
    }
    return cnv;
}

char const* libpd_get_object_class_name(void* ptr)
{
    return class_getname(pd_class((t_pd*)ptr));
}

void libpd_get_object_text(void* ptr, char** text, int* size)
{
    *text = NULL;
    *size = 0;
    binbuf_gettext(((t_text*)ptr)->te_binbuf, text, size);
}

void libpd_get_object_bounds(void* patch, void* ptr, int* x, int* y, int* w, int* h)
{
    t_canvas* cnv = patch;
    while (cnv->gl_owner && !cnv->gl_havewindow && cnv->gl_isgraph)
        cnv = cnv->gl_owner;

    *x = 0;
    *y = 0;
    *w = 0;
    *h = 0;

    gobj_getrect((t_gobj*)ptr, cnv, x, y, w, h);

    *w -= *x;
    *h -= *y;
}

t_garray* libpd_array_get_byname(char const* name)
{
    return (t_fake_garray*)pd_findbyclass(gensym((char*)name), garray_class);
}

int libpd_array_get_saveit(void* garray)
{
    return ((t_fake_garray*)garray)->x_saveit;
}

int libpd_array_get_size(void* garray)
{
    return garray_getarray(garray)->a_n;
}

char const* libpd_array_get_name(void* array)
{
    t_fake_garray* nptr = (t_fake_garray*)array;
    return nptr->x_realname->s_name;
}
char const* libpd_array_get_unexpanded_name(void* array)
{
    t_fake_garray* nptr = (t_fake_garray*)array;
    return nptr->x_name->s_name;
}

void libpd_array_get_scale(void* array, float* min, float* max)
{
    t_canvas const* cnv;
    if (array) {
        cnv = ((t_fake_garray*)array)->x_glist;
        if (cnv) {
            *min = cnv->gl_y2;
            *max = cnv->gl_y1;
            return;
        }
    }
    *min = -1;
    *max = 1;
}

void libpd_array_set_scale(void* array, float min, float max)
{
    t_canvas* cnv;
    if (array) {
        cnv = ((t_fake_garray*)array)->x_glist;
        if (cnv) {
            cnv->gl_y2 = min;
            cnv->gl_y1 = max;
            return;
        }
    }
}

int libpd_array_get_style(void* array)
{
    t_fake_garray* arr = (t_fake_garray*)array;
    if (arr && arr->x_scalar) {
        t_scalar* scalar = arr->x_scalar;
        t_template* scalartplte = template_findbyname(scalar->sc_template);
        if (scalartplte) {
            return (int)template_getfloat(scalartplte, gensym("style"), scalar->sc_vec, 0);
        }
    }
    return 0;
}

unsigned int convert_from_iem_color(int const color)
{
    unsigned int const c = (unsigned int)(color << 8 | 0xFF);
    return ((0xFF << 24) | ((c >> 24) << 16) | ((c >> 16) << 8) | (c >> 8));
}

unsigned int convert_to_iem_color(char const* hex)
{
    if (strlen(hex) == 8)
        hex += 2; // remove alpha channel if needed
    int col = (int)strtol(hex, 0, 16);
    return col & 0xFFFFFF;
}

unsigned int libpd_iemgui_get_background_color(void* ptr)
{
    return convert_from_iem_color(((t_iemgui*)ptr)->x_bcol);
}

unsigned int libpd_iemgui_get_foreground_color(void* ptr)
{
    return convert_from_iem_color(((t_iemgui*)ptr)->x_fcol);
}

unsigned int libpd_iemgui_get_label_color(void* ptr)
{
    return convert_from_iem_color(((t_iemgui*)ptr)->x_lcol);
}

void libpd_iemgui_set_background_color(void* ptr, char const* hex)
{
    ((t_iemgui*)ptr)->x_bcol = convert_to_iem_color(hex);
}

void libpd_iemgui_set_foreground_color(void* ptr, char const* hex)
{
    ((t_iemgui*)ptr)->x_fcol = convert_to_iem_color(hex);
}

void libpd_iemgui_set_label_color(void* ptr, char const* hex)
{
    ((t_iemgui*)ptr)->x_lcol = convert_to_iem_color(hex);
}

float libpd_get_canvas_font_height(t_canvas* cnv)
{
    int const fontsize = glist_getfont(cnv);
    float const zoom = (float)glist_getzoom(cnv);
    //[8 :8.31571] [10 :9.9651] [12 :11.6403] [16 :16.6228] [24 :23.0142] [36 :36.0032]
    if (fontsize == 8) {
        return 8.31571 * zoom; // 9.68f * zoom;
    } else if (fontsize == 10) {
        return 9.9651 * zoom; // 11.6f * zoom;
    } else if (fontsize == 12) {
        return 11.6403 * zoom; // 13.55f * zoom;
    } else if (fontsize == 16) {
        return 16.6228 * zoom; // 19.35f * zoom;
    } else if (fontsize == 24) {
        return 23.0142 * zoom; // 26.79f * zoom;
    } else if (fontsize == 36) {
        return 36.0032 * zoom; // 41.91f * zoom;
    }
    return glist_fontheight(cnv);
}

int libpd_array_resize(void* garray, long size)
{
    sys_lock();
    garray_resize_long(garray, size);
    sys_unlock();
    return 0;
}

#define MEMCPY(_x, _y)                                              \
    if (n < 0 || offset < 0 || offset + n > garray_npoints(garray)) \
        return -2;                                                  \
    t_word* vec = ((t_word*)garray_vec(garray)) + offset;           \
    int i;                                                          \
    for (i = 0; i < n; i++)                                         \
        _x = _y;

int libpd_array_read(float* dest, void* garray, int offset, int n)
{
    sys_lock();
    MEMCPY(*dest++, (vec++)->w_float)
    sys_unlock();
    return 0;
}

int libpd_array_write(void* garray, int offset, float const* src, int n)
{
    sys_lock();
    MEMCPY((vec++)->w_float, *src++)
    sys_unlock();
    return 0;
}

#define PROCESS_NODSP()                                      \
    size_t n_in = STUFF->st_inchannels * DEFDACBLKSIZE;      \
    size_t n_out = STUFF->st_outchannels * DEFDACBLKSIZE;    \
    t_sample* p;                                             \
    size_t i;                                                \
    sys_lock();                                              \
    sys_pollgui();                                           \
    for (p = STUFF->st_soundin, i = 0; i < n_in; i++) {      \
        *p++ = 0.0;                                          \
    }                                                        \
    memset(STUFF->st_soundout, 0, n_out * sizeof(t_sample)); \
    sched_tick_nodsp();                                      \
    sys_unlock();                                            \
    return 0;

#define TIMEUNITPERMSEC (32. * 441.)
#define TIMEUNITPERSECOND (TIMEUNITPERMSEC * 1000.)
#define SYSTIMEPERTICK \
    ((STUFF->st_schedblocksize / STUFF->st_dacsr) * TIMEUNITPERSECOND)

struct _clock {
    double c_settime; // in TIMEUNITS; <0 if unset
    void* c_owner;
    void (*c_fn)(void*); // clock fn
    struct _clock* c_next;
    t_float c_unit; // >0 if in TIMEUNITS; <0 if in samples
};

static void sched_tick_nodsp(void)
{
    double next_sys_time = pd_this->pd_systime + SYSTIMEPERTICK;
    int countdown = 5000;
    while (pd_this->pd_clock_setlist && pd_this->pd_clock_setlist->c_settime < next_sys_time) {
        t_clock* c = pd_this->pd_clock_setlist;
        pd_this->pd_systime = c->c_settime;
        clock_unset(pd_this->pd_clock_setlist);
        outlet_setstacklim();
        (*c->c_fn)(c->c_owner);
        if (!countdown--) {
            countdown = 5000;
            (void)sys_pollgui();
        }
    }
    pd_this->pd_systime = next_sys_time;
}

int libpd_process_nodsp(void)
{
    PROCESS_NODSP()
}
