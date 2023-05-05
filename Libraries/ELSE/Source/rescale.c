// Porres 2016 - 2023
 
#include "m_pd.h"
#include <math.h>
#include <stdlib.h>

static t_class *rescale_class;

typedef struct _rescale{
    t_object    x_obj;
    t_outlet   *x_outlet;
    int         x_clip;
    int         x_log;
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
    float maxout = x->x_maxout;
    float rangein = x->x_maxin - minin;
    float rangeout = maxout - minout;
    if(f == minin)
        return(minout);
    if(f == x->x_maxin)
        return(maxout);
    if(x->x_clip){
        if(rangeout < 0){
            if(f > minin)
                return(minout);
            else if(f < x->x_maxin)
                return(maxout);
        }
        else{
            if(f < minin)
                return(minout);
            else if(f > x->x_maxin)
                return(maxout);
        }
    }
    float p = (f-minin)/rangein; // position
    if(x->x_log){ // 'log'
        if((minout <= 0 && maxout >= 0) || (minout >= 0 && maxout <= 0)){
            pd_error(x, "[rescale]: output range cannot contain '0' in log mode");
            return(0);
        }
        return(exp(p * log(maxout / minout)) * minout);
    }
    if(fabs(x->x_exp) == 1 || x->x_exp == 0) // linear
        return(minout + rangeout * p);
    if(x->x_exp > 0) // exponential
        return(pow(p, x->x_exp) * rangeout + minout);
    else // negative exponential
        return((1 - pow(1-p, -x->x_exp)) * rangeout + minout);
}

static void rescale_list(t_rescale *x, t_symbol *s, int ac, t_atom *av){
    s = NULL;
    if(ac == 0)
        post("[rescale]: no method for bang");
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

static void rescale_in(t_rescale *x, t_floatarg f1, t_floatarg f2){
    x->x_minin = f1;
    x->x_maxin = f2;
}

static void rescale_exp(t_rescale *x, t_floatarg f){
    x->x_exp = f;
}

static void rescale_log(t_rescale *x, t_floatarg f){
    x->x_log = (f != 0);
}

static void *rescale_new(t_symbol *s, int ac, t_atom *av){
    s = NULL;
    t_rescale *x = (t_rescale *)pd_new(rescale_class);
    x->x_minin = 0;
    x->x_maxin = 127;
    x->x_minout = 0;
    x->x_maxout = 1;
    x->x_exp = 0.f;
    x->x_log = 0;
    x->x_clip = 1;
    t_int numargs = 0;
    while(ac){
        if(av->a_type == A_SYMBOL){
            t_symbol *sym = atom_getsymbol(av);
            if(sym == gensym("-noclip") && !numargs)
                x->x_clip = 0;
            else if(sym == gensym("-log") && !numargs)
                x->x_log = 1;
            else if(ac >= 2 && sym == gensym("-exp") && !numargs){
                ac--, av++;
                x->x_exp = atom_getfloat(av);
            }
            else if(ac >= 3 && sym == gensym("-in") && !numargs){
                ac--, av++;
                x->x_minin = atom_getfloat(av);
                ac--, av++;
                x->x_maxin = atom_getfloat(av);
            }
            else
                goto errstate;
            ac--, av++;
        }
        else{
            switch(numargs){
                case 0:
                    x->x_minout = atom_getfloat(av);
                    break;
                case 1:
                    x->x_maxout = atom_getfloat(av);
                    break;
                case 2:
                    x->x_exp = atom_getfloat(av);
                    break;
                default:
                    break;
            };
            numargs++;
            ac--, av++;
        }
    }
    floatinlet_new(&x->x_obj, &x->x_minout);
    floatinlet_new(&x->x_obj, &x->x_maxout);
    x->x_outlet = outlet_new(&x->x_obj, 0);
    return(x);
errstate:
    post("[rescale]: improper args");
    return(NULL);
}

void rescale_setup(void){
    rescale_class = class_new(gensym("rescale"), (t_newmethod)rescale_new, 0,
        sizeof(t_rescale), 0, A_GIMME, 0);
    class_addlist(rescale_class, (t_method)rescale_list);
    class_addmethod(rescale_class, (t_method)rescale_in, gensym("in"), A_FLOAT, A_FLOAT, 0);
    class_addmethod(rescale_class, (t_method)rescale_exp, gensym("exp"), A_FLOAT, 0);
    class_addmethod(rescale_class, (t_method)rescale_log, gensym("log"), A_FLOAT, 0);
    class_addmethod(rescale_class, (t_method)rescale_clip, gensym("clip"), A_FLOAT, 0);
}
