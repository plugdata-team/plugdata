// porres 2018

#include "m_pd.h"
#include <string.h>

typedef struct _notein{
    t_object       x_ob;
    t_int          x_omni;
    t_int          x_rel;
    t_float        x_ch;
    t_float        x_ch_in;
    unsigned char  x_ready;
    unsigned char  x_status;
    unsigned char  x_channel;
    unsigned char  x_pitch;
    t_outlet      *x_velout;
    t_outlet      *x_flagout;
    t_outlet      *x_chanout;
}t_notein;

static t_class *notein_class;

static void notein_float(t_notein *x, t_float f){
    int ival = (int)f;
    if(ival < 0)
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
    if(ival < 256){
        unsigned char bval = ival;
        if(bval & 0x80){
            unsigned char status = bval & 0xF0;
            if(status == 0xF0 && bval < 0xF8)
                x->x_status = x->x_ready = 0; // clear
            else if(status == 0x80 || status == 0x90){
                unsigned char channel = bval & 0x0F;
                if(x->x_omni)
                    x->x_channel = channel;
                x->x_status = (x->x_channel == channel ? status : 0);
                x->x_ready = 0;
            }
            else
                x->x_status = x->x_ready = 0; // clear
        }
        else if(x->x_ready){
            int flag = (x->x_status == 0x90);
            if(x->x_omni){
                outlet_float(x->x_chanout, x->x_channel + 1);
            }
            if(x->x_rel){
                outlet_float(x->x_flagout, flag);
                outlet_float(x->x_velout, bval);
            }
            else
               outlet_float(x->x_velout, bval * flag);
            outlet_float(((t_object *)x)->ob_outlet, x->x_pitch);
            x->x_ready = 0;
        }
        else if(x->x_status){
            x->x_pitch = bval;
            x->x_ready = 1;
        }
    }
    else
        x->x_status = x->x_ready = 0; // clear
}

static void *notein_new(t_symbol *s, t_int ac, t_atom *av){
    t_notein *x = (t_notein *)pd_new(notein_class);
    t_symbol *curarg = NULL;
    curarg = s; // get rid of warning
    t_int channel = 0;
    x->x_rel = x->x_status = x->x_ready = 0;
    if(ac){
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                channel = (t_int)atom_getfloatarg(0, ac, av);
                ac--;
                av++;
            }
            else if(av->a_type == A_SYMBOL){
                curarg = atom_getsymbolarg(0, ac, av);
                if(!strcmp(curarg->s_name, "-rel")){
                    x->x_rel = 1;
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
    x->x_velout = outlet_new((t_object *)x, &s_float);
    if(x->x_rel)
        x->x_flagout = outlet_new((t_object *)x, &s_float);
    x->x_chanout = outlet_new((t_object *)x, &s_float);
    return(x);
    errstate:
        pd_error(x, "[notein]: improper args");
        return NULL;
}

void setup_note0x2ein(void){
    notein_class = class_new(gensym("note.in"), (t_newmethod)notein_new,
                            0, sizeof(t_notein), 0, A_GIMME, 0);
    class_addfloat(notein_class, notein_float);
}
