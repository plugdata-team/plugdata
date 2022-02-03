/* Copyright (c) 2002-2005 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* This is a modified version of Joseph A. Sarlo's code.
   The most important changes are listed in "pd-lib-notes.txt" file.  */

#include "m_pd.h"
#include <common/api.h>

#define BANGBANG_MINOUTS      1
#define BANGBANG_C74MAXOUTS  40  // CHECKED (just clipped without warning)
#define BANGBANG_DEFOUTS      2

typedef struct _bangbang{
    t_object    x_ob;
    int         x_nouts;
    t_outlet  **x_outs;
    t_outlet   *x_outbuf[BANGBANG_DEFOUTS];
}t_bangbang;

static t_class *bangbang_class;

static void bangbang_bang(t_bangbang *x){
    int i = x->x_nouts;
    while(i--)
        outlet_bang(x->x_outs[i]);
}

static void bangbang_anything(t_bangbang *x, t_symbol *s, int ac, t_atom *av){
    bangbang_bang(x);
}

static void bangbang_free(t_bangbang *x){
    if (x->x_outs != x->x_outbuf)
        freebytes(x->x_outs, x->x_nouts * sizeof(*x->x_outs));
}

static void *bangbang_new(t_floatarg f){
    t_bangbang *x;
    int i, nouts = (int)f;
    t_outlet **outs;
    if(nouts < BANGBANG_MINOUTS)
        nouts = BANGBANG_DEFOUTS;
    if(nouts > BANGBANG_C74MAXOUTS)
        nouts = BANGBANG_C74MAXOUTS;
    if(nouts > BANGBANG_DEFOUTS){
        if (!(outs = (t_outlet **)getbytes(nouts * sizeof(*outs))))
            return (0);
    }
    else
        outs = 0;
    x = (t_bangbang *)pd_new(bangbang_class);
    x->x_nouts = nouts;
    x->x_outs = (outs ? outs : x->x_outbuf);
    for (i = 0; i < nouts; i++)
        x->x_outs[i] = outlet_new((t_object *)x, &s_bang);
    return (x);
}

CYCLONE_OBJ_API void bangbang_setup(void){
    bangbang_class = class_new(gensym("bangbang"), (t_newmethod)bangbang_new,
        (t_method)bangbang_free, sizeof(t_bangbang), 0, A_DEFFLOAT, 0);
    class_addbang(bangbang_class, bangbang_bang);
    class_addanything(bangbang_class, bangbang_anything);
}
