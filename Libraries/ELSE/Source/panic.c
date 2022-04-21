#include <string.h>
#include "m_pd.h"

#define PANIC_NCHANNELS  16
#define PANIC_NPITCHES   128
#define PANIC_VOIDPITCH  0xFF

typedef struct _panic{
    t_object       x_ob;
    unsigned char  x_status;
    unsigned char  x_channel;
    unsigned char  x_pitch;
    unsigned char  x_notes[PANIC_NCHANNELS][PANIC_NPITCHES];
}t_panic;

static t_class *panic_class;

static void panic_float(t_panic *x, t_float f){
    int ival = (int)f;
    if(ival >= 0 && ival < 256){
        unsigned char bval = ival;
        outlet_float(((t_object *)x)->ob_outlet, bval);
        if(bval & 0x80){
            x->x_status = bval & 0xF0;
            if(x->x_status == 0x80 || x->x_status == 0x90)
                x->x_channel = bval & 0x0F;
            else
                x->x_status = 0;
        }
        else if(x->x_status){
            if (x->x_pitch == PANIC_VOIDPITCH){
                x->x_pitch = bval;
                return;
            }
            else if(x->x_status == 0x90 && bval)
                x->x_notes[x->x_channel][x->x_pitch]++;
            else
                x->x_notes[x->x_channel][x->x_pitch]--;
        }
    }
    x->x_pitch = PANIC_VOIDPITCH;
}

static void panic_bang(t_panic *x){
    int chn, pch;
    for(chn = 0; chn < PANIC_NCHANNELS; chn++){
        for (pch = 0; pch < PANIC_NPITCHES; pch++){
            int status = 0x090 | chn;
            while(x->x_notes[chn][pch]){
                outlet_float(((t_object *)x)->ob_outlet, status);
                outlet_float(((t_object *)x)->ob_outlet, pch);
                outlet_float(((t_object *)x)->ob_outlet, 0);
                x->x_notes[chn][pch]--;
            }
        }
    }
}

static void panic_clear(t_panic *x){
    memset(x->x_notes, 0, sizeof(x->x_notes));
}

static void *panic_new(void){
    t_panic *x = (t_panic *)pd_new(panic_class);
    x->x_status = 0;
    x->x_pitch = PANIC_VOIDPITCH;
    panic_clear(x);
    outlet_new((t_object *)x, &s_float);
    return (x);
}

void panic_setup(void){
    panic_class = class_new(gensym("panic"), 
        (t_newmethod)panic_new, 0, sizeof(t_panic), 0, 0);
    class_addfloat(panic_class, panic_float);
    class_addbang(panic_class, panic_bang);
    class_addmethod(panic_class, (t_method)panic_clear, gensym("clear"), 0);
}
