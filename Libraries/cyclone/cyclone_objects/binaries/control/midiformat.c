/* Copyright (c) 2002-2003 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include <common/api.h>
#include <string.h>

//adding attributes, x_hires and associated modes - Derek Kwan 2016

typedef struct _midiformat
{
    t_object  x_ob;
    t_float   x_channel;
    int         x_hires;
} t_midiformat;

static t_class *midiformat_class;

static void midiformat_hires(t_midiformat * x, t_float _hires){
    int hires = (int)_hires;
    if(hires < 0){
        hires = 0;
    }
    else if(hires > 2){
        hires = 2;
    };

    x->x_hires = hires;
}

static int midiformat_channel(t_midiformat *x)
{
    int ch = (int)x->x_channel;
    return (ch > 0 ? (ch - 1) & 0x0F : 0);
}

static void midiformat_note(t_midiformat *x, t_symbol *s, int ac, t_atom *av)
{
    if (ac >= 2 && av[0].a_type == A_FLOAT && av[1].a_type == A_FLOAT)
    {
	int pitch = (int)av[0].a_w.w_float;  /* CHECKED: anything goes */
	int velocity = (int)av[1].a_w.w_float;
	outlet_float(((t_object *)x)->ob_outlet, 0x90 | midiformat_channel(x));
	outlet_float(((t_object *)x)->ob_outlet, pitch);
	outlet_float(((t_object *)x)->ob_outlet, velocity);
    }
}

static void midiformat_polytouch(t_midiformat *x,
				 t_symbol *s, int ac, t_atom *av)
{
    if (ac >= 2 && av[0].a_type == A_FLOAT && av[1].a_type == A_FLOAT)
    {
	int touch = (int)av[0].a_w.w_float;
	int key = (int)av[1].a_w.w_float;
	outlet_float(((t_object *)x)->ob_outlet, 0xA0 | midiformat_channel(x));
	outlet_float(((t_object *)x)->ob_outlet, key);
	outlet_float(((t_object *)x)->ob_outlet, touch);
    }
}

static void midiformat_controller(t_midiformat *x,
				  t_symbol *s, int ac, t_atom *av)
{
    if (ac >= 2 && av[0].a_type == A_FLOAT && av[1].a_type == A_FLOAT)
    {
	int val = (int)av[0].a_w.w_float;
	int ctl = (int)av[1].a_w.w_float;
	outlet_float(((t_object *)x)->ob_outlet, 0xB0 | midiformat_channel(x));
	outlet_float(((t_object *)x)->ob_outlet, ctl);
	outlet_float(((t_object *)x)->ob_outlet, val);
    }
}

static void midiformat_program(t_midiformat *x, t_floatarg f)
{
    int pgm = (int)f;
    outlet_float(((t_object *)x)->ob_outlet, 0xC0 | midiformat_channel(x));
    outlet_float(((t_object *)x)->ob_outlet, pgm);
}

static void midiformat_touch(t_midiformat *x, t_floatarg f)
{
    int touch = (int)f;
    outlet_float(((t_object *)x)->ob_outlet, 0xD0 | midiformat_channel(x));
    outlet_float(((t_object *)x)->ob_outlet, touch);
}

static void midiformat_bend(t_midiformat *x, t_floatarg f)
{
    outlet_float(((t_object *)x)->ob_outlet, 0xE0 | midiformat_channel(x));
    if(x->x_hires == 0){
        //legacy 0-127;
        int val = (int)f;
        if(val <0){
            val = 0;
        }
        else if(val > 127){
            val = 127;
        };
        outlet_float(((t_object *)x)->ob_outlet, 0);
        outlet_float(((t_object *)x)->ob_outlet, val);
    }
    else{
        int val = 0;
        switch(x->x_hires){
            case 1:
                //[-1, 1)
                val = (int)((f+1)*8192.);
                break;
            case 2:
                // [-8192, 8191]
                val = (int)f + 8192; 
                break;
            default:
                break;
        };
        if(val < 0){
            val = 0;
        }
        else if(val > 16383){ //max of 14 bit - 1
            val = 16383;
        };

	outlet_float(((t_object *)x)->ob_outlet, val & 0x7F);
	outlet_float(((t_object *)x)->ob_outlet, val >> 7);
    };
}

static void *midiformat_new(t_symbol * s, int argc, t_atom * argv)
{
    t_midiformat *x = (t_midiformat *)pd_new(midiformat_class);
   
    //defaults
    t_float hires = 0;
    t_float channel = 0;

    while(argc){
        if(argv -> a_type == A_SYMBOL){
            if(argc >= 2){
            t_symbol * cursym = atom_getsymbolarg(0, argc, argv);
            t_float curfloat = atom_getfloatarg(1, argc, argv);
            if(strcmp(cursym->s_name, "@hires") == 0){
                hires = curfloat;
                }
            else{
                goto errstate;
                 };
            argc-=2;
            argv+=2;
            }
            else{
                goto errstate;
            };
        }
        else{
            //argv is a float
            t_float curfloat = atom_getfloatarg(0, argc, argv);
            channel = curfloat;
            argc--;
            argv++;
        };

    };
    midiformat_hires(x, hires);
    x->x_channel = channel;
    inlet_new((t_object *)x, (t_pd *)x, &s_list, gensym("lst1"));
    inlet_new((t_object *)x, (t_pd *)x, &s_list, gensym("lst2"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft3"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft4"));
    inlet_new((t_object *)x, (t_pd *)x, &s_float, gensym("ft5"));
    floatinlet_new((t_object *)x, &x->x_channel);
    outlet_new((t_object *)x, &s_float);
    return (x);
    errstate:
        post("midiformat: improper args");
        return NULL;
}

CYCLONE_OBJ_API void midiformat_setup(void)
{
    midiformat_class = class_new(gensym("midiformat"), 
				 (t_newmethod)midiformat_new, 0,
				 sizeof(t_midiformat), 0,
				 A_GIMME, 0);
    class_addlist(midiformat_class, midiformat_note);
    class_addmethod(midiformat_class, (t_method)midiformat_polytouch,
		    gensym("lst1"), A_GIMME, 0);
    class_addmethod(midiformat_class, (t_method)midiformat_controller,
		    gensym("lst2"), A_GIMME, 0);
    class_addmethod(midiformat_class, (t_method)midiformat_program,
		    gensym("ft3"), A_FLOAT, 0);
    class_addmethod(midiformat_class, (t_method)midiformat_touch,
		    gensym("ft4"), A_FLOAT, 0);
    class_addmethod(midiformat_class, (t_method)midiformat_bend,
		    gensym("ft5"), A_FLOAT, 0);
    class_addmethod(midiformat_class, (t_method)midiformat_hires,  gensym("hires"), A_FLOAT, 0);
}
