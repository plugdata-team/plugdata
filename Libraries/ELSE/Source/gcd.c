// porres

#include "m_pd.h"
#include <math.h>

static t_class *gcd_class;

typedef struct _gcd{
    t_object    x_obj;
    float        x_right;
    float        x_output;
    t_outlet   *x_outlet;
}t_gcd;

float gcd_calculate(float f1, float f2){
    long a = (long)f1, b = (long)f2;
    if(a  == 0 || b == 0)
        return(1);
    if(a % b != 0)
        return(gcd_calculate(b, a % b));
    else
        return(fabs((float)b));
}

static void gcd_bang(t_gcd *x){
    outlet_float(x->x_outlet, (t_float)x->x_output);
}

static void gcd_float(t_gcd *x, t_floatarg f){
    x->x_output = gcd_calculate(f, x->x_right);
    gcd_bang(x);
}

static void gcd_list(t_gcd *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    float a = 0, b = 0;
    a = atom_getfloat(av);
    for(int i = 1; i < ac; i++){
        b = atom_getfloat(av+i);
        a = gcd_calculate(a, b);
    }
    x->x_output = a;
    gcd_bang(x);
}

static void *gcd_new(t_floatarg f){
    t_gcd *x= (t_gcd *)pd_new(gcd_class);
    x->x_right = f;
    floatinlet_new(&x->x_obj, &x->x_right);
    x->x_outlet = outlet_new(&x->x_obj, gensym("float"));
    return(x);
}

void gcd_setup(void){
    gcd_class = class_new(gensym("gcd"), (t_newmethod)gcd_new, 0, sizeof(t_gcd), 0, A_DEFFLOAT, 0);
    class_addbang(gcd_class, (t_method)gcd_bang);
    class_addfloat(gcd_class, gcd_float);
    class_addlist(gcd_class, gcd_list);
}
