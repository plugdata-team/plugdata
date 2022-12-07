/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _trough{
    t_object   x_ob;
    t_float    x_value;
    t_outlet  *x_out2;
    t_outlet  *x_out3;
}t_trough;

static t_class *trough_class;

static void trough_bang(t_trough *x){
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void trough_ft1(t_trough *x, t_floatarg f){
    outlet_float(x->x_out3, 0);
    outlet_float(x->x_out2, 1);
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = f);
}

static void trough_float(t_trough *x, t_float f){
    if (f < x->x_value)
        trough_ft1(x, f);
    else{
        outlet_float(x->x_out3, 1);
        outlet_float(x->x_out2, 0);
    }
}

static void *trough_new(t_symbol *s, int argc, t_atom *argv){
    t_trough *x = (t_trough *)pd_new(trough_class);
    t_float f1 = 128;
    if(argc)
      f1 = atom_getfloatarg(0, argc, argv);
    x->x_value = f1;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    x->x_out3 = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void trough_setup(void){
    trough_class = class_new(gensym("trough"), (t_newmethod)trough_new, 0,
        sizeof(t_trough), 0, A_GIMME, 0);
    class_addbang(trough_class, trough_bang);
    class_addfloat(trough_class, trough_float);
    class_addmethod(trough_class, (t_method)trough_ft1,
        gensym("ft1"), A_FLOAT, 0);
}

CYCLONE_OBJ_API void Trough_setup(void){
    trough_class = class_new(gensym("Trough"), (t_newmethod)trough_new, 0,
        sizeof(t_trough), 0, A_GIMME, 0);
    class_addbang(trough_class, trough_bang);
    class_addfloat(trough_class, trough_float);
    class_addmethod(trough_class, (t_method)trough_ft1,
        gensym("ft1"), A_FLOAT, 0);
    pd_error(trough_class, "Cyclone: please use [trough] instead of [Trough] to suppress this error");
    class_sethelpsymbol(trough_class, gensym("trough"));
}
