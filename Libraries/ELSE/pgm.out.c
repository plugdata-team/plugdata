// porres 2018

#include "m_pd.h"
#include <string.h>

typedef struct _pgmout{
    t_object  x_ob;
    t_float   x_channel;
    t_int     x_raw;
}t_pgmout;

static t_class *pgmout_class;

static void pgmout_float(t_pgmout *x, t_float f){
    if(f >= 0 && f <= 127){
        t_int channel = (int)x->x_channel;
        if(channel <= 0)
            channel = 1;
        outlet_float(((t_object *)x)->ob_outlet, 192 + ((channel-1) & 0x0F));
        outlet_float(((t_object *)x)->ob_outlet, (t_int)f);
    }
}

static void *pgmout_new(t_symbol *s, t_int ac, t_atom *av){
    t_pgmout *x = (t_pgmout *)pd_new(pgmout_class);
    t_symbol *curarg = NULL;
    curarg = s; // get rid of warning
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    t_float channel = 1;
    if(ac){
        while(ac > 0){
            if(av->a_type == A_FLOAT){
                channel = (t_int)atom_getfloatarg(0, ac, av);
                ac--, av++;
            }
            else
                goto errstate;
        }
    }
    x->x_channel = (channel > 0 ? channel : 1);
    return(x);
    errstate:
        pd_error(x, "[pgmout]: improper args");
        return NULL;
}

void setup_pgm0x2eout(void){
    pgmout_class = class_new(gensym("pgm.out"), (t_newmethod)pgmout_new,
            0, sizeof(t_pgmout), 0, A_GIMME, 0);
    class_addfloat(pgmout_class, pgmout_float);
}
