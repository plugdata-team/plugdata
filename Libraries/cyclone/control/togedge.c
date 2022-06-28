/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _togedge{
    t_object   x_ob;
    int        x_wason;
    t_outlet  *x_out1;
}t_togedge;

static t_class *togedge_class;

static void togedge_bang(t_togedge *x){
    if (x->x_wason){
        x->x_wason = 0;
        outlet_bang(x->x_out1);
    }
    else{
        x->x_wason = 1;
        outlet_bang(((t_object *)x)->ob_outlet);
    }
}

static void togedge_float(t_togedge *x, t_float f){
    if((int)f != f){
        pd_error(x, "[togedge]: doesn't deal with non integer floats");
    }
    else{
        int i = f != 0;
        if(x->x_wason){
            if (!i){
                x->x_wason = 0;
                outlet_bang(x->x_out1);
            }
        }
        else{
            if(i){
                x->x_wason = 1;
                outlet_bang(((t_object *)x)->ob_outlet);
            }
        }
    }
}

static void *togedge_new(void){
    t_togedge *x = (t_togedge *)pd_new(togedge_class);
    x->x_wason = 0;  /* CHECKED */
    outlet_new((t_object *)x, &s_bang);
    x->x_out1 = outlet_new((t_object *)x, &s_bang);
    return (x);
}

CYCLONE_OBJ_API void togedge_setup(void){
    togedge_class = class_new(gensym("togedge"),
        (t_newmethod)togedge_new, 0, sizeof(t_togedge), 0, 0);
    class_addbang(togedge_class, togedge_bang);
    class_addfloat(togedge_class, togedge_float);
}

CYCLONE_OBJ_API void TogEdge_setup(void){
    togedge_class = class_new(gensym("TogEdge"),
        (t_newmethod)togedge_new, 0, sizeof(t_togedge), 0, 0);
    class_addbang(togedge_class, togedge_bang);
    class_addfloat(togedge_class, togedge_float);
    pd_error(togedge_class, "Cyclone: please use [togedge] instead of [TogEdge] to suppress this error");
    class_sethelpsymbol(togedge_class, gensym("togedge"));
}
