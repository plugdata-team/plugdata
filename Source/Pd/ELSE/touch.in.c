// porres 2018

#include "m_pd.h"
#include <string.h>

typedef struct _touchin{
    t_object       x_ob;
    t_int          x_omni;
    t_int          x_poly;
    t_float        x_ch;
    t_float        x_ch_in;
    unsigned char  x_pressure;
    unsigned char  x_ready;
    unsigned char  x_atouch;
    unsigned char  x_channel;
    t_outlet      *x_chanout;
    t_outlet      *x_pressout;
}t_touchin;

static t_class *touchin_class;

static void touchin_float(t_touchin *x, t_float f){
    if(f < 0 || f > 256){
        x->x_atouch = 0;
        return;
    }
    else{
        t_int ch = (t_int)x->x_ch_in;
        if(ch != x->x_ch && ch >= 0 && ch <= 16)
            x->x_omni = ((x->x_ch = (t_int)ch) == 0);
        unsigned char val = (int)f;
        if(val & 0x80){ // message type > 128)
            x->x_ready = 0;
            if(x->x_poly && (x->x_atouch = ((val & 0xF0) == 0xA0))) // if poly atouch
                x->x_channel = (val & 0x0F) + 1; // get channel
            else if((x->x_atouch = ((val & 0xF0) == 0xD0))) // if channel atouch
                x->x_channel = (val & 0x0F) + 1; // get channel
        }
        else if(x->x_atouch && val < 128){
            if(x->x_omni){
                outlet_float(x->x_chanout, x->x_channel);
                if(x->x_poly){
                    if(!x->x_ready){
                        x->x_pressure = val;
                        x->x_ready = 1;
                    }
                    else{ // ready
                        outlet_float(x->x_pressout, x->x_pressure);
                        outlet_float(((t_object *)x)->ob_outlet, val);
                        x->x_atouch = x->x_ready = 0;
                    }
                }
                else
                    outlet_float(((t_object *)x)->ob_outlet, val);
            }
            else if(x->x_ch == x->x_channel){
                if(x->x_poly){
                    if(!x->x_ready){
                        x->x_pressure = val;
                        x->x_ready = 1;
                    }
                    else{
                        outlet_float(x->x_pressout, x->x_pressure);
                        outlet_float(((t_object *)x)->ob_outlet, val);
                        x->x_atouch = x->x_ready = 0;
                    }
                }
                else
                    outlet_float(((t_object *)x)->ob_outlet, val);
            }
        }
        else
            x->x_atouch = x->x_ready = 0;
    }
}

static void *touchin_new(t_symbol *s, t_int ac, t_atom *av){
    t_touchin *x = (t_touchin *)pd_new(touchin_class);
    t_symbol *curarg = s; // get rid of warning
    x->x_atouch = x->x_poly =  x->x_ready = x->x_pressure = 0;
    t_float channel = 0;
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
    if(channel < 0)
        channel = 0;
    if(channel > 16)
        channel = 16;
    x->x_omni = (channel == 0);
    x->x_ch = x->x_ch_in = channel;
    floatinlet_new((t_object *)x, &x->x_ch_in);
    outlet_new((t_object *)x, &s_float);
    if(x->x_poly)
        x->x_pressout = outlet_new((t_object *)x, &s_float);
    x->x_chanout = outlet_new((t_object *)x, &s_float);
    return(x);
    errstate:
        pd_error(x, "[touchin]: improper args");
        return NULL;
}

void setup_touch0x2ein(void){
    touchin_class = class_new(gensym("touch.in"), (t_newmethod)touchin_new,
        0, sizeof(t_touchin), 0, A_GIMME, 0);
    class_addfloat(touchin_class, touchin_float);
}
