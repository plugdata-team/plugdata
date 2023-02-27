

#include "m_pd.h"
#include <math.h>

static t_class *common_div_class;

typedef struct _common_div{
    t_object    x_obj;
    long        x_left;
    long        x_right;
    long        x_output;
    t_outlet   *x_outlet;
}t_common_div;

long calculate(long a, long b){
    if(b == 0)
        return 0;
    if(a % b != 0)
        return calculate(b, a % b);
    else
        return fabs((float)b);
}

static void common_div_bang(t_common_div *x){
    outlet_float(x->x_outlet, (t_float)x->x_output);
}

static void common_div_float(t_common_div *x, t_floatarg f){
    x->x_left = (long)f;
    x->x_output = calculate(x->x_left, x->x_right);
    common_div_bang(x);
}

static void common_div_ft1(t_common_div *x, t_floatarg f){
    x->x_right = (long)f;
}

static void common_div_list(t_common_div *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    long a = 0, b = 0;
    a = atom_getfloat(av);
    for(int i = 1; i < ac; i++){
        b = atom_getfloat(av+i);
        a = calculate(a, b);
    }
    x->x_output = a;
    common_div_bang(x);
}

static void *common_div_new(void){
    t_common_div *x= (t_common_div *)pd_new(common_div_class);
    x->x_left = x->x_right = 0;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    x->x_outlet = outlet_new(&x->x_obj, gensym("float"));
    return(x);
}

void setup_common0x2ediv(void){
    common_div_class = class_new(gensym("common.div"), (t_newmethod)common_div_new, 0, sizeof(t_common_div), 0, 0);
    class_addbang(common_div_class, (t_method)common_div_bang);
    class_addfloat(common_div_class, common_div_float);
    class_addmethod(common_div_class, (t_method)common_div_ft1, gensym("ft1"), A_DEFFLOAT, 0);
    class_addlist(common_div_class, common_div_list);
}
