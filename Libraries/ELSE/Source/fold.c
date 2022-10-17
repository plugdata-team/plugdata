// Porres 2016
 
#include "m_pd.h"
#include <stdlib.h>

static t_class *fold_class;

typedef struct _fold{
    t_object    x_obj;
    t_outlet   *x_outlet;
    t_float     x_f;
    t_float     x_min;
    t_float     x_max;
}t_fold;

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
    else if(f <= max && f >= min)
        result = f; // if f range, = in
    else{ // folding
        float range = max - min;
        if(f < min){
            float diff = min - f; // positive diff between f and min
            int mag = (int)(diff/range); // f is > range from min
            if(mag % 2 == 0)
            { // even # of ranges away = counting up from min
                diff = diff - ((float)mag * range);
                result = diff + min;
            }
            else{ // odd # of ranges away = counting down from max
                diff = diff - ((float)mag * range);
                result = max - diff;
            };
        }
        else{ // f > max
            float diff = f - max; // positive diff between f and max
            int mag  = (int)(diff/range); // f is > range from max
            if(mag % 2 == 0){ // even # of ranges away = counting down from max
                diff = diff - ((float)mag * range);
                result = max - diff;
            }
            else{ // odd # of ranges away = counting up from min
                diff = diff - ((float)mag * range);
                result = diff + min;
            };
        };
    }
    return(result);
}

static void fold_list(t_fold *x, t_symbol *s, int ac, t_atom *av){
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

static void fold_set(t_fold *x, t_float f){
    x->x_f = f;
}

static void *fold_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_fold *x = (t_fold *) pd_new(fold_class);
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
    pd_error(x, "[fold]: improper args");
    return(NULL);
}

void fold_setup(void){
    fold_class = class_new(gensym("fold"), (t_newmethod)fold_new, 0,
        sizeof(t_fold),0, A_GIMME, 0);
    class_addlist(fold_class,(t_method)fold_list);
    class_addmethod(fold_class,(t_method)fold_set,gensym("set"),A_DEFFLOAT,0);
}
