// porres 2018

#include "m_pd.h"
#include <string.h>

typedef struct _noteout{
    t_object  x_ob;
    t_float   x_channel;
    t_float   x_flag;
    t_float   x_velocity;
    t_int     x_pitch;
    t_int     x_rel;
}t_noteout;

static t_class *noteout_class;

static void noteout_float(t_noteout *x, t_float f){
    int pitch = (int)f;
    if(pitch >= 0 && pitch <= 127){
        int channel = (int)x->x_channel;
        if(channel <= 0)
            channel = 1;
        if(channel > 16)
            channel = 16;
        int velocity = (int)x->x_velocity;
        if(velocity < 0)
            velocity = 0;
        if(velocity > 127)
            velocity = 127;
        if(x->x_rel){
            int status = ((int)x->x_flag ? 0x90 : 0x80);
            outlet_float(((t_object *)x)->ob_outlet, status + ((channel-1)));
            outlet_float(((t_object *)x)->ob_outlet, pitch);
            outlet_float(((t_object *)x)->ob_outlet, velocity);
        }
        else{
            int status = 0x90;
            outlet_float(((t_object *)x)->ob_outlet, status + ((channel-1)));
            outlet_float(((t_object *)x)->ob_outlet, pitch);
            outlet_float(((t_object *)x)->ob_outlet, velocity);
        }
    }
}

static void *noteout_new(t_symbol *s, t_int ac, t_atom *av){
    t_noteout *x = (t_noteout *)pd_new(noteout_class);
    t_symbol *curarg = s; // get rid of warning
    float channel = 1;
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
    floatinlet_new((t_object *)x, &x->x_velocity);
    if(x->x_rel)
        floatinlet_new((t_object *)x, &x->x_flag);
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    x->x_channel = (channel > 0 ? channel : 1);
    x->x_flag = 0;
    x->x_velocity = 0;
    x->x_pitch = -1;
    return(x);
    errstate:
        pd_error(x, "[noteout]: improper args");
        return NULL;
}

void setup_note0x2eout(void){
    noteout_class = class_new(gensym("note.out"), (t_newmethod)noteout_new, 0,
        sizeof(t_noteout), 0, A_GIMME, 0);
    class_addfloat(noteout_class, noteout_float);
}
