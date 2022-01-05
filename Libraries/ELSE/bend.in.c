// porres 2018

#include "m_pd.h"
#include <string.h>

typedef struct _bendin{
    t_object       x_ob;
    t_int          x_omni;
    t_int          x_raw;
    t_float        x_ch;
    t_float        x_ch_in;
    unsigned char  x_ready;
    unsigned char  x_status;
    unsigned char  x_channel;
    unsigned char  x_lsb;
    t_outlet      *x_chanout;
}t_bendin;

static t_class *bendin_class;

static void bendin_float(t_bendin *x, t_float f){
    if(f < 0)
        return;
    t_int ch = x->x_ch_in;
    if(ch != x->x_ch){
        if(ch == 0){
            x->x_ch = ch;
            x->x_omni = 1;
        }
        else if(ch > 0){
            x->x_ch = ch;
            x->x_omni = 0;
            x->x_channel = (unsigned char)--ch;
        }
    }
    if(f < 256){
        unsigned char bval = (t_int)f;
        if(bval & 0x80){
            unsigned char status = bval & 0xF0;
            if(status == 0xF0 && bval < 0xF8)
                x->x_status = x->x_ready = 0; // clear
            else if(status == 0xE0){
                unsigned char channel = bval & 0x0F;
                if(x->x_omni)
                    x->x_channel = channel;
                x->x_status = (x->x_channel == channel);
                x->x_ready = 0;
            }
            else
                x->x_status = x->x_ready = 0; // clear
        }
        else if(x->x_ready){
            if(x->x_omni)
                outlet_float(x->x_chanout, x->x_channel + 1);
            float bend = (bval << 7) + x->x_lsb;
            if(!x->x_raw){ // normalize
                bend = (bend - 8192) / 8191;
                if(bend < -1)
                    bend = -1;
            }
            outlet_float(((t_object *)x)->ob_outlet, bend);
            x->x_ready = 0;
        }
        else if(x->x_status){
            x->x_lsb = bval;
            x->x_ready = 1;
        }
    }
    else
        x->x_status = x->x_ready = 0; // clear
}

static void *bendin_new(t_symbol *s, t_int ac, t_atom *av){
    t_bendin *x = (t_bendin *)pd_new(bendin_class);
    t_symbol *curarg = s; // get rid of warning
    t_int channel = 0;
    x->x_raw = x->x_status = x->x_ready = 0;
    if(ac){
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                channel = (t_int)atom_getfloatarg(0, ac, av);
                ac--;
                av++;
            }
            else if(av->a_type == A_SYMBOL){
                curarg = atom_getsymbolarg(0, ac, av);
                if(!strcmp(curarg->s_name, "-raw")){
                    x->x_raw = 1;
                    ac--;
                    av++;
                }
                else
                    goto errstate;
            }
            else
                goto errstate;
        }
    }
    x->x_omni = (channel == 0);
    if(!x->x_omni)
        x->x_channel = (unsigned char)--channel;
    floatinlet_new((t_object *)x, &x->x_ch_in);
    outlet_new((t_object *)x, &s_float);
    x->x_chanout = outlet_new((t_object *)x, &s_float);
    return(x);
    errstate:
        pd_error(x, "[bendin]: improper args");
        return NULL;
}

void setup_bend0x2ein(void){
    bendin_class = class_new(gensym("bend.in"), (t_newmethod)bendin_new,
                            0, sizeof(t_bendin), 0, A_GIMME, 0);
    class_addfloat(bendin_class, bendin_float);
}
