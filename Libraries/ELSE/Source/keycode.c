// seb shader 2023

#include "m_pd.h"

// common mapping for every platform, from https://github.com/depp/keycode 
#ifdef __APPLE__

static const unsigned char KEYCODE_TO_HID[128] = {
    4,22,7,9,11,10,29,27,6,25,0,5,20,26,8,21,28,23,30,31,32,33,35,34,46,38,36,
    45,37,39,48,18,24,47,12,19,158,15,13,52,14,51,49,54,56,17,16,55,43,44,53,42,
    0,41,231,227,225,57,226,224,229,230,228,0,108,220,0,85,0,87,0,216,0,0,127,
    84,88,0,86,109,110,103,98,89,90,91,92,93,94,95,111,96,97,0,0,0,62,63,64,60,
    65,66,0,68,0,104,107,105,0,67,0,69,0,106,117,74,75,76,61,77,59,78,58,80,79,
    81,82,0
};

static unsigned keycode_to_hid(unsigned scancode) {
    if(scancode >= 128)
        return 0;
    return KEYCODE_TO_HID[scancode];
}

#elif defined _WIN32

#include <windows.h>

static const unsigned char KEYCODE_TO_HID[256] = {
    0,41,30,31,32,33,34,35,36,37,38,39,45,46,42,43,20,26,8,21,23,28,24,12,18,19,
    47,48,158,224,4,22,7,9,10,11,13,14,15,51,52,53,225,49,29,27,6,25,5,17,16,54,
    55,56,229,0,226,0,57,58,59,60,61,62,63,64,65,66,67,72,71,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,68,69,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,228,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,70,230,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,74,82,75,0,80,0,79,0,77,81,78,73,76,0,0,0,0,0,0,0,227,231,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static HKL layout;

static unsigned keycode_to_hid(unsigned scancode){
    scancode = MapVirtualKeyEx(scancode, MAPVK_VK_TO_VSC, layout);
    if(scancode >= 256)
        return(0);
    return(KEYCODE_TO_HID[scancode]);
}

#else

static const unsigned char KEYCODE_TO_HID[256] = {
    0,41,30,31,32,33,34,35,36,37,38,39,45,46,42,43,20,26,8,21,23,28,24,12,18,19,
    47,48,158,224,4,22,7,9,10,11,13,14,15,51,52,53,225,49,29,27,6,25,5,17,16,54,
    55,56,229,85,226,44,57,58,59,60,61,62,63,64,65,66,67,83,71,95,96,97,86,92,
    93,94,87,89,90,91,98,99,0,0,100,68,69,0,0,0,0,0,0,0,88,228,84,154,230,0,74,
    82,75,80,79,77,81,78,73,76,0,0,0,0,0,103,0,72,0,0,0,0,0,227,231,0,0,0,0,0,0,
    0,0,0,0,0,0,118,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,104,105,106,107,108,109,110,111,112,113,114,115,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static unsigned keycode_to_hid(unsigned scancode){
    // xorg differs from kernel values
    scancode -= 8;
    if(scancode >= 256)
        return 0;
    return KEYCODE_TO_HID[scancode];
}

#endif

static t_class *keycode_class;

typedef struct _keycode{
    t_object x_obj;
    t_outlet *x_outlet1;
    t_outlet *x_outlet2;
}t_keycode;

// keep our own list of objects
typedef struct _listelem{
    t_pd *e_who;
    struct _listelem *e_next;
}t_listelem;

typedef struct _objectlist{
    t_pd b_pd;
    t_listelem *b_list;
}t_objectlist;

static t_objectlist *object_list;

static void object_list_bind(t_pd *x) {
    t_listelem *e = (t_listelem *)getbytes(sizeof(t_listelem));
    e->e_next = object_list->b_list;
    e->e_who = x;
    object_list->b_list = e;
}

static void object_list_unbind(t_pd *x) {
    t_listelem *e, *e2;
    if((e = object_list->b_list)->e_who == x){
        object_list->b_list = e->e_next;
        freebytes(e, sizeof(t_listelem));
        return;
    }
    for (;(e2 = e->e_next); e = e2)
        if (e2->e_who == x){
            e->e_next = e2->e_next;
            freebytes(e2, sizeof(t_listelem));
            return;
        }
}

static void *keycode_new(void){
    t_keycode *x = (t_keycode *)pd_new(keycode_class);
    x->x_outlet1 = outlet_new(&x->x_obj, &s_float);
    x->x_outlet2 = outlet_new(&x->x_obj, &s_float);
    object_list_bind(&x->x_obj.ob_pd);
    return(x);
}

static void keycode_list(t_keycode *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    outlet_float(x->x_outlet2, atom_getfloatarg(1, ac, av));
    outlet_float(x->x_outlet1, atom_getfloatarg(0, ac, av));
}

static void object_list_iterate(t_objectlist *x, t_symbol *s, int argc, t_atom *argv) {
    t_listelem *e;
    if (argc < 2){
        pd_error(0, "keycode: not enough args");
        return;
    }
    argv[1].a_w.w_float = keycode_to_hid(argv[1].a_w.w_float);
    for(e = x->b_list; e; e = e->e_next)
        pd_list(e->e_who, s, argc, argv);
}

static void keycode_free(t_keycode *x){
    object_list_unbind(&x->x_obj.ob_pd);
}

void keycode_setup(void){
    // since it's a singleton, I won't keep track of the class
    t_class *objectlist_class;
    keycode_class = class_new(gensym("keycode"), (t_newmethod)keycode_new,
        (t_method)keycode_free, sizeof(t_keycode), CLASS_NOINLET, 0);
    class_addlist(keycode_class, keycode_list);
    // we have already been called from a different path
    if (object_list) return;
    objectlist_class = class_new(NULL, 0, 0, sizeof(t_objectlist), CLASS_PD, 0);
    class_addlist(objectlist_class, object_list_iterate);
    object_list = (t_objectlist *)pd_new(objectlist_class);
    object_list->b_list = NULL;
    pd_bind(&object_list->b_pd, gensym("#keycode"));
    // Tk stores the actual code in the high byte on MacOs, hopefully bitfield
    // layout doesn't change too much based on compiler or something
    #ifdef __APPLE__
    sys_vgui("bind all <KeyPress> {+ pdsend \"#keycode 1 [expr %%k >> 24]\"}\n");
    sys_vgui("bind all <KeyRelease> {+ pdsend \"#keycode 0 [expr %%k >> 24]\"}\n");
    #else
    #ifdef _WIN32
    layout = GetKeyboardLayout(0);
    #endif
    sys_vgui("bind all <KeyPress> {+ pdsend \"#keycode 1 %%k\"}\n");
    sys_vgui("bind all <KeyRelease> {+ pdsend \"#keycode 0 %%k\"}\n");
    #endif
}

