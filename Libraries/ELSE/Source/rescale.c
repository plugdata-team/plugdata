// Porres 2016
 
#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *rescale_class;

typedef struct _rescale{
    t_object    x_obj;
    t_outlet   *x_outlet;
    int         x_clip;
    float       x_f;
    float       x_minin;
    float       x_maxin;
    float       x_minout;
    float       x_maxout;
    float       x_exp;
}t_rescale;

static void rescale_clip(t_rescale *x, t_floatarg f){
    x->x_clip = f != 0;
}

static float convert(t_rescale *x, float f){
    float minin = x->x_minin;
    float minout = x->x_minout;
    float rangein = x->x_maxin - minin;
    float rangeout = x->x_maxout - minout;
    float exp = x->x_exp;
    if(f == minin)
        return(minout);
    if(f == x->x_maxin)
        return(x->x_maxout);
    if(x->x_clip){
        if(f < minin)
            return(minout);
        else if(f > x->x_maxin)
            return(x->x_maxout);
    }
    if(fabs(exp) == 1) // linear
        return(minout + rangeout * (f-minin)/rangein);
    else if(exp >= 0){ // exponential
        float p = (f-minin)/rangein;
        return(minout + rangeout * copysign(pow(fabs(p), exp), p));
    }
    else{ // negative exponential
        float p = 1 - ((f-minin)/rangein);
        return(minout + (rangeout * (1 - copysign(pow(fabs(p), -exp), p))));
    }
}

static void rescale_list(t_rescale *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 0)
        outlet_float(x->x_outlet, convert(x, x->x_f));
    else if(ac == 1)
        outlet_float(x->x_obj.ob_outlet, convert(x, atom_getfloat(av)));
    else{
        t_atom* at = calloc(ac, sizeof(t_atom));
        for(int i = 0; i < ac; i++)
            SETFLOAT(at+i, convert(x, atom_getfloatarg(i, ac, av)));
        outlet_list(x->x_obj.ob_outlet, &s_list, ac, at);
        free(at);
    }
}

static void rescale_set(t_rescale *x, t_floatarg f){
    x->x_f = f;
}

static void rescale_exp(t_rescale *x, t_floatarg f){
    x->x_exp = f;
}

static void *rescale_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_rescale *x = (t_rescale *)pd_new(rescale_class);
    x->x_minin = 0;
    x->x_maxin = 127;
    x->x_minout = 0;
    x->x_maxout = 1;
    x->x_exp = 1.f;
    t_int numargs = 0;
    if(ac > 0){
        if(av->a_type == A_SYMBOL){
            if(atom_getsymbolarg(0, ac, av) == gensym("-clip") && !numargs)
                x->x_clip = 1;
            else
                goto errstate;
            ac--, av++;
        }
        if(ac <= 3){
            while(ac){
                if(av->a_type == A_FLOAT){
                    float argval = atom_getfloatarg(0, ac, av);
                    switch(numargs){
                        case 0:
                            x->x_minout = argval;
                            break;
                        case 1:
                            x->x_maxout = argval;
                            break;
                        case 2:
                            x->x_exp = argval < 0 ? 0 : argval;
                            break;
                        default:
                            break;
                    };
                }
                else
                    goto errstate;
                numargs++;
                ac--, av++;
            }
        }
        else if(ac <= 5){
            while(ac){
                if(av->a_type == A_FLOAT){
                    float argval = atom_getfloatarg(0, ac, av);
                    switch(numargs){
                        case 0:
                            x->x_minin = argval;
                            break;
                        case 1:
                            x->x_maxin = argval;
                            break;
                        case 2:
                            x->x_minout = argval;
                            break;
                        case 3:
                            x->x_maxout = argval;
                            break;
                        case 4:
                            x->x_exp = argval;
                            break;
                        default:
                            break;
                    };
                    numargs++;
                    ac--, av++;
                }
                else
                    goto errstate;
            }
        }
        else
            goto errstate;
    }
    x->x_outlet = outlet_new(&x->x_obj, 0);
    if(numargs <= 3){
        floatinlet_new(&x->x_obj, &x->x_minout);
        floatinlet_new(&x->x_obj, &x->x_maxout);
    }
    else{
        floatinlet_new(&x->x_obj, &x->x_minin);
        floatinlet_new(&x->x_obj, &x->x_maxin);
        floatinlet_new(&x->x_obj, &x->x_minout);
        floatinlet_new(&x->x_obj, &x->x_maxout);
    }
    return(x);
errstate:
    post("[rescale]: improper args");
    return(NULL);
}

void rescale_setup(void){
    rescale_class = class_new(gensym("rescale"), (t_newmethod)rescale_new, 0,
        sizeof(t_rescale), 0, A_GIMME, 0);
    class_addlist(rescale_class, (t_method)rescale_list);
    class_addmethod(rescale_class, (t_method)rescale_set, gensym("set"), A_DEFFLOAT,0 );
    class_addmethod(rescale_class, (t_method)rescale_exp, gensym("exp"), A_DEFFLOAT, 0);
    class_addmethod(rescale_class, (t_method)rescale_clip, gensym("clip"), A_FLOAT, 0);
}
