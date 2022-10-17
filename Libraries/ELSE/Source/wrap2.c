// Porres 2016
 
#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *wrap2_class;

typedef struct _wrap2{
    t_object    x_obj;
    t_outlet   *x_outlet;
    t_float     x_f;
    t_float     x_min;
    t_float     x_max;
}t_wrap2;

static t_float convert(t_float f, t_float min, t_float max){
    float result;
    if(min > max){ // swap values
        float temp;
        temp = max;
        max = min;
        min = temp;
    };
    if(min == max)
        result = min;
    else if(f < max && f >= min)
        result = f; // if f range, = in
    else{ // wrap
        float range = max - min;
        if(f < min){
            result = f;
            while(result < min)
                result += range;
        }
        else
            result = fmod(f - min, range) + min;
    }
    return(result);
}

static void wrap2_list(t_wrap2 *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(!ac){
        outlet_float(x->x_outlet, convert(x->x_f, x->x_min, x->x_max));
        return;
    }
    if(ac == 1){
        outlet_float(x->x_outlet, convert(x->x_f = atom_getfloat(av), x->x_min, x->x_max));
        return;
    }
    t_atom* at = calloc(ac, sizeof(t_atom));
    for(int i = 0; i < ac; i++)
        SETFLOAT(at+i, convert(atom_getfloatarg(i, ac, av), x->x_min, x->x_max));
    outlet_list(x->x_outlet, 0, ac, at);
    free(at);
}

static void wrap2_set(t_wrap2 *x, t_float f){
    x->x_f = f;
}

static void *wrap2_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_wrap2 *x = (t_wrap2 *) pd_new(wrap2_class);
///////////////////////////
    x->x_min = 0.;
    x-> x_max = 1.;
    if(ac == 1){
        if(av -> a_type == A_FLOAT){
            x->x_min = 0;
            x->x_max = atom_getfloat(av);
        }
        else
            goto errstate;
    }
    else if(ac == 2){
        int numargs = 0;
        while(ac > 0 ){
            if(av -> a_type == A_FLOAT)
            { // if nullpointer, should be float or int
                switch(numargs){
                    case 0: x->x_min = atom_getfloatarg(0, ac, av);
                        numargs++;
                        ac--;
                        av++;
                        break;
                    case 1: x->x_max = atom_getfloatarg(0, ac, av);
                        numargs++;
                        ac--;
                        av++;
                        break;
                    default:
                        ac--;
                        av++;
                        break;
                };
            }
            else // not a float
                goto errstate;
        };
    }
    else if(ac > 2)
        goto errstate;
///////////////////////////
    floatinlet_new((t_object *)x, &x->x_min);
    floatinlet_new((t_object *)x, &x->x_max);
    x->x_outlet = outlet_new(&x->x_obj, 0);
    return(x);
errstate:
    pd_error(x, "[wrap2]: improper args");
    return(NULL);
}

void wrap2_setup(void){
    wrap2_class = class_new(gensym("wrap2"), (t_newmethod)wrap2_new, 0,
        sizeof(t_wrap2),0, A_GIMME, 0);
    class_addlist(wrap2_class,(t_method)wrap2_list);
    class_addmethod(wrap2_class,(t_method)wrap2_set,gensym("set"),A_DEFFLOAT,0);
}
