/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>

typedef struct _peak{
    t_object   x_ob;
    t_float    x_value;
    t_outlet  *x_out2;
    t_outlet  *x_out3;
}t_peak;

static t_class *peak_class;

static void peak_bang(t_peak *x){
    outlet_float(((t_object *)x)->ob_outlet, x->x_value);
}

static void peak_ft1(t_peak *x, t_floatarg f){
    outlet_float(x->x_out3, 0);
    outlet_float(x->x_out2, 1);
    outlet_float(((t_object *)x)->ob_outlet, x->x_value = f);
}

static void peak_float(t_peak *x, t_float f){
    if(f > x->x_value)
        peak_ft1(x, f);
    else{
        outlet_float(x->x_out3, 1);
        outlet_float(x->x_out2, 0);
    }
}

static void *peak_new(t_symbol *s, int argc, t_atom *argv){
    t_peak *x = (t_peak *)pd_new(peak_class);
    t_float f1 = 0;
    if(argc)
      f1 = atom_getfloatarg(0, argc, argv);
    x->x_value = f1;
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft1"));
    outlet_new((t_object *)x, &s_float);
    x->x_out2 = outlet_new((t_object *)x, &s_float);
    x->x_out3 = outlet_new((t_object *)x, &s_float);
    return (x);
}

CYCLONE_OBJ_API void peak_setup(void){
    peak_class = class_new(gensym("peak"), (t_newmethod)peak_new, 0,
			   sizeof(t_peak), 0, A_GIMME, 0);
    class_addbang(peak_class, peak_bang);
    class_addfloat(peak_class, peak_float);
    class_addmethod(peak_class, (t_method)peak_ft1, gensym("ft1"), A_FLOAT, 0);
}

CYCLONE_OBJ_API void Peak_setup(void){
    peak_class = class_new(gensym("Peak"), (t_newmethod)peak_new, 0,
        sizeof(t_peak), 0, A_GIMME, 0);
    class_addbang(peak_class, peak_bang);
    class_addfloat(peak_class, peak_float);
    class_addmethod(peak_class, (t_method)peak_ft1, gensym("ft1"), A_FLOAT, 0);
    pd_error(peak_class, "Cyclone: please use [peak] instead of [Peak] to suppress this error");
    class_sethelpsymbol(peak_class, gensym("peak"));
}
