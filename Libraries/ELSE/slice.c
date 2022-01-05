// porres

#include "m_pd.h"
#include <math.h>

typedef struct _slice{
    t_object    x_obj;
    float       x_n;
    t_outlet   *x_out1;
    t_outlet   *x_out2;
}t_slice;

static t_class *slice_class;

static void slice_list(t_slice *x, t_symbol *s, int ac, t_atom *av){
    int n = (int)x->x_n;
    if(n == 0)
        outlet_anything(x->x_out1, s, ac, av);
    else if(n > 0){
        if(n >= ac)
            outlet_anything(x->x_out1, s, ac, av);
        else{
            outlet_anything(x->x_out2, s, ac-n, av+n);
            outlet_anything(x->x_out1, s, n, av);
        }
    }
    else{ // n < 0
        n = n * -1;
        if(n >= ac)
            outlet_anything(x->x_out2, s, ac, av);
        else{
            outlet_anything(x->x_out2, s, n, av+(ac-n));
            outlet_anything(x->x_out1, s, ac-n, av);
        }
    }
}

static void *slice_new(t_floatarg f){
    t_slice *x = (t_slice *)pd_new(slice_class);
    x->x_n = f;
    floatinlet_new(&x->x_obj, &x->x_n);
    x->x_out1  = outlet_new(&x->x_obj, &s_list);
    x->x_out2  = outlet_new(&x->x_obj, &s_list);
    return(x);
}

void slice_setup(void){
    slice_class = class_new(gensym("slice"), (t_newmethod)slice_new,
        0, sizeof(t_slice), 0, A_DEFFLOAT, 0);
    class_addlist(slice_class, slice_list);
}
