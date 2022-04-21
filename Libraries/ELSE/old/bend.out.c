// porres 2018

#include "m_pd.h"
#include <string.h>

typedef struct _bendout{
    t_object  x_ob;
    t_float   x_channel;
    t_int     x_raw;
}t_bendout;

static t_class *bendout_class;

static void bendout_float(t_bendout *x, t_float f){
    t_int bend;
    t_int channel = (int)x->x_channel;
    if(channel <= 0)
        channel = 1;
    if(x->x_raw)
        bend = (t_int)f;
    else
        bend = (t_int)(f * 8191) + 8192;
    if(bend >= 0 && bend <= 16383){
        outlet_float(((t_object *)x)->ob_outlet, 224 + ((channel-1) & 0x0F));
        outlet_float(((t_object *)x)->ob_outlet, bend & 0x7F);
        outlet_float(((t_object *)x)->ob_outlet, bend >> 7);
    }
}

static void *bendout_new(t_symbol *s, t_int ac, t_atom *av){
    t_bendout *x = (t_bendout *)pd_new(bendout_class);
    t_symbol *curarg = s; // get rid of warning
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    t_float channel = 1;
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
    x->x_channel = (channel > 0 ? channel : 1);
    return(x);
    errstate:
        pd_error(x, "[bendout]: improper args");
        return NULL;
}

void setup_bend0x2eout(void){
    bendout_class = class_new(gensym("bend.out"), (t_newmethod)bendout_new,
            0, sizeof(t_bendout), 0, A_GIMME, 0);
    class_addfloat(bendout_class, bendout_float);
}
