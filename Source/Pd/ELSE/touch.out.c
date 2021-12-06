// porres 2018

#include "m_pd.h"
#include <string.h>

typedef struct _touchout{
    t_object  x_ob;
    t_float   x_channel;
    t_float   x_polytouch;
    t_int     x_poly;
}t_touchout;

static t_class *touchout_class;

static void touchout_float(t_touchout *x, t_float f){
    if(f >= 0 && f <= 127){
        t_int channel = (int)x->x_channel;
        if(channel <= 0)
            channel = 1;
        if(x->x_poly){
            if(x->x_polytouch >= 0 && x->x_polytouch <= 127){
                outlet_float(((t_object *)x)->ob_outlet, 160 + ((channel-1) & 0x0F));
                outlet_float(((t_object *)x)->ob_outlet, (t_int)x->x_polytouch);
                outlet_float(((t_object *)x)->ob_outlet, (t_int)f);
            }
        }
        else{
            outlet_float(((t_object *)x)->ob_outlet, 208 + ((channel-1) & 0x0F));
            outlet_float(((t_object *)x)->ob_outlet, (t_int)f);
        }
    }
}

static void *touchout_new(t_symbol *s, t_int ac, t_atom *av){
    t_touchout *x = (t_touchout *)pd_new(touchout_class);
    t_symbol *curarg = NULL;
    curarg = s; // get rid of warning
    t_float channel = 1;
    x->x_poly = 0;
    if(ac){
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                channel = (t_int)atom_getfloatarg(0, ac, av);
                ac--, av++;
            }
            else if(av->a_type == A_SYMBOL){
                curarg = atom_getsymbolarg(0, ac, av);
                if(!strcmp(curarg->s_name, "-poly")){
                    x->x_poly = 1;
                    ac--, av++;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
    }
    x->x_channel = (channel > 0 ? channel : 1);
    if(x->x_poly)
        floatinlet_new((t_object *)x, &x->x_polytouch);
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    return(x);
    errstate:
        pd_error(x, "[touchout]: improper args");
        return NULL;
}

void setup_touch0x2eout(void){
    touchout_class = class_new(gensym("touch.out"), (t_newmethod)touchout_new,
            0, sizeof(t_touchout), 0, A_GIMME, 0);
    class_addfloat(touchout_class, touchout_float);
}
